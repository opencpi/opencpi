/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * Abstact:
 *   This file contains the Interface for the CORBA URLs
 */

#include <list>
#include "OcpiUtilIOP.h"
#include "OcpiMessageEndpoint.h"
#include "DtMsgDriver.h"


//namespace OX = OCPI::Util::EzXml;
namespace OU = OCPI::Util;
//namespace OA = OCPI::API;
namespace OT = OCPI::DataTransport;

namespace OCPI {  
  namespace Msg {
    namespace CORBA {

      // Note that this message channel for connecting by URL is to connect to CORBA on the other side,
      // and that the assumption is that there is a way to talk to the other CORBA using ocpi rdma transports.
      // Thus the URL is of the form:
      // corbaloc:<omni-ocpi-endpoint>[,<omni-ocpi-endpoint>]*/<key-string>

      // Where <omni-ocpi-endpoint> is "omniocpi:" followed by the ocpi endpoint string.

      // Note that double slash (//) immediately after the ocpi sub-protocol colon is ignored and 
      // doesn't count as the slash before the key.

      // Note that this string is ALSO usable by CORBA as a real corbaloc (for omniorb anyway).
      
      class XferFactory;
      class XferServices;	
      class MsgChannel : public DataTransfer::Msg::TransferBase<XferServices,MsgChannel>
      {
      private:
	typedef std::list<std::string> RdmaEndpoints;
	RdmaEndpoints m_rdmaEndpoints;
	std::string m_key, m_url;
	OT::MessageCircuit *m_circuit;
      public:

	MsgChannel(XferServices & xf,
		   const OU::Protocol &protocol,
		   const char  * url,
		   const OCPI::Util::PValue *ourParams,
		   const OCPI::Util::PValue *otherParams)
	  : DataTransfer::Msg::TransferBase<XferServices,MsgChannel>(xf, *this),
	    m_url(url), m_circuit(0)
	{
	  (void)ourParams, (void)otherParams;
	  std::string corbalocURL;
	  if (!strncasecmp(url, "ior:", sizeof("ior:") - 1)) {
	    // Convert IOR to corbaloc
	    OU::IOP::IOR ior(url);
	    corbalocURL = ior.corbaloc();
	  } else
	    corbalocURL = url;

	  ocpiAssert(!strncasecmp(url, "corbaloc:", sizeof("corbaloc:") - 1));
	  url += sizeof("corbaloc:") - 1;
	  for (const char *p; *url && url[-1] != '/'; url = p + 1) {
	    for (p = url; *p && *p != ',' && *p != '/'; p++)
	      // Skip over slashes after colon
	      if (*p == ':' && p[1] == '/' && p[2] == '/')
		p += 2;
	    if (!*p)
	      throw OU::Error("No key found (after slash) in url: %s", m_url.c_str());
	    if (!strncmp(url, "omniocpi:", sizeof("omniocpi:") - 1))
	      url += sizeof("omniocpi:") - 1;
	    if (!strncmp(url, "ocpi-", 5))
	      m_rdmaEndpoints.push_back(std::string(url, p - url));
	  }
	  if (m_rdmaEndpoints.empty())
	    throw OU::Error("No usable opencpi endpoints in url: %s", m_url.c_str());
	  // The key is encoded according to RFC 2396, but we will leave it that way
	  m_key = url;

	  // Now must encode the procotol info in a string, with the key being the first line
	  // Until the protocol printing stuff uses strings or string streams, we need to use a temp
	  // file. Ugh.
	  char *temp = strdup("/tmp/tmpXXXXXXX");
	  int tempfd = mkstemp(temp);
	  free(temp);
	  if (tempfd < 0)
	    throw OU::Error("Can't open temp file for protocol processing");
	  FILE *f = fdopen(tempfd, "w+");
	  // We use the # to terminate the key since that is also how URIs work:  the end of
	  // a URI is either null or #.
	  fprintf(f, "%s#", url);
	  protocol.printXML(f, 0);
	  fflush(f);
	  off_t size = ftello(f);
	  char *info = new char[size];
	  fseeko(f, 0, SEEK_SET);
	  fread(info, size, 1, f);
	  fclose(f);
	  // End of kludge that can be fixed when XML printing is to a stream...

	  // Here we try all endpoints in turn.
	  for (RdmaEndpoints::const_iterator i = m_rdmaEndpoints.begin();
	       i != m_rdmaEndpoints.end(); i++)
	    try {
	      m_circuit = &OT::MessageEndpoint::connect(i->c_str(), 4096, info, NULL);
	    } catch (...) {
#ifndef NDEBUG
	      printf("CORBA URL connection failed: endpoint '%s' key '%s'\n",
		     i->c_str(), m_key.c_str());
#endif
	    }
	  delete [] info;
	  if (!m_circuit)
	    throw OU::Error("No endpoints in CORBA URL '%s' could be connected",
			    m_url.c_str());
	}

