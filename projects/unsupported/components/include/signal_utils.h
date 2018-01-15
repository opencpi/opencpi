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

#include <math.h>


#define byteLen2Complex(x) ((x)/4)
#define byteLen2Real(x) ((x)/2)
#define Complex2bytes(x) ((x)*4)
#define real2bytes(x) ((x)*2)
#define ComplexLen2Real(x) ((x)/2)

#define min(x,y) (((x)<(y))?(x):(y))
#define mems(t,m) sizeof(((t*)0)->m)

#define QMASK 0xefff
#define Uscale(x)  (double)((double)(x) / pow(2,15))
#define Scale(x)   (int16_t)((double)(x) * pow(2,15))
#define Gain(x) (double)((double)(x) * pow(2,15))

// Qs15 complex absolute value returned as a double
#define scabs( r, i ) sqrt(pow(Uscale(r),2)+pow(Uscale(i),2))

#define PI 3.14159265358979
