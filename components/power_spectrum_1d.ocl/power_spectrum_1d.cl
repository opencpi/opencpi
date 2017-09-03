/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Aug 25 06:42:24 2011 EDT
 * BASED ON THE FILE: power_spectrum_1d.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: power_spectrum_1d
 */

#include "power_spectrum_1d-worker.h"

#define BLOCK_SIZE 64

/*
 * Required work group size for worker fft1d run() function.
 */
#define OCL_WG_X BLOCK_SIZE
#define OCL_WG_Y 1
#define OCL_WG_Z 1

static void realfft( __global float* src_dst,
              const unsigned n )
{
    const unsigned int blockSize = BLOCK_SIZE;
    const float PI = 3.14159265359f;
    const float ph = -1 * 2.0f * PI/n;

    const size_t bx = get_group_id(0);
    const size_t tx = get_local_id(0);

    const int addr = bx * blockSize + tx;
    const int start = (addr / n)* n;
    const int end = (addr / n + 1) * n;

    float real = 0.0f, imag = 0.0f;
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

static void cvmagsx (  __global float* src,
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

static void vlogx (  __global float* src,
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


static void vsmulx ( __global float* src,
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

static OCLResult
power_spectrum_1d_run(Power_spectrum_1dWorker* self,
				__global Power_spectrum_1dProperties *properties) {
  /* Compute the FFT of the real signal */
  float scale_factor = 10.0f;

  realfft((__global float *)self->ports.in.current.data, 1 << properties->log2n);
  cvmagsx(self->ports.in.current.data, self->ports.out.current.data, BLOCK_SIZE);
  vlogx(self->ports.out.current.data, self->ports.out.current.data, BLOCK_SIZE);
  vsmulx(self->ports.out.current.data, scale_factor, self->ports.out.current.data, BLOCK_SIZE);
  self->ports.out.current.length = self->ports.in.current.length / 2;
  self->ports.out.current.opCode = 0;
  return OCL_ADVANCE;
}
