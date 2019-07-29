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

#ifndef CLOCK_H
#define CLOCK_H
#include "ocpigen.h"

struct Clock {
  Worker &m_worker;
  std::string m_name, m_signal, m_reset;
  Port  *m_port;     // If not NULL, the port of the worker that this clock is owned by.
  size_t m_ordinal;  // within the worker
  bool   m_output;   // This clock is an output of its worker on its owned port or globally
  bool   m_internal; // This clock is internal to an assembly and not externalized.
  Clock(Worker &w);
  const char *parse(ezxml_t x);
  void rename(const char *name, Port *port);
  const char *cname() const { return m_name.c_str(); }
  const char *signal() const { return m_signal.c_str(); }
  const char *reset() const { return m_reset.c_str(); }
};

typedef std::vector<Clock*> Clocks;

#endif
