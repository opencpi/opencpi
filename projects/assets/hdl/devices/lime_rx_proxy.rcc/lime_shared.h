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

#ifndef _LIME_SHARED_H_
#define _LIME_SHARED_H_
#include <stdint.h>
// Shared functions between lime rx and tx proxies.
namespace OCPI {
  namespace Lime {
  struct Divider {
    uint8_t nint, nfrac_hi, nfrac_mid, nfrac_lo;
  };
  void calcDividers(double pll_hz, double lo_hz, Divider &div);
  // Return a string error if there was one.  Otherwise NULL
  const char
    *setVcoCap(uint8_t (*readVtune)(void *arg), void (*writeVcoCap)(void *arg, uint8_t val),
	      void *arg),
    *getLpfBwValue(float hz, uint8_t &regval),
    *getFreqValue(float hz, uint8_t &regval);
  }
}
#endif
