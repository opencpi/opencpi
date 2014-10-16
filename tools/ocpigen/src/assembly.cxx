#include <assert.h>
#include "assembly.h"
// Generic (actually non-HDL) assembly support
// This isn't as purely generic as it should be  FIXME
namespace OU = OCPI::Util;
namespace OA = OCPI::API;

Assembly::
Assembly(Worker &w)
  : m_assyWorker(w), m_nInstances(0), m_nWCIs(0), m_instances(NULL), m_utilAssembly(NULL),
    m_language(w.m_language) {
}

Assembly::
~Assembly() {
}

InstanceProperty::
InstanceProperty() : property(NULL) {
}

void
Worker::
deleteAssy() {
  delete m_assembly;
}

// Find the OU::Assembly::Instance's port in the instance's worker
const char *Assembly::
findPort(OU::Assembly::Port &ap, InstancePort *&found) {
  Instance &i = m_instances[ap.m_instance];
  found = NULL;
  unsigned nn = 0;
  for (PortsIter pi = i.worker->m_ports.begin(); pi != i.worker->m_ports.end(); pi++, nn++) {
    Port &p = **pi;
    if (ap.m_name.empty()) {
	// Unknown ports can be found for data ports that have matching known roles
      if (ap.m_role.m_knownRole && p.matchesDataProducer(!ap.m_role.m_provider))
	if (found)
	  return OU::esprintf("Ambiguous connection to unnamed %s port on %s:%s",
			      ap.m_role.m_provider ? "input" : "output", i.wName, i.name);
	else
	  found = &i.m_ports[nn];
    } else if (!strcasecmp(p.name(), ap.m_name.c_str()))
      found = &i.m_ports[nn];;
  }
  if (!found)
    return OU::esprintf("Port '%s' not found for instance '%s' of worker '%s'",
			ap.m_name.c_str(), i.name, i.wName);
  return NULL;
}

// A key challenge here is that we may not know the width of the connection until we look at
// real ports
 const char *Assembly::
parseConnection(OU::Assembly::Connection &aConn) {
   const char *err;
  Connection &c = *new Connection(&aConn);
  m_connections.push_back(&c);
  //  findBool(aConn.m_parameters, "signal", c.m_isSignal);
  // In case the connection has no count, we default it to the 
  // width of the narrowest attached port
  InstancePort *found;
  size_t minCount = 1000;
  for (OU::Assembly::Connection::PortsIter api = aConn.m_ports.begin();
       api != aConn.m_ports.end(); api++) {
    OU::Assembly::Port &ap = *api;
    if ((err = findPort(ap, found)))
      return err;
    if (ap.m_index + (c.m_count ? c.m_count : 1) > found->m_port->count)
      return OU::esprintf("invalid index/count (%zu/%zu) for connection %s, port %s of "
			  "instance %s has count %zu",
			  ap.m_index, c.m_count ? c.m_count: 1, c.m_name.c_str(),
			  ap.m_name.empty() ? "<unknown>" : ap.m_name.c_str(),
			  m_instances[ap.m_instance].worker->m_name.c_str(),
			  found->m_port->count);
    size_t count = found->m_port->count - ap.m_index;
    if (count < minCount)
      minCount = count;
  }
  if (!c.m_count)
    c.m_count = minCount;
  for (OU::Assembly::Connection::PortsIter api = aConn.m_ports.begin();
       api != aConn.m_ports.end(); api++) {
    OU::Assembly::Port &ap = *api;
    if ((err = findPort(ap, found)) ||
	(err = found->m_port->fixDataConnectionRole(ap.m_role)))
      return err;
    if (!found->m_role.m_knownRole || found->m_role.m_bidirectional)
      found->m_role = ap.m_role;
    // Note that the count may not be known here yet
    if ((err = c.attachPort(*found, ap.m_index))) //, aConn.m_count)))
      return err;
    assert(c.m_count != 0);
  }
  if (aConn.m_externals.size() > 1)
    return "multiple external attachments on a connection unsupported";
  // Create instance ports (and underlying ports of this assembly worker).
  for (OU::Assembly::ExternalsIter ei = aConn.m_externals.begin(); ei != aConn.m_externals.end(); ei++) {
    OU::Assembly::External &ext = *ei;
    assert(aConn.m_ports.size() == 1);
    OU::Assembly::Port &ap = aConn.m_ports.front();
    if (!ext.m_role.m_knownRole) {
      assert(ap.m_role.m_knownRole);
      ext.m_role = ap.m_role;
    }
    assert(c.m_attachments.size() == 1);
    InstancePort &intPort = c.m_attachments.front()->m_instPort; // intPort corresponds to ap
    assert(intPort.m_port);
    if (ext.m_index + ext.m_count > intPort.m_port->count)
      return OU::esprintf("External port '%s' can't have index/count %zu/%zu "
			  "when internal port has count: %zu",
			  ext.m_name.c_str(), ext.m_index, ext.m_count, intPort.m_port->count);
    // Create the external port of this assembly
    // Start with a copy of the port, then patch it
    Port &p = intPort.m_port->clone(m_assyWorker, ext.m_name,
				    ext.m_count ? ext.m_count : c.m_count,
				    &ext.m_role, err);
    if (err)
      return OU::esprintf("External connection %s for port %s of instance %s error: %s",
			  c.m_name.c_str(), intPort.m_port->name(), intPort.m_instance->name,
			  err);
    InstancePort *ip = new InstancePort;
    ip->init(NULL, &p, &ext);
    if ((err = c.attachPort(*ip, 0)))
      return err;
  }
  return NULL;
}

