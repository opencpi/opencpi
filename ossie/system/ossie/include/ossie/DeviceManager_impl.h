/****************************************************************************

Copyright 2008, Virginia Polytechnic Institute and State University

This file is part of the OSSIE Core Framework.

OSSIE Core Framework is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Core Framework is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Core Framework; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/

#ifndef DEVICE_MANAGER_IMPL_H
#define DEVICE_MANAGER_IMPL_H

#include <sys/types.h>
#include <csignal>

#include "ossieSupport.h"

#include "PropertySet_impl.h"
#include "PortSupplier_impl.h"
#include "FileManager_impl.h"
#include "DCDParser.h"
#include "SPDParser.h"
#include "PRFParser.h"
#include "SPDImplementation.h"
#include "DPDParser.h"

class OSSIECF_API DeviceManager_impl:public virtual
        POA_CF::DeviceManager,
        public
        PropertySet_impl,
        public
        PortSupplier_impl
{
public:
    DeviceManager_impl (const char *, const char*);

// Run this after the constructor and the caller has created the object reference
    void post_constructor(CF::DeviceManager_var) throw (CORBA::SystemException);

    ~
    DeviceManager_impl ();
    char *deviceConfigurationProfile ()
    throw (CORBA::SystemException);
    CF::FileSystem_ptr
    fileSys ()
    throw (CORBA::SystemException);
    char *identifier ()
    throw (CORBA::SystemException);
    char *label ()
    throw (CORBA::SystemException);
    CF::DeviceSequence *
    registeredDevices ()
    throw (CORBA::SystemException);
    CF::DeviceManager::ServiceSequence *
    registeredServices ()
    throw (CORBA::SystemException);
    void registerDevice (CF::Device_ptr registeringDevice)
    throw (CF::InvalidObjectReference, CORBA::SystemException);
    void unregisterDevice (CF::Device_ptr registeredDevice)
    throw (CF::InvalidObjectReference, CORBA::SystemException);
    void shutdown ()
    throw (CORBA::SystemException);
    void registerService (CORBA::Object_ptr registeringService, const char *name)
    throw (CF::InvalidObjectReference, CORBA::SystemException);
    void unregisterService (CORBA::Object_ptr registeredService, const char *name)
    throw (CF::InvalidObjectReference, CORBA::SystemException);
    char * getComponentImplementationId (const char *componentInstantiationId)
    throw (CORBA::SystemException);


private:
    DeviceManager_impl ();   // No default constructor
    DeviceManager_impl(DeviceManager_impl &); // No copying

    pid_t m_pid;
// read only attributes
    std::string _identifier;
    std::string _label;
    std::string _deviceConfigurationProfile;
    std::string _fsroot;
    FileSystem_impl *fs_servant;
    CF::FileSystem_var _fileSys;
    CF::DeviceManager_var myObj;
    CF::DeviceSequence
    _registeredDevices;
    CF::DeviceManager::ServiceSequence
    _registeredServices;

    ossieSupport::ORB *orb_obj;

    enum DevMgrAdmnType {
        DEVMGR_REGISTERED,
        DEVMGR_UNREGISTERED,
        DEVMGR_SHUTTING_DOWN
    };
    DevMgrAdmnType
    _adminState;
    CF::DomainManager_var
    _dmnMgr;
    bool deviceIsRegistered (CF::Device_ptr);
    bool serviceIsRegistered (const char*);
    void getDomainManagerReference (char *);
    void init ();

    void printDPD(std::vector <DPDParser*> DPD);
    void parseDPDFiles(std::vector <DPDParser*> &_DPD, DCDParser &_DCD);
    void printChildHWRecursive(ChildHWDevice *child, int indent);
    SPDImplementation* matchGPP(std::vector <SPDImplementation*> *impls, SPDImplementation *device);
    bool checkImplForGPP(std::vector <SPDImplementation*> *impls);
    bool matchProcessor(SPDImplementation *devMan, SPDImplementation *device);
    bool matchOperatingSystem(SPDImplementation *devMan, SPDImplementation *device);
    SPDImplementation* matchOtherDevice(std::vector <SPDImplementation*> *impls, SPDImplementation *devMan);


    struct ComponentMapping {
        std::string _instantiationid;
        std::string _implementationid;
    };

    std::vector<ComponentMapping> _componentImplMap;
};

#endif                                            /* __DEVICEMANAGER_IMPL__ */

