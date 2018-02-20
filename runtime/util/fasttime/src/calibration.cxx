
/* $Id: calibration.c,v 1.12 2005/08/27 08:24:45 alexholkner Exp $ 
 *
 * Copyright (c) Internet2, 2005.  All rights reserved.
 * See LICENSE file for conditions.
 */

/* Calibrate a TSC conversion table for either the fasttimed daemon
 * or a client process not relying on the daemon.
 */

//#include "rcs.h"
//RCS_ID("@(#) $Id: calibration.c,v 1.12 2005/08/27 08:24:45 alexholkner Exp $")

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <inttypes.h>
#include "fasttime_private.h"
#include "calibration.h"

#define ABS(x) \
    (x >= 0 ? x : -x)

/* Number of samples to take of the time before determining the
 * offset. */
#define SUBSAMPLES 10

/* How big an offset is considered "outlying"; if an offset exceeds
 * this value the normal small adjustments are not made, and a change
 * in state is usually made.
 */
#define OFFSET_OUTLIER 10000  /* 10ms */

/* The most recent CLOCK_FILTER_SIZE offsets are recorded; any which
 * exceeds CLOCK_FILTER_TOLERANCE times the median of these offsets
 * is discarded (but added to the filter).
 */
#define CLOCK_FILTER_SIZE 10
#define CLOCK_FILTER_TOLERANCE 5 
static int clock_filter(int offset);
int clock_filter_buffer[CLOCK_FILTER_SIZE];
int clock_filter_size;      /* size grows from 0 to CLOCK_FILTER_SIZE */
int clock_filter_index;     /* index into circular buffer */

double pll_gain;
#define DEFAULT_PLL_GAIN 0.1


int loop_delay;
int loop_delay_min;
int loop_delay_max;
int loop_delay_adjust;
int loop_delay_initial;
#define DEFAULT_LOOP_DELAY_MIN  15
#define DEFAULT_LOOP_DELAY_MAX  1000
#define DEFAULT_LOOP_DELAY_ADJUST  5
#define DEFAULT_LOOP_DELAY_INITIAL  15

int debug_calibrate;
    
void step_clock(fasttime_t *storage, 
                int64_t new_intercept,
                double new_gradient);
void slew_clock(fasttime_t *storage, 
                int intercept_adjust,
                double new_gradient,
                uint64_t sync);

/* Most recent */
static conversion_t current_conversion;
static int32_t offset = 0;
static double old_gradient = 0;
static int delay;
/*static int dispersion;*/
static double jitter;
static double stability;

/* Time of last calibration (ticks) */
static int64_t last_time;

static int state;
#define STATE_UNSET 0
#define STATE_SYNC 1
#define STATE_SPIKE 2

void calibrate_set_defaults()
{
    /* Default values for constants */
    pll_gain = DEFAULT_PLL_GAIN;
    loop_delay_min = DEFAULT_LOOP_DELAY_MIN;
    loop_delay_max = DEFAULT_LOOP_DELAY_MAX;
    loop_delay_adjust = DEFAULT_LOOP_DELAY_ADJUST;
    loop_delay_initial = DEFAULT_LOOP_DELAY_INITIAL;
}

time_t calibrate_get_ready_time()
{
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time );
    return current_time.tv_sec + 1 + loop_delay_initial * 2;
}

/* Do a quick calibration to determine an initial rate and offset
 * which can then be tuned.  We can't use any more advanced sampling
 * method at this stage, since we have no idea about the rate.
 */
void calibrate_init()
{
    struct timespec tp_start, tp_end;
    tick_t ticks_start, ticks_end;
    int valid_sample = 0;
   
    /* Get rate (ticks per microsecond) */
    while (!valid_sample)
    {
      clock_gettime( CLOCK_REALTIME, &tp_start );
      get_tick_count(&ticks_start);

        /* Long interval of time here ensures more accurate initial rate
         * estimate. */
        sleep(1); 

        clock_gettime(CLOCK_REALTIME, &tp_end );
        get_tick_count(&ticks_end);

        /* Disregard if ticks rolled over */
        if (ticks_end.ll < ticks_start.ll)
            continue;

        valid_sample = 1;
    }

    current_conversion.gradient = 
        ((tp_end.tv_sec * 1000000000.0 + tp_end.tv_nsec) -
         (tp_start.tv_sec * 1000000000.0 + tp_start.tv_nsec)) /
        (ticks_end.ll - ticks_start.ll);
    
    /* Get intercept */
    /* XXX the multiply by 1000000000 here might overflow */
    current_conversion.intercept = (int64_t) 
        ((tp_start.tv_sec * (uint64_t) 1000000000 + tp_start.tv_nsec)
        - ((double)ticks_start.ll * current_conversion.gradient));

    old_gradient = current_conversion.gradient;
    state = STATE_UNSET;
    offset = 0;
    last_time = ticks_end.ll;
    jitter = 0.0f;
    stability = 0.0f;

    loop_delay = loop_delay_initial;
    clock_filter_size = 0;
    clock_filter_index = -1;
}


