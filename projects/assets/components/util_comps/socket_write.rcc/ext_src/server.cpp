//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "server.hpp"
#include <iostream> // AV-1784
#ifndef ASIO_STANDALONE
#include <boost/bind.hpp>
#endif
#include <signal.h>

namespace http {
namespace server {

server::server(const std::string& address, const short port /*const std::string& port,
    const std::string& doc_root*/)
  : io_service_(),
    signals_(io_service_),
    acceptor_(io_service_),
    connection_manager_(),
    new_connection_()
    /*,
    request_handler_(doc_root) */
{
  // Register to handle the signals that indicate when the server should exit.
  // It is safe to register for the same signal multiple times in a program,
  // provided all registration for the specified signal is made through Asio.
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
#ifdef ASIO_STANDALONE
  signals_.async_wait(std::bind(&server::handle_stop, this));
#else
  signals_.async_wait(boost::bind(&server::handle_stop, this));
#endif
  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  asio::ip::tcp::resolver resolver(io_service_);
  // AV took out the port name:
  asio::ip::tcp::resolver::query query(address, /* port */ "");
  asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
  // AV forcing a numeric port:
  endpoint.port(port);
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();

  start_accept();
}

void server::run()
{
  // The io_service::run() call will block until all asynchronous operations
  // have finished. While the server is running, there is always at least one
  // asynchronous operation outstanding: the asynchronous accept call waiting
  // for new incoming connections.
  // if (is_debugging) std::cerr << "SocketWriter: server::run() start" << std::endl;
  io_service_.run();
  // if (is_debugging) std::cerr << "SocketWriter: server::run() end" << std::endl;
}

void server::start_accept()
{
  if (is_debugging) std::cerr << "SocketWriter: server::start_accept() accepting a connection" << std::endl;
  new_connection_.reset(new connection(io_service_,
        connection_manager_/*, request_handler_*/));
  acceptor_.async_accept(new_connection_->socket(),
#ifdef ASIO_STANDALONE
      std::bind(&server::handle_accept, this,
        std::placeholders::_1));
#else
      boost::bind(&server::handle_accept, this,
        asio::placeholders::error));
#endif
}

void server::handle_accept(const asio::error_code& e)
{
  // Check whether the server was stopped by a signal before this completion
  // handler had a chance to run.
  if (!acceptor_.is_open())
  {
    // if (is_debugging) std::cerr << "SocketWriter: server::handle_accept() says it is not open(?)" << std::endl;
    // if (is_debugging) std::cerr << "SocketWriter: " << e << " :: " << e.message() << std::endl;
    return;
  }

  if (!e)
  {
    // if (is_debugging) std::cerr << "SocketWriter: " << acceptor_.local_endpoint() << std::endl;
    connection_manager_.start(new_connection_);
  } else {
    if (is_debugging) std::cerr << "SocketWriter: server::handle_accept() has error: " << e << " :: " << e.message() << std::endl;
  }

  // if (is_debugging) std::cerr << "SocketWriter: server::handle_accept() re-calling start_accept()" << std::endl;
  start_accept();
}

void server::handle_stop()
{
  if (is_debugging) std::cerr << "SocketWriter: server::handle_stop() closing all connections" << std::endl;
  // The server is stopped by cancelling all outstanding asynchronous
  // operations. Once all operations have finished the io_service::run() call
  // will exit.
  acceptor_.close();
  connection_manager_.stop_all();
  io_service_.stop(); // AV added
}

void server::push_back(outbound::shared_metapair_t in_data) { // new AV
  connection_manager_.push_back(in_data);
}

/// How many listeners are there
size_t server::listeners() const { // new AV
  return connection_manager_.listeners();
}

} // namespace server
} // namespace http
