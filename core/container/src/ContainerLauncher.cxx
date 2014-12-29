#include "DtSharedMemoryInterface.h" // FIXME This should simply be "DtEndPoint.h"
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

void LocalLauncher::
createWorker(Launcher::Instance &i) {
  i.m_worker = &i.m_containerApp->createWorker(i.m_impl->m_artifact,
					       i.m_name.c_str(),
					       i.m_impl->m_metadataImpl.m_xml,
					       i.m_impl->m_staticInstance,
					       i.m_slave ? i.m_slave->m_worker : NULL);
  // Now we need to set the initial properties - either from instance or from defaults
  for (unsigned p = 0; p < i.m_propValues.size(); p++)
    i.m_worker->setProperty(i.m_propOrdinals[p], i.m_propValues[p]);
  unsigned nProps = i.m_impl->m_metadataImpl.m_nProperties;
  OU::Property *prop = i.m_impl->m_metadataImpl.m_properties;
  for (unsigned nn = 0; nn < nProps; nn++, prop++)
    if (prop->m_default && !prop->m_isParameter) {
      bool found = false;
      for (unsigned m = 0; m < i.m_propValues.size(); m++)
	if (i.m_propOrdinals[m] == prop->m_ordinal) {
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
launch(Launcher::Instances &instances, Launcher::Connections &connections) {
  m_more = false;
  Launcher::Instance *i = &instances[0];
  for (unsigned n = 0; n < instances.size(); n++, i++)
    if (&i->m_container->launcher() == this && i->m_hasMaster)
      createWorker(*i);
  i = &instances[0];
  for (unsigned n = 0; n < instances.size(); n++, i++)
    if (&i->m_container->launcher() == this && !i->m_hasMaster)
      createWorker(*i);
  Launcher::Connection *c = &connections[0];
  for (unsigned n = 0; n < connections.size(); n++, c++) {
    c->prepare();
    if (c->m_in.m_launcher == this) {
      OA::Worker &wIn = *c->m_in.m_instance->m_worker;
      c->m_in.m_port = &wIn.getPort(c->m_in.m_name);
      if (c->m_out.m_launcher == this) {
	// Both ports of the connection is under this launcher
	OA::Worker &wOut = *c->m_out.m_instance->m_worker;
	c->m_out.m_port = &wOut.getPort(c->m_out.m_name);
	// Connection is entirely under the purview of this launcher.
	c->m_in.m_port->connect(*c->m_out.m_port, c->m_in.m_params, c->m_out.m_params);
      } else if (c->m_url) {
	// Input that is connected to a URL.  We will do this locally
	c->m_in.m_port->connectURL(c->m_url, c->m_in.m_params, c->m_out.m_params);
      } else {
	// We are the input side, some other launcher has the output
	c->m_in.m_port->containerPort().getInitialProviderInfo(c->m_in.m_params,
							       c->m_in.m_initial);
	m_more = true;
      }
    } else if (c->m_out.m_launcher == this) {
      // Output is here, but input is elsewhere or external
      OA::Worker &wOut = *c->m_out.m_instance->m_worker;
      c->m_out.m_port = &wOut.getPort(c->m_out.m_name);
      if (c->m_url)
	// Input that is connected to a URL.
	// We will do this locally
	c->m_out.m_port->connectURL(c->m_url, c->m_out.m_params, c->m_in.m_params);
      else
	// Since input is accessed first, we do nothing here at this time.
	// But we need the info, so we "need more"
	m_more = true;
    }
  }
  return m_more;
}

bool LocalLauncher::
work(Launcher::Instances &, Launcher::Connections &connections) {
  m_more = false;
  Launcher::Connection *c = &connections[0];
  for (unsigned n = 0; n < connections.size(); n++, c++)
    if (c->m_in.m_launcher == this) {
      if (c->m_out.m_initial.length()) {
	if (c->m_in.m_port->containerPort().setInitialUserInfo(c->m_out.m_initial,
							       c->m_in.m_final))
	  m_more = true;
	c->m_out.m_initial.clear();
      } else if (c->m_out.m_final.length()) {
	c->m_out.m_port->containerPort().setFinalUserInfo(c->m_out.m_final);
	c->m_out.m_final.clear();
      }
    } else if (c->m_out.m_launcher == this) {
      if (c->m_in.m_initial.length()) {
	if (c->m_out.m_port->containerPort().setInitialProviderInfo(c->m_out.m_params,
								    c->m_in.m_initial,
								    c->m_out.m_initial))
	  m_more = true;
	c->m_in.m_initial.clear();
      } else if (c->m_in.m_final.length()) {
	if (c->m_out.m_port->containerPort().setFinalProviderInfo(c->m_in.m_final,
								  c->m_out.m_final))
	  m_more = true;
	c->m_in.m_final.length();
      }      
    }
  return m_more;
}

Launcher::Instance::
Instance()
  : m_containerApp(NULL), m_container(NULL), m_impl(NULL), m_hasMaster(false),
    m_doneInstance(false), m_slave(NULL), m_worker(NULL), m_crewSize(1), m_member(0) {
}
Launcher::Port::
Port()
  : m_launcher(NULL), m_instance(NULL), m_port(NULL), m_name(NULL) {
}

Launcher::Connection::
Connection()
    : m_url(NULL) {
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
  if (m_in.m_launcher != m_out.m_launcher) {
    // For now, force connections to use the sockets transport if the
    // launchers are different, and there is non specified
    const char *endpoint = NULL, *transport = NULL;
    if (!OU::findString(m_in.m_params, "endpoint", endpoint) &&
	!OU::findString(m_in.m_params, "transport", transport))
      m_in.m_params.add("transport", "socket");
  }
}


  }
}
