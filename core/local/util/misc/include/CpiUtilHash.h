/**
 * \file
 * \brief This file contains the Interface for hashing functions.
 *
 * Author: John F. Miller
 *
 * Date: 7/20/04
 *
 * Revision History:
 *
 *     05/09/2005 - Frank Pilhofer
 *                  Moved to CPI::Util::Misc.
 *
 */

#ifndef CPI_COMMON_HASH
#define CPI_COMMON_HASH


namespace CPI {
        namespace Util {

                namespace Misc {

                        /**
                         * \brief Generate a hash code from the string.
                         */

                        unsigned int hashCode( const char* string );

                }

        }
}

#endif

