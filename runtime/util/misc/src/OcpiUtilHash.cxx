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

/*
 * Abstract:
 *   This file contains the implementation for hashing functions.
 *
 * Author: John F. Miller
 *
 * Date: 1/31/05
 *
 */

#include "OcpiUtilHash.h"
#include <cstring>

unsigned int
OCPI::Util::Misc::hashCode( const char* string )
{
  /* ELF Hash Function
   *
   * Similar to the hash algorithm based on work by Peter
   * J. Weinberger of AT&T Bell Labs, but tweaked for 32-bit
   * processors. It's the hash function widely used on most
   * UNIX systems.
   *
   * Source: http://www.partow.net/programming/hashfunctions/#top
   *
   * Use is permitted under the guidelines and in accordance with 
   * the most current version of "Common Public License"
   *
   */
  unsigned int mask = 0x7fffffff;
  unsigned int hash_value = 0;
  unsigned int x = 0;

  unsigned int hash_len = (unsigned int)(std::strlen( string ));

  for (unsigned int i=0; i < hash_len; i++)
  { 
    hash_value = (hash_value << 4) + (unsigned int)string[i];
    if ( (x = hash_value & 0xF0000000L) != 0 )
    {
      hash_value ^= ( x >> 24 );
      hash_value &= ~x;
    }
  }
 
  return (hash_value & mask);

}
