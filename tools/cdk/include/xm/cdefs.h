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

#ifndef INCLUDED_CDEFS_H
#define INCLUDED_CDEFS_H

/* ---- OpenOCPI for X-Midas types ---------------------------------------- */

typedef signed char	int_1;
typedef unsigned char	int_u1;

typedef	short int_2;
typedef unsigned short int_u2;

typedef int int_4;
typedef unsigned int int_u4;

typedef long long int_8;
typedef unsigned long long int_u8;

typedef int_u1 bool_1;
typedef int_u2 bool_2;
typedef int_u4 bool_4;

typedef float real_4;
typedef double real_8;

/* ---- OpenOCPI for X-Midas forward declarations ------------------------- */

struct HEADER;

#endif // End: #ifndef INCLUDED_CDEFS_H
