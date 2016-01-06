/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Aug 24 13:48:40 2011 EDT
 * BASED ON THE FILE: fft1d.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: fft1d
 */

#include "fft1d-worker.h"

#define BLOCK_SIZE 16

/*
 * Required work group size for worker fft1d run() function.
 */
#define OCL_WG_X BLOCK_SIZE
#define OCL_WG_Y 1
#define OCL_WG_Z 1

/*
  Slow fft implementation.
 */

__kernel void
slowfft( __global float* f_real, __global float* f_imag,
         __global float* r_real, __global float* r_imag,
          const unsigned n )
{
    const unsigned int blockSize = BLOCK_SIZE;
    const float PI = 3.14159265359;
    const float ph = -1 * 2.0 * PI/n;

    const size_t bx = get_group_id(0);
    const size_t tx = get_local_id(0);

    const int addr = bx * blockSize + tx;
    const int start = (addr / n)* n;
    const int end = (addr / n + 1) * n;


    float real = 0.0, imag = 0.0;
    for (int k = start; k < end; k++) {
        const float rx = f_real[k];
        const float ix = f_imag[k];

        const float val = ph * (k-start) * (addr % n);
        const float cs  = cos(val);
        const float sn  = sin(val);
        /* cos(ph*k*w) --> where k from 1 to n and w from 1 to n. */
        real+= rx* cs - ix * sn;
        imag+= rx* sn + ix * cs;
    }

    r_real[addr]= real;
    r_imag[addr]= imag;
}



/*
   Cooley tukey Implementation of FFT
 */


__kernel void __attribute__((reqd_work_group_size(BLOCK_SIZE,1,1)))
CooleyTukey1DFFT ( __global float* const r_real,
                   __global float* const r_imag,
                   __global float* out_real,
                   __global float* out_imag,
                  const unsigned  n,
                  const unsigned powN )

{
       const unsigned blockSize = BLOCK_SIZE;

        const float TWOPI = 2*3.14159265359;
        const size_t bx = get_group_id(0);
        const size_t tx = get_local_id(0);
        const unsigned  addr = bx * blockSize + tx;
        //   Swap position
//        unsigned int Target = 0;
        //   Process all positions of input signal
        unsigned int lIndex =  addr % n;
        unsigned int lPosition  = 0;
        unsigned int lReverse= 0;
        while(lIndex) {
                lReverse = lReverse << 1;
                lReverse += lIndex %2;
                lIndex = lIndex>>1;
                lPosition++;
        }
        if (lPosition < powN) {
                lReverse =lReverse<<(powN-  lPosition);
        }

        // The Input vertex will be used as the Buffer vertex
        // We will keep on changing the input and output
        // So tht we dont need to use another Buffer Array
        lIndex = addr%n;
        if(lReverse > lIndex)
        {
                float lRevTemp;
                const unsigned to = lReverse + (addr / n) * n;
                lRevTemp   = r_real[addr];
                r_real[addr] = r_real[to];
                r_real[to] = lRevTemp;
                lRevTemp   = r_imag[addr];
                r_imag[addr] = r_imag[to];
                r_imag[to]  = lRevTemp;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        // Now we have to iterate powN times Iteratively

        const unsigned lThread = (addr/(n/2))*n + addr%(n/2);

        if(lThread>n)
                return;
        lIndex =  lThread %n;
        unsigned Iter,nIter;
        for(Iter =0 ,nIter =1;Iter<(powN);Iter ++,nIter*=2)
        {
                const unsigned lIndexAdd  =  (lThread/n)*n + (lIndex/nIter)*2*nIter + lIndex%nIter ;
                const unsigned  lIndexMult =  lIndexAdd +nIter ;
                const unsigned k  = lIndex%nIter;
                const float cs =  cos(TWOPI*k/(2*nIter));
                const float sn =  sin(TWOPI*k/(2*nIter));
                const float add_real =  r_real[lIndexAdd];
                const float add_imag =  r_imag[lIndexAdd] ;
                const float mult_real = r_real[lIndexMult];
                const float mult_imag = r_imag[lIndexMult];
                const float tmp_real = cs*mult_real
                                       + sn * mult_imag;
                const float tmp_imag = cs*mult_imag - sn
                                       * mult_real;
                out_real[lIndexAdd]=r_real[lIndexAdd] = add_real + tmp_real;
                out_imag[lIndexAdd]=r_imag[lIndexAdd] = add_imag + tmp_imag;
                out_real[lIndexMult]=r_real[lIndexMult] = add_real - tmp_real;
                out_imag[lIndexMult]=r_imag[lIndexMult] = add_imag - tmp_imag;
                barrier(CLK_LOCAL_MEM_FENCE);
        }
}


/*
 * Methods to implement for worker fft1d, based on metadata.
 */

static OCLResult
fft1d_run(Fft1dWorker* self, __global Fft1dProperties *properties) {
  CooleyTukey1DFFT((__global float* )(self->in_real.current.data),
                   (__global float* )(self->in_imag.current.data),
                   (__global float* )(self->out_real.current.data),
                   (__global float* )(self->out_imag.current.data),
                   1 << properties->log2n,
                   properties->log2n );
  return OCL_ADVANCE;
}
