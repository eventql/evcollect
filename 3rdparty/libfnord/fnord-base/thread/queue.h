/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_THREAD_QUEUE_H
#define _FNORDMETRIC_THREAD_QUEUE_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include "fnord-base/option.h"

namespace fnord {
namespace thread {

/**
 * A queue is threadsafe
 */
template <typename T>
class Queue {
public:

  Queue(size_t max_size = -1);

  void insert(const T& job, bool block = false);
  T pop();
  Option<T> interruptiblePop();
  Option<T> poll();

  size_t length() const;
  void wakeup();

protected:
  std::deque<T> queue_;
  mutable std::mutex mutex_;
  std::condition_variable wakeup_;
  size_t max_size_;
  size_t length_;
};

}
}

#include "queue_impl.h"
#endif
