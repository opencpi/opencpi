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

// -*- c++ -*-

#ifndef OCPIUTILUUID_H__
#define OCPIUTILUUID_H__

/**
 * \file
 * \brief Utilities related to Universally Unique Identifiers.
 *
 * Revision History:
 *
 *     12/16/2008 - Frank Pilhofer
 *                  Initial version.
 */

#include <string>

#include "OcpiOsDataTypes.h"

namespace OCPI {
  namespace Util {

    /**
     * \brief Utilities related to Universally Unique Identifiers.
     *
     * See ITU-T X.667: Information technology - Open Systems Interconnection -
     * Procedures for the operation of OSI Registration Authorities: Generation
     * and registration of Universally Unique Identifiers (UUIDs) and their use
     * as ASN.1 object identifier components.
     */

    namespace UUID {

      /**
       * \brief Binary representation of a UUID.
       */

      struct BinaryUUID {
        uint8_t data[16];
      };

      /**
       * \brief Produce a random-number-based UUID.
       *
       * \return The UUID in binary representation.
       *
       * \note Pseudo-random numbers may produce the same value multiple
       * times.
       */

      BinaryUUID produceRandomUUID ()
        throw ();

      /**
       * \brief Convert a binary UUID to the hexadecimal representation.
       *
       * \param[in] uuid The UUID in binary representation.
       * \return The UUID in hexadecimal representation.
       */

      std::string binaryToHex (const BinaryUUID &uuid)
        throw ();

    }

  }
}

#endif
