#ifndef RCC_WORKER_ROT_H__
#define RCC_WORKER_ROT_H__

/*
 * Header file for worker Rot.
 * Generated at 2011-01-19 09:52:31
 * from output/rot.scd.xml
 */

#include <RCC_Worker.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*
 * Worker port ordinals.
 */

enum RotPortOrdinal {
  ROT_DATAIN,
  ROT_DATAOUT
};

#define ROT_N_INPUT_PORTS 1
#define ROT_N_OUTPUT_PORTS 1

/*
 * Worker property space.
 */

typedef struct {
  int16_t key;
} RotProperties;

#if defined (__cplusplus)
}
#endif
#endif
