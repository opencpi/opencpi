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
 * \brief Time Analyzer classes
 *
 * Revision History:
 *
 *     04/2/2012 - John F. Miller
 *                  Initial version.
 */


/*

    Environment options:

    // Coarse catagory control
    "OCPI_TIME_EMIT_CAT"


*/


#ifndef OCPI_TIME_CATEGORIES_H_
#define OCPI_TIME_CATEGORIES_H_

#include <OcpiTimeEmit.h>

#define OCPI_EMIT_CAT_TUNING      1   // Internal timing events used for tuning OCPI

#define OCPI_EMIT_CAT_TUNING_DP   1   // Data plane timing
#define OCPI_EMIT_CAT_TUNING_WC   2   // Worker control timing

#define OCPI_EMIT_CAT_APP_DEV     2   // Application Developer timing events

// Worker developer category
#define OCPI_EMIT_CAT_WORKER_DEV  4   // Worker Developer timing events

// Worker developer sub-cats
#define OCPI_EMIT_CAT_WORKER_DEV_BUFFER_FLOW   0x4
#define OCPI_EMIT_CAT_WORKER_DEV_BUFFER_VALUES 0x10
#define OCPI_EMIT_CAT_WORKER_DEV_RUN_TIME      0x8

#define OCPI_EMIT_NO_SUBCAT       ~0


#define OCPI_EMIT_CAT_( name, category, subcat ) \
  do { \
    if ( OCPI::Time::Emit::qualifyCategory( category, subcat ) ) OCPI_EMIT_( name ); \
  } while ( 0 );

#define OCPI_EMIT_CAT__( name, category, subcat, c )	\
  do { \
    if ( OCPI::Time::Emit::qualifyCategory( category, subcat ) ) OCPI_EMIT__( name, c ); \
  } while ( 0 );

#define OCPI_EMIT_CAT( name, category, subcat ) \
  do { \
    if ( OCPI::Time::Emit::qualifyCategory( category, subcat ) ) OCPI_EMIT( name ); \
  } while ( 0 );

#define OCPI_EMIT_STATE_CAT_NR_( re, state, category, subcat )	\
  do { \
    if ( OCPI::Time::Emit::qualifyCategory( category, subcat ) ) OCPI_EMIT_STATE_NR_( re, state ); \
  } while ( 0 );

#define OCPI_EMIT_STATE_CAT_NR__( re, state, category, subcat, c )	\
  do { \
    if ( OCPI::Time::Emit::qualifyCategory( category, subcat ) ) OCPI_EMIT_STATE_NR__( re, state, c ); \
  } while ( 0 );

#define OCPI_EMIT_STATE_CAT_NR( re, state, category, subcat )	\
  do { \
    if ( OCPI::Time::Emit::qualifyCategory( category, subcat ) ) OCPI_EMIT_STATE_NR( re, state ); \
  } while ( 0 );


#define OCPI_EMIT_VALUE_CAT_NR_( re, state, category, subcat )	\
  do { \
    if ( OCPI::Time::Emit::qualifyCategory( category, subcat ) ) OCPI_EMIT_STATE_NR_( re, state ); \
  } while ( 0 );

#define OCPI_EMIT_VALUE_CAT_NR__( re, state, category, subcat, c )	\
  do { \
    if ( OCPI::Time::Emit::qualifyCategory( category, subcat ) ) OCPI_EMIT_STATE_NR__( re, state, c ); \
  } while ( 0 );

#define OCPI_EMIT_VALUE_CAT_NR( re, state, category, subcat )	\
  do { \
    if ( OCPI::Time::Emit::qualifyCategory( category, subcat ) ) OCPI_EMIT_STATE_NR( re, state ); \
  } while ( 0 );


#endif
