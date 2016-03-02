/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Aug 24 12:00:45 2011 EDT
 * BASED ON THE FILE: mmul.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: mmul
 */

#include "mmul-worker.h"

#define BLOCK_SIZE 64

/*
 * Required work group size for worker mmul run() function.
 */
#define OCL_WG_X BLOCK_SIZE
#define OCL_WG_Y 1
#define OCL_WG_Z 1

/*
  OpenCL Matrix Multiply from:
  http://gpgpu-computing4.blogspot.com/2009/10/matrix-multiplication-3-opencl.html
*/
//__kernel - when this is a standalone kernel
static void matrixMul (__global float* C,
		__global float* A,
		__global float* B,
		int wA,
		int wB,
		__local float (*As) [ BLOCK_SIZE ] [ BLOCK_SIZE ],
		__local float (*Bs) [ BLOCK_SIZE ] [ BLOCK_SIZE ]) {
  // Block index
  int bx = get_group_id ( 0 );
  int by = get_group_id ( 1 );

  // Thread index
  int tx = get_local_id ( 0 );
  int ty = get_local_id ( 1 );

  // Index of the first sub-matrix of A processed
  // by the block

  int aBegin = wA * BLOCK_SIZE * by;

  // Index of the last sub-matrix of A processed by the block

  int aEnd = aBegin + wA - 1;
  // Step size used to iterate through the

  // sub-matrices of A
  int aStep = BLOCK_SIZE;

  // Index of the first sub-matrix of B processed by the block
  int bBegin = BLOCK_SIZE * bx;

  // Step size used to iterate through the sub-matrices of B
  int bStep  = BLOCK_SIZE * wB;

  // Loop over all the sub-matrices of A and B
  // required to compute the block sub-matrix

  float Csub = 0;

#if 1 // def NOT_VALID_OCL_CODE

  for ( int a = aBegin, b = bBegin; a <= aEnd; a += aStep, b += bStep )
  {
    // Load the matrices from global memory
    // to local memory; each thread loads
    // one element of each matrix
    (*As) [ ty ] [ tx ] = A [ a + wA * ty + tx ];
    (*Bs) [ ty ] [ tx ] = B [ b + wB * ty + tx ];
    // Synchronize to make sure the matrices
    barrier(CLK_LOCAL_MEM_FENCE);

    // Multiply the two matrices together;
    // each thread computes one element of the block sub-matrix
    for ( int k = 0; k < BLOCK_SIZE; ++k )
    {
      Csub += (*As)[ty][k] * (*Bs)[k][tx];
      // Synchronize to make sure that the preceding
      // computation is done before loading two new
      // sub-matrices of A and B in the next iteration
      barrier ( CLK_LOCAL_MEM_FENCE );
    }
  }

#endif


  // Write the block sub-matrix to device memory
  // each thread writes one element
  int c = wB * BLOCK_SIZE * by + BLOCK_SIZE * bx;
  C [ c + wB * ty + tx ] = Csub;
}

/*
 * Methods to implement for worker mmul, based on metadata.
 */

// Declaration of the local memory array As
// used to store the sub-matrix of A
#define OCL_LOCALS \
  OCL_L(As, float As[ BLOCK_SIZE ] [ BLOCK_SIZE ]) \
  OCL_L(Bs, float Bs[ BLOCK_SIZE ] [ BLOCK_SIZE ]) \

static OCLResult mmul_run(MmulWorker* self, __global MmulProperties *properties,
			  __local float (*As) [ BLOCK_SIZE ] [ BLOCK_SIZE ],
			  __local float (*Bs) [ BLOCK_SIZE ] [ BLOCK_SIZE ]) {
  matrixMul((__global float*) self->C.current.data,
            (__global float*) self->A.current.data,
	    (__global float* ) self->B.current.data,
            properties->widthA,
            properties->widthB,
	    As, Bs);
  return OCL_ADVANCE;
}

