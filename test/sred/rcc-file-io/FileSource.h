#ifndef RCC_WORKER_FILESOURCE_H__
#define RCC_WORKER_FILESOURCE_H__

/*
 * Header file for worker FileSource.
 * Generated at 2010-07-02 18:58:40
 * from output/FileSource.scd.xml
 */

#include <RCC_Worker.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*
 * Worker port ordinals.
 */

enum FileSourcePortOrdinal {
  FILESOURCE_DATAOUT
};

#define FILESOURCE_N_INPUT_PORTS 0
#define FILESOURCE_N_OUTPUT_PORTS 1

/*
 * Worker property space.
 */

typedef struct {
  RCCChar fileName[257];
  uint32_t bytesPerPacket;
  uint32_t offset;
  uint32_t testId;
  uint32_t errnoValue;
} FileSourceProperties;

#if defined (__cplusplus)
}
#endif
#endif
