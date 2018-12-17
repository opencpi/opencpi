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

#ifndef _AD9361_COMMON
#define _AD9361_COMMON

// AD9361_Register_Map_Reference_Manual_UG-671.pdf refers to register bits
// as D7 - D0
#define BITMASK_D7 0x80
#define BITMASK_D6 0x40
#define BITMASK_D5 0x20
#define BITMASK_D4 0x10
#define BITMASK_D3 0x08
#define BITMASK_D2 0x04
#define BITMASK_D1 0x02
#define BITMASK_D0 0x01

struct regs_general_rfpll_divider_t {
  uint8_t general_rfpll_dividers;
};

struct regs_clock_bbpll_t {
  uint8_t clock_bbpll;
};

#define DEFINE_AD9361_SETTING(AD9361_setting_value, AD9361_setting_type) \
typedef AD9361_setting_type AD9361_setting_value##_t; \
AD9361_setting_value##_t AD9361_setting_value; \

#endif // _AD9361_COMMON
