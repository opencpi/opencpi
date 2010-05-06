/* $Id: fasttime_test.c,v 1.4 2005/08/27 08:24:44 alexholkner Exp $ 
 *
 * Copyright (c) Internet2, 2005.  All rights reserved.
 * See LICENSE file for conditions.
 */

/* Simple test program that outputs a sample reading from fasttime
 * and compares it with gettimeofday, and also times the execution
 * of 1 million runs and compares the performance with gettimeofday.
 */

#include "rcs.h"
#include <stdlib.h>
#include <stdio.h>
#include <fasttime.h>
#include <CpiOsMisc.h>

#define RUNS 1000000

int main()
{
    struct timespec tp_fast, tp_actual;
    struct timespec tp_start, tp_end;
    unsigned long long fast_time, actual_time;
    int i;
    int method;
    int result;
    int wait_time;
    fasttime_statistics_t stats;
    
    method = fasttime_init_context(NULL, 
                 FASTTIME_METHOD_CLIENT | FASTTIME_METHOD_DAEMON);
    switch (method & ~FASTTIME_METHOD_SYSTEM)
    {
        case FASTTIME_METHOD_CLIENT:
            printf("Using client calibration");
            break;

        case FASTTIME_METHOD_DAEMON:
            printf("Using daemon calibration");
            break;

        default:
            printf("Error in init; quitting\n");
            return EXIT_FAILURE;
    }

    if (method & FASTTIME_METHOD_SYSTEM)
        printf(" with system fallback.\n");
    else
        printf(".\n");

    /* Check availability */
    while (fasttime_getstatistics(NULL, &stats) != 0)
        ;

    if (!stats.ready)
    {
      clock_gettime(CLOCK_REALTIME, &tp_actual);
      wait_time = stats.ready_time - tp_actual.tv_sec;
        if (wait_time > 0)
        {
            printf("Waiting %d secs for fasttime to get ready...\n", wait_time);
            CPI::OS::sleep(wait_time);
        }
    }
    
    printf("Check accuracy:\n");
    do {
      clock_gettime(CLOCK_REALTIME, &tp_actual); 
      result = fasttime_gettime(&tp_fast);  
    } while (result);
    printf(" Fast:   %u secs, %u nsecs\n", tp_fast.tv_sec, tp_fast.tv_nsec);
    printf(" Actual: %u secs, %u nsecs\n", tp_actual.tv_sec, tp_actual.tv_nsec); 
    printf("Check speed:\n");
    fasttime_gettime(&tp_start);
    for (i = 0; i < RUNS; i++)
        fasttime_gettime(&tp_end);  
    fast_time = ((uint64_t) (tp_end.tv_sec - tp_start.tv_sec)) * 1000000000 +
                            (tp_end.tv_nsec - tp_start.tv_nsec);

    fasttime_gettime(&tp_start);
    for (i = 0; i < RUNS; i++)
      clock_gettime(CLOCK_REALTIME, &tp_end ); 
    actual_time = ((uint64_t) (tp_end.tv_sec - tp_start.tv_sec)) * 1000000000 +
                              (tp_end.tv_nsec - tp_start.tv_nsec);
    
    printf(" Fast:   %f secs\n", (double) fast_time / 1000000000.0);
    printf(" Actual: %f secs\n", (double) actual_time / 1000000000.0);
    
    printf("Improvement: %f times faster\n", 
        (double) actual_time / fast_time);

    return EXIT_SUCCESS;
}
