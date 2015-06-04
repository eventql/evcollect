/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <memory>
#include <sys/select.h>
#include <thread>
#include <unistd.h>
#include "fnord-base/thread/FixedSizeThreadPool.h"

namespace fnord {
namespace thread {

FixedSizeThreadPool::FixedSizeThreadPool(
    size_t nthreads) :
    FixedSizeThreadPool(
        nthreads,
        std::unique_ptr<fnord::ExceptionHandler>(
            new fnord::CatchAndAbortExceptionHandler())) {}

FixedSizeThreadPool::FixedSizeThreadPool(
    size_t nthreads,
    std::unique_ptr<ExceptionHandler> error_handler) :
    nthreads_(nthreads),
    error_handler_(std::move(error_handler)) {}

void FixedSizeThreadPool::start() {
  for (int i = 0; i < nthreads_; ++i) {
    threads_.emplace_back([this] () {
      while (true) {
        auto job = queue_.pop();

        try {
          job();
        } catch (const std::exception& e) {
          error_handler_->onException(e);
        }
      }
    });
  }
}

void FixedSizeThreadPool::stop() {
  // FIXPAUL
}

void FixedSizeThreadPool::run(std::function<void()> task) {
  queue_.insert(task);
}

void FixedSizeThreadPool::runOnReadable(std::function<void()> task, int fd) {
  RAISE(
      kNotImplementedError,
      "not suppported: FixedSizeThreadPool::runOnReadable");
}

void FixedSizeThreadPool::runOnWritable(std::function<void()> task, int fd) {
  RAISE(
      kNotImplementedError,
      "not suppported: FixedSizeThreadPool::runOnWritable");
}

void FixedSizeThreadPool::runOnWakeup(
    std::function<void()> task,
    Wakeup* wakeup,
    long generation) {
  RAISE(
      kNotImplementedError,
      "not suppported: FixedSizeThreadPool::runOnWakeup");
}

}
}
