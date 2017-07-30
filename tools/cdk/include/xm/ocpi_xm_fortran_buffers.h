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

#ifndef INCLUDED_OCPI_XM_FORTRAN_BUFFERS_H
#define INCLUDED_OCPI_XM_FORTRAN_BUFFERS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- OpenOCPI for X-Midas types ---------------------------------------- */

#include <stdint.h>

#ifndef INCLUDED_FORTRAN_H
#define INCLUDED_FORTRAN_H
#define LPROTOTYPE
#include <fortran.h>
#endif

/* ---- OpenOCPI for X-Midas buffer replacement --------------------------- */

typedef struct
{
  union
  {
    char bbuf [ 32768 ];
    char abuf [ 32768 ];
    short ibuf [ 16384 ];
    int lbuf [ 8192 ];
    float fbuf [ 8192 ];
    double dbuf [ 4096 ];
    complex cfbuf [ 4096 ];
    dcomplex cdbuf [ 2048 ];
    char sbuf [ 32768 ];
  };
} Sbuffer;

/* ---- OpenOCPI for X-Midas Sm_ptr replacement --------------------------- */

typedef struct
{
  int p;
  int fill;
} Sm_ptr;

#ifdef __cplusplus
}
#endif

#endif /* End: #ifndef INCLUDED_OCPI_XM_FORTRAN_BUFFERS_H */
