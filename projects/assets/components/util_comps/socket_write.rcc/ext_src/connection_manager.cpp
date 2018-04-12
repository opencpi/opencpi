//
// connection_manager.cpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection_manager.hpp"
#include <iostream> // AV-1784
#include <algorithm>
#ifndef ASIO_STANDALONE
#include <boost/bind.hpp>
#endif

namespace http {
namespace server {

void connection_manager::start(connection_ptr c)
{
  if (is_debugging) std::cerr << "SocketWriter: connection_manager::start() starting a connection" << std::endl;
  connections_.insert(c);
//  c->start();
}

void connection_manager::stop(connection_ptr c)
{
  if (is_debugging) std::cerr << "SocketWriter: connection_manager::stop() ending a connection" << std::endl;
  connections_.erase(c);
//  c->stop();
}

void connection_manager::stop_all()
{
#if 0
  std::for_each(connections_.begin(), connections_.end(),
      boost::bind(&connection::stop, _1));
#endif
  if (is_debugging) std::cerr << "SocketWriter: connection_manager::stop_all() clearing list" << std::endl;
  connections_.clear();
}

/// Push new data out to listener(s)
void connection_manager::push_back(outbound::shared_metapair_t in_data) { // new AV
  if (connections_.size())
    std::for_each(connections_.begin(), connections_.end(),
#ifdef ASIO_STANDALONE
        std::bind(&connection::push_back, std::placeholders::_1, in_data));
#else
        boost::bind(&connection::push_back, _1, in_data));
#endif
  else
    if (is_debugging) std::cerr << "SocketWriter: connection_manager::push_back() sent data with no listener(s)!" << std::endl;
}

/// How many listeners are there
size_t connection_manager::listeners() const { // new AV
  return connections_.size();
}

} // namespace server
} // namespace http
