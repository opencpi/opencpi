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

/**
 * \file
 * \brief Event Trace classes
 *
 * Revision History:
 *
 *     08/19/2009 - John F. Miller
 *                  Initial version.
 */


#ifndef OCPI_EVENT_TRACE_H_
#define OCPI_EVENT_TRACE_H_

#ifdef _c_plus_plus
extern "C" {
#endif

int OcpiTimeARegister( const char* signal_name );
void OcpiEmit( int sig );


#define OCPI_TIME_EMIT_C( name ) \
{\
  static int sig = -1; \
  if ( sig == -1 ) {                                \
    sig = OcpiTimeARegister( name );        \
  } \
  OcpiEmit( sig );\
}

#ifdef _c_plus_plus
};
#endif

#endif
