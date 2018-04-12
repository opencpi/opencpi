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

// Include asio FIRST always - http://stackoverflow.com/a/33082763
#include "server.hpp"

#include <unistd.h>
#include <fcntl.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <vector>
#include <string>
#include <memory>
#include <cstdlib>
#include <cinttypes>

#ifdef ASIO_STANDALONE
#include <thread>
#else
#include <boost/thread.hpp>
#endif

#include "socket_write-worker.hh"

//TODO: Move this to proper client/worker logging
#include "OcpiOsDebugApi.hh"
#define ocpiInfo(...) ::OCPI::OS::logPrint(OCPI_LOG_INFO, __VA_ARGS__)
#define ocpiDebug(...) ::OCPI::OS::logPrint(OCPI_LOG_DEBUG_MIN, __VA_ARGS__)

#ifndef likely
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Socket_writeWorkerTypes;

class Socket_writeWorker : public Socket_writeWorkerBase {
protected:
  // buffer_t: Binds a shared_memptr_t (which is a non-owning shared memory pointer) with the owning
  // RCCUserBuffer pointer from the Framework. Note the shared_memptr_t is copied and NOT a reference or
  // pointer because we want the internal shared pointer to acknowledge we are still using the memory.
  typedef std::pair< http::server::outbound::shared_memptr_t, RCCUserBuffer* > buffer_t;
  // buffer_fifo_t: A FIFO of buffer_t.
  typedef std::deque<buffer_t> buffer_fifo_t;

  // Helper class that does the real work. Up to 256 of them instantiated on startup.
  // On construction, queries properties and launches port listener.
  class SocketWriter {
    Socket_writeWorker &parent;     // non-const because of calls we make to it
    const short id;                 // opcode
    const short port;               // outbound port
    const bool message_mode;        // determines insertion of opcode and length in outbound stream
    http::server::server my_server; // Poorly named socket server (copied sample code)
  public:
    SocketWriter(Socket_writeWorker &parent_in, const short id_in, const short port_in) :
      parent(parent_in),
      id((id_in == -1)?0:id_in), // -1 is special meaning of all, but properties refer to 0
      port(port_in),
      message_mode(parent.properties().outSocket.messagesInStream[id]),
      my_server(parent.properties().outSocket.address, port)
    {
      // Kick off background thread for server
      ocpiDebug("SocketWriter: Launching background thread for opcode %d (%s:%d) (message_mode=%d)", id, parent.properties().outSocket.address, port, message_mode);
#ifdef ASIO_STANDALONE
      std::thread t(&http::server::server::run, &my_server);
#else
      boost::thread t(&http::server::server::run, &my_server);
#endif
      t.detach();
    } // Constructor

    RCCResult work(const buffer_t buf) {
      // Intentionally called by-value because we want the shared pointer to know it is being used.
      const auto len = buf.second->length();
      // Update stats regardless of client state
      ++parent.properties().current.Opcode[id].messages;
      ++parent.properties().current.Total.messages;
      parent.properties().current.Opcode[id].bytes += len;
      parent.properties().current.Total.bytes += len;
      const bool ZLM_trigger = (0 == len) and (buf.second->opCode() == parent.properties().stopOn.ZLM);

      if (active()) {
        // ocpiDebug("SocketWriter: Sending to %d client(s).", my_server.listeners());
        // TODO: Should probably do something about endianness with hdr_out.
        const http::server::outbound::metadata_t hdr_out = {message_mode, {static_cast<uint32_t>(buf.first->len), buf.second->opCode()}};
        my_server.push_back( std::make_pair(hdr_out, buf.first) );
      } else {
        ocpiDebug("SocketWriter: For opcode %d (port %d) received data with no client(s). Dropping data.", id, port);
      }

      // Various ways to declare done here
      if (unlikely(
          // ZLM (using original opcode)
          ZLM_trigger ||
          // This specific Opcode...
          compare_prop(parent.properties().stopOn.Opcode[id].bytes,    parent.properties().current.Opcode[id].bytes) ||
          compare_prop(parent.properties().stopOn.Opcode[id].messages, parent.properties().current.Opcode[id].messages) ||
          // Any Opcode...
          compare_prop(parent.properties().stopOn.Any.bytes,    parent.properties().current.Opcode[id].bytes) ||
          compare_prop(parent.properties().stopOn.Any.messages, parent.properties().current.Opcode[id].messages) ||
          // All Opcodes...
          compare_prop(parent.properties().stopOn.Total.bytes,    parent.properties().current.Total.bytes) ||
          compare_prop(parent.properties().stopOn.Total.messages, parent.properties().current.Total.messages)
        ))
        return RCC_DONE;
      return RCC_OK;
    } // work

    inline bool active() const { return (my_server.listeners() > 0); }

    // Utility function that compares a value to a property if the property is non-zero
    template <typename T>
    inline bool compare_prop (const T prop, const T val) const {
      return prop && (prop <= val);
    }

    ~SocketWriter() {
      ocpiDebug("SocketWriter: [%d] Destructor called.", id);
      my_server.handle_stop();
      usleep(100 * 1000); // 100 ms. Not sure if really needed, but got rid of a lot of Valgrind errors as the threads were cleaning up.
      uint_fast32_t cnt = 1;
      // Wait up to 2.5s for all listeners to go away
      while ((cnt <= 5) and (my_server.listeners() > 0)) {
        ocpiDebug("SocketWriter: [%d] Still have %zd listeners in destructor: %" PRIuFAST32 " of 5", id, my_server.listeners(), cnt);
        if (0 == usleep(500 * 1000)) // 500 ms
          ++cnt;
        else
          ocpiDebug("SocketWriter: [%d] destructor sleep interrupted and has %zd listeners", id, my_server.listeners());
      };
    } // Destructor
  }; // SocketWriter

