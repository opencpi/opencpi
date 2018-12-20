/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// These calls are not OS specific, but may be CPU specific - hopefully captured
// in the "cycle.h" file from fftw.org

// Get best real time
// Could be from our FPGA, could be from fast time, could be from OS.
// Get fastest tick time
// Get fastest tick and our time together.

// So we have our time source:
// - initialize from system to FPGA, then CPU.
// - initialize time from FPGA + GPS



#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#if defined(__arm__) || defined(__ARM_ARCH)
static inline uint64_t getticks()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  
  return
    ((uint64_t)tv.tv_usec << 32) |
    ((tv.tv_usec * ((uint64_t)0x100000000ull + 500))/1000);
}
#else
#include "cycle.h" // This is an externally supplied header file from fftw.org
#endif

class Time {
public:
  static inline uint64_t getTicks() { return getticks(); }

  typedef uint64_t GpsTime, UtcTime, OcpiTime;
  static GpsTime utc2gps(time_t utc);
  static UtcTime gps2utc(GpsTime gps);
};