Instance::
Instance()
  : instance(NULL), name(NULL), wName(NULL), worker(NULL), m_clocks(NULL), m_ports(NULL),
    m_iType(Application), attach(NULL), hasConfig(false), config(0) {
}

// This parses the assembly using the generic assembly parser in OU::
// It then does the binding to actual implementations.
const char *Assembly::
parseAssy(ezxml_t xml, const char **topAttrs, const char **instAttrs, bool noWorkerOk,
	  const char *outDir) {
  (void)noWorkerOk; // FIXME: when containers are generated.
  try {
    m_utilAssembly = new OU::Assembly(xml, m_assyWorker.m_implName, topAttrs, instAttrs);
  } catch (std::string &e) {
    return OU::esprintf("%s", e.c_str());
  }
  m_nInstances = m_utilAssembly->nUtilInstances();
  //   m_nConnections = m_utilAssembly->m_connections.size();
  const char *err;
 
  Instance *i = m_instances = new Instance[m_utilAssembly->nUtilInstances()];
  // Initialize our instances based on the generic assembly instances
  for (unsigned n = 0; n < m_utilAssembly->nUtilInstances(); n++, i++) {
    OU::Assembly::Instance *ai = &m_utilAssembly->utilInstance(n);
    i->instance = ai;
    i->name = i->instance->m_name.c_str();
    i->wName = i->instance->m_implName.size() ? i->instance->m_implName.c_str() : NULL;
    // Find the real worker/impl for each instance, sharing the Worker among instances
    Worker *w = NULL;
    if (!i->wName)
      return OU::esprintf("instance %s has no worker", i->name);
    for (Instance *ii = m_instances; ii < i; ii++)
      if (ii->wName && !strcmp(i->wName, ii->wName))
	w = ii->worker;
    // There are two instance attributes that we use when considering workers
    // in our worker assembly:
    // 1.  Whether the instance is reentrant, which means one "instance" here can actually
    //     be dynamically used simultaneously for multiple application instances.
    //     This implies there will be no hard connections to its ports.
    // 2.  Which configuration of parameters it is built with.
    // FIXME: better modularity would be a core worker, a parameterized worker, etc.
    size_t paramConfig; // zero is with default parameter values
    ezxml_t ix = ai->xml();
    bool hasConfig;
    if ((err = OE::getNumber(ix, "paramConfig", &paramConfig, &hasConfig, 0)))
      return err;
    // We consider the property values in two places.
    // Here we are saying that a worker is unique if it has (non-default) properties,
    // of if it specifically has a non-default parameter configuration.
    // So we create a new worker and hand it the properties, so that it can
    // actually use these values DURING PARSING since some aspects of parsing
    // need them.  But later below, we really parse the properties after we
    // know the actual properties and their types from the worker.
    // FIXME: we are assuming that properties here must be parameters?
    if (!w || ai->m_properties.size() || paramConfig) {
      if (!(w = Worker::create(i->wName, m_assyWorker.m_file, NULL, outDir,
			       hasConfig ? NULL : &ai->m_properties, paramConfig, err)))
	return OU::esprintf("for worker %s: %s", i->wName, err);
      m_workers.push_back(w);
    }
    i->worker = w;
    // Determine instance type as far as we can now
    switch (w->m_type) {
    case Worker::Application:   i->m_iType = Instance::Application; break;
    case Worker::Platform:      i->m_iType = Instance::Platform; break;
    case Worker::Device:        i->m_iType = Instance::Device; break;
    case Worker::Configuration: i->m_iType = Instance::Configuration; break;
    case Worker::Assembly:      i->m_iType = Instance::Assembly; break;
    default:;
      assert("Invalid worker type as instance" == 0);
    }
    // Initialize the instance ports
    InstancePort *ip = i->m_ports = new InstancePort[i->worker->m_ports.size()];
    for (unsigned n = 0; n < i->worker->m_ports.size(); n++, ip++) {
      ip->init(i, i->worker->m_ports[n], NULL);
      // If the instance in the OU::Assembly has "m_externals=true",
      // and this instance port has no connections in the OU::Assembly
      // then we add an external connection for the instance port. Prior to this,
      // we didn't have access to the worker metadata to know what all the ports are.
      if (ai->m_externals && ip->m_port->isData()) {
	Port *p = NULL;
	for (OU::Assembly::Instance::PortsIter pi = ai->m_ports.begin();
	     pi != ai->m_ports.end(); pi++)
	  if ((*pi)->m_name.empty()) {
	    // Port name empty means we don't know it yet.
	    InstancePort *found;
	    // Ignore errors here
	    if (!findPort(**pi, found))
	      if (ip == found)
		p = ip->m_port;
	  } else if (!strcasecmp((*pi)->m_name.c_str(), ip->m_port->name())) {
	    p = ip->m_port;
	    break;
	  } 
	if (!p)
	  m_utilAssembly->addExternalConnection(i->instance->m_ordinal, ip->m_port->name());
      }
    }
    // Parse property values now that we know the actual workers.
    OU::Assembly::Property *ap = &ai->m_properties[0];
    i->properties.resize(ai->m_properties.size());
    InstanceProperty *ipv = &i->properties[0];
    for (size_t n = ai->m_properties.size(); n; n--, ap++, ipv++)
      for (PropertiesIter pi = w->m_ctl.properties.begin(); pi != w->m_ctl.properties.end(); pi++) {
	OU::Property &pr = **pi;
	if (!strcasecmp(pr.m_name.c_str(), ap->m_name.c_str())) {
	  if (!pr.m_isParameter)
	    return OU::esprintf("property '%s' is not a parameter", ap->m_name.c_str());
	  ipv->property = &pr;
	  ipv->value.setType(pr);
	  if ((err = ipv->property->parseValue(ap->m_value.c_str(), ipv->value)))
	    return err;
	  break;
	}
      }
    // Parse type-specific aspects of the instance.
    if ((err = w->parseInstance(*i, ix)))
      return err;
  }
  // All parsing is done.
  // Now we fill in the top-level worker stuff.
  asprintf((char**)&m_assyWorker.m_specName, "local.%s", m_assyWorker.m_implName);
  // Properties:  we only set the canonical hasDebugLogic property, which is a parameter.
  if ((err = m_assyWorker.doProperties(xml, m_assyWorker.m_file.c_str(), true, false)))
    return err;
  // Parse the Connections, creating external ports for this assembly worker as needed.
  for (OU::Assembly::ConnectionsIter ci = m_utilAssembly->m_connections.begin();
       ci != m_utilAssembly->m_connections.end(); ci++)
    if ((err = parseConnection(*ci)))
      return err;
  // Check for unconnected non-optional data ports
  i = m_instances;
  for (unsigned n = 0; n < m_nInstances; n++, i++)
    if (i->worker && !i->worker->m_reusable) {
      InstancePort *ip = i->m_ports;
      for (unsigned nn = 0; nn < i->worker->m_ports.size(); nn++, ip++) {
	Port *pp = ip->m_port;
	if (ip->m_attachments.empty() && pp->isData() && !pp->isOptional())
	  return OU::esprintf("Port %s of instance %s of worker %s"
			      " is not connected and not optional",
			      pp->name(), i->name, i->worker->m_implName);
      }
    }
  return 0;
}