  // Socket_writeWorker's variables:
  bool single_port;                                    // Using a single port or demuxing
  std::vector<std::unique_ptr<SocketWriter> > workers; // SocketWriters for each of the 256 opcodes
  buffer_fifo_t buffer_fifo;                           // FIFO of "take"n buffers
  RunCondition m_RunCondition;                         // Run Condition to specify when run() is called

public:
  // Constructor
  Socket_writeWorker() : single_port(false) {
#if __GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 4))
    workers.resize(256);
#else
    // workaround for CentOS 6, whose vector.resize() blows up trying to call copy constructor
    workers.reserve(256);
    for (int i = 0; i <256; ++i)
      workers.emplace_back(static_cast<SocketWriter *>(NULL));
#endif
    // Tell framework to call us every 100ms even if no incoming data (to clear out completed buffers avoiding deadlock)
    // Bug? AV-4109 - we should optionally do output port here
    m_RunCondition.setPortMasks(1<<SOCKET_WRITE_IN, RCC_NO_PORTS);
    m_RunCondition.enableTimeout(10 * 1000); // 10ms
    setRunCondition(&m_RunCondition);
  }

  RCCResult start() {
    // Create SocketWriter if there is a port defined
    // We guard against <1024, but our documentation states it should really be <49151 but nobody cares
    if (properties().outSocket.port > 0) { // Using single port
      const auto port = properties().outSocket.port;
      if (port <= 1024)
        return setError("SocketWriter: Cannot use reserved port %d!", port);
      ocpiInfo("SocketWriter: Using single port %d", port);
      workers[0].reset(new SocketWriter(*this, -1, port));
      single_port = true;
    } else {
      const auto ports = properties().outSocket.ports;
      for (auto i = 0; i<256; ++i)
        if (ports[i] > 1024) {
          ocpiInfo("SocketWriter: Using port %d for opcode %d", ports[i], i);
          workers[i].reset(new SocketWriter(*this, i, ports[i]));
        } else {
          if (0 != ports[i])
            return setError("SocketWriter: Cannot use reserved port %d for opcode %d!", ports[i], i);
        }
      single_port = false;
    }
    sleep(1); // Let all listeners spin up and test clients connect
    return RCC_OK;
  }

  RCCResult stop() {
    ocpiDebug("SocketWriter: Stopping all workers.");
    // Stop all workers when paused (for reconfiguration)
    for (auto i = 0; i<256; ++i)
      workers[i].reset();
    return RCC_OK;
  }

  RCCResult run(bool
#ifdef SW_FIFO_DEBUG
                     timedout
#endif
               ) {
    // Determine if things should be popped off (always; don't care why run() was called)
    while ((not buffer_fifo.empty()) and (1 == buffer_fifo.front().first.use_count()) ) {
      ocpiDebug("SocketWriter: Popping queued data found at %p", buffer_fifo.front().first.get());
      if (out.isConnected()) // Want to send this buffer to next Component
        out.send(*buffer_fifo.front().second); // OCPI call to hand buffer back to framework for transmit
      else // Drop it
        buffer_fifo.front().second->release(); // OCPI call to release buffer to framework
      buffer_fifo.pop_front(); // Remove our copy of it (~shared_ptr will handle the memptr_t object)
      // ocpiDebug("Queue now holds %zu memory pointers", buffer_fifo.size());
    }
#ifdef SW_FIFO_DEBUG
    if (buffer_fifo.size()) {
      ocpiDebug("SocketWriter: Master queue now holds %zu memory pointers (timedout=%d)", buffer_fifo.size(), timedout);
      for (auto &x : buffer_fifo) {
        ocpiDebug("SocketWriter: >> Smart pointer = %p, users = %zu, opcode=%d, length=%zd", x.first.get(), x.first.use_count(), x.second->opCode(), x.second->length());
      }
    }
#endif

    if (in.hasBuffer()) {
      const uint8_t op = in.opCode();
      if (unlikely(not single_port and !workers[op])) { // Incoming data on unhandled port
        setError("Incoming data on opcode %d without matching port number defined at start()", op);
        return RCC_ERROR;
      }
      // Massage the data for the asio server code
      // This data is redundant with what RCCUserBuffer knows about itself, but the magic is the shared_ptr;
      //   we don't want the shared_ptr to erase the RCCUserBuffer, so we allocate this small struct and see
      //   how many other objects are using it.
      auto memptr = std::make_shared<http::server::outbound::memptr_t>();
      memptr->len = in.length();
      memptr->data = reinterpret_cast<http::server::outbound::shared_metapair_t::second_type::element_type::value_type *>(in.data());

      ocpiDebug("SocketWriter: Adding to queue %zd bytes of data with smart pointer at %p", memptr->len, memptr.get());
      // buffer_fifo holds a pair of what the server wants and the RCCUserBuffer we need to recycle or transmit later
      buffer_fifo.emplace_back( std::make_pair(
                                  memptr,      // shared pointer to server requested len+data
                                  &(in.take()) // our buffer
                                )
                              );
      ocpiDebug("SocketWriter: Queue now holds %zu memory pointers", buffer_fifo.size());
#ifdef SW_FIFO_DEBUG
      for (auto &x : buffer_fifo) {
        ocpiDebug("SocketWriter: >> Smart pointer = %p, users = %zu, opcode=%d, length=%zd", x.first.get(), x.first.use_count(), x.second->opCode(), x.second->length());
      }
#endif
      return workers[single_port?0:op]->work(buffer_fifo.back()); // Give worker a copy of the latest buffer
    }
    // No buffer waiting
    return RCC_OK;
  } // run
};

SOCKET_WRITE_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
SOCKET_WRITE_END_INFO
