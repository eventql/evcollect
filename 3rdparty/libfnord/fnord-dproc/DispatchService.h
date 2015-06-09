/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DPROC_DISPATCHSERVICE_H
#define _FNORD_DPROC_DISPATCHSERVICE_H
#include <fnord-base/stdtypes.h>
#include <fnord-dproc/Application.h>
#include <fnord-dproc/Scheduler.h>
#include <fnord-dproc/TaskSpec.pb.h>
#include <fnord-dproc/TaskResultFuture.h>

namespace fnord {
namespace dproc {

class DispatchService {
public:

  void registerApp(
      RefPtr<Application> app,
      RefPtr<Scheduler> scheduler);

  RefPtr<TaskResultFuture> run(const TaskSpec& task);

protected:

  struct AppRef {
    RefPtr<Application> app;
    RefPtr<Scheduler> scheduler;
  };

  HashMap<String, AppRef> apps_;
};

} // namespace dproc
} // namespace fnord

#endif
