
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */



#ifndef INCLUDED_TIME_UTILS_H
#define INCLUDED_TIME_UTILS_H

#include <stdio.h>

#define USE_TIMEBASE

#ifdef USE_TIMEBASE


#if defined( __GNUC__ )

  #define TBU( t ) __asm volatile ("mfspr %0,269" : "=r" ( t ) )
  #define TBL( t ) __asm volatile ("mfspr %0,268" : "=r" ( t ) )

#else

  #define TBU( t ) tm_move_from_tbr_upper ( t )
  #define TBL( t ) tm_move_from_tbr_lower ( t )

void tm_move_from_tbr_upper ( unsigned int time_val );
void tm_move_from_tbr_lower ( unsigned int time_val );

asm void tm_move_from_tbr_upper ( unsigned int time_val )
{
%reg time_val
  mfspr time_val,269;
}

asm void tm_move_from_tbr_lower ( unsigned int time_val )
{
%reg time_val
  mfspr time_val,268;
}
#endif 

typedef unsigned long long Timespec;

static void get_timestamp ( Timespec* ts )
{
  register unsigned int tb_lower = 0,
                        tb_upper = 0,
                        tb_upper_tmp = 0;

  unsigned long long timestamp;

  do
  {
    TBU( tb_upper );
    TBL( tb_lower );
    TBU( tb_upper_tmp );
  }
  while ( tb_upper != tb_upper_tmp );  

  timestamp = tb_upper;
  
  timestamp <<= 32;
  
  timestamp |= tb_lower;
  
  *ts = timestamp;
  
  return;
}

static double elapsed_usecs ( Timespec* p_ts_beg,
                              Timespec* p_ts_end )
{
  const double timebase = 66666666.66666666; /* 66 MHz = 15 nsec period */

  const double nanos_per_second = 1.0E9;
  
  double elapsed_time;

  double elapsed_nanos;

  double elapsed_secs;

  unsigned long long elapsed_full_secs;

  unsigned long long elapsed_ticks;
                    
  elapsed_ticks = *p_ts_end - *p_ts_beg;
  
  elapsed_secs = ( elapsed_ticks / timebase );

  elapsed_full_secs = ( unsigned long long ) elapsed_secs;

  elapsed_nanos = ( elapsed_secs - elapsed_full_secs ) * nanos_per_second;

  elapsed_time = ( ( ( double ) elapsed_full_secs * 1.0E6 ) +
                   ( ( double ) elapsed_nanos     / 1.0E3 ) );
 
  return  elapsed_time;
}

#else /* Use system clocks */

#include <time.h>

typedef struct timespec Timespec;
                              
#define get_timestamp( ts ) clock_gettime ( CLOCK_REALTIME, ( ts ) );
                              
static double elapsed_usecs ( Timespec* p_ts_beg,
                              Timespec* p_ts_end )
{
  double usecs_beg;
  double usecs_end;
  
  const double nanos_per_usecs = 1000.0;

  const double usecs_per_second = 1000000.0;
  
  usecs_beg = ( ( double ) p_ts_beg->tv_sec * usecs_per_second ) +
              ( ( double ) p_ts_beg->tv_nsec / nanos_per_usecs );

  usecs_end = ( ( double ) p_ts_end->tv_sec * usecs_per_second ) +
              ( ( double ) p_ts_end->tv_nsec / nanos_per_usecs );

  return ( usecs_end - usecs_beg );
}                                   
#endif 

static void check_timer ( void )
{
  int n;

  double usecs;
  
  Timespec ts_beg;
  Timespec ts_end;

  for ( n = 1; n <= 4; n++ )
  {
    get_timestamp( &ts_beg );
  
  /*   sleep ( n ); */
  
    get_timestamp( &ts_end );
  
    usecs = elapsed_usecs ( &ts_beg, &ts_end );
    
    printf ( "Expected %2d actual %16.8f\n", n, usecs / 1000000.0 );
  }
  
}

#endif /* End: #ifndef INCLUDED_TIME_UTILS_H */