void calibrate(fasttime_t *storage)
{
    struct timespec time2;
    tick_t ticks1, ticks3;
    int64_t t1, t2, t3;         /* Send, respond and receive times */

    int64_t min_dt1;
    int64_t min_dt2;

    int old_offset = offset;
    int epsil;
    int prediction;
    int correction;
    double gtemp;
    double old_jitter;
    double old_stability;
    int64_t mu;                 /* ticks since last calibration */
    int i;

    int intercept_adjust;
    double gradient_adjust;

    int64_t intercept_new;
    double gradient_new;

    assert(storage);

    /* Find current offset, delay and dispersion */
    //    min_dt1 = INT64_MAX;
    //    min_dt2 = INT64_MAX;

    min_dt2 = min_dt1 = 0x7fffffffffffffffLL;
    for (i = 0; i < SUBSAMPLES; i++)
    {
        /* Obtain t1, t2 and t3 */
        get_tick_count(&ticks1);
        clock_gettime(CLOCK_REALTIME, &time2 );
        get_tick_count(&ticks3);

        /* Disregard sample if ticks rolled over (unlikely) */
        if (ticks3.ll < ticks1.ll)
        {
            i--;
            continue;
        }

        /* Convert the timespec into microsecs measurement */
        t2 = time2.tv_sec * (uint64_t) 1000000000 + time2.tv_nsec;
        
        /* Convert the tick measurements to microsecs using the most recent
         * known conversion. */
        t1 = ( int64_t ) (current_conversion.intercept + 
                ticks1.ll * current_conversion.gradient);
        t3 = ( int64_t ) (current_conversion.intercept + 
                ticks3.ll * current_conversion.gradient);

        /* Keep minimum dt2 and dt1 */
        if (t2 - t1 < min_dt1)
            min_dt1 = t2 - t1;
        if (t3 - t2 < min_dt2)
            min_dt2 = t3 - t2;
    }
    
    /* mu: time in ticks between loop invocation */
    mu = ticks3.ll - last_time;
    offset = (int32_t)((min_dt1 - min_dt2) / 2);
    last_time = ticks3.ll;

    /* Calculate NTP-like stats */
    prediction = ( int ) ( old_offset + mu * (current_conversion.gradient - old_gradient));
    correction = (offset - prediction/2);
    delay = (int)(min_dt1 + min_dt2);
    /* dispersion = delay + PRECISION; */ /*  Or is this completely wrong? */
    old_jitter = jitter*jitter;
    epsil = offset - old_offset;
    jitter = sqrt(old_jitter + (epsil*epsil - old_jitter) / 4);

    /* Determine a rate and offset correction with PLL */
    gradient_adjust = pll_gain * correction / (double) mu;
    intercept_adjust = correction;

    /* Calculate stability */
    old_stability = stability*stability;
    gtemp = (offset - old_offset) / (double) (mu * old_gradient);
    stability = sqrt(old_stability + (gtemp*gtemp - old_stability) / 4);

    if (debug_calibrate)
    {
        printf("mu              %" PRIu64 "\n", mu);
        printf("offset          %d\n", offset);
        printf("prediction      %d\n", prediction);
        printf("correction      %d\n", correction);
        printf("jitter          %.12f\n", jitter);
        printf("stability       %.12f\n", stability);
        printf("intercept_adjust %d\n", intercept_adjust);
        printf("gradient_adjust  %.12f\n", gradient_adjust);
    }

    /* Clock filter */
    if (!clock_filter(offset))
    {
        if (debug_calibrate)
            printf("FILTERED\n");

        /* Offset is being filtered out, apply the last known
         * gradient and intercept */
        step_clock(storage, 
                   current_conversion.intercept,
                   current_conversion.gradient);
        return;
    }
    
    /* Apply a correction to the intercept: ensure that at some time
     * (the most recent reading) the old and new gradients intersect
     * the same point.  Without this, a change in gradient produces
     * a large error in offset.
     */
    gradient_new = current_conversion.gradient + gradient_adjust;
    intercept_new = (int64_t) 
        (ticks3.ll * (current_conversion.gradient - gradient_new) + 
          current_conversion.intercept + intercept_adjust);
    
    old_gradient = current_conversion.gradient;

    if (ABS(offset) > OFFSET_OUTLIER)
    {
        switch (state)
        {
            case STATE_SPIKE:
                /* This is the second spike in a row: step the clock,
                 * adjust rate.  Reset loop delay */
                state = STATE_SYNC;
                step_clock(storage, 
                           intercept_new,
                           gradient_new);
                loop_delay = loop_delay_initial;
                break;

            case STATE_SYNC:
                /* First outlier in otherwise normal operation;
                 * filter it.
                 */
                state = STATE_SPIKE;
                break;

            case STATE_UNSET:
                state = STATE_SYNC;
                break;
        }
    }
    else
    {
        switch (state)
        {
            case STATE_SPIKE:
                /* Back to normal operation; fall through to sync */
                state = STATE_SYNC;
                /* FALLTHROUGH */

            case STATE_SYNC:
                /* Normal operation; slew the clock to desired frequency
                 * and offset */
                slew_clock(storage, 
                           intercept_adjust,
                           gradient_new,
                           ticks3.ll);
                break;

            case STATE_UNSET:
                state = STATE_SYNC;
                break;
        }
    }

    /* Adjust loop delay, with hysterisis in changing direction */
    if (state == STATE_SYNC)
    {
        if (stability * loop_delay * 1000000000 < 1)
        {
             loop_delay += loop_delay_adjust;
             if (loop_delay > loop_delay_max)
                 loop_delay = loop_delay_max;

        }
        else if (stability * loop_delay * 1000000000 > 2)
        {
             loop_delay -= loop_delay_adjust;
             if (loop_delay < loop_delay_min)
                 loop_delay = loop_delay_min;
        }
    }

    if (debug_calibrate)
        printf("New loop delay:     %d\n", loop_delay);

#ifdef RECORD_STATISTICS
    storage->loop_delay = loop_delay;
    storage->stability = stability;
    storage->offset = offset;
#endif
}

