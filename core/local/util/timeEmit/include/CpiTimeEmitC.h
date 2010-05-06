// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.


/**
 * \file
 * \brief Event Trace classes
 *
 * Revision History:
 *
 *     08/19/2009 - John F. Miller
 *                  Initial version.
 */


#ifndef CPI_EVENT_TRACE_H_
#define CPI_EVENT_TRACE_H_

#ifdef _c_plus_plus
extern "C" {
#endif

int CpiTimeARegister( char* signal_name );
void CpiEmit( int sig );


#define CPI_TIME_EMIT_C( name ) \
{\
  static int sig = -1; \
  if ( sig == -1 ) {                                \
    sig = CpiTimeARegister( name );        \
  } \
  CpiEmit( sig );\
}

#ifdef _c_plus_plus
};
#endif

#endif
