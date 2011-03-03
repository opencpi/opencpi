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

/* Matrix multiplication: C = A * B.
 * Device code.
 */

#pragma OPENCL EXTENSION cl_amd_printf:enable

#define AS(i, j) As[j + i * BLOCK_SIZE]
#define BS(i, j) Bs[j + i * BLOCK_SIZE]

__kernel void
matrixMul( __global void* inputBuffer0, __global void* outputBuffer0,
           __global OCLBufferInfo* bufferInfoArray,
	       __local float* As, __local float* Bs, __global MatrixMulProperties* properties)
{		
	OCLWorker worker;
	worker.properties = properties;
	worker.ports[MATRIXMUL_IN0].current.data = inputBuffer0;
	worker.ports[MATRIXMUL_IN0].input.length = bufferInfoArray[MATRIXMUL_IN0].length;
	worker.ports[MATRIXMUL_IN0].input.u.operation = bufferInfoArray[MATRIXMUL_IN0].operation_or_exception_ordinal;
	
//	printf("length is %u\n", bufferInfoArray[MATRIXMUL_IN0].length);
	
	worker.ports[MATRIXMUL_OUT0].current.data = outputBuffer0;
//	worker.ports[MATRIXMUL_OUT0].output.length = bufferInfoArray[MATRIXMUL_OUT0].length;
//	worker.ports[MATRIXMUL_OUT0].output.u.operation = bufferInfoArray[MATRIXMUL_OUT0].operation_or_exception_ordinal;
	
	run(&worker, As, Bs);
}
///////////////////////////////////////////////////////////////////////////////
//! Matrix multiplication on the device: C = A * B
//! uiWA is A's width and uiWB is B's width
////////////////////////////////////////////////////////////////////////////////
//__kernel void
//matrixMul( __global float* AB, __global float* C,
//	   __local float* As, __local float* Bs, __global MatrixMulProperties* properties)
void run (OCLWorker* self, __local float* As, __local float* Bs)
{
    __global float* AB = self->ports[MATRIXMUL_IN0].current.data;
	__global float* C = self->ports[MATRIXMUL_OUT0].current.data;
	__global MatrixMulProperties* properties = self->properties;
	
	__global float* A;
	__global float* B;
	
    // Block index
    int bx = get_group_id(0);
    int by = get_group_id(1);

    // Thread index
    int tx = get_local_id(0);
    int ty = get_local_id(1);

	// cok Temporary, so we can test it as one input port
	A = AB;
	B = &AB[12800];
	
	// cok Added OpenCPI style properties for describe matrix width
	
    int uiWA = properties->WA;
	int uiWB = properties->WB;
		  
    // Index of the first sub-matrix of A processed by the block
    int aBegin = uiWA * BLOCK_SIZE * by;

    // Index of the last sub-matrix of A processed by the block
    int aEnd   = aBegin + uiWA - 1;

    // Step size used to iterate through the sub-matrices of A
    int aStep  = BLOCK_SIZE;

    // Index of the first sub-matrix of B processed by the block
    int bBegin = BLOCK_SIZE * bx;

    // Step size used to iterate through the sub-matrices of B
    int bStep  = BLOCK_SIZE * uiWB;

    // Csub is used to store the element of the block sub-matrix
    // that is computed by the thread
    float Csub = 0.0f;

    // Loop over all the sub-matrices of A and B
    // required to compute the block sub-matrix
    for (int a = aBegin, b = bBegin;
             a <= aEnd;
             a += aStep, b += bStep) {

        // Load the matrices from device memory
        // to shared memory; each thread loads
        // one element of each matrix
        AS(ty, tx) = A[a + uiWA * ty + tx];
        BS(ty, tx) = B[b + uiWB * ty + tx];
	
        // Synchronize to make sure the matrices are loaded
        barrier(CLK_LOCAL_MEM_FENCE);

        // Multiply the two matrices together;
        // each thread computes one element
        // of the block sub-matrix        
        #pragma unroll
        for (int k = 0; k < BLOCK_SIZE; ++k)
            Csub += AS(ty, k) * BS(k, tx);

        // Synchronize to make sure that the preceding
        // computation is done before loading two new
        // sub-matrices of A and B in the next iteration
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // Write the block sub-matrix to device memory;
    // each thread writes one element
    C[get_global_id(1) * get_global_size(0) + get_global_id(0)] = Csub;

}

