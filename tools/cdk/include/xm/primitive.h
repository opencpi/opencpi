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

#ifndef INCLUDED_PRIMITIVE_H
#define INCLUDED_PRIMITIVE_H

/* ---- C/C++ language includes ------------------------------------------ */

#include <cmath>

#include <string>

using std::string;

#include <vector>

using std::vector;

#include <complex>

using std::complex;

#include <iostream>

using std::cout;
using std::endl;

#include <algorithm>

using std::min;
using std::max;

#include <stdexcept>

using std::logic_error;

/* ---- Unmodified X-Midas header files ---------------------------------- */

#include "complex_n.h"

/* ----------------------------------------------------------------------- */

#define WORKER_INTERNAL
#include "RCC_Worker.h"
#include "ocpi_xm_intercept_c++.h"

#endif // End: #ifndef INCLUDED_PRIMITIVE_H
