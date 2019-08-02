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

#include <fcntl.h> // open()
#include <unistd.h> // truncate
#include <sys/stat.h> // O_RDONLY etc
#include <cassert>
#include <cstdio> // C-style file I/O
#include <cerrno> // C's errno
#include <cstring> // C's strerror
#include <cstdlib> // atol, free
#include <fstream>
#include <limits> // numeric_limits
#include <memory> // unique_ptr
#include "OcpiOsAssert.h" // ocpiInfo et al
#include "OcpiXmlEmbedded.h"
#include "OcpiUtilException.h" // OU::Error
#include "OcpiUtilHash.h"
#include "OcpiUtilMisc.h"

// In the name of security, some checks are excessively robust...
// Using C-style file I/O is not RAII-compliant, so throw()ing may lose a file descriptor

namespace {
namespace OU = OCPI::Util;
namespace OUM = OCPI::Util::Misc;
enum xmlmode_t {readonly, readwrite};

// Forward-declare some of the helpers:
void extract_XML(FILE *, const off_t, const off_t, std::string &, const bool);
bool verify_XML_metadata_hash(FILE *, const off_t, const off_t);
bool verify_payload_hash(FILE *, const off_t, const off_t);
off_t file_size(FILE *file);

// Private helper function to open a file or throw (std::string => FILE *).
void open_file(const std::string &name, const xmlmode_t mode, FILE **f, OU::EzXml::extra_t *extra_info = NULL) {
  if (extra_info) extra_info->init();
  // Non-blocking so we don't hang on opening a FIFO
  const char *cname = name.c_str();
  int fd = open(cname, ((readonly == mode)?O_RDONLY:O_RDWR)|O_NONBLOCK);
  if (fd < 0) throw OU::Error("Cannot open file: \"%s\"", cname);
  struct stat info;
  if (fstat(fd, &info)) {
    close(fd);
    throw OU::Error("Cannot get modification time: \"%s\" (%d)", cname, errno);
  }
  if ((info.st_mode & S_IFMT) != S_IFREG) {
    close(fd);
    throw OU::Error("File: \"%s\" is not a normal file", cname);
  }
  if (extra_info) {
    extra_info->filesize = info.st_size; // This may change if XML added
    extra_info->timestamp = info.st_mtim;
  }
  // Convert to stream-based from plain fd
  // "The file descriptor is not dup'ed, and will be closed when the stream created by fdopen() is closed."
  FILE *ff = fdopen(fd, (readonly == mode)?"rb":"r+b");
  if (!ff) throw OU::Error("Cannot re-open file: \"%s\": %s", cname, strerror(errno));
  *f = ff;
}

// Private helper function that looks for artifact signature and verifies XML
// looks fairly reasonable. Returns pointer into file and length of XML.
// Doubt a file will ever be that long, but uses 20 bytes to match original.
// It will also verify SHA256 hashes of the data and the metadata.
//  NOTE: If given "trunc", will only detect the XML and not do hash checks.
bool get_valid_artifact_XML(FILE *file, off_t &xml_start, off_t &xml_length, const off_t trunc = 0) {
  xml_start = 0;
  xml_length = 0;
  const auto fsize = file_size(file);
  if (trunc and trunc > fsize) throw OU::Error("Asked for too many bytes to be read");
  auto end = trunc?trunc:fsize;
  if (end < 21) return false; // throw OU::Error("File too small");
  assert(end < std::numeric_limits<ssize_t>::max());
  // First/quick check - see if end of file is 'O' for OpenCPI
  auto res = fseeko(file, end-1, SEEK_SET);
  if (res) throw OU::Error("Could not seek to end of file: %s", strerror(errno));
  char o;
  if (1 != fread(&o, 1, 1, file)) throw OU::Error("Could not read 1 byte");
  if ('O' != o) return false;
  // Now we'll look for signature
  res = fseeko(file, end-20, SEEK_SET);
  if (res) throw OU::Error("Could not seek to end-20 in file: %s", strerror(errno));
  std::string tail(21, '\0');
  if (20 != fread(&tail[0], 1, 20, file)) throw OU::Error("Could not read 20 bytes");
  // The signature is '>' then 0x0a then 'X'
  std::string::size_type loc = tail.rfind(">\x0aX");
  if (std::string::npos == loc) return false;
  loc+=2; // want that last > and 0x0a in the XML
  if ((loc <= 0) || (loc >= 20)) return false; // Should we throw error here?
  // After the X is a number, eg. 1762.
  // no string number manipulations until C++11
  long read_offset = atol(tail.data()+loc+1);
  if (read_offset <= 0) return false; // Should we throw error here?
  loc = 20 - loc; // Now loc is the length of the X and trailing newline
  if (read_offset > static_cast<long>(end - static_cast<off_t>(loc))) // Should we throw error here?
    return false;
  xml_start = end - static_cast<off_t>(loc) - static_cast<off_t>(read_offset);
  xml_length = static_cast<off_t>(read_offset);
  if (trunc) return true; // Only verify XML exists
  return verify_XML_metadata_hash(file, xml_start, xml_length) && // Verify SHA of metadata
         true &&                                                  // TODO: Verify GPG signatures of metadata here?
         verify_payload_hash(file, xml_start, xml_length);        // Verify SHA of payload
}

// Private helper function to verify SHA256 of metadata
bool verify_XML_metadata_hash(FILE *file, const off_t xml_start, const off_t xml_length) {
  std::string xml;
  extract_XML(file, xml_start, xml_length, xml, false); // False = we do our own verification
  if (xml.length() < 170) {
    ocpiInfo("verify_XML_metadata_hash: XML too short!");
    return false;
  }
  // The following comparison is explicitly anchored to the beginning of the metadata
  if (0 != xml.compare(0, 14, "<!-- SHA256:M:")) {
    ocpiInfo("verify_XML_metadata_hash: Did not find SHA256:M signature in XML");
    return false;
  }
  const std::string proposed_hash(xml.substr(14,64));
  ocpiLog(10, "verify_XML_metadata_hash: Input hash is    %s", proposed_hash.c_str());
  // Re-zero out the hash to (hopefully) match expected
  xml.replace(14, 64, "0000000000000000000000000000000000000000000000000000000000000000");
  const std::string metadata_hash = OUM::hashCode256_as_string(xml);
  ocpiLog(10, "verify_XML_metadata_hash: Computed hash is %s", metadata_hash.c_str());
  if (proposed_hash != metadata_hash) return false;
  // Check file size
  const long proposed_filesize = atol(xml.data()+14+64+1); // Leader + hash + :
  if (proposed_filesize <= 0) {
    ocpiInfo("verify_XML_metadata_hash: Did not find valid file size in SHA256:M XML comment");
    return false;
  }
  ocpiLog(10, "verify_XML_metadata_hash: Silent filesize check is next");
  assert(proposed_filesize < std::numeric_limits<off_t>::max());
  return (static_cast<off_t>(proposed_filesize) == file_size(file));
}

// Private helper function to verify SHA256 of payload data
bool verify_payload_hash(FILE *file, const off_t xml_start, const off_t xml_length) {
  std::string xml;
  extract_XML(file, xml_start, xml_length, xml, false); // We do our own verification
  // By time this function is called, we have deemed the XML "safe" so willing to use standard
  // searching, etc. vs. absolute positions.
  auto hash_offset = xml.find("<!-- SHA256:D:");
  if (std::string::npos == hash_offset) {
    ocpiInfo("verify_payload_hash: Did not find SHA256:D signature in XML");
    return false;
  }
  // Verify that the payload is not "double wrapped"
  {
    off_t dbl_start, dbl_length;
    const bool dbl_valid = get_valid_artifact_XML(file, dbl_start, dbl_length, xml_start);
    if (dbl_valid) {
      ocpiBad("verify_payload_hash: Payload looks like it has another metadata entry!?");
      return false;
    }
  } // anon block
  const std::string proposed_hash(xml.substr(hash_offset+14,64)); // Skip past search term
  ocpiLog(10, "verify_payload_hash: Input hash is    %s", proposed_hash.c_str());
  // Now to compute the hash of everything in the file that isn't the XML
  auto metadata_hash = OUM::sha256_file(file, xml_start);
  ocpiLog(10, "verify_payload_hash: Computed hash is %s", metadata_hash.c_str());
  return (proposed_hash == metadata_hash);
}

// Private helper function that is a start for verifying valid XML metadata.
// Note: it is used BEFORE signatures added, so cannot do SHA256 checks here.
// Should we have EZXML parse it or something?
bool verify_XML_block(const std::string &xml) {
  // This can definitely be expanded!
  // Should we check here for "<artifact" and "/artifact>"?
  if (xml[0] != '<') return false;
  if (xml[xml.length()-1] != '\x0a') return false;
  if (xml[xml.length()-2] != '>') return false;

  return true;
}

// Private helper function that gives XML string when given start/length from get_valid_artifact_XML.
void extract_XML(FILE *file, const off_t xml_start, const off_t xml_length, std::string &xml, const bool verify = true) {
  xml.clear();
  assert(xml_start > 0);
  assert(xml_length > 0);
  assert(xml_length == static_cast<off_t>(static_cast<size_t>(xml_length)));
  const size_t xml_length_st = static_cast<size_t>(xml_length);
  if (fseeko(file, xml_start, SEEK_SET)) return;
  xml.resize(xml_length_st);
  const size_t ress = fread(&xml[0], 1, xml_length_st, file);
  if (xml_length_st != ress or (verify and not verify_XML_block(xml)))
    xml.clear();
}

// Private helper function that gives size of an open file when given FILE*
// (vs. OCPI::OS::FileSystem::size).
// Uses off_t to fix lots of math warnings
off_t file_size(FILE *file) {
  if (fseeko(file, 0, SEEK_END)) throw OU::Error("Could not seek to end of file: %s", strerror(errno));
  auto end = ftello(file);
  if (end < 0) throw OU::Error("Call to ftello() failed: %s", strerror(errno));
  return end;
}

} // anon namespace

