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

#ifndef Xfer_Factory_H_
#define Xfer_Factory_H_

#include "OcpiUtilSelfMutex.h"
#include "OcpiDriverManager.h"
#include "XferEndPoint.h"

namespace DataTransfer {

  class XferServices;
  // This map class is for keeping track of templates.
  // It is used by the XferFactory to know what templates exist,
  // and it is also used by ports to know which templates it holds a reference count on.
  typedef std::pair<EndPoint *, EndPoint *> TemplatePair;
  // FIXME unordered someday
  typedef std::map<TemplatePair, XferServices*> TemplateMap;
  typedef TemplateMap::iterator TemplateMapIter;

  // This is the base class for a factory configuration sheet
  // that is common to the manager, drivers, and devices
  class FactoryConfig {
  public:
    // These are arguments to allow different defaults
    FactoryConfig(size_t smbSize = 0, size_t retryCount = 0);
    void parse(FactoryConfig *p, ezxml_t config );
    size_t getSMBSize() const { return m_SMBSize; }
    size_t m_SMBSize;
    size_t m_retryCount;
    ezxml_t  m_xml; // the element that these attributes were parsed from
  };

  // This class is the transfer driver base class.
  // All drivers indirectly inherit this by using the template class in DtDriver.h
  // Its parent relationship with actual devices is managed by that transfer driver
  // template and thus is not evident here.
  class XferManager;
  class XferFactory
    : public OCPI::Driver::DriverType<XferManager, XferFactory>,
      public FactoryConfig,
      virtual protected OCPI::Util::SelfMutex {
    TemplateMap m_templates; // which templates exist for this driver?
    EndPoints m_endPoints;
  public:
    XferFactory(const char *);

    // Destructor
    virtual ~XferFactory();

    void cleanForContext(void *context);
    // Configure from xml
    void configure(ezxml_t x);

    // Get our protocol string
    virtual const char* getProtocol()=0;
    bool supportsEndPoint(const char *name);
    EndPoint &getEndPoint(const char *endpoint, bool local=false, bool cantExist = false,
			  size_t size = 0);
    // Find it or return NULL if you can't find it.  Remote or local.
    EndPoint *findEndPoint(const char *endPoint);
    inline EndPoint &getEndPoint(const std::string &s, bool local=false) {
      return getEndPoint(s.c_str(), local);
    }
    EndPoint &addCompatibleLocalEndPoint(const char *remote);
  private:
    EndPoint &addEndPoint(const char *endpoint, const char *other, bool local, size_t size = 0);
    virtual XferServices &createXferServices(EndPoint &source, EndPoint &target) = 0;
  public:
    // Create an endpoint with some protocol info specific to this protocol,
    // and possibly a string for the "other" endpoint that we are trying to avoid.
    virtual EndPoint &createEndPoint(const char *protoInfo, const char *eps, const char *other,
				     bool local, size_t size, const OCPI::Util::PValue *params)
      = 0;
    // Tell the factory about a new mailbox (beyond automatic parent/child relationship).
    // Return value is the new mailbox number.  Other is mailbox to avoid.
    // (this will disappear at some point)
    DtOsDataTypes::MailBox setNewMailBox(const char *other);
    void removeEndPoint(EndPoint &ep);
    XferServices &getTemplate(EndPoint &source, EndPoint &target);
    //    void removeTemplate(XferServices &xfs);

    /***************************************
     *  Gets the first node specified by name, otherwise null
     ***************************************/    
    static ezxml_t getNode( ezxml_t tn, const char* name );
    // The range of mailboxes is common across all transports.
    // This will allow us to share memory between protocol someday.
    uint16_t getMaxMailBox(), getNextMailBox();
  };
  // OCPI::Driver::Device is virtually inherited to give access
  // to the class that is not normally inherited here.
  // This also delays the calling of the destructor
  class Device : public FactoryConfig, virtual public OCPI::Driver::Device {
    virtual XferFactory &driverBase() = 0;
  protected:
    void configure(ezxml_t x);
  };
}


#endif
