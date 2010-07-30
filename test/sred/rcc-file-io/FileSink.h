#ifndef RCC_WORKER_FILESINK_H__
#define RCC_WORKER_FILESINK_H__

/*
 * Header file for worker FileSink.
 * Generated at 2010-07-02 18:58:41
 * from output/FileSink.scd.xml
 */

#include <RCC_Worker.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*
 * Worker port ordinals.
 */

enum FileSinkPortOrdinal {
  FILESINK_DATAIN
};

#define FILESINK_N_INPUT_PORTS 1
#define FILESINK_N_OUTPUT_PORTS 0

/*
 * Worker property space.
 */

typedef struct {
  RCCChar fileName[257];
  uint32_t offset;
} FileSinkProperties;

#if defined (__cplusplus)
}
#endif
#endif
