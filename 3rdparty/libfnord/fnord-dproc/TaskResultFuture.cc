/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-dproc/TaskResultFuture.h>

namespace fnord {
namespace dproc {

Future<RefPtr<Task>> TaskResultFuture::result() const {
  return promise_.future();
}

void TaskResultFuture::returnResult(RefPtr<Task> result) {
  promise_.success(result);
}

void TaskResultFuture::returnError(const StandardException& e) {
  promise_.failure(e);
}

void TaskResultFuture::updateStatus(Function<void (TaskStatus* status)> fn) {
  std::unique_lock<std::mutex> lk(status_mutex_);
  fn(&status_);

  if (on_status_change_) {
    auto cb = on_status_change_;
    lk.unlock();
    cb();
  }
}

void TaskResultFuture::onStatusChange(Function<void ()> fn) {
  std::unique_lock<std::mutex> lk(status_mutex_);
  on_status_change_ = fn;
}

TaskStatus TaskResultFuture::status() const {
  std::unique_lock<std::mutex> lk(status_mutex_);
  return status_;
}

TaskStatus::TaskStatus() :
    num_subtasks_total(1),
    num_subtasks_completed(0) {}

String TaskStatus::toString() const {
  return StringUtil::format(
      "$0/$1 ($2%)",
      num_subtasks_completed,
      num_subtasks_total,
      progress() * 100);
}

double TaskStatus::progress() const {
  return num_subtasks_completed / (double) num_subtasks_total;
}


} // namespace dproc
} // namespace fnord
