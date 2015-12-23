
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


// -*- c++ -*-

#ifndef OCPIOSDATATYPES_H__
#define OCPIOSDATATYPES_H__

/**
 * \file
 * \brief OS and platform independent data type definitions.
 *
 * Defines exact-width data types for 8, 16, 32 and 64 bits.  Shown on
 * this page are the type definitions for 32 bit Windows.  The base types
 * on other platforms may be different, but they always represent the
 * desired exact-width data type.
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Initial version.
 *
 *     01/25/2008 - VxWorks has stdint.h -- but provides it only in
 *                  RTP land.
 */

#if defined (__linux__) || defined(__APPLE__) || (defined (__vxworks) && !defined (_WRS_KERNEL))
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <inttypes.h>
#endif

namespace OCPI {
  namespace OS {

#if defined (WIN32) || defined (_WRS_KERNEL)
    /*
     * Note: We define WIN32 when running doxygen; therefore, only this
     * section is instrumented for doxygen.
     */

    /**
     * 8 bit signed integer data type.
     */

    typedef signed char int8_t;

    /**
     * 8 bit unsigned integer data type.
     */

    typedef unsigned char uint8_t;

    /**
     * 16 bit signed integer data type.
     */

    typedef short int16_t;

    /**
     * 16 bit unsigned integer data type.
     */

    typedef unsigned short uint16_t;

    /**
     * 32 bit signed integer data type.
     */

    typedef int int32_t;

    /**
     * 32 bit unsigned integer data type.
     */

    typedef unsigned int uint32_t;

    /**
     * 64 bit signed integer data type.
     */

    typedef long long int64_t;

    /**
     * 64 bit unsigned integer data type.
     */

    typedef unsigned long long uint64_t;

    /**
     * Signed integer pointer data type.
     */

    typedef long intptr_t;

    /**
     * Unsigned integer pointer data type.
     */

    typedef unsigned long uintptr_t;

    
#elif defined (__linux__) || defined (__vxworks) || defined (__APPLE__)

    using ::int8_t;
    using ::uint8_t;

    using ::int16_t;
    using ::uint16_t;

    using ::int32_t;
    using ::uint32_t;

    using ::int64_t;
    using ::uint64_t;

    using ::intptr_t;
    using ::uintptr_t;

#else
#error "unknown os"
#endif

  }
}

#endif
