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

#include "XferEndPoint.h"
#include "Container.h"
#include "ContainerPort.h"
#include "ContainerWorker.h"
#include "ContainerApplication.h"
#include "ContainerLauncher.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OT = DataTransfer;
namespace OCPI {
  namespace Container {

LocalLauncher::~LocalLauncher() {}

void LocalLauncher::
createWorker(Launcher::Member &i) {
  i.m_worker = &i.m_containerApp->createWorker(i.m_impl->m_artifact,
					       i.m_name.c_str(),
					       i.m_impl->m_metadataImpl.m_xml,
					       i.m_impl->m_staticInstance,
					       i.m_slave ? i.m_slave->m_worker : NULL,
					       i.m_hasMaster,
					       i.m_member, i.m_crew ? i.m_crew->m_size : 1);
  // Now we need to set the initial properties - either from instance or from defaults
  for (unsigned p = 0; p < i.m_crew->m_propValues.size(); p++) {
    ocpiDebug("Setting the initial specified value of property '%s' of instance '%s'",
	      i.m_impl->m_metadataImpl.properties()[i.m_crew->m_propOrdinals[p]].cname(),
	      i.m_name.c_str());
    i.m_worker->setProperty(i.m_crew->m_propOrdinals[p], i.m_crew->m_propValues[p]);
  }
  unsigned nProps = i.m_impl->m_metadataImpl.nProperties();
  OU::Property *prop = i.m_impl->m_metadataImpl.properties();
  for (unsigned nn = 0; nn < nProps; nn++, prop++)
    if (prop->m_default && !prop->m_isParameter) {
      bool found = false;
      for (unsigned m = 0; m < i.m_crew->m_propValues.size(); m++)
	if (i.m_crew->m_propOrdinals[m] == prop->m_ordinal) {
	  found = true;
	  break;
	}
      if (!found) {
	ocpiDebug("Setting the default value of property '%s' of instance '%s'",
		  prop->m_name.c_str(), i.m_name.c_str());
	i.m_worker->setProperty(prop->m_ordinal, *prop->m_default);
      }
    }
}

// Local launcher: do local launching for the instances managed by this launcher.
// Do the initial connection work that can be done in this first pass.
bool LocalLauncher::
launch(Launcher::Members &instances, Launcher::Connections &connections) {
  m_more = false;
  Launcher::Member *i = &instances[0];
  for (unsigned n = 0; n < instances.size(); n++, i++)
    if (&i->m_container->launcher() == this && i->m_hasMaster)
      createWorker(*i);
  i = &instances[0];
  for (unsigned n = 0; n < instances.size(); n++, i++)
    if (&i->m_container->launcher() == this && !i->m_hasMaster) {
      if (i->m_slave && !i->m_slave->m_worker)
	m_more = true; // instance is local, but its slave is remote
      else
	createWorker(*i);
    }
  for (unsigned n = 0; n < connections.size(); n++) {
    Launcher::Connection &c = connections[n];
    c.prepare();
    // First create the local worker or external ports
    if (c.m_in.m_launcher == this) {
      if (c.m_in.m_member)
	c.m_in.m_port = &c.m_in.m_member->m_worker->getPort(c.m_in.m_name, c.m_out.m_scale,
							    c.m_in.m_params);
      else if (c.m_in.m_name)
	c.m_in.m_port = new ExternalPort(c, true);
    }
    if (c.m_out.m_launcher == this) {
      if (c.m_out.m_member)
	c.m_out.m_port = &c.m_out.m_member->m_worker->getPort(c.m_out.m_name, c.m_in.m_scale,
							      c.m_out.m_params);
      else if (c.m_out.m_name)
	c.m_out.m_port = new ExternalPort(c, false);
    }
    if (c.m_in.m_launcher == this && c.m_in.m_port && c.m_in.m_port->initialConnect(c))
      m_more = true;
    if (c.m_out.m_launcher == this && c.m_out.m_port && !c.m_out.m_done &&
	c.m_out.m_port->initialConnect(c))
      m_more = true;
  }
  return m_more;
}

bool LocalLauncher::
work(Launcher::Members &instances, Launcher::Connections &connections) {
  m_more = false;
  Launcher::Member *i = &instances[0];
  for (unsigned n = 0; n < instances.size(); n++, i++)
    if (&i->m_container->launcher() == this && !i->m_hasMaster && !i->m_worker) {
      if (i->m_slave && !i->m_slave->m_worker)
	m_more = true; // instance is local, but its slave is remote
      else
	createWorker(*i);
    }
  for (unsigned n = 0; n < connections.size(); n++) {
    Launcher::Connection &c = connections[n];
    if (c.m_in.m_port) {
      if (c.m_out.m_initial.length() || c.m_out.m_final.length()) {
	if (c.m_in.m_port->finalConnect(c))
	  m_more = true;
	c.m_out.m_initial.clear();
	c.m_out.m_final.clear();
      }
      if (!c.m_in.m_done)
	m_more = true;
    }
    if (c.m_out.m_port) {
      if (c.m_in.m_initial.length() || c.m_in.m_final.length()) {
	if (c.m_out.m_port->finalConnect(c))
	  m_more = true;
	c.m_in.m_initial.clear();
	c.m_in.m_final.clear();
      }
      if (!c.m_out.m_done)
	m_more = true;
    }
  }
  return m_more;
}

Launcher::Member::
Member()
  : m_containerApp(NULL), m_container(NULL), m_impl(NULL), m_hasMaster(false),
    m_doneInstance(false), m_slave(NULL), m_worker(NULL), m_member(0), m_crew(NULL) {
}
Launcher::Crew::
Crew()
  : m_size(1) {
}
Launcher::Port::
Port()
  : m_launcher(NULL), m_container(NULL), m_containerApp(NULL), m_member(NULL), m_port(NULL),
    m_name(NULL), m_metaPort(NULL), m_scale(0), m_index(0), m_url(NULL),
    m_transportBridged(false), m_started(false), m_done(false) {
}

Launcher::Connection::
Connection()
  : m_bufferSize(SIZE_MAX), m_done(false) {
}
void Launcher::Connection::
prepare() {
  // Make sure that the input side knows about any transports implied at the
  // output side.
  const char *cp;
  if (!OU::findString(m_in.m_params, "endpoint", cp) &&
      !OU::findString(m_in.m_params, "transport", cp)) {
    // There is no transport specified on the input side,
    // check the output side.
    std::string transport;
    if (OU::findString(m_out.m_params, "endpoint", cp))
      OT::EndPoint::getProtocolFromString(cp, transport);
    else if (OU::findString(m_out.m_params, "transport", cp))
      transport = cp;
    if (transport.length())
      m_in.m_params.add("transport", transport.c_str());
  }
}
Transport::
Transport()
  : roleIn(OCPI::RDT::NoRole), roleOut(OCPI::RDT::NoRole), optionsIn(0), optionsOut(0) {
}
  }
}