	virtual ~MsgChannel()
	{
	  if (m_circuit)
	    delete m_circuit;
	}
	  
	OCPI::DataTransport::BufferUserFacet*  getNextEmptyOutputBuffer(void *&data, uint32_t &length)
	{
	  return m_circuit->getNextEmptyOutputBuffer(data, length, NULL);
	}

	void sendOutputBuffer(OCPI::DataTransport::BufferUserFacet* b, uint32_t msg_size, uint8_t opcode )
	{
	  m_circuit->sendOutputBuffer(b, msg_size, opcode);
	}

	OCPI::DataTransport::BufferUserFacet*  getNextFullInputBuffer(void *&data, uint32_t &length,
								      uint8_t &opcode)
	{
	  return m_circuit->getNextFullInputBuffer(data, length, opcode);
	}

	void releaseInputBuffer(OCPI::DataTransport::BufferUserFacet* b)
	{
	  m_circuit->releaseInputBuffer(b);
	}
      };


      class XferServices : public DataTransfer::Msg::ConnectionBase<XferFactory,XferServices,MsgChannel>
      {
	const OU::Protocol &m_protocol;
      public:
	XferServices ( const OCPI::Util::Protocol & protocol , const char  * other_url, 
		       const OCPI::Util::PValue *our_props=0,
		       const OCPI::Util::PValue *other_props=0 );
	MsgChannel* getMsgChannel( const char  * url,
				   const OCPI::Util::PValue *ourParams,
				   const OCPI::Util::PValue *otherParams)
	{
	  return new MsgChannel( *this, m_protocol, url, ourParams, otherParams);
	}
	virtual ~XferServices ()
	{
	}
      };

      class Device
	: public DataTransfer::Msg::DeviceBase<XferFactory,Device>
      {
      public:
	Device(const char* name)
	  : DataTransfer::Msg::DeviceBase<XferFactory,Device>(name, *this)
	{

	}
	void configure(ezxml_t x);
	virtual ~Device(){}
      };
     
      class XferFactory
	: public DataTransfer::Msg::DriverBase<XferFactory, Device, XferServices,
					       DataTransfer::Msg::msg_transfer>
      {

      public:
	inline const char* getProtocol() {return "corbaloc";};
	XferFactory()throw ();
	virtual ~XferFactory()throw ();
 
	void configure(ezxml_t)
	{
	  // Empty
	}
	  
	bool supportsTx( const char* url,
			 const OCPI::Util::PValue * /* ourParams */,
			 const OCPI::Util::PValue * /*otherParams */ )
	{
	  return !strncasecmp(url, "corbaloc:", sizeof("corbaloc:") - 1) ||
	    !strncasecmp(url, "ior:", sizeof("ior:") - 1);
	}

	virtual XferServices* getXferServices( const OCPI::Util::Protocol & protocol,
					       const char* url,
					       const OCPI::Util::PValue *ourParams,
					       const OCPI::Util::PValue *otherParams)
	{
	  if (!m_services)
	    m_services = new XferServices( protocol, url, ourParams, otherParams);
	  return m_services;
	}

      private:
	XferServices* m_services;

      };

	
      XferServices::
      XferServices ( const OCPI::Util::Protocol & protocol , const char  * other_url,
		     const OCPI::Util::PValue *ourParams,
		     const OCPI::Util::PValue *otherParams)
	: DataTransfer::Msg::ConnectionBase<XferFactory,XferServices,MsgChannel>
	  (*this, protocol, other_url, ourParams, otherParams),
	  m_protocol(protocol)
      {
      }

      XferFactory::
      XferFactory()
	throw ()
	: m_services(NULL)
      {
	// Empty
      }

      XferFactory::	
      ~XferFactory()
	throw ()
      {
	

      }
      void
      Device::
      configure(ezxml_t x) {
	DataTransfer::Msg::Device::configure(x);
      }

      DataTransfer::Msg::RegisterTransferDriver<XferFactory> driver;

    }
  }
}

