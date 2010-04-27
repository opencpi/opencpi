// -*- c++ -*-

#ifndef CPI_CFUTIL_MISC_H__
#define CPI_CFUTIL_MISC_H__

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

namespace CPI {
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
