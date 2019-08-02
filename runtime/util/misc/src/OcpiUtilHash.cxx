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
 */

#include <cstdio> // C-style file I/O
#include <cerrno> // C's errno
#include <cstring> // C's strerror
#include <memory>
#include <gmpxx.h>
#include "OcpiOsAssert.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilException.h" // OU::Error
#include "OcpiUtilHash.h"

namespace OCPI {
  namespace Util {
    namespace Misc {
namespace OU = OCPI::Util;

unsigned int
hashCode( const char* string )
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

sha256_hash_t hashCode256(const void *buf, const size_t len) {
  sha256_hash_t hash_out;
  context_sha256_t ctx;
  hash_init_sha256(&ctx);
  hash_update_sha256(&ctx, reinterpret_cast<const unsigned char *>(buf), len);
  hash_final_sha256(&ctx, hash_out.data());
  return hash_out;
}

const std::string sha256_to_string(const sha256_hash_t &hash_in) {
  mpz_t gmp_sum;
  mpz_init2(gmp_sum, 256);
  mpz_import(gmp_sum, // https://gmplib.org/manual/Integer-Import-and-Export.html
             32, // count many words are read
             1, // each size bytes
             1, // order can be 1 for most significant word first or -1 for least significant first
             0, // endian can be 1 for most significant byte first, -1 for least significant first, or 0 for the native endianness of the host CPU [doesn't matter since reading by byte]
             0, // most significant nails bits of each word are skipped
             hash_in.data());
  char *sum_str = mpz_get_str(NULL, // will allocate string pointer
                              16, // hexadecimal
                              gmp_sum);
  std::string digest_str(sum_str);
  // Push leading zeros
  digest_str.insert(0, 64-digest_str.length(), '0');
  mpz_clear(gmp_sum);
  free(sum_str);
  return digest_str;
}

sha256_wrapper::
sha256_wrapper() : done(false) {
  hash_init_sha256(&ctx);
}

sha256_wrapper::
~sha256_wrapper() {};

const std::string sha256_wrapper::
to_string() {
  ocpiAssert(!done);
  sha256_hash_t hash_out;
  done = true;
  hash_final_sha256(&ctx, hash_out.data());
  return sha256_to_string(hash_out);
}
void sha256_wrapper::
add(const void *buf, const size_t len) {
  ocpiAssert(!done);
  hash_update_sha256(&ctx, reinterpret_cast<const unsigned char *>(buf), len);
}

std::string sha256_file(const std::string &fname, const off_t trunc /* = 0 */) {
  FILE *file = fopen(fname.c_str(), "r");
  if (!file) throw OU::Error("Could not open \"%s\": %s", fname.c_str(), strerror(errno));
  auto res = sha256_file(file, trunc);
  fclose(file);
  return res;
} // sha256_file (string version)

std::string sha256_file(FILE *file, const off_t trunc /* = 0 */) {
  static const off_t chunk_size = 512*1024; // 512KB
  // static const off_t chunk_size = 128; // for testing
  ocpiLog(11, "sha256_file reading file in %jd byte chunks", static_cast<intmax_t>(chunk_size));
  if (trunc) ocpiLog(10, "sha256_file asked to stop early at byte %jd", static_cast<intmax_t>(trunc));
  std::unique_ptr<unsigned char[]> mem (new unsigned char[chunk_size]);
  sha256_wrapper hash;
  if (fseeko(file, 0, SEEK_END)) throw OU::Error("Could not seek to end of file: %s", strerror(errno));
  const off_t fsize = ftello(file);
  if (fseeko(file, 0, SEEK_SET)) throw OU::Error("Could not seek to beginning of file: %s", strerror(errno));
  ocpiAssert(trunc < fsize);
  off_t bytes_to_read = trunc?trunc:fsize;
  ocpiAssert(bytes_to_read > 0);
  while (bytes_to_read) {
    const size_t num = fread(mem.get(), 1, std::min(bytes_to_read, chunk_size), file);
    // ocpiLog(11, "read in %zd bytes", num);
    hash.add(mem.get(), num);
    bytes_to_read -= num;
    if (!num and bytes_to_read)
      throw OU::Error("Strange error counting bytes! If num (%jd) is 0, then bytes_to_read (%jd) should be as well.",
        static_cast<intmax_t>(num),
        static_cast<intmax_t>(bytes_to_read));
  }
  return hash.to_string();
} // sha256_file (FILE version)

}}} // Namespaces
