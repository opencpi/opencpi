#include "Container.h"
#include "ContainerPort.h"
#include "ContainerWorker.h"
#include "ContainerApplication.h"
#include "ContainerLauncher.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
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
  for (unsigned n = 0; n < connections.size(); n++, c++)
    if (c->m_launchIn == this) {
      OA::Worker &wIn = *c->m_instIn->m_worker;
      c->m_input = &wIn.getPort(c->m_nameIn);
      if (c->m_launchOut == this) {
	// Both ports of the connection is under this launcher
	OA::Worker &wOut = *c->m_instOut->m_worker;
	c->m_output = &wOut.getPort(c->m_nameOut);
	// Connection is entirely under the purview of this launcher.
	c->m_input->connect(*c->m_output, c->m_paramsIn, c->m_paramsOut);
      } else if (c->m_launchOut) {
	// A connect between ports with different launchers
	c->m_input->containerPort().getInitialProviderInfo(c->m_paramsIn, c->m_ipi);
	m_more = true;
      } else if (c->m_url)
	// Input that is connected to a URL.  We will do this locally
	c->m_input->connectURL(c->m_url, c->m_paramsIn, c->m_paramsOut);
    } else if (c->m_launchOut == this) {
      // Output is here, but input is elsewhere or external
      OA::Worker &wOut = *c->m_instOut->m_worker;
      c->m_output = &wOut.getPort(c->m_nameOut);
      if (c->m_launchIn) {
	// A connect between ports with different launchers
	// Since input is accessed first, we do nothing here at this time.
      } else if (c->m_url)
	// Input that is connected to a URL.
	// We will do this locally
	c->m_output->connectURL(c->m_url, c->m_paramsOut, c->m_paramsIn);
    }
  return m_more;
}

bool LocalLauncher::
work(Launcher::Instances &, Launcher::Connections &connections) {
  m_more = false;
  Launcher::Connection *c = &connections[0];
  for (unsigned n = 0; n < connections.size(); n++, c++)
    if (c->m_launchIn == this) {
      if (c->m_iui.length()) {
	if (c->m_input->containerPort().setInitialUserInfo(c->m_iui, c->m_fpi))
	  m_more = true;
	c->m_iui.clear();
      } else if (c->m_fui.length()) {
	c->m_output->containerPort().setFinalUserInfo(c->m_fui);
	c->m_fui.clear();
      }
    } else if (c->m_launchOut == this) {
      if (c->m_ipi.length()) {
	if (c->m_output->containerPort().setInitialProviderInfo(c->m_paramsOut,
								c->m_ipi, c->m_iui))
	  m_more = true;
	c->m_ipi.clear();
      } else if (c->m_fpi.length()) {
	if (c->m_output->containerPort().setFinalProviderInfo(c->m_fpi, c->m_fui))
	  m_more = true;
	c->m_fpi.length();
      }      
    }
  return m_more;
}

Launcher::Instance::
Instance()
  : m_containerApp(NULL), m_container(NULL), m_impl(NULL), m_hasMaster(false),
    m_doneInstance(false), m_slave(NULL), m_worker(NULL) {
}
Launcher::Connection::
Connection()
  : m_launchIn(NULL), m_launchOut(NULL), m_instIn(NULL), m_instOut(NULL),
    m_input(NULL), m_output(NULL), m_nameIn(NULL), m_nameOut(NULL), m_url(NULL),
    m_paramsIn(NULL), m_paramsOut(NULL) {
}

  }
}
