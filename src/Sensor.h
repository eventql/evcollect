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
#include "fnord-base/stdtypes.h"
#include "fnord-base/autoref.h"

using namespace fnord;

namespace sensord {

class Sensor : public RefCounted {

  virtual ~Sensor() {};

  /**
   * The key under which this sensor will be available. e.g. "host/load_average"
   */
  virtual const String& key() const = 0;

  /**
   * Fetch this sensors most latest data snapshot
   */
  virtual BufferRef fetchData() const = 0;

  /**
   * Return the schema for the data snapshots returned by this sensor
   */
  virtual msg::MessageSchema schema() const = 0;

};

};
