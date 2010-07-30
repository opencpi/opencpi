#ifndef RCC_WORKER_FILETEE_H__
#define RCC_WORKER_FILETEE_H__

/*
 * Header file for worker FileTee.
 * Generated at 2010-07-02 18:58:41
 * from output/FileTee.scd.xml
 */

#include <RCC_Worker.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*
 * Worker port ordinals.
 */

enum FileTeePortOrdinal {
  FILETEE_DATAIN,
  FILETEE_DATAOUT
};

#define FILETEE_N_INPUT_PORTS 1
#define FILETEE_N_OUTPUT_PORTS 1

/*
 * Worker property space.
 */

typedef struct {
  RCCChar fileName[257];
  uint32_t offset;
} FileTeeProperties;

#if defined (__cplusplus)
}
#endif
#endif
