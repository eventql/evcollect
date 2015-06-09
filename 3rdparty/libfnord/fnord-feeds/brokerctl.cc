/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <unistd.h>
#include "fnord-base/stdtypes.h"
#include "fnord-base/inspect.h"
#include "fnord-base/application.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-base/cli/CLI.h"
#include "fnord-base/thread/eventloop.h"
#include "fnord-base/io/file.h"
#include "fnord-base/random.h"
#include "fnord-base/option.h"
#include "fnord-http/httpconnectionpool.h"
#include "fnord-feeds/BrokerClient.h"
#include "fnord-feeds/ExportCursor.pb.h"
#include "fnord-msg/msg.h"

using namespace fnord;
using namespace fnord::feeds;

void cmd_monitor(const cli::FlagParser& flags) {
  fnord::iputs("monitor", 1);

}

void cmd_export(const cli::FlagParser& flags) {
  Random rnd;
  fnord::thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  http::HTTPConnectionPool http(&ev);
  BrokerClient broker(&http);

  Duration poll_interval(0.5 * kMicrosPerSecond);
  uint64_t maxsize = 4 * 1024 * 1024;
  size_t batchsize = 1024;
  auto topic = flags.getString("topic");
  String prefix = StringUtil::stripShell(topic);
  auto path = flags.getString("datadir");
  auto cursorfile_path = FileUtil::joinPaths(path, prefix + ".cur");

  Vector<InetAddr> servers;
  for (const auto& s : flags.getStrings("server")) {
    servers.emplace_back(InetAddr::resolve(s));
  }

  if (servers.size() == 0) {
    RAISE(kUsageError, "no servers specified");
  }

  fnord::logInfo("brokerctl", "Exporting topic '$0'", topic);

  ExportCursor cursor;
  if (FileUtil::exists(cursorfile_path)) {
    auto buf = FileUtil::read(cursorfile_path);
    msg::decode<ExportCursor>(buf.data(), buf.size(), &cursor);

    auto cur_topic = cursor.topic_cursor().topic();
    if (cur_topic != topic) {
      RAISEF(kRuntimeError, "topic mismatch: '$0' vs '$1;", cur_topic, topic);
    }

    fnord::logInfo(
        "brokerctl",
        "Resuming export from sequence $0",
        cursor.head_sequence());
  } else {
    cursor.mutable_topic_cursor()->set_topic(topic);
    fnord::logInfo("brokerctl", "Starting new export from epoch...");
  }

  Vector<String> rows;
  size_t rows_size = 0;
  for (;;) {
    size_t n = 0;
    for (const auto& server : servers) {
      auto msgs = broker.fetchNext(
          server,
          cursor.mutable_topic_cursor(),
          batchsize);

      for (const auto& msg : msgs.messages()) {
        rows.emplace_back(msg.data());
        rows_size += msg.data().size();
      }

      n += msgs.messages().size();
    }

    if (rows_size >= maxsize) {
      auto next_seq = cursor.head_sequence() + 1;
      fnord::logInfo("brokerctl", "Writing sequence $0", next_seq);

      auto dstpath = FileUtil::joinPaths(
          path,
          StringUtil::format("$0.$1.$2", prefix, next_seq, "json"));

      {
        auto tmpfile = File::openFile(
            dstpath + "~",
            File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

        tmpfile.write("[");
        for (int i = 0; i < rows.size(); ++i) {
          if (i > 0) { tmpfile.write(", "); }
          tmpfile.write(rows[i]);
        }
        tmpfile.write("]\n");
      }

      cursor.set_head_sequence(next_seq);
      FileUtil::mv(dstpath + "~", dstpath);
      rows.clear();
      rows_size = 0;

      {
        auto cursorfile = File::openFile(
            cursorfile_path + "~",
            File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

        cursorfile.write(*msg::encode(cursor));
      }

      FileUtil::mv(cursorfile_path + "~", cursorfile_path);
    }

    if (n == 0) {
      usleep(poll_interval.microseconds());
    }
  }

  evloop_thread.join();
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "loglevel",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  cli::CLI cli;

  /* command: monitor */
  auto monitor_cmd = cli.defineCommand("monitor");
  monitor_cmd->onCall(std::bind(&cmd_monitor, std::placeholders::_1));

  /* command: export */
  auto export_cmd = cli.defineCommand("export");
  export_cmd->onCall(std::bind(&cmd_export, std::placeholders::_1));

  export_cmd->flags().defineFlag(
      "topic",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "topic",
      "<topic>");

  export_cmd->flags().defineFlag(
      "datadir",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "output file path",
      "<path>");

  export_cmd->flags().defineFlag(
      "server",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "backend servers",
      "<host>");


  cli.call(flags.getArgv());
  return 0;
}

