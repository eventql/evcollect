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
#pragma once
#include <ctime>
#include <inttypes.h>
#include <limits>
#include <string>
#include "time_constants.h"

class UnixTime {
public:

  /**
   * Create a new UTC UnixTime instance with time = now
   */
  UnixTime();

  /**
   * Create a new UTC UnixTime instance
   *
   * @param timestamp the UTC microsecond timestamp
   */
  constexpr UnixTime(uint64_t utc_time);

  /**
   * Return a representation of the date as a string (strftime)
   *
   * @param fmt the strftime format string (optional)
   */
  std::string toString(const char* fmt = "%Y-%m-%d %H:%M:%S") const;

  UnixTime& operator=(const UnixTime& other);

  constexpr bool operator==(const UnixTime& other) const;
  constexpr bool operator!=(const UnixTime& other) const;
  constexpr bool operator<(const UnixTime& other) const;
  constexpr bool operator>(const UnixTime& other) const;
  constexpr bool operator<=(const UnixTime& other) const;
  constexpr bool operator>=(const UnixTime& other) const;

  /**
   * Cast the UnixTime object to a UTC unix microsecond timestamp represented as
   * an uint64_t
   */
  constexpr explicit operator uint64_t() const;

  /**
   * Cast the UnixTime object to a UTC unix microsecond timestamp represented as
   * a double
   */
  constexpr explicit operator double() const;

  /**
   * Return the represented date/time as a UTC unix microsecond timestamp
   */
  constexpr uint64_t unixMicros() const;

  /**
   * Return a new UnixTime instance with time 00:00:00 UTC, 1 Jan. 1970
   */
  static inline UnixTime epoch();

  /**
   * Return a new UnixTime instance with time = now
   */
  static inline UnixTime now();

  /**
   * Return a new UnixTime instance with time = now + days
   */
  static inline UnixTime daysFromNow(double days);

protected:

  /**
   * The utc microsecond timestamp of the represented moment in time
   */
  uint64_t utc_micros_;
};

namespace std {
template <> class numeric_limits<UnixTime> {
public:
  static UnixTime max();
  static UnixTime min();
};
}

#include "unix_time_impl.h"
