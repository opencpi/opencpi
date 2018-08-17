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

#ifndef OCPI_CONFIG_API_H
/**********************************************************************************************
 * This file contains widely used definitions that are purposely exposed to the API for
 * workers and the ACI.  Since this impinges on user code and requires documentation it should
 * be limited to what is truly necessary, maintainable, supportable etc.
 * It carries the "published API burden".
 */

// Since the purpose of these macros is to communicate deprecation to the user, this must be here

// clang and GCC above 4.5 allow custom messages
#if defined(clang) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
  #define OCPI_API_DEPRECATED(ver, msg) __attribute__((deprecated(msg " (Removal expected in version " ver ")")))
#else
  #define OCPI_API_DEPRECATED(ver, msg) __attribute__((deprecated))
#endif
#endif
