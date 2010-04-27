// -*- c++ -*-

#ifndef CPIUTILUUID_H__
#define CPIUTILUUID_H__

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

namespace CPI {
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
	unsigned char data[16];
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

      std::string binaryToHex (BinaryUUID uuid)
	throw ();

    }

  }
}

#endif
