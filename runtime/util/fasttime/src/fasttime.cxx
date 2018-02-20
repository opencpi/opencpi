
/* $Id: fasttime.c,v 1.11 2005/08/27 08:24:45 alexholkner Exp $ 
 *
 * Copyright (c) Internet2, 2005.  All rights reserved.
 * See LICENSE file for conditions.
 */

/* Library API for fasttime
 *
 */
//#include "rcs.h"
//RCS_ID("@(#) $Id: fasttime.c,v 1.11 2005/08/27 08:24:45 alexholkner Exp $")

#define _XPG4_2
#define __EXTENSIONS__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <fasttime.h>
#include "fasttime_private.h"
#include "calibration.h"

/* Opaque context structure */
struct _fasttime_context_t {
    int method;         /* Method in use -- one of FASTTIME_METHOD_* */
    union {
        struct {
            fasttime_buffers_t *storage;
            int sem;
        } daemon;
        struct {
            fasttime_buffers_t *storage;
            char terminate;
            char ready;
        } client;
    }; 
};

/* Default context when none is specified */
struct _fasttime_context_t default_context;

/* Client thread (when daemon is not used) */
void *client_thread(void *arg);

fasttime_context_t fasttime_create_context()
{
  fasttime_context_t context;
  context = (fasttime_context_t)malloc(sizeof *context);
  context->method = FASTTIME_METHOD_ERROR;
  return context;
}

void fasttime_destroy_context(fasttime_context_t context)
{
    if (context->method & FASTTIME_METHOD_CLIENT)
        context->client.terminate = 1;
    else
        free(context);
}

fasttime_context_t fasttime_get_default_context()
{
    return &default_context;
}

int fasttime_init_context(fasttime_context_t context, int methods)
{
    int shmid = -1;         /* ID of shared memory segment */

    /* Use default context if unspecified */
    if (!context)
        context = &default_context;
   
    /* Try to init with daemon */
    if (methods & FASTTIME_METHOD_DAEMON)
    {
        shmid = shm_open(SHM_NAME, O_RDONLY, 0444);

        /* Map shared memory if daemon was found */
        if (shmid != -1)
        {
          context->daemon.storage = (fasttime_buffers_t*)
                mmap(NULL, sizeof *context->daemon.storage, PROT_READ,
                     MAP_SHARED, shmid, 0);
            context->method = FASTTIME_METHOD_DAEMON | 
                (methods & FASTTIME_METHOD_SYSTEM);
            return context->method;
        }
        
        /* ... otherwise fall through to another method */
    }

    /* Try to init with client thread */
    if (methods & FASTTIME_METHOD_CLIENT)
    {
        pthread_t thread;
        
        context->client.terminate = 0;
        context->client.ready = 0;
        pthread_create(&thread, NULL, client_thread, context);
        pthread_detach(thread);
        context->method = FASTTIME_METHOD_CLIENT |
            (methods & FASTTIME_METHOD_SYSTEM);
        return context->method;
    }

    /* Try to init with system-call fallback */
    if (methods & FASTTIME_METHOD_SYSTEM)
    {
        context->method = FASTTIME_METHOD_SYSTEM;
        return context->method;
    }

    /* None of the desired methods could be initialised, return error */
    return FASTTIME_METHOD_ERROR;
}

int fasttime_gettime_context(fasttime_context_t context, 
                                  struct timespec *tp)
{
    tick_t ticks;
    unsigned int conversion_index;
    conversion_t *conversion;
    int64_t nsecs;
    fasttime_t *current = NULL;

    if (!context)
        context = &default_context;

    switch (context->method)
    {
        case FASTTIME_METHOD_SYSTEM:
          return clock_gettime(CLOCK_REALTIME, tp);

        case FASTTIME_METHOD_DAEMON:
        case FASTTIME_METHOD_DAEMON | FASTTIME_METHOD_SYSTEM:
            current = &context->daemon.storage->buffer
                        [context->daemon.storage->buffer_idx 
                          % FASTTIME_BUFFERS];
            break;

        case FASTTIME_METHOD_CLIENT:
        case FASTTIME_METHOD_CLIENT | FASTTIME_METHOD_SYSTEM:
            if (context->client.ready)
                current = &context->client.storage->buffer
                        [context->client.storage->buffer_idx 
                          % FASTTIME_BUFFERS];
            break;

        default:
            return -1;
    }

    if (!current || !current->valid)
    {
        /* The preferred daemon or client calibration is not available
         * at the moment, fall back on system if allowed.
         */
        if (context->method & FASTTIME_METHOD_SYSTEM)
          return clock_gettime(CLOCK_REALTIME, tp );
        else
            return -1;
    }

    /* Determine which conversion to use (which piecewise linear
     * segment) */
    get_tick_count(&ticks);
    conversion_index = (unsigned)((ticks.ll - current->tick_first) >> 
				  current->log_ticks_per_segment);

