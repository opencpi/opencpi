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

#include <unistd.h>
#include <fcntl.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <vector>
#include <string>
#include <memory>
#include <cstdlib>

#include "file_write_demux-worker.hh"

#ifdef __APPLE__
// On MacOS this exists but is missing from the header
extern int fdatasync(int);
#endif

#ifndef likely
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace File_write_demuxWorkerTypes;

class File_write_demuxWorker : public File_write_demuxWorkerBase {
protected:
  // Helper class that does the real work. Up to 256 of them instantiated on demand.
  class FileWriter {
    File_write_demuxWorker &parent;
    const uint8_t id;
    const bool message_mode;
    int fd;
    uint32_t hdr[2]; // length + opcode

  public:
    FileWriter(File_write_demuxWorker &parent_in, const uint8_t id_in) :
      parent(parent_in), id(id_in), message_mode(parent.properties().outFile.messagesInFile[id]), fd(-1)
    {
      // limit digits used 1..3
      const int dig = std::max(1, std::min(3, static_cast<int>(parent.properties().outFile.digits)));
      // create filename based on properties
      char *fname;
      const int res = asprintf(&fname, "%s%0*d%s", parent.properties().outFile.prefix, dig, id, parent.properties().outFile.suffix);
      assert (res > 0);
      // check for remaining '%' signs for strftime
      if (strchr(fname, '%')) {
        const size_t newsize = strlen(fname)+2000;
        char *fname2 = static_cast<char *>(calloc(newsize,1));
        const time_t now = time(NULL);
        const struct tm* tmp = localtime(&now);
        if (strftime(fname2, newsize-1, fname, tmp))
          std::swap(fname, fname2);
        free(fname2);
      }
      fd = open(fname, O_CREAT|O_WRONLY|O_TRUNC, 0666);
      if (!active())
        parent.setError("error creating file \"%s\": %s", fname, strerror(errno));
      free(fname);
      hdr[1] = id;
    } // Constructor

    RCCResult work() {
      if (unlikely(!active())) // already set error in constructor
        return RCC_ERROR;
      // Purposely do NOT update these properties unless we are in a good state
      ++parent.properties().current.Opcode[id].messages;
      ++parent.properties().current.Total.messages;
      hdr[0] = parent.in.length();
      parent.properties().current.Opcode[id].bytes += hdr[0];
      parent.properties().current.Total.bytes += hdr[0];
      ssize_t res;
      if (message_mode) {
        if (unlikely((res = write(fd, hdr, sizeof(hdr))) < 0 || static_cast<size_t>(res) != sizeof(hdr))) {
          parent.setError("error writing header to file: %s", strerror(errno));
          return RCC_ERROR;
        }
      }
      if (unlikely((res = write(fd, parent.in.data(), parent.in.length())) < 0 || static_cast<size_t>(res) != parent.in.length())) {
        parent.setError("error writing data to file: %s", strerror(errno));
        return RCC_ERROR;
      }
      // Various ways to declare done here
      const bool done =
        // ZLM
        (!parent.in.length() and id == parent.properties().stopOn.ZLM) ||
        // This specific Opcode...
        compare_prop(parent.properties().stopOn.Opcode[id].bytes,    parent.properties().current.Opcode[id].bytes) ||
        compare_prop(parent.properties().stopOn.Opcode[id].messages, parent.properties().current.Opcode[id].messages) ||
        // Any Opcode...
        compare_prop(parent.properties().stopOn.Any.bytes,    parent.properties().current.Opcode[id].bytes) ||
        compare_prop(parent.properties().stopOn.Any.messages, parent.properties().current.Opcode[id].messages) ||
        // All Opcodes...
        compare_prop(parent.properties().stopOn.Total.bytes,    parent.properties().current.Total.bytes) ||
        compare_prop(parent.properties().stopOn.Total.messages, parent.properties().current.Total.messages);

      // If we have a listener, they get the message forwarded
      if (parent.out.isConnected()) // Want to send this buffer to next Component
        parent.out.send(parent.in); // OCPI call for zero-copy send from input to output
      else // Drop it
        parent.in.advance();
      return done ? RCC_DONE : RCC_OK;
    } // work

    inline bool active() const { return (fd >= 0); }

    void flush() { if (active()) fdatasync(fd); }

    // Utility function that compares a value to a property if the property is non-zero
    template <typename T>
    inline bool compare_prop (const T prop, const T val) const {
      return prop && (prop <= val);
    }

    ~FileWriter() {
      if (active())
        close(fd);
    } // Destructor
  }; // FileWriter

  // File_write_demuxWorker's variables:
  std::vector<std::unique_ptr<FileWriter> > workers; // On-demand FileWriters for each of the 256 opcodes
  RunCondition m_RunCondition;                       // Run Condition to specify when run() is called

public:
  // Constructor
  File_write_demuxWorker() {
#if __GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 4))
    workers.resize(256);
#else
    // workaround for CentOS 6, whose vector.resize() blows up trying to call copy constructor
    workers.reserve(256);
    for (int i = 0; i <256; ++i)
      workers.emplace_back(static_cast<FileWriter *>(NULL));
#endif
    // Bug? AV-4109 - we should optionally do output port here
    m_RunCondition.setPortMasks(1<<FILE_WRITE_DEMUX_IN, RCC_NO_PORTS);
    setRunCondition(&m_RunCondition);
  }

RCCResult stop() {
    // Flush output files when paused
    /* CentOS6's default GCC doesn't allow range operator:
    for (auto& ptr : workers)
      if (ptr)
        ptr->flush();
    */
    for (auto i = 0; i<256; ++i)
      if (workers[i])
        workers[i]->flush();
    return RCC_OK;
  }

  RCCResult run(bool /*timedout*/) {
    const uint8_t op = in.opCode();
    if (unlikely(!workers[op])) // Instantiate new worker on new opcode
      workers[op].reset(new FileWriter(*this, op));
    return workers[op]->work();
  } // run
};

FILE_WRITE_DEMUX_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
FILE_WRITE_DEMUX_END_INFO
