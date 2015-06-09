/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <fnord-base/exception.h>
#include <fnord-base/exceptionhandler.h>
#include <fnord-base/inspect.h>
#include <fnord-base/logging.h>
#include <fnord-base/StackTrace.h>

namespace fnord {

using fnord::Exception;

CatchAndLogExceptionHandler::CatchAndLogExceptionHandler(
    const String& component) :
    component_(component) {}

void CatchAndLogExceptionHandler::onException(
    const std::exception& error) const {
  logError(component_, error, "Uncaught exception");
}

CatchAndAbortExceptionHandler::CatchAndAbortExceptionHandler(
    const std::string& message) :
    message_(message) {}

void CatchAndAbortExceptionHandler::onException(
    const std::exception& error) const {
  fprintf(stderr, "%s\n\n", message_.c_str()); // FIXPAUL

  try {
    auto rte = dynamic_cast<const fnord::Exception&>(error);
    rte.debugPrint();
  } catch (const std::exception& cast_error) {
    fprintf(stderr, "foreign exception: %s\n", error.what());
  }

  fprintf(stderr, "Aborting...\n");
  abort(); // core dump if enabled
}

static std::string globalEHandlerMessage;

static void globalSEGVHandler(int sig) {
  fprintf(stderr, "%s\n", globalEHandlerMessage.c_str());
  fprintf(stderr, "signal: %s\n", strsignal(sig));

  StackTrace strace;
  strace.debugPrint(2);

  exit(EXIT_FAILURE);
}

static void globalEHandler() {
  fprintf(stderr, "%s\n", globalEHandlerMessage.c_str());

  auto ex = std::current_exception();
  if (ex == nullptr) {
    fprintf(stderr, "<no active exception>\n");
    return;
  }

  try {
    std::rethrow_exception(ex);
  } catch (const fnord::Exception& e) {
    e.debugPrint();
  } catch (const std::exception& e) {
    fprintf(stderr, "foreign exception: %s\n", e.what());
  }

  exit(EXIT_FAILURE);
}

void CatchAndAbortExceptionHandler::installGlobalHandlers() {
  globalEHandlerMessage = message_;
  std::set_terminate(&globalEHandler);
  std::set_unexpected(&globalEHandler);
  signal(SIGILL, &globalSEGVHandler);
  signal(SIGABRT, &globalSEGVHandler);
  signal(SIGFPE, &globalSEGVHandler);
  signal(SIGSEGV, &globalSEGVHandler);
  signal(SIGBUS, &globalSEGVHandler);
  signal(SIGSYS, &globalSEGVHandler);
}

} // namespace fnord
