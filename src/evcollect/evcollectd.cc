/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <regex>
#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <curl/curl.h>
#include <evcollect/util/flagparser.h>
#include <evcollect/util/logging.h>
#include <evcollect/evcollect.h>
#include <evcollect/plugin.h>
#include <evcollect/dispatch.h>
#include <evcollect/config.h>
#include <evcollect/logfile.h>

using namespace evcollect;

void shutdown(int);
ReturnCode daemonize();

static Dispatch* dispatch;

int main(int argc, const char** argv) {
  signal(SIGTERM, shutdown);
  signal(SIGINT, shutdown);
  signal(SIGHUP, shutdown);
  signal(SIGPIPE, SIG_IGN);

  FlagParser flags;

  flags.defineFlag(
      "help",
      FlagParser::T_SWITCH,
      false,
      "?",
      NULL);

  flags.defineFlag(
      "version",
      FlagParser::T_SWITCH,
      false,
      "V",
      NULL);

  flags.defineFlag(
      "spool_dir",
      ::FlagParser::T_STRING,
      false,
      "s",
      NULL);

  flags.defineFlag(
      "config",
      ::FlagParser::T_STRING,
      false,
      "c",
      NULL);

  flags.defineFlag(
      "plugin",
      ::FlagParser::T_STRING,
      false,
      "p",
      NULL);

  flags.defineFlag(
      "plugin_path",
      ::FlagParser::T_STRING,
      false,
      "P",
      "/usr/local/lib/evcollect/plugins");

  flags.defineFlag(
      "loglevel",
      FlagParser::T_STRING,
      false,
      NULL,
      "INFO");

  flags.defineFlag(
      "daemonize",
      FlagParser::T_SWITCH,
      false,
      NULL,
      NULL);

  flags.defineFlag(
      "pidfile",
      FlagParser::T_STRING,
      false,
      NULL,
      NULL);

  flags.defineFlag(
      "log_to_syslog",
      FlagParser::T_SWITCH,
      false,
      NULL,
      NULL);

  flags.defineFlag(
      "nolog_to_stderr",
      FlagParser::T_SWITCH,
      false,
      NULL,
      NULL);

  /* parse flags */
  {
    auto rc = flags.parseArgv(argc, argv);
    if (!rc.isSuccess()) {
      logFatal(rc.getMessage());
      return 1;
    }
  }

  /* setup logging */
  if (!flags.isSet("nolog_to_stderr") && !flags.isSet("daemonize")) {
    Logger::logToStderr("evcollectd");
  }

  if (flags.isSet("log_to_syslog")) {
    Logger::logToSyslog("evcollectd");
  }

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  /* print help */
  if (flags.isSet("help") || flags.isSet("version")) {
    std::cerr <<
        StringUtil::format(
            "evcollectd $0\n"
            "Copyright (c) 2016, DeepCortex GmbH. All rights reserved.\n\n",
            EVCOLLECT_VERSION);
  }

  if (flags.isSet("version")) {
    return 0;
  }

  if (flags.isSet("help")) {
    std::cerr <<
        "Usage: $ evcollectd [OPTIONS]\n\n"
        "   -s, --spool_dir <dir>     Where to store temporary files\n"
        "   -c, --config <file>       Load config from file\n"
        "   -p, --plugin <path>       Load a plugin (.so)\n"
        "   -P, --plugin_path <dir>   Set the plugin search path\n"
        "   --daemonize               Daemonize the server\n"
        "   --pidfile <file>          Write a PID file\n"
        "   --loglevel <level>        Minimum log level (default: INFO)\n"
        "   --[no]log_to_syslog       Do[n't] log to syslog\n"
        "   --[no]log_to_stderr       Do[n't] log to stderr\n"
        "   -?, --help                Display this help text and exit\n"
        "   -v, --version             Display the version of this binary and exit\n"
        "                                                       \n"
        "Examples:                                              \n"
        "   $ evcollectd --daemonize --config /etc/evcollect.conf\n";

    return 0;
  }

  /* init libraries */
  curl_global_init(CURL_GLOBAL_DEFAULT);

  /* load config */
  ProcessConfig conf;
  conf.plugin_dir = flags.getString("plugin_path");

  {
    auto config_path = flags.getString("config");
    if (config_path.empty()) {
      logFatal("error: --config is required");
      return 1;
    }

    auto rc = loadConfig(config_path, &conf);
    if (!rc.isSuccess()) {
      logFatal("invalid config: $0", rc.getMessage());
      return 1;
    }
  }

  if (flags.isSet("spool_dir")) {
    conf.spool_dir = flags.getString("spool_dir");
  }

  if (conf.spool_dir.empty()) {
    logFatal("error: --spool_dir is required");
    return 1;
  }

  for (const auto& plugin_path : flags.getStrings("plugin")) {
    conf.load_plugins.push_back(plugin_path);
  }

  /* load plugins */
  std::unique_ptr<PluginMap> plugin_map(new PluginMap(&conf));
  LogfileSourcePlugin::registerPlugin(plugin_map.get());

  PluginContext plugin_ctx;
  plugin_ctx.plugin_map = plugin_map.get();
  for (const auto& plugin : conf.load_plugins) {
    auto rc = plugin_map->loadPlugin(plugin, &plugin_ctx);
    if (!rc.isSuccess()) {
      logFatal(
          "error while loading plugin '$0': $1",
          plugin,
          rc.getMessage());

      return 1;
    }
  }

  /* initialize event bindings */
  auto rc = ReturnCode::success();
  std::vector<std::unique_ptr<EventBinding>> event_bindings;
  for (const auto& binding : conf.event_bindings) {
    if (!rc.isSuccess()) {
      break;
    }

    std::unique_ptr<EventBinding> ev_binding(new EventBinding());
    ev_binding->event_name = binding.event_name;
    ev_binding->interval_micros = binding.interval_micros;

    for (const auto& source : binding.sources) {
      EventSourceBinding ev_source;
      rc = plugin_map->getSourcePlugin(
          source.plugin_name,
          &ev_source.plugin);

      if (!rc.isSuccess()) {
        break;
      }

      rc = ev_source.plugin->pluginAttach(
          source.properties,
          &ev_source.userdata);

      if (!rc.isSuccess()) {
        break;
      }

      ev_binding->sources.emplace_back(ev_source);
    }

    if (rc.isSuccess()) {
      event_bindings.emplace_back(std::move(ev_binding));
    }
  }

  /* initialize target bindings */
  std::vector<std::unique_ptr<TargetBinding>> target_bindings;
  for (const auto& binding : conf.target_bindings) {
    if (!rc.isSuccess()) {
      break;
    }

    std::unique_ptr<TargetBinding> trgt_binding(new TargetBinding());

    rc = plugin_map->getOutputPlugin(
        binding.plugin_name,
        &trgt_binding->plugin);

    if (!rc.isSuccess()) {
      break;
    }

    rc = trgt_binding->plugin->pluginAttach(
        binding.properties,
        &trgt_binding->userdata);

    if (!rc.isSuccess()) {
      break;
    }

    target_bindings.emplace_back(std::move(trgt_binding));
  }

  /* daemonize */
  if (rc.isSuccess() && flags.isSet("daemonize")) {
    rc = daemonize();
  }

  /* write pidfile */
  int pidfile_fd = -1;
  auto pidfile_path = flags.getString("pidfile");
  if (rc.isSuccess() && !pidfile_path.empty()) {
    pidfile_fd = open(
        pidfile_path.c_str(),
        O_WRONLY | O_CREAT,
        0666);

    if (pidfile_fd < 0) {
      rc = ReturnCode::error(
          "IO_ERROR",
          "writing pidfile failed: %s",
          std::strerror(errno));
    }

    if (rc.isSuccess() && flock(pidfile_fd, LOCK_EX | LOCK_NB) != 0) {
      close(pidfile_fd);
      pidfile_fd = -1;
      rc = ReturnCode::error("IO_ERROR", "locking pidfile failed");
    }

    if (rc.isSuccess() && ftruncate(pidfile_fd, 0) != 0) {
      close(pidfile_fd);
      pidfile_fd = -1;
      rc = ReturnCode::error("IO_ERROR", "writing pidfile failed");
    }

    auto pid = StringUtil::toString(getpid());
    if (rc.isSuccess() &&
        write(pidfile_fd, pid.data(), pid.size()) != pid.size()) {
      close(pidfile_fd);
      pidfile_fd = -1;
      rc = ReturnCode::error("IO_ERROR", "writing pidfile failed");
    }
  }

  /* main dispatch loop */
  if (rc.isSuccess()) {
    logInfo("Starting with $0 event bindings", event_bindings.size());
    dispatch = new Dispatch();
    for (const auto& binding : event_bindings) {
      dispatch->addEventBinding(binding.get());
    }
    for (const auto& binding : target_bindings) {
      dispatch->addTargetBinding(binding.get());
    }

    rc = dispatch->run();
  }

  if (!rc.isSuccess()) {
    logFatal("error: $0", rc.getMessage());
  }

  /* shutdown */
  for (auto& binding : event_bindings) {
    for (auto& source : binding->sources) {
      source.plugin->pluginDetach(source.userdata);
    }
  }

  for (auto& binding : target_bindings) {
    binding->plugin->pluginDetach(binding->userdata);
  }

  logInfo("Exiting...");

  delete dispatch;
  plugin_map.reset(nullptr);

  /* unlock pidfile */
  if (pidfile_fd > 0) {
    unlink(pidfile_path.c_str());
    flock(pidfile_fd, LOCK_UN);
    close(pidfile_fd);
  }

  /* shutdown libraries*/
  curl_global_cleanup();

  return rc.isSuccess() ? 0 : 1;
}

void shutdown(int) {
  if (dispatch) {
    dispatch->kill();
  }
}

ReturnCode daemonize() {
#if defined(_WIN32)
#error "Application::daemonize() not yet implemented for windows"

#elif defined(__APPLE__) && defined(__MACH__)
  // FIXME: deprecated on OSX
  if (::daemon(true /*no chdir*/, true /*no close*/) < 0) {
    return ReturnCode::error("INIT_FAIL", "daemon() failed");
  }

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
  if (::daemon(true /*no chdir*/, true /*no close*/) < 0) {
    return ReturnCode::error("INIT_FAIL", "daemon() failed");
  }

#else
#error Unsupported OS
#endif
  return ReturnCode::success();
}

