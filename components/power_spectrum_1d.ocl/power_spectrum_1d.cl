/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Aug 25 06:42:24 2011 EDT
 * BASED ON THE FILE: power_spectrum_1d.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: power_spectrum_1d
 */

#include "power_spectrum_1d_Worker.h"

#define BLOCK_SIZE 64

/*
 * Required work group size for worker fft1d run() function.
 */
#define OCL_WG_X BLOCK_SIZE
#define OCL_WG_Y 1
#define OCL_WG_Z 1

void realfft( __global float* src_dst,
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
        const float rx = src_dst[k];
        const float ix = 0; // Real FFT

        const float val = ph * (k-start) * (addr % n);
        const float cs  = cos(val);
        const float sn  = sin(val);
        /* cos(ph*k*w) --> where k from 1 to n and w from 1 to n. */
        real += rx* cs - ix * sn;
        imag += rx* sn + ix * cs;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // Ignoring the packing of the DC term
    const int iaddr = bx * ( 2 * blockSize ) + tx; // Interleaved output
    src_dst[iaddr + 0] = real;
    src_dst[iaddr + 1] = imag;
}

void cvmagsx (  __global float* src,
                __global float* dst,
                size_t len )
{
  for ( size_t n = 0; n < len; n++ )
  {
    const size_t s_idx = get_group_id ( 0 ) + n;
    const size_t d_idx = get_group_id ( 0 ) + ( 2 * n );

    dst [ d_idx ] = ( src [ s_idx + 0 ] * src [ s_idx + 0 ] ) +
                    ( src [ s_idx + 1 ] * src [ s_idx + 1 ] );
  }
  barrier ( CLK_LOCAL_MEM_FENCE );
}

void vlogx (  __global float* src,
              __global float* dst,
              size_t len )
{
  for ( size_t n = 0; n < len; n++ )
  {
    const size_t idx = get_group_id ( 0 );

    dst [ idx ] = log10 ( src [ idx ] );
  }
  barrier ( CLK_LOCAL_MEM_FENCE );
}


void vsmulx ( __global float* src,
              float scale,
              __global float* dst,
              size_t len )
{
  for ( size_t n = 0; n < len; n++ )
  {
    const size_t idx = get_group_id ( 0 );

    dst [ idx ] = src [ idx ] * scale;
  }
  barrier ( CLK_LOCAL_MEM_FENCE );
}

#define BLOCK_SIZE 64

/* __kernel void __attribute__((reqd_work_group_size(BLOCK_SIZE,1,1))) */
/*
 * Methods to implement for worker power_spectrum_1d, based on metadata.
 */

OCLResult power_spectrum_1d_run ( __local OCLWorkerPower_spectrum_1d* self,
                                  OCLBoolean timedOut,
                                  __global OCLBoolean* newRunCondition )
{
  (void)timedOut;
  (void)newRunCondition;

  /* Compute the FFT of the real signal */
  realfft ( ( __global float* ) self->in.current.data,
             ( 1 << self->properties->log2n ) );

  cvmagsx ( self->in.current.data,
            self->out.current.data,
            BLOCK_SIZE );

  vlogx ( self->out.current.data,
          self->out.current.data,
          BLOCK_SIZE );

  float scale_factor = 10.0;

  vsmulx ( self->out.current.data,
           scale_factor,
           self->out.current.data,
           BLOCK_SIZE );

  self->out.attr.length = self->in.attr.length / 2;
  self->out.attr.u.operation = 0;

  return OCL_ADVANCE;
}