// Make this port an external port
// Not called for WCIs that are aggreated...
// Note that this is called for ports that are IMPLICITLY made external,
// rather than those that are explicitly connected as eternal
const char *Assembly::
externalizePort(InstancePort &ip, const char *name, size_t &ordinal) {
  Port &p = *ip.m_port;
  assert(!p.isData());
  std::string extName;
  OU::format(extName, "%s%zu", name, ordinal++);
  Connection &c = *new Connection(NULL, extName.c_str());
  c.m_count = p.count;
  m_connections.push_back(&c);
  const char *err;
  Port &extPort = p.clone(m_assyWorker, extName, p.count, NULL, err);
  if (err)
    return err;
  c.m_clock = extPort.clock = ip.m_instance->m_clocks[ip.m_port->clock->ordinal];
  assert(extPort.clock);
  OU::Assembly::External *ext = new OU::Assembly::External;
  ext->m_name = extPort.m_name;
  ext->m_role.m_provider = !p.master; // provisional
  ext->m_role.m_bidirectional = false;
  ext->m_role.m_knownRole = true;
  InstancePort &extIp = *new InstancePort(NULL, &extPort, ext);
  if ((err = c.attachPort(ip, 0)) ||
      (err = c.attachPort(extIp, 0)))
    return err;
  return NULL;
}

