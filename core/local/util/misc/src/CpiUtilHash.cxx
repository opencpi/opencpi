/*
 * Abstact:
 *   This file contains the implementation for hashing functions.
 *
 * Author: John F. Miller
 *
 * Date: 1/31/05
 *
 */

#include "CpiUtilHash.h"
#include <cstring>

static const int num_bits=16;

unsigned int
CPI::Util::Misc::hashCode( const char* string )
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
