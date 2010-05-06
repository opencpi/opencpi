/* $Id: fasttime.h.in,v 1.3 2005/08/27 08:24:45 alexholkner Exp $ 
 *
 * Copyright (c) Internet2, 2005.  All rights reserved.
 * See LICENSE file for conditions.
 */

/* Library for generating accurate timestamps quickly.
 *
 * The library must be initialised before use.  If the fasttimed daemon
 * is running at initialisation it will be used to generate
 * timestamps, otherwise the client library can either perform
 * its own calibration or fall-back to using the system call.
 * 
 * If the path of the daemon executable is known it should be
 * passed in to the init function (this is required to establish
 * an IPC session; the daemon will not actually be started).
 * NULL can be passed in to search the standard locations "." and
 * "/usr/local/bin".
 *
 * Once initialised, clients may call fasttime_gettimeofday(), passing
 * in a timeval struct (described in the gettimeofday(3) man page).
 *
 * For convenience and diagnostics clients may also access the value
 * of the TSC register directly, using fasttime_gettickcount().
 */

#ifndef FASTTIME_H
#define FASTTIME_H

#include <stdint.h>
#include <sys/time.h>
#include <time.h>

/* Allow inlining */
#define INLINE inline

#define FASTTIME_METHOD_ERROR  0
#define FASTTIME_METHOD_DAEMON 1
#define FASTTIME_METHOD_CLIENT 2
#define FASTTIME_METHOD_SYSTEM 4
#define FASTTIME_METHOD_BEST   (FASTTIME_METHOD_DAEMON | \
                                FASTTIME_METHOD_CLIENT | \
                                FASTTIME_METHOD_SYSTEM)

/* Context is an opaque structure */
typedef struct _fasttime_context_t *fasttime_context_t;

/* Create a context for later use.  This is generally not required,
 * but may be of use to applications needing access to several
 * methods simultaneously.
 */
fasttime_context_t fasttime_create_context();

/* Deallocate a context previously created. */
void fasttime_destroy_context(fasttime_context_t context);

/* Get the default static context (useful?) */
fasttime_context_t fasttime_get_default_context();

/* Initialise a context with options.
 *
 * context          A context previously created with 
 *                  fasttime_create_context(), or NULL to use the default
 *                  static context.
 *
 * methods          Logical OR of the allowed methods.  For example,
 *                  pass in (FASTTIME_METHOD_DAEMON |
 *                  FASTTIME_METHOD_CLIENT) to use only the daemon or
 *                  client methods.  If none of the supplied methods
 *                  can be used, the init fails and returns
 *                  FASTTIME_METHOD_ERROR.  Pass in
 *                  FASTTIME_METHOD_BEST to select any method.
 *                  
 * daemon_path      Path to fasttimed (including the name of the executable), 
 *                  used to establish an IPC session.  If NULL, the
 *                  default locations "./fasttimed" and
 *                  "/usr/local/bin/fasttimed" will be tried.
 *
 * Returns:
 *      The method selected, or FASTTIME_METHOD_ERROR.
 */
int fasttime_init_context(fasttime_context_t context, int methods);

/* Initialise the fasttime library using the default context and
 * the best available method.  The daemon will only be found if
 * it is at "./fasttimed" or "/usr/local/bin/fasttimed".
 * Returns the method that will be used.
 */
static INLINE int fasttime_init()
{
    return fasttime_init_context(NULL, FASTTIME_METHOD_BEST);
}

/* Obtain the current time of day using the given context. */
int fasttime_gettime_context(fasttime_context_t context,
                             struct timespec *tp);

/* Obtain the current time of day using the default context.  The
 * time is recorded in a timespec struct, as used by the gettimeofday()
 * system call.  Returns 0 on success, -1 if there was an error.
 */
static INLINE int fasttime_gettime(struct timespec *tp)
{
  return fasttime_gettime_context(NULL, tp);
}


/* Obtain the current value of the TSC register (tick counter) */
uint64_t fasttime_getticks();

/* Convert time from ticks to timespec.  context may be NULL to use
 * default context.  Return 0 on success, -1 on error.
 */
int fasttime_make_timespec(fasttime_context_t context,
                          uint64_t ticks_in, 
                          struct timespec *tp_out);

/* Retrieve statistics for the current reading */
typedef struct _fasttime_statistics_t
{
    int loop_delay;
    double stability;
    int offset;
    int ready;
    time_t ready_time;
} fasttime_statistics_t;

/* The structure will be filled in.  Returns 0 on success */
int fasttime_getstatistics(fasttime_context_t context,
                           fasttime_statistics_t *stats);





/*
 *   THIS SECTION WAS ADDED FOR CPI
 */
enum TimeBase {
  FTTB_System,
  FTTB_FPGA,
  FTTB_GPS
};

void setTimeBase( TimeBase tb=FTTB_System );
uint64_t getBinaryTime();






#endif
