// A message circuit endpoint class, which works for both client and server.
// There is a message endpoint for each supported protocol.
// Static methods are provided that implicitly deal with multiple protocols.
// The "connect" static method chooses the message endpoint for the protocol of the 
// remote endpoint your are connecting to.
// The "accept" static method accepts a connection from all available protocols.

#include "OcpiOsMisc.h"
#include "OcpiMessageEndpoint.h"
#include "DtTransferInternal.h"

namespace OS = OCPI::OS;
namespace OD = DataTransfer;
namespace OCPI {
  namespace DataTransport {

    // Public methods

    // We're creating two things and have to resource-manage them during construction.
    // Getting our endpoint might end up creating a new one, and if so we want to nuke it
    MessageEndpoint::
    MessageEndpoint(const char *endpoint)
      : m_transportGlobal(*new TransportGlobal(0, (char**)0)),
	m_transport(NULL), m_endpoint(NULL) {
      try {
	m_transport = new Transport(&m_transportGlobal, true);
	m_transport->setNewCircuitRequestListener(this);
	m_endpoint = &m_transport->getLocalCompatibleEndpoint(endpoint, true);
      } catch (...) {
	if (m_transport)
	  delete m_transport;
	delete &m_transportGlobal;
	throw;
      }
      m_transport->setListeningEndpoint(m_endpoint);
      s_messageEndpoints.insert(this);
    }

    MessageEndpoint::
    ~MessageEndpoint() {
      lock();
      while (!m_circuits.empty()) {
	MessageCircuits::iterator i = m_circuits.begin();
	delete *i;
	m_circuits.erase(i);
      }
      while (!m_halfCircuits.empty()) {
	HalfCircuits::iterator i = m_halfCircuits.begin();
	delete *i;
	m_halfCircuits.erase(i);
      }
      delete m_transport;
      delete &m_transportGlobal;
      s_messageEndpoints.erase(this);
    }

    // static: get the message endpoint for the protocol or endpoint, creating one if needed

    MessageEndpoint &MessageEndpoint::
    getMessageEndpoint(const char *endpoint) {
      // Find one we can use.  But bury the issue in the transport layer.
      for (MessageEndpoints::iterator i = s_messageEndpoints.begin(); i != s_messageEndpoints.end(); i++)
	if ((*i)->canSupport(endpoint))
	  return *(*i);
      // This constructor implies that we know a new endpoint is required.
      return *new MessageEndpoint(endpoint);
    }      

    // static

    void MessageEndpoint::
    destroyMessageEndpoints() {
      while (!s_messageEndpoints.empty())
	delete *s_messageEndpoints.begin();
    }

    // static
    
    MessageCircuit & MessageEndpoint::
    connect(const char *server_endpoint, unsigned bufferSize, const char *protocol, OS::Timer *timer) {
      return getMessageEndpoint(server_endpoint).connectTo(server_endpoint, bufferSize, protocol, timer);
    }
    
    // static 
    
#if 0
    MessageCircuit *MessageEndpoint::
    accept(OCPI::OS::ElapsedTime delay) {
      if (s_messageEndpoints.empty()) {
	std::vector<std::string> protocols =
	  DataTransfer::XferFactoryManager::getFactoryManager().getListOfSupportedProtocols();
	for (unsigned n = 0; n < protocols.size(); n++)
	  getMessageEndpoint(protocols[n].c_str());
      }
      // loop for anyone
      OS::Timer timer(delay != 0);
      // OCPI::OS::Time start = timer.getStart();
      do {
	for (MessageEndpoints::iterator i = s_messageEndpoints.begin(); i != s_messageEndpoints.end(); i++) {
	  MessageEndpoint *mep = *i;
	  mep->dispatch();
	  MessageCircuit *c = mep->accept();
	  if (c)
	    return c;
	}
	OCPI::OS::sleep(1);
      } while (delay != 0 && timer.getElapsed() < delay);
      return NULL;
    }
#endif
    // Private
    
    // static
    MessageEndpoint::MessageEndpoints MessageEndpoint::
    s_messageEndpoints;
  
    bool MessageEndpoint::canSupport(const char *endpoint) const {
      return m_endpoint->canSupport(endpoint);
    }

    // the internal, non-static method, with same args as the static one, hence "To".
    MessageCircuit &MessageEndpoint::
    connectTo(const char *server_endpoint, unsigned bufferSize, const char *protocol, OCPI::OS::Timer *timer) {
      return *new MessageCircuit(*m_transport, /* *this, */ m_endpoint->end_point.c_str(),
				 server_endpoint, bufferSize, protocol, timer);
    }

    // Callback from the transport when a new circuit is started.
    void MessageEndpoint::
    newCircuitAvailable(Circuit* circuit) {

#ifndef NDEBUG
      printf("In MessageEndpoint::newCircuitAvailable, got a new circuit \n");
#endif
      m_halfCircuits.push_back(circuit);
    }
    
    MessageCircuit *MessageEndpoint::
    accept(OCPI::OS::Timer *timer) {
      do {
	dispatch();
      // Search oldest first, and look forward
	for (HalfCircuits::iterator i = m_halfCircuits.begin(); i != m_halfCircuits.end(); i++) {
	  if (!(*i)->ready())
	    continue;
	  //	  (*i)->initializeDataTransfers();
	  // We need to match two circuits that have the same other side
	  Port *iInput = (*i)->getInputPortSet(0)->getPortFromIndex(0);
	  if (!iInput->isShadow()) {
	    OD::EndPoint *remote = iInput->getMetaData()->m_shadow_location;
	    for (HalfCircuits::iterator j = m_halfCircuits.begin(); j != m_halfCircuits.end(); j++) {
	      if (i == j || !(*j)->ready())
		continue;
	      // (*i)->initializeDataTransfers();
	      Port *jOutput = (*j)->getOutputPortSet()->getPortFromIndex(0);
	      if (!jOutput->isShadow() && jOutput->getMetaData()->m_shadow_location == remote) {
		MessageCircuit *mc = new MessageCircuit(*m_transport, /* *this/, */ **j, **i);
		m_halfCircuits.erase(i);
		m_halfCircuits.erase(j);
		return mc;
	      }	    
	    }
	  }
	}
	OCPI::OS::sleep(1);
	//	OS::ElapsedTime t = timer->getElapsed();
	//	printf("Time is %lus %luns\n", t.seconds(), t.nanoseconds()); 
      } while (!timer || !timer->expired());
      return NULL;
    }
  }
}
