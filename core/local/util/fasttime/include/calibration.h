
/* $Id: calibration.h,v 1.7 2005/08/27 08:24:45 alexholkner Exp $ 
 *
 * Copyright (c) Internet2, 2005.  All rights reserved.
 * See LICENSE file for conditions.
 */

/* Calibrate a TSC conversion table for either the fasttimed daemon
 * or a client process not relying on the daemon.
 */

#include "fasttime_private.h"

void calibrate_set_defaults();
time_t calibrate_get_ready_time();
void calibrate_init();
void calibrate(fasttime_t *storage);

/* Defined in calibration.c, defines how many seconds between each
 * calibration update.  This can vary depending on clock stability.
 * In practice the loop delay will be slightly longer as no compensation
 * is made for processing time.  This doesn't matter though: a seperate,
 * accurate measurement is taken (mu).
 */
extern int loop_delay;

/* The following constants are initialised to default values in
 * calibrate_init.
 */

/* calibrate will take care of adjusting loop_delay (daemon process should
 * sleep by that amount.  The following constants define how loop_delay
 * is adjusted.
 */
extern int loop_delay_min;
extern int loop_delay_max;
extern int loop_delay_adjust;
extern int loop_delay_initial;

/* Set to 1 to enable verbose debugging on stdout */
extern int debug_calibrate;

/* PLL gain decides how much to change the gradient (rate) given the
 * amount of offset correction required.  Optimal value determined
 * experimentally.
 */
extern double pll_gain;