namespace OCPI {
namespace Util {
namespace EzXml {

void artifact_addXML(const std::string &fname, const std::string &xml_in) {
  // TODO allow for blank lines at the end of a file
  // Verify XML buffer
  if (not verify_XML_block(xml_in)) throw OU::Error("XML input not valid!");
  // Difference between script version and C++ version: script version blindly appended
  while (artifact_stripXML(fname)) {};
  FILE *f;
  open_file(fname, readwrite, &f);
  // local version of string with checksum info (zeroed metadata):
  std::string xml("<!-- SHA256:M:0000000000000000000000000000000000000000000000000000000000000000:0000000000 -->\n");
  xml += "<!-- SHA256:D:" + OUM::sha256_file(fname) + " -->\n"; // checksum of datafile
  xml += xml_in;
  std::unique_ptr<char[]> sig(new char[20]);
  if (snprintf(sig.get(), 20, "X%zdO", xml.length()) >= 20) throw OU::Error("XML Signature unbelievably large");
  const size_t siglen = strlen(sig.get());
  // Now that we have the proposed length of the metadata and signature, fill in the size field (10 chars)
  const std::string total_size_str = OU::unsignedToString(static_cast<unsigned long long>(static_cast<unsigned long long>(file_size(f)) + xml.length() + siglen), 10, 10);
  assert(10 == total_size_str.length());
  xml = xml.replace(14+64+1, 10, total_size_str);
  auto metadata_hash = OUM::hashCode256_as_string(xml);
  xml = xml.replace(14, 64, metadata_hash); // 14 is 0 after ":M:"
  if (fseeko(f, 0, SEEK_END)) throw OU::Error("Could not seek to end of file: %s", strerror(errno));
  size_t res = fwrite(xml.data(), 1, xml.length(), f);
  if (res != xml.length()) throw OU::Error("Wrote %zd instead of %zd bytes (XML payload) to file: %s", res, xml.length(), strerror(errno));
  res = fwrite(sig.get(), 1, siglen, f);
  if (res != siglen) throw OU::Error("Wrote %zd instead of %zd bytes (artifact signature) to end of file: %s", res, siglen, strerror(errno));
  fclose(f);
};

bool artifact_getXML(const std::string &fname, std::string &xml, extra_t *extra_info /* = NULL */) {
  xml.clear();
  FILE *f;
  open_file(fname, readonly, &f, extra_info);
  off_t start, length;
  if (get_valid_artifact_XML(f, start, length)) {
    extract_XML(f, start, length, xml);
    if (extra_info) {
      extra_info->metadata.start = start;
      extra_info->metadata.length = length;
    }
  }
  fclose(f);
  return (!xml.empty());
};

bool artifact_stripXML(const std::string &fname) {
  FILE *f;
  open_file(fname, readonly, &f);
  off_t start, length;
  const bool valid = get_valid_artifact_XML(f, start, length);
  fclose(f);
  if (!valid)
    return false;
  const int res = truncate(fname.c_str(), static_cast<off_t>(start));
  if (res) throw OU::Error("Could not truncate() file \"%s\": %s", fname.c_str(), strerror(errno));
  return true;
};

void artifact_getPayload(const std::string &fname, std::ostream &os /* = std::cout */) {
  std::string xml;
  extra_t extra_info;
  if (!artifact_getXML(fname, xml, &extra_info))
    throw OCPI::Util::Error("Failed to parse metadata file \"%s\"!", fname.c_str());
  // Now we want to stream the file to output stream
  std::ifstream src(fname.c_str(), std::ios::binary);
  std::filebuf* srcbuf = src.rdbuf();
  off_t bytes_to_read = extra_info.metadata.start;
  static const off_t chunk_size = 512*1024; // 512KB
  // static const off_t chunk_size = 128; // for testing
  std::unique_ptr<char[]> mem (new char[chunk_size]);
  while (bytes_to_read) {
    auto num = srcbuf->sgetn(mem.get(), std::min(bytes_to_read, chunk_size));
    os.write(mem.get(), num);
    bytes_to_read -= num;
    if (!num and bytes_to_read)
      throw OCPI::Util::Error("Strange error counting bytes! If num (%jd) is 0, then bytes_to_read (%jd) should be as well.",
        static_cast<intmax_t>(num),
        static_cast<intmax_t>(bytes_to_read));
  }
};

} // EzXml
} // Util
} // OCPI
