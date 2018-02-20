//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <iostream> // AV-1784
#include <vector>
#ifndef ASIO_STANDALONE
#include <boost/bind.hpp>
#endif
#include "connection_manager.hpp"
// #include "request_handler.hpp"

namespace http {
namespace server {

connection::connection(asio::io_service& io_service,
    connection_manager& manager/*, request_handler& handler*/)
  : socket_(io_service),
    connection_manager_(manager),
    strand_(io_service) // new AV
    //,
    //request_handler_(handler)
{
}

asio::ip::tcp::socket& connection::socket()
{
  return socket_;
}
#if 0
void connection::start()
{
  socket_.async_read_some(asio::buffer(buffer_),
      boost::bind(&connection::handle_read, shared_from_this(),
        asio::placeholders::error,
        asio::placeholders::bytes_transferred));
}

void connection::stop()
{
  socket_.close();
}

void connection::handle_read(const asio::error_code& e,
    std::size_t bytes_transferred)
{
  if (!e)
  {
    boost::tribool result;
    boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
        request_, buffer_.data(), buffer_.data() + bytes_transferred);

    if (result)
    {
      request_handler_.handle_request(request_, reply_);
      asio::async_write(socket_, reply_.to_buffers(),
          boost::bind(&connection::handle_write, shared_from_this(),
            asio::placeholders::error));
    }
    else if (!result)
    {
      reply_ = reply::stock_reply(reply::bad_request);
      asio::async_write(socket_, reply_.to_buffers(),
          boost::bind(&connection::handle_write, shared_from_this(),
            asio::placeholders::error));
    }
    else
    {
      socket_.async_read_some(asio::buffer(buffer_),
          boost::bind(&connection::handle_read, shared_from_this(),
            asio::placeholders::error,
            asio::placeholders::bytes_transferred));
    }
  }
  else if (e != asio::error::operation_aborted)
  {
    connection_manager_.stop(shared_from_this());
  }
}

void connection::handle_write(const asio::error_code& e)
{
  if (!e)
  {
    // Initiate graceful connection closure.
    asio::error_code ignored_ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
  }

  if (e != asio::error::operation_aborted)
  {
    connection_manager_.stop(shared_from_this());
  }
}
#endif

void connection::send_done(const asio::error_code& e) // new AV
{
  if (!e) {
    if (is_debugging) std::cerr << "SocketWriter: connection::send_done() sent a chunk of size " << outbound_queue[0].content.second->len << std::endl;
    outbound_queue.pop_front(); // Remove the one we just sent
    if (!outbound_queue.empty())
      send();
  } else {
    // There's an error
    if (is_debugging) std::cerr << "SocketWriter: ERROR: connection::send_done() " << e << std::endl;
    connection_manager_.stop(shared_from_this());
  }
}

void connection::send() { // new AV
    if (is_debugging) std::cerr << "SocketWriter: connection::send() sending a chunk of size " << outbound_queue[0].content.second->len << std::endl;
    asio::async_write(socket_, outbound_queue[0].to_buffer(),
      strand_.wrap(
#ifdef ASIO_STANDALONE
        std::bind(&connection::send_done, shared_from_this(),
          std::placeholders::_1)
#else
        boost::bind(&connection::send_done, shared_from_this(),
          asio::placeholders::error)
#endif
      ));
}

void connection::push_back(outbound::shared_metapair_t in_data) { // new AV
  if (is_debugging) std::cerr << "SocketWriter: connection::push_back() queued a chunk of size " << in_data.second->len << std::endl;
  strand_.post(
#ifdef ASIO_STANDALONE
        std::bind(
#else
        boost::bind(
#endif
        &connection::push_back_internal,
        this,
        in_data
        )
    );
}

void connection::push_back_internal(outbound::shared_metapair_t in_data) { // new AV
  if (is_debugging) std::cerr << "SocketWriter: connection::push_back_internal() queued a chunk of size " << in_data.second->len << std::endl;
  outbound_queue.push_back(in_data);
  // If currently sending, just quit now. Otherwise, start a new send sequence
  if (outbound_queue.size() > 1)
    return;
  send();
}

connection::~connection() { // new AV, old handle_write copied/modified
  // Initiate graceful connection closure.
  asio::error_code ec;
  socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
  if (ec != asio::error::operation_aborted)
    socket_.close();
}

} // namespace server
} // namespace http
