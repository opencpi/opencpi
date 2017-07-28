
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
