//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP
// Include asio FIRST always - http://stackoverflow.com/a/33082763
#include <asio.hpp>
#include <string>
#ifndef ASIO_STANDALONE
#include <boost/noncopyable.hpp>
#endif
#include "connection.hpp"
#include "connection_manager.hpp"
// #include "request_handler.hpp"

namespace http {
namespace server {

/// The top-level class of the HTTP server.
class server
#ifndef ASIO_STANDALONE
  : private boost::noncopyable
#endif
{
public:
  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit server(const std::string& address, const short port /*const std::string& port,
      const std::string& doc_root*/);

  /// Run the server's io_service loop.
  void run();

  /// Push new data out to listener(s)
  void push_back(outbound::shared_metapair_t in_data); // new AV

  /// How many listeners are there
  size_t listeners() const; // new AV

private:
  /// Initiate an asynchronous accept operation.
  void start_accept();

  /// Handle completion of an asynchronous accept operation.
  void handle_accept(const asio::error_code& e);

#if 1
  // new AV
public:
#endif

  /// Handle a request to stop the server.
  void handle_stop();

#if 1
  // new AV
private:
#endif

  /// The io_service used to perform asynchronous operations.
  asio::io_service io_service_;

  /// The signal_set is used to register for process termination notifications.
  asio::signal_set signals_;

  /// Acceptor used to listen for incoming connections.
  asio::ip::tcp::acceptor acceptor_;

  /// The connection manager which owns all live connections.
  connection_manager connection_manager_;

  /// The next connection to be accepted.
  connection_ptr new_connection_;
#if 0
  /// The handler for all incoming requests.
  request_handler request_handler_;
#endif
};

} // namespace server
} // namespace http

#endif // HTTP_SERVER_HPP
