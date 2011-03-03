/*
 * Copyright 1993-2010 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

#ifndef _MATRIXMUL_H_
#define _MATRIXMUL_H_

// Thread block size
#define BLOCK_SIZE 16

// Basic Matrix dimensions (can be amplified by command line switch)
// (chosen as multiples of the thread block size for simplicity)
//#define WA (5 * BLOCK_SIZE) // Matrix A width
#define HA (10 * BLOCK_SIZE) // Matrix A height
//#define WB (5 * BLOCK_SIZE) // Matrix B width
#define HB WA  // Matrix B height
#define WC WB  // Matrix C width 
#define HC HA  // Matrix C height

#define MATRIXMUL_N_PORTS (2)
#define MATRIXMUL_IN0 (0)
#define MATRIXMUL_OUT0 (1)

typedef struct {
  int WA;
  int WB;
} MatrixMulProperties;

typedef unsigned int uint32_t;

typedef struct {
  uint32_t length;
  uint32_t operation_or_exception_ordinal;
} OCLBufferInfo;

typedef struct {
  __global void *data;
//  uint32_t maxLength;
} OCLBuffer;

typedef struct {
  OCLBuffer current;
  struct {
    uint32_t length;
    union {
      uint32_t operation;
      uint32_t exception;
    } u;
  } input;
  struct {
    uint32_t length;
    union {
      uint32_t operation;
      uint32_t exception;
    } u;
  } output;
} OCLPort;

typedef struct {
  __global void * properties;
//  void * RCC_CONST       * RCC_CONST memories;
//  RCC_CONST RCCContainer * container;
//  RCCRunCondition        * runCondition;
//  RCCPortMask              connectedPorts;
//  char                   * errorString;
  OCLPort                  ports[MATRIXMUL_N_PORTS];
} OCLWorker;

void run (OCLWorker* self, __local float* As, __local float* Bs);

#endif // _MATRIXMUL_H_

