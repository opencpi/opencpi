//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP
// Include asio FIRST always - http://stackoverflow.com/a/33082763
#include <asio.hpp>
#include <deque>
#ifdef ASIO_STANDALONE
#include <memory> // enable_shared_from_this
#else
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif
#include "outbound.hpp" // new AV
// #include "reply.hpp"
// #include "request.hpp"
// #include "request_handler.hpp"
// #include "request_parser.hpp"

namespace http {
namespace server {

class connection_manager;

/// Represents a single connection from a client.
class connection
#ifdef ASIO_STANDALONE
  : public std::enable_shared_from_this<connection>
#else
  : public boost::enable_shared_from_this<connection>,
    private boost::noncopyable
#endif
{
public:
  /// Construct a connection with the given io_service.
  explicit connection(asio::io_service& io_service,
      connection_manager& manager/*, request_handler& handler*/);

  ~connection(); // new AV

  /// Get the socket associated with the connection.
  asio::ip::tcp::socket& socket();

  /// Queue data to be sent
  void push_back(outbound::shared_metapair_t in_data); // new AV

#if 0
  /// Start the first asynchronous operation for the connection.
  void start();

  /// Stop all asynchronous operations associated with the connection.
  void stop();

private:
  /// Handle completion of a read operation.
  void handle_read(const asio::error_code& e,
      std::size_t bytes_transferred);

  /// Handle completion of a write operation.
  void handle_write(const asio::error_code& e);

#else
private:
#endif
  /// Socket for the connection.
  asio::ip::tcp::socket socket_;

  /// The manager for this connection.
  connection_manager& connection_manager_;
#if 0
  /// The handler used to process the incoming request.
  request_handler& request_handler_;

  /// Buffer for incoming data.
  boost::array<char, 8192> buffer_;

  /// The incoming request.
  request request_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  /// The reply to be sent back to the client.
  reply reply_;
#endif
  void send_done(const asio::error_code& e);
  // Copying "strand" usage from https://stackoverflow.com/a/7756894 (AV-3257 Thanks, Dave P!)
  void push_back_internal(outbound::shared_metapair_t in_data); // new AV
  asio::io_service::strand strand_; // new AV: Forces safe serialization
  void send(); // new AV
  std::deque<outbound> outbound_queue; // new AV
};

#ifdef ASIO_STANDALONE
typedef std::shared_ptr<connection> connection_ptr;
#else
typedef boost::shared_ptr<connection> connection_ptr;
#endif

} // namespace server
} // namespace http

#endif // HTTP_CONNECTION_HPP
