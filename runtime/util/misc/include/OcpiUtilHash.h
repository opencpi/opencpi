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

/**
 * \file
 * \brief This file contains the Interface for hashing functions.
 */

#ifndef OCPI_COMMON_HASH
#define OCPI_COMMON_HASH

#include <array>
#include <string>
#include "sha256.h"

namespace OCPI {
  namespace Util {
    namespace Misc {
      typedef std::array<unsigned char, 32> sha256_hash_t;

      /**
       * \brief Generate a hash code from the string.
       */
      unsigned int hashCode( const char* string );

      /** \brief Wrapper to keep state of checksum */
      class sha256_wrapper {
        public:
          sha256_wrapper();
          ~sha256_wrapper();
          const std::string to_string();
          void add(const void *, const size_t);
        protected:
          bool done;
          context_sha256_t ctx;
        private:
          // Don't allow copying
          sha256_wrapper( const sha256_wrapper& );
          sha256_wrapper& operator=( const sha256_wrapper& );
      };

      /** \brief Wrapper to simplify "just give me SHA256 of this whole buffer" */
      sha256_hash_t hashCode256(const void *, const size_t);

      /** \brief Wrapper to simplify "just give me SHA256 of this C++ [contiguous] container" */
      template <typename C>
      inline sha256_hash_t hashCode256(const C &cont) {
        return hashCode256(cont.data(), cont.length());
      }

      /** \brief Uses GMP to convert the 256-bit hash into a printable string */
      const std::string sha256_to_string(const sha256_hash_t &);

      /** \brief Generic wrapper to get string version of anything hashCode256 would take */
      template <typename... T>
      inline const std::string hashCode256_as_string(T... params) {
        return sha256_to_string(hashCode256(params...));
      }

      /** \brief Helper function that will return the SHA256 checksum of a data file (up to a certain point) */
      std::string sha256_file(const std::string &, const off_t = 0);

      /** \brief Helper function that will return the SHA256 checksum of a data file (up to a certain point) */
      std::string sha256_file(FILE *file, const off_t trunc = 0);

    }
  }
}

#endif
