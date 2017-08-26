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

#ifndef XferManager_H
#define XferManager_H

#include <string>
#include <vector>
#include "OcpiOsMutex.h"
#include "OcpiDriverManager.h"
#include "XferException.h"
#include "XferFactory.h"

namespace DataTransfer {
  struct SMBResources;

  // This is the transfer driver (XferFactory) manager
  extern const char *xfer;
  class XferManager : 
    public OCPI::Driver::ManagerBase<XferManager, XferFactory, xfer>,
    public FactoryConfig
  {
    friend class XferFactory;
    OCPI::OS::Mutex m_mutex;
    bool            m_configured;
  public:
    XferManager();
    ~XferManager();
    inline static XferManager& getFactoryManager() {
      return getSingleton();
    }
    inline size_t getSMBSize() { return m_SMBSize; }
    // For some tests...
    //    std::vector<std::string> getListOfSupportedEndpoints();  
    std::vector<std::string> getListOfSupportedProtocols();  
    // Configure the manager and it drivers
    void configure(ezxml_t config);

    EndPoint &getEndPoint(std::string &s);
    // Retrieves the factory based upon the transfer type
    static std::string null;
    XferFactory *find(std::string& end_point1,std::string& end_point2 = null);
    XferFactory *find(const char* end_point1,const char* end_point2 = NULL);

    // Creates a transfer service template - should be expunged.
    XferServices *getService(EndPoint *s_endpoint, EndPoint *t_endpoint);        

    // Create an endpoint for remote hardware
    EndPoint &allocateProxyEndPoint(const char *epString, bool local, size_t size);
  };
  // Convenience - FIXME: change callers of getFactoryManager() to this
  inline XferManager &getManager() { return XferManager::getSingleton(); }
}
#endif