/* Step the clock so that it immediately has the adjusted
 * gradient and intercept.
 */
void step_clock(fasttime_t *storage, 
                int64_t intercept,
                double gradient)
{
    current_conversion.intercept = intercept;
    current_conversion.gradient = gradient;
    storage->linear_segments = 1;
    storage->tick_first = 0;
    storage->log_ticks_per_segment = 0;
    storage->conversions[0].intercept = intercept;
    storage->conversions[0].gradient = gradient; 

    storage->valid = 1;
}

/* Slew the clock so that over the next calibration delay the
 * gradient and intercept are gradually stepped to the correct
 * values.
 */
void slew_clock(fasttime_t *storage,
                int intercept_adjust,
                double gradient,
                uint64_t sync)
{
    unsigned i;
    double ticks_per_segment; 
    double tmp_pwr;
    int64_t intercept_new;

    storage->tick_first = sync;

    /* Amortise at one approximately second intervals. */
    storage->linear_segments = 
        loop_delay > MAX_LINEAR_SEGMENTS ? MAX_LINEAR_SEGMENTS : loop_delay;

    /* Find an appropriate shift for ticks to segment. */
    ticks_per_segment = 
        loop_delay * 1000000000.0 / (storage->linear_segments * gradient);
    storage->log_ticks_per_segment = 1;
    tmp_pwr = 2;
    while (tmp_pwr < ticks_per_segment)
    {
        storage->log_ticks_per_segment++;
        tmp_pwr *= 2;
    }

    storage->log_ticks_per_segment--;
    // clang-anayzer unused: ticks_per_segment = tmp_pwr / 2;
    
    /* Intersect old and new gradients at sync point */
    intercept_new = (int64_t) 
        (sync * (current_conversion.gradient - gradient) + 
          current_conversion.intercept);

    for (i = 0; i < storage->linear_segments; i++)
    {
        /* Gradient is immediately adjusted to new */
        storage->conversions[i].gradient = gradient;
        
        /* Intercept is interpolated from sync'd old to new. */
        storage->conversions[i].intercept = intercept_new +
            (i * intercept_adjust) / storage->linear_segments;
        
    }

    storage->valid = 1;

    current_conversion.intercept = intercept_new + intercept_adjust;
    current_conversion.gradient = gradient;
}

/* Compare two int64_t's */
static int cmp(const void *v1, const void *v2)
{
    return *((int *) v1) - *((int *) v2);
}

/* Add the new offset to the filter buffer; if it exceeds TOLERANCE times
 * the median, returns false.
 */
static int clock_filter(int offset)
{
    int sorted_buffer[CLOCK_FILTER_SIZE];
    
    clock_filter_index++;
    if (clock_filter_size < CLOCK_FILTER_SIZE)
        clock_filter_size = clock_filter_index + 1;
    if (clock_filter_index == CLOCK_FILTER_SIZE)
        clock_filter_index = 0;

    clock_filter_buffer[clock_filter_index] = ABS(offset);
    memcpy(sorted_buffer, clock_filter_buffer,
           clock_filter_size * sizeof(sorted_buffer[0]));
    qsort(sorted_buffer, clock_filter_size, sizeof(sorted_buffer[0]), cmp);

    /* Don't filter out values before the buffer is full */
    return clock_filter_size < CLOCK_FILTER_SIZE ||
        ABS(offset) <= 
        CLOCK_FILTER_TOLERANCE * sorted_buffer[(CLOCK_FILTER_SIZE - 1)/2];
}