    /* If daemon has fallen behind, just use the most recent conversion */
    if (conversion_index >= current->linear_segments)
        conversion_index = current->linear_segments - 1;

    conversion = &current->conversions[conversion_index];

    /* Interpolate time */
    nsecs = (int64_t)(ticks.ll * conversion->gradient + conversion->intercept);

    tp->tv_sec = (long) (nsecs / 1000000000);
    tp->tv_nsec = (long) (nsecs - tp->tv_sec * (int64_t) 1000000000);

    return 0;
}

/* Wrapper for get_tick_count for user apps */
uint64_t fasttime_getticks()
{
    tick_t ticks;
    get_tick_count(&ticks);
    return ticks.ll;
}

/* Convert time from ticks to timespec XXX Currently unsupported */
int fasttime_make_timespec(fasttime_context_t context,
                          uint64_t ticks_in,
                          struct timespec *tp_out)
{
    ( void ) ticks_in;
    ( void ) tp_out; 
    /*
    fasttime_t *current;
    uint64_t nsecs;
    */

    if (!context)
        context = &default_context;

    switch (context->method)
    {

        /*
        case FASTTIME_METHOD_DAEMON:
            current = &context->daemon.storage->buffer
                        [context->daemon.storage->buffer_idx 
                          % FASTTIME_BUFFERS];
            break;

        case FASTTIME_METHOD_CLIENT:
            current = &context->client.storage;
            break;
*/
        default:
            return -1;
    }

    /*
      nsecs = ticks_in * current->gradient + current->intercept; 
    tp_out->tv_sec = (long) (nsecs / 1000000000);
    tp_out->tv_nsec = (long) (nsecs - (uint64_t) tp_out->tv_sec * 1000000000);
    */

    return 0;
}

int fasttime_getstatistics(fasttime_context_t context,
                           fasttime_statistics_t *stats)
{
    fasttime_t *current;

#ifndef RECORD_STATISTICS
    return -1;
#endif

    if (!context)
        context = &default_context;

    switch (context->method & ~FASTTIME_METHOD_SYSTEM)
    {
        case FASTTIME_METHOD_DAEMON:
            current = &context->daemon.storage->buffer
                        [context->daemon.storage->buffer_idx 
                          % FASTTIME_BUFFERS];
            stats->ready_time = context->daemon.storage->ready_time;
            break;

        case FASTTIME_METHOD_CLIENT:
            if (!context->client.ready)
                return -1;
            current = &context->client.storage->buffer
                        [context->client.storage->buffer_idx 
                          % FASTTIME_BUFFERS];
            stats->ready_time = context->client.storage->ready_time;
            break;

        default:
            return -1;
    }

    stats->ready = current->valid;

    if (!current->valid)
    {
        stats->loop_delay = 0;
        stats->stability = 0;
        stats->offset = 0;
    }
    else
    {
        stats->loop_delay = current->loop_delay;
        stats->stability = current->stability;
        stats->offset = current->offset;
    }

    return 0;
}

/* Run loop calibration in a separate thread.  Besides not requiring
 * a special shared memory segment, this is identical to the daemon
 * loop.  Arg is a fasttime_context_t.
 */
void *client_thread(void *arg)
{
    int i;
    int next_idx;
    fasttime_t *next_storage;
    
    fasttime_context_t context = (fasttime_context_t) arg;
    fasttime_buffers_t *ft_storage;

    context->client.storage = (fasttime_buffers_t*)malloc(sizeof *context->client.storage);
    ft_storage = context->client.storage;

    ft_storage->buffer_idx = 0;
    for (i = 0; i < FASTTIME_BUFFERS; i++)
        ft_storage->buffer[i].valid = 0;

    debug_calibrate = 0;
    calibrate_set_defaults();
    ft_storage->ready_time = calibrate_get_ready_time();

    /* Valid flags and buffer index are all set, so no problems if
     * client tries to read from now on
     */
    context->client.ready = 1;

    calibrate_init();

    for (;;)
    {
        if (!context->client.terminate)
            sleep(loop_delay);
        if (context->client.terminate)
            break;

        next_idx = (ft_storage->buffer_idx + 1) % FASTTIME_BUFFERS;
        next_storage = &ft_storage->buffer[next_idx];

        calibrate(next_storage);

        atomic_inc(&ft_storage->buffer_idx);
    }

    /* Terminate thread -- only called from destroy_context */
    free(context);
    return NULL;
}




/*
 *   THIS SECTION WAS ADDED FOR OCPI
 */



void setTimeBase( TimeBase tb )
{
  ( void ) tb;
  /*
 switch( tb ) {

 case FTTB_System:
 case FTTB_FPGA:
 case FTTB_GPS:


 }
  */
}


uint64_t getBinaryTime()
{

  return 0;
}


int fasttime_clock_gettime( clockid_t clk_id, struct timespec * tp)
{
  ( void ) clk_id;
  ( void ) tp;  
  return 0;
}

