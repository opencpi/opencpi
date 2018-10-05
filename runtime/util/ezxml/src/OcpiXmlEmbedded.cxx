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

#include <cstdio> // C-style file I/O
#include <cerrno> // C's errno
#include <cstring> // C's strerror
#include <cstdlib> // atol, free
#include <unistd.h> // truncate
#include <stdexcept>
#include "OcpiXmlEmbedded.h"

// In the name of security, these checks are excessively robust...
// Using C-style file I/O is not RAII-compliant, so throw()ing may lose a file descriptor

namespace {
enum xmlmode_t {readonly, readwrite};

// Private helper function to open a file or throw.
void open_file_or_throw(const std::string &name, const char *mode, FILE **f) {
  FILE *ff = fopen(name.c_str(), mode);
  if (!ff) throw std::runtime_error(strerror(errno));
  *f = ff;
}

// Private helper function for std::string => FILE * with error checking.
void open_file(const std::string &name, const xmlmode_t mode, FILE **f) {
  open_file_or_throw(name, "r", f);
  if (readonly == mode) return;
  open_file_or_throw(name, "r+", f);
}

// Private helper function that looks for artifact signature and verifies XML
// looks fairly reasonable. Returns pointer into file and length of XML.
// Doubt a file will every be that long, but uses 20 bytes to match original.
bool is_valid_signature(FILE *file, size_t &xml_start, size_t &xml_length) {
  xml_start = 0;
  xml_length = 0;
  int res = fseek(file, 0, SEEK_END);
  if (res) throw std::runtime_error(strerror(errno));
  size_t end = ftell(file);
  if (end < 21) return false; // throw std::runtime_error("File too small");
  res = fseek(file, end-20, SEEK_SET);
  if (res) throw std::runtime_error(strerror(errno));
  std::string tail(21, '\0');
  size_t ress = fread(&tail[0], 1, 20, file);
  if (20 != ress) throw std::runtime_error("Could not read 20 bytes");
  // The signature is '>' then 0x0a then 'X'
  std::string::size_type loc = tail.rfind(">\x0aX"); // no auto until C++11
  if (std::string::npos == loc) return false;
  loc+=2; // want that last > and 0x0a in the XML
  if (loc >= 20) return false; // Should we throw error here?
  // After the X is a number, eg. 1762.
  // no string number manipulations until C++11
  long read_offset = atol(tail.data()+loc+1);
  if (0 == read_offset) return false; // Should we throw error here?
  loc = 20 - loc; // Now loc is the length of the X and trailing newline
  if (read_offset > static_cast<long>(end - loc)) // Should we throw error here?
    return false;
  xml_start = end - loc - static_cast<size_t>(read_offset);
  xml_length = static_cast<size_t>(read_offset);
  return true;
}

// Private helper function that is a start for verifying valid XML metadata.
// Should we have EZXML parse it or something?
bool verify_XML_block(const std::string &xml) {
  // This can definitely be expanded!
  // Should we check here for "<artifact" and "/artifact>"?
  if (xml[0] != '<') return false;
  if (xml[xml.length()-1] != '\x0a') return false;
  if (xml[xml.length()-2] != '>') return false;
  return true;
}
// Private helper function that gives XML string when given start/length from is_valid_signature.
void extract_XML(FILE *file, const size_t xml_start, const size_t xml_length, std::string &xml) {
  xml.clear();
  const int res = fseek(file, xml_start, SEEK_SET);
  if (res) return;
  xml.resize(xml_length);
  const size_t ress = fread(&xml[0], 1, xml_length, file);
  if (xml_length != ress or not verify_XML_block(xml))
    xml.clear();
}
} // anon namespace

namespace OCPI {
namespace Util {
namespace EzXml {

void artifact_addXML(const std::string &fname, const std::string &xml) {
  // Verify XML buffer
  //TODO allow for blank lines at the end of a file
  if (not verify_XML_block(xml)) throw std::runtime_error("XML input not valid!");
  // Difference between old version and new version: old version blindly appended
  while (artifact_stripXML(fname)) {};
  FILE *f;
  open_file(fname, readwrite, &f);
  if (fseek(f, 0, SEEK_END)) throw std::runtime_error(strerror(errno));
  size_t res = fwrite(xml.data(), 1, xml.length(), f);
  if (res != xml.length()) throw std::runtime_error(strerror(errno));
  char *sig;
  const int res2 = asprintf(&sig, "X%zd\x0a", xml.length());
  if (res2 == -1) throw std::runtime_error("asprintf could not allocate memory");
  const size_t siglen = strlen(sig);
  res = fwrite(sig, 1, siglen, f);
  free(sig);
  if (res != siglen) throw std::runtime_error(strerror(errno));
  fclose(f);
};

void artifact_getXML(const std::string &fname, std::string &xml) {
  xml.clear();
  FILE *f;
  open_file(fname, readonly, &f);
  size_t start, length;
  if (is_valid_signature(f, start, length)) {
    extract_XML(f, start, length, xml);
  }
  fclose(f);
};

bool artifact_stripXML(const std::string &fname) {
  FILE *f;
  open_file(fname, readwrite, &f);
  size_t start, length;
  const bool valid = is_valid_signature(f, start, length);
  fclose(f);
  if (!valid)
    return false;
  const int res = truncate(fname.c_str(), start);
  if (res) throw std::runtime_error(strerror(errno));
  return true;
};
} // EzXml
} // Util
} // OCPI