InstancePort *Assembly::
findInstancePort(const char *name) {
  // First, find the external
  for (ConnectionsIter cci = m_connections.begin(); cci != m_connections.end(); cci++) {
    Connection &cc = **cci;
    if (cc.m_external && !strcasecmp(cc.m_external->m_instPort.m_port->name(), name))
      // We found the external port, now find the internal connected port
      for (AttachmentsIter ai = cc.m_attachments.begin(); ai != cc.m_attachments.end(); ai++)
	if (cc.m_external != *ai)
	  return &(*ai)->m_instPort;
  }
  return NULL;
}

void Worker::
emitXmlWorker(FILE *f) {
  fprintf(f, "<worker name=\"%s", m_implName);
  // FIXME - share this param-named implname with emitInstance
  if (m_paramConfig && m_paramConfig->nConfig)
    fprintf(f, "-%zu", m_paramConfig->nConfig);
  fprintf(f, "\" model=\"%s\"", m_modelString);
  fprintf(f, " package=\"%s\"", m_package.c_str());
  if (m_specName && strcasecmp(m_specName, m_implName))
    fprintf(f, " specname=\"%s\"", m_specName);
  if (m_ctl.sizeOfConfigSpace)
    fprintf(f, " sizeOfConfigSpace=\"%llu\"", (unsigned long long)m_ctl.sizeOfConfigSpace);
  if (m_ctl.controlOps) {
    bool first = true;
    for (unsigned op = 0; op < OU::Worker::OpsLimit; op++)
      if (op != OU::Worker::OpStart &&
	  m_ctl.controlOps & (1 << op)) {
	fprintf(f, "%s%s", first ? " controlOperations=\"" : ",",
		OU::Worker::s_controlOpNames[op]);
	first = false;
      }
    if (!first)
      fprintf(f, "\"");
  }
  if (m_wci && m_wci->timeout())
    //  if (m_ports.size() && m_ports[0]->type == WCIPort && m_ports[0]->u.wci.timeout)
    fprintf(f, " Timeout=\"%zu\"", m_wci->timeout());
  if (m_slave)
    fprintf(f, "  Slave='%s.%s'", m_slave->m_implName, m_slave->m_modelString);
  fprintf(f, ">\n");
  unsigned nn;
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
    OU::Property *prop = *pi;
    prop->printAttrs(f, "property", 1, prop->m_isParameter); // suppress default values for parameters
    if (prop->m_isVolatile)
      fprintf(f, " volatile='1'");
    else if (prop->m_isReadable)
      fprintf(f, " readable='1'");
    if (prop->m_isInitial)
      fprintf(f, " initial='1'");
    else if (prop->m_isWritable)
      fprintf(f, " writable='1'");
    if (prop->m_readSync)
      fprintf(f, " readSync='1'");
    if (prop->m_writeSync)
      fprintf(f, " writeSync='1'");
    if (prop->m_readError)
      fprintf(f, " readError='1'");
    if (prop->m_writeError)
      fprintf(f, " writeError='1'");
    if (!prop->m_isReadable && !prop->m_isWritable && !prop->m_isParameter)
      fprintf(f, " padding='1'");
    if (prop->m_isIndirect)
      fprintf(f, " indirect=\"%zu\"", prop->m_indirectAddr);
    if (prop->m_isParameter) {
      fprintf(f, " parameter='1'");
      OU::Value *v = 
	m_paramConfig && prop->m_paramOrdinal < m_paramConfig->params.size() &&
	m_paramConfig->params[prop->m_paramOrdinal].value ?
	m_paramConfig->params[prop->m_paramOrdinal].value : prop->m_default;
      if (v) {
	std::string value;
	v->unparse(value);
	fprintf(f, " default='%s'", value.c_str());
      }
    }
    prop->printChildren(f, "property");
  }
  for (nn = 0; nn < m_ports.size(); nn++)
    m_ports[nn]->emitXML(f);
  for (nn = 0; nn < m_localMemories.size(); nn++) {
    LocalMemory* m = m_localMemories[nn];
    fprintf(f, "  <localMemory name=\"%s\" size=\"%zu\"/>\n", m->name, m->sizeOfLocalMemory);
  }
  fprintf(f, "</worker>\n");
}

void Worker::
emitXmlWorkers(FILE *f) {
  assert(m_assembly);
  // Define all workers
  for (WorkersIter wi = m_assembly->m_workers.begin();
       wi != m_assembly->m_workers.end(); wi++)
    (*wi)->emitXmlWorker(f);
}

InstancePort::
InstancePort()
{
  init(NULL, NULL, NULL);
}
InstancePort::
InstancePort(Instance *i, Port *p, OU::Assembly::External *ext) {
  init(i, p, ext);
}

void InstancePort::
init(Instance *i, Port *p, OU::Assembly::External *ext) {
  m_instance = i;
  m_port = p;
  m_connected.assign(p ? p->count : 1, false);
  if (p)
    p->initRole(m_role);
  // If the external port tells us the direction and we're bidirectional, capture it.
  m_external = ext;
  if (ext && ext->m_role.m_knownRole && !ext->m_role.m_bidirectional)
    m_role = ext->m_role;
}
