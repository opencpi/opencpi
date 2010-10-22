
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

#ifndef OCPI_CFUTIL_MISC_H__
#define OCPI_CFUTIL_MISC_H__

/**
 * \file
 * \brief Various SCA related helpers.
 *
 * Revision History:
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>
#include <CF.h>

namespace OCPI {
  namespace CFUtil {

    /**
     * Map a CF::Device::UsageType value to a string.
     *
     * \param[in] usageType A UsageType value.
     * \return String representation of the value.
     */

    std::string usageTypeToString (CF::Device::UsageType usageType)
      throw ();

    /**
     * Map a CF::Device::AdminType value to a string.
     *
     * \param[in] adminType An AdminType value.
     * \return String representation of the value.
     */

    std::string adminTypeToString (CF::Device::AdminType adminType)
      throw ();

    /**
     * Map a CF::Device::OperationalType value to a string.
     *
     * \param[in] opType An OperationalType value.
     * \return String representation of the value.
     */

    std::string operationalTypeToString (CF::Device::OperationalType opType)
      throw ();

    /**
     * Map a CF::LoadableDevice::LoadType value to a string.
     *
     * \param[in] loadType A LoadType value.
     * \return String representation of the value.
     */

    std::string loadTypeToString (CF::LoadableDevice::LoadType loadType)
      throw ();

    /**
     * Map a CF::ErrorNumberType value to a string.
     *
     * \param[in] ent A CF error number.
     * \return String representation of the value.
     */

    std::string errorNumberToString (CF::ErrorNumberType ent)
      throw ();

  }
}

#endif
