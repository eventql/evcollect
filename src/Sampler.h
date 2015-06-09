/**
 * This file is part of the "sensord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Finn Zirngibl
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <thread>
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-base/thread/DelayedQueue.h>
#include "Sensor.h"
#include "SensorRepository.h"
#include "SamplerConfig.pb.h"
#include "SampleEnvelope.pb.h"

using namespace fnord;

namespace sensord {

class Sampler {
public:

  Sampler(SamplerConfig config, SensorRepository* sensors);

  void run();

protected:

  void executeRule(SampleRule* rule);
  void startWorker();

  SamplerConfig config_;
  SensorRepository* sensors_;
  bool running_;
  thread::DelayedQueue<SampleRule*> queue_;
};

};
