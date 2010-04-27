
/* ****

  This file is licensed from, and is a trade secret of:

    Mercury Computer Systems, Incorporated
    199 Riverneck Road
    Chelmsford, Massachusetts 01824-2820
    United States of America
    Telephone + 1 978 256-1300
    Telecopy/FAX + 1 978 256-3599
    US Customer Support (800) 872-0040

  Refer to your Software License Agreements for provisions on use,
  duplication, third party rights and disclosure. This file: (a) was
  developed at private expense, and no part was developed with government
  funds, (b) is a trade secret of Mercury Computer Systems, Inc. for the
  purposes of the Freedom of Information Act, (c) is "commercial computer
  software" subject to limited utilization as provided in the above noted
  Software License Agreements and (d) is in all respects data belonging
  exclusively to either Mercury Computer Systems, Inc. or its third party
  providers.

  Copyright (c) Mercury Computer Systems, Inc., Chelmsford MA., 1984-2007,
  and all third party embedded software sources. All rights reserved under
  the Copyright laws of US. and international treaties.

************************************************************************** */

#ifndef INCLUDED_PAG_TIMER_H
#define INCLUDED_PAG_TIMER_H

#ifdef __cplusplus
  extern "C" {
#endif

#include <stdio.h>

#include <string.h>

#include <stdint.h>

/* ---- Timer structures ------------------------------------------------- */

/* Timestamp timer snapshot */
typedef struct pag_timer_timestamp_s
{
  uint32_t hi;
  /* Upper 32-bits of a 64-bit timestamp */

  uint32_t lo;
  /* Lower 32-bits of a 64-bit timestamp */

} PAG_timer_timestamp;


 /* Elapsed time */
typedef struct pag_timer_elapsed_s
{
  uint32_t seconds;
  /* Completely elapsed seconds */

  uint32_t nanoseconds;
  /* Elapsed nanoseconds of the partially elapsed second */

} PAG_timer_elapsed;


/* ****

  Take a timestamp.

************************************************************************** */

static void pag_timer_timestamp ( PAG_timer_timestamp* p_timestamp )
{
  register uint32_t tb_lower = 0,
                    tb_upper = 0,
                    tb_upper_tmp = 0;

  /* ---- Loop to ensure that a consistent pair of values is obtained ---- */

#if defined ( __PPC__ ) || ( __PPC64__ )

  __asm volatile ( "1:\n"
                   "  mftbu %0;\n"      /* Load from TBU to orig */
                   "  mftb  %1;\n"      /* Load from TBL */
                   "  mftbu %2;\n"      /* Load from TBU to temp */
                   "  cmpw %2, %0;\n"   /* Compare TBU orig to TBU temp */
                   "bne 1b;\n"
                   :
                   "=r" ( tb_upper ),
                   "=r" ( tb_lower ),
                   "=r" ( tb_upper_tmp )
                 : );

#elif defined ( __x86_64__ ) || defined ( __i386__ )

  ( void ) tb_upper_tmp;

  __asm__ __volatile__ ( "rdtsc" : "=a"( tb_lower ), "=d" ( tb_upper ) );

#else

  #error "pag_timer_timestamp(): Not supported."

#endif

  p_timestamp->hi = tb_upper;
  p_timestamp->lo = tb_lower;

  return;
}


/* ****

  Get the timebase value.

************************************************************************** */

#define PAG__TIMER_MAX_FILE_NAME_LEN ( 256 )

static uint32_t pag__timer_get_timebase ( void )
{
  int32_t timebase = 0;

  FILE * fp = 0;

  char * p_match_cpu_mhz = 0;
  char * p_match_timebase = 0;

  char junk0 [ PAG__TIMER_MAX_FILE_NAME_LEN ];
  char junk1 [ PAG__TIMER_MAX_FILE_NAME_LEN ];
  char input [ PAG__TIMER_MAX_FILE_NAME_LEN ];

  /* ---- Get the value of the timebase register ------------------------- */

  /* Open the cpuinfo file */

  fp = fopen ( "/proc/cpuinfo", "r" );

  if ( fp == 0 )
  {
    perror ( "fopen ( /proc/cpuinfo )" );
    return ( 0.0 );
  }

  /* ---- Look for the special string. ----------------------------------- */

  while ( ( fgets ( input, PAG__TIMER_MAX_FILE_NAME_LEN, fp ) ) != 0 )
  {
    p_match_cpu_mhz = strstr ( input, "cpu MHz" );

    p_match_timebase = strstr ( input, "timebase" );

    if ( ( p_match_timebase != 0 ) || ( p_match_cpu_mhz != 0 ) )
    {
      break; /* Found a match */
    }

  } /* End: while ( ( fgets ( sys_info, PAG__TIMER_MAX_FILE_NAME_LEN, fp...*/

  /* ---- Close the cpuinfo file ----------------------------------------- */

  fclose ( fp );

  /* ---- Check if we found the timbase string --------------------------- */

  if ( p_match_timebase != 0 )
  {
    sscanf ( input,"%s %s %d ",
             junk0,
             junk1,
             &timebase );
  }
  else if ( p_match_cpu_mhz != 0 )
  {
    float cpu_freq = 0.0;

    char junk2 [ PAG__TIMER_MAX_FILE_NAME_LEN ];

    sscanf ( input,"%s %s %s %f ",
             junk0,
             junk1,
             junk2,
             &cpu_freq );
#if 0
    cpu_freq += 1000.0; /* HACK for IBM machines that lie */
#endif 
    timebase = ( uint32_t ) ( cpu_freq * 1000000.0 );
  }
  else
  {
    printf ( "\nError: %s:%d : unable to find timbase.\n",
             __FILE__,
             __LINE__ );
    return ( 0.0 );
  }

  /* ---- Extract and record the timebase value -------------------------- */

  sscanf ( input,"%s %s %d ",
           junk0,
           junk1,
           &timebase );

  return ( timebase );
}


/* ****

  Compute the difference between two time stamps

************************************************************************** */

static void pag_timer_elapsed ( PAG_timer_timestamp ts_beg,
                                PAG_timer_timestamp ts_end,
                                PAG_timer_elapsed* p_elapsed )
{
  double elapsed_secs;

  double elapsed_nanos;

  uint64_t elapsed_full_secs;

  uint64_t hi_beg;
  uint64_t lo_beg;

  uint64_t hi_end;
  uint64_t lo_end;

  uint64_t beg,
           end,
           elapsed_ticks;

  const double nanos_per_second = 1.0E9;

  static double timebase = 0.0;

  /* ---- First time through, get the timebase --------------------------- */

  if ( timebase == 0.0 )
  {
    timebase = ( double ) pag__timer_get_timebase ( );
  }

  /* ---- Create a 64-bit time value from hi and lo ---------------------- */

  hi_beg = ( ( uint64_t ) ts_beg.hi << 32 );
  lo_beg = ts_beg.lo;

  hi_end = ( ( uint64_t ) ts_end.hi << 32 );
  lo_end = ts_end.lo;

  beg = hi_beg | lo_beg;
  end = hi_end | lo_end;

  /* ---- Compute the elapsed clock ticks -------------------------------- */

  elapsed_ticks = end - beg;

  /* ---- Convert the elapsed clock ticks into seconds and nanoseconds --- */

  elapsed_secs = ( elapsed_ticks / timebase );

  elapsed_full_secs = ( uint64_t ) elapsed_secs;

  elapsed_nanos = ( elapsed_secs - elapsed_full_secs ) * nanos_per_second;

  p_elapsed->seconds = ( uint32_t ) elapsed_full_secs;

  p_elapsed->nanoseconds = ( uint32_t ) elapsed_nanos;

  return;
}

/* ****

  Quick timer sanity check.

************************************************************************** */

#include <unistd.h>

#include <sys/time.h>

static void pag_timer_check ( void )
{
  PAG_timer_timestamp ts_beg;
  PAG_timer_timestamp ts_end;
  PAG_timer_elapsed elapsed;

  struct timeval tv_beg;
  struct timeval tv_end;
  struct timeval tv_elp;

  uint32_t n;

  for ( n = 1; n <= 4; n++ )
  {
    uint32_t sleep_time = n;

    gettimeofday ( &tv_beg, 0 );
    pag_timer_timestamp ( &ts_beg );

    sleep ( sleep_time );

    pag_timer_timestamp ( &ts_end );
    gettimeofday ( &tv_end, 0 );

    pag_timer_elapsed ( ts_beg, ts_end, &elapsed );

    tv_elp.tv_sec = tv_end.tv_sec - tv_beg.tv_sec;
    tv_elp.tv_usec = tv_end.tv_usec - tv_beg.tv_usec;
    if ( tv_elp.tv_usec < 0 )
    {
      --tv_elp.tv_sec;
      tv_elp.tv_usec += 1000000;
    }

    printf ( "\n\nTimer test:\n"
             "  Expected %2d seconds\n"
             "  Actual   %2d seconds %d nanoseconds\n\n",
             sleep_time,
             elapsed.seconds,
             elapsed.nanoseconds );

 } /* End: for ( n = 0; n <= 4; n++ ) */

  return;
}

#ifdef __cplusplus
  }
#endif

#endif /* INCLUDED_PAG_TIMER_H */

