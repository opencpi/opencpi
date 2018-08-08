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

#ifndef OUTBOUND_HPP
#define OUTBOUND_HPP
// Include asio FIRST always - http://stackoverflow.com/a/33082763
#include <asio.hpp>
#include <string>
#include <vector>
#include <memory>

// Put into namespace to match stuff copied from Boost
namespace http {
namespace server {

static const bool is_debugging = false;

/// Data to be sent to a client.
struct outbound
{
  /// Shared data pointer types
  typedef struct {
    typedef uint8_t value_type;
    size_t len;
    value_type *data;
  } memptr_t;
  typedef struct {
    bool message_mode;
    struct {
      uint32_t len, opcode;
    } hdr;
  } metadata_t;

  typedef std::shared_ptr< const memptr_t > shared_memptr_t;
  typedef std::pair< metadata_t, shared_memptr_t > shared_metapair_t;

  /// Constructor takes a shared pointer to the data
  outbound(shared_metapair_t in_data) : content(in_data) {};
  // Copy constructor
  outbound(outbound & /* unused */) {throw(std::string("Copy constructor called and not implemented"));};
  // Move constructor
  outbound(outbound &&x) { // AV-2006
    content.first = std::move(x.content.first);
    content.second = std::move(x.content.second);
  };

  /// The content to be sent.
  shared_metapair_t content;

  /// Convert the content into an asio buffer. The buffer does not own the
  /// underlying memory blocks, therefore the object must remain valid and
  /// not be changed until the write operation has completed.
  std::vector<asio::const_buffer> to_buffer() const {
    std::vector<asio::const_buffer> buf;
    if (content.first.message_mode) { // message mode so insert header
      buf.reserve(2);
      buf.emplace_back((void *) &content.first.hdr, sizeof(content.first.hdr));
    }
    // create "data"
    buf.emplace_back(content.second->data, content.second->len);
    return buf;
  }
};

} // namespace server
} // namespace http

#endif // OUTBOUND_HPP
