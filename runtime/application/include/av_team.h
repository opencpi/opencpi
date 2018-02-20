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

/* Top-level header that brings together items from autoconf, etc... */
#ifndef HAVE_AV_TEAM_H
#define HAVE_AV_TEAM_H

// Bring in architecture-specific configuration from <CDK>/platforms/<platform>/
#include "av_config.h"
// These undefine stupid autoconf macros when cross-compiling:
#undef malloc
#undef realloc

// Other attributes to consider... https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html
#define OCPI_NORETURN __attribute__((noreturn))
#define OCPI_USED     __attribute__((used))
#define OCPI_UNUSED   __attribute__((unused))

// clang and GCC above 4.5 allow custom messages
#if defined(clang) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
  #define OCPI_API_DEPRECATED(ver, msg) __attribute__((deprecated(msg " (Removal expected in version " ver ")")))
#else
  #define OCPI_API_DEPRECATED(ver, msg) __attribute__((deprecated))
#endif

#endif // HAVE_AV_TEAM_H
