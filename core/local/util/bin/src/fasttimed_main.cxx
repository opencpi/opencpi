/* $Id: fasttimed.c,v 1.15 2005/08/27 08:24:45 alexholkner Exp $ 
 *
 * Copyright (c) Internet2, 2005.  All rights reserved.
 * See LICENSE file for conditions.
 */

/* Daemon to update TSC conversion tables for all fasttime processes.
 *
 * In order to avoid locking the shared memory segment (which
 * would require a system call for the client processes, defeating
 * the purpose of the library) a circular array of conversion
 * tables is maintained, with a single index pointer to the
 * current active table.  This index is written using machine-atomic
 * instructions, greatly reducing the chance of a dirty read by
 * a client (increasing the number of buffers and delay between
 * updates reduces the chance to virtually zero).
 *
 * The shared memory segment requires a system-wide unique key
 * in order to be allocated.  Currently this is generated with
 * ftok(), using the daemon's executable file as the seed (argv[0]).
 * Clearly this requires clients to know the location of fasttimed,
 * either by trying some default locations (./, /usr/local/bin/, etc.),
 * or by having it specified through some environment variable.
 */

#include "rcs.h"
RCS_ID("@(#) $Id: fasttimed.c,v 1.15 2005/08/27 08:24:45 alexholkner Exp $")

#define _XPG4_2
#define __EXTENSIONS__

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "fasttime_private.h"
#include "calibration.h"

static fasttime_buffers_t *ft_storage;
static volatile int shutdown_flag;

void signal_handler(int sig);

const char *usage_msg = 
  "Usage: fasttimed <options>\n"
  "Start the fasttime daemon.\n"
  "\n"
  "Options:\n"
  " -ldm  --loop-delay-min <val>      Minimum loop delay\n"
  " -ldx  --loop-delay-max <val>      Maximum loop delay\n"
  " -ldi  --loop-delay-initial <val>  Initial loop delay\n"
  " -lda  --loop-delay-adjust <val>   Amount to inc/decrement loop delay by\n"
  " -g    --pll-gain <val>            PLL gain constant\n"
  " -d    --debug                     Print debug info to stdout\n";
  

void usage()
{
    fprintf(stderr, usage_msg);
    exit(EXIT_SUCCESS);
}


typedef struct _option_t {
    const char *name;
    void *val;
    enum {
        type_int,
        type_double,
        type_bool
    } type;
} option_t;


static option_t options[] = {
  {"--loop-delay-min", &loop_delay_min, option_t::type_int},
    {"-ldm", &loop_delay_min, option_t::type_int},
    {"--loop-delay-max", &loop_delay_max, option_t::type_int},
    {"-ldx", &loop_delay_max, option_t::type_int},
    {"--loop-delay-initial", &loop_delay_initial, option_t::type_int},
    {"-ldi", &loop_delay_initial, option_t::type_int},
    {"--loop-delay-adjust", &loop_delay_adjust, option_t::type_int},
    {"-lda", &loop_delay_adjust, option_t::type_int},
    {"--pll-gain", &pll_gain, option_t::type_double},
    {"-g", &pll_gain, option_t::type_double},
    {"--debug", &debug_calibrate, option_t::type_bool},
    {"-d", &debug_calibrate, option_t::type_bool},
    {NULL, NULL, option_t::type_int}          /* sentinal */
};

static void parse_options(int argc, char **argv)
{
    option_t *option; 
    int matched;
    int error;
    char *endptr;
    
    argc--; argv++;
    while (argc)
    {
        matched = 0;
        for (option = options; !matched && option->name; option++)
        {
            if (strcmp(*argv, option->name) == 0)
            {
                matched = 1;
                error = 0;
                if (option->type == option_t::type_bool)
                    *((int *) option->val) = 1;
                else
                {
                    error = 1;
                    argc--; argv++;
                    if (!argc)
                    {
                        fprintf(stderr, "Option %s requires a value\n",
                                option->name);
                        break;
                    }
                    else if (option->type == option_t::type_int)
                        *((int *) option->val) = strtol(*argv, &endptr, 10);
                    else if (option->type == option_t::type_double)
                        *((double *) option->val) = strtod(*argv, &endptr);
                    
                    if (endptr == *argv)
                        fprintf(stderr, "Option %s requires a number value\n",
                                option->name);
                    else
                    {
                        matched = 1;
                        error = 0;
                    }
                }
            }
        }

        if (!matched)
        {
            fprintf(stderr, "Unknown option: %s\n", *argv);
            usage();
            exit(EXIT_FAILURE);
        }
        else if (error)
        {
            usage();
            exit(EXIT_FAILURE);
        }

        argc--; argv++;
    }
}

int main(int argc, char **argv)
{
    int shmid;
    unsigned int next_idx;
    fasttime_t *next_storage;
    int i;

    /* Parse arguments */
    calibrate_set_defaults();
    parse_options(argc, argv);

    /* Register signal handler */
    shutdown_flag = 0;
    signal(SIGHUP, signal_handler);    
    signal(SIGINT, signal_handler);    
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialise IPC */
    shmid = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (shmid == -1)
    {
        fprintf(stderr, 
            "Couldn't create shared memory segment; perhaps another "
            "fasttimed process is running?\n");
        return EXIT_FAILURE;
    }
    ftruncate(shmid, sizeof *ft_storage);
    ft_storage = (fasttime_buffers_t*) mmap(NULL, sizeof *ft_storage, PROT_READ | PROT_WRITE, 
                      MAP_SHARED, shmid, 0);
    if (ft_storage == NULL)
    {
        fprintf(stderr,
            "Couldn't map shared memory segment.\n");
        return EXIT_FAILURE;
    }
    ft_storage->buffer_idx = 0;
    for (i = 0; i < FASTTIME_BUFFERS; i++)
        ft_storage->buffer[i].valid = 0;

    ft_storage->ready_time = calibrate_get_ready_time();
    calibrate_init();

    /* Calibration loop */
    for (;;)
    {
        if (!shutdown_flag)
            sleep(loop_delay);
        if (shutdown_flag)
            break;

        /* Select the next buffer to use. */
        next_idx = (ft_storage->buffer_idx + 1) % FASTTIME_BUFFERS;
        next_storage = &ft_storage->buffer[next_idx];
        
        /* Fill in the buffer with new gradient/intercept data */
        calibrate(next_storage);

        /* Switch to the new buffer */
        atomic_inc(&ft_storage->buffer_idx);
    }

    /* Shutdown; free shared segment */
    for (i = 0; i < FASTTIME_BUFFERS; i++)
        ft_storage->buffer[i].valid = 0;
    
    shm_unlink(SHM_NAME);
    munmap(ft_storage, sizeof *ft_storage);

    return EXIT_SUCCESS;
}

void signal_handler(int sig)
{
    shutdown_flag = 1;
}
