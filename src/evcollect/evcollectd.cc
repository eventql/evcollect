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
#include <evcollect/util/flagparser.h>
#include <evcollect/util/logging.h>
#include <evcollect/evcollect.h>
#include <evcollect/plugin.h>
#include <evcollect/dispatch.h>

ReturnCode daemonize();

int main(int argc, const char** argv) {
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

  FlagParser flags;

  flags.defineFlag(
      "help",
      FlagParser::T_SWITCH,
      false,
      "?",
      NULL,
      "help",
      "<help>");

  flags.defineFlag(
      "version",
      FlagParser::T_SWITCH,
      false,
      "V",
      NULL,
      "print version",
      "<switch>");

  flags.defineFlag(
      "config",
      ::FlagParser::T_STRING,
      false,
      "c",
      NULL,
      "path to config file",
      "<config_file>");

  flags.defineFlag(
      "loglevel",
      FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "daemonize",
      FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "daemonize",
      "<switch>");

  flags.defineFlag(
      "pidfile",
      FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "pidfile",
      "<path>");

  flags.defineFlag(
      "log_to_syslog",
      FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "log to syslog",
      "<switch>");

  flags.defineFlag(
      "nolog_to_stderr",
      FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "don't log to stderr",
      "<switch>");

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
    Logger::logToStderr("evqld");
  }

  if (flags.isSet("log_to_syslog")) {
    Logger::logToSyslog("evqld");
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
        "   -c, --config <file>       Load config from file\n"
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

  /* daemonize */
  if (flags.isSet("daemonize")) {
    auto rc = daemonize();
    if (!rc.isSuccess()) {
      logFatal(rc.getMessage());
      return 1;
    }
  }

  //ScopedPtr<FileLock> pidfile_lock;
  //if (process_config->hasProperty("server.pidfile")) {
  //  auto pidfile_path = process_config->getString("server.pidfile").get();
  //  pidfile_lock = mkScoped(new FileLock(pidfile_path));
  //  pidfile_lock->lock(false);

  //  auto pidfile = File::openFile(
  //      pidfile_path,
  //      File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

  //  pidfile.write(StringUtil::toString(getpid()));
  //}

  logInfo("Exiting...");

  //if (pidfile_lock.get()) {
  //  pidfile_lock.reset(nullptr);
  //  FileUtil::rm(process_config->getString("server.pidfile").get());
  //}

  return 0;
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

