
/* $Id: fasttime_private.h,v 1.9 2005/08/27 08:24:45 alexholkner Exp $ 
 *
 * Copyright (c) Internet2, 2005.  All rights reserved.
 * See LICENSE file for conditions.
 */

/* Structures, constants and inline functions used by both the
 * daemon and client library.
 */

#ifndef FASTTIME_PRIVATE_H
#define FASTTIME_PRIVATE_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "config.h"

// FIXME Fix this in ocpios
#ifdef __APPLE__
#ifndef CLOCK_REALTIME
#include <sys/time.h>
typedef int clockid_t;
#define CLOCK_REALTIME ((clockid_t)0)
static inline int clock_gettime(int id, struct timespec *tp) {
  struct timeval tv;
  (void)id;
  gettimeofday(&tv, NULL);
  TIMEVAL_TO_TIMESPEC(&tv, tp);
  return 0;
}
#endif
#endif

/* Maximum number of piecewise linear segments within one calibration
 * interval.
 */
#define MAX_LINEAR_SEGMENTS 32

/* Record statistics so they can be read by clients */
#define RECORD_STATISTICS

/* Shared memory structure */
typedef struct _conversion_t {
    int64_t intercept;
    double gradient;
} conversion_t;

typedef struct _fasttime_t {
#ifdef RECORD_STATISTICS
    int loop_delay;
    double stability;
    int offset;
#endif
    int valid;
    unsigned linear_segments;
    conversion_t conversions[MAX_LINEAR_SEGMENTS];
    uint64_t tick_first;
    unsigned char log_ticks_per_segment;
} fasttime_t;

/* IPC key generation */
#define SHM_NAME "/fasttime001"

/* Rather than shared/exclusive locking one fasttime_t structure in 
 * shared memory (which is expensive), we maintain an array of N
 * structures, one of which is in use at any time (similar to double-
 * buffering).  The daemon will update the next buffer before
 * atomically setting the buffer_idx to point to it.
 *
 * Set FASTTIME_BUFFERS to a number high enough that in the worst
 * case the daemon cannot possibly update that many times in the
 * time that a client would read buffer_idx and then fetch the buffer.
 * Currently the daemon takes at least 2 seconds for each update,
 * so the setting of 8 is absurdly pessimistic.
 */
#define FASTTIME_BUFFERS 8
typedef struct _fasttime_buffers_t {
    unsigned int buffer_idx;
    fasttime_t buffer[FASTTIME_BUFFERS];
    int valid;
    time_t ready_time;
} fasttime_buffers_t;

/* Convenient cross platform union to access high and low order words
 * of 64 bit long */
typedef union _tick_t {
    struct _l {
        uint32_t high;
        uint32_t low;
    } l;
    uint64_t ll;
} tick_t;

#ifdef _CPU_ARM
static inline void get_tick_count(tick_t *t)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  t->l.high = tv.tv_sec;
  t->l.low = (uint32_t)((tv.tv_usec * ((uint64_t)0x100000000ull + 500))/1000);
}
#endif
/* Tick count code for PowerPC (Apple G3, G4, G5) */
#ifdef _CPU_POWERPC
static inline void get_tick_count(tick_t *t)
{
  register uint32_t tb_lower = 0,
                    tb_upper = 0,
                    tb_upper_tmp = 0;

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


  t->l.high = tb_upper;
  t->l.low  = tb_lower;
}
#endif /* _CPU_POWERPC */


/* Tick count code for IA-32 architecture (Intel and AMD x86, and 64-bit 
 * versions) */
#if (defined(_CPU_IA32) || defined(_CPU_IA64))
static inline void get_tick_count(tick_t *t)
{
    __asm__ __volatile__ (
        "rdtsc"
        : "=a" (t->l.high), "=d" (t->l.low));

}
#endif  /* _CPU_IA32 */


/* Tick count code for SPARC architecture */
#ifdef _CPU_SPARC_V8PLUS
static inline void get_tick_count(tick_t *t)
{
    __asm__ __volatile__ (
        "rd %%tick, %0"
        : "=r" (t->ll));
}
#endif /* _CPU_SPARC_V8PLUS */

/* Atomic set code */
#if (defined(_CPU_IA32) || defined(_CPU_IA64))
static inline void atomic_inc(unsigned int *addr)
{
    __asm__ (
        "lock incl %0"
        : 
        : "m" (*addr)
        : "memory");
}
#endif



#ifdef _CPU_SPARC_V8PLUS
static inline void atomic_inc(unsigned int *addr)
{
    /* CAS is one of the only atomic ops on SPARC.  We don't actually
     * care if the compare is different (it shouldn't be!), just
     * make sure the memory location is updated atomically.
     */
    __asm__ (
        "cas    %0, %1, %2"
        :
        : "m" (*addr), "r" (*addr), "r" (*addr + 1)
        : "memory");
}
#endif

/* Atomic increment for powerpc or arm not implemented yet */
#if defined(_CPU_POWERPC) || defined(_CPU_ARM)
static inline void atomic_inc(unsigned int *addr)
{
    (*addr)++;
}
#endif /* Other archs */

#endif /* FASTTIME_PRIVATE_H */
