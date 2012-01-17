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

#ifndef __DOMAINMANAGER_IMPL__
#define __DOMAINMANAGER_IMPL__

#include <vector>
#include <string>

#ifdef HAVE_OMNIEVENTS
#include "CosEventComm.hh"
#include "CosEventChannelAdmin.hh"
#endif

#include "cf.h"
#include "FileManager_impl.h"
#include "DeviceManager_impl.h"
#include "ApplicationFactory_impl.h"
#include "DMDParser.h"
#include "SPDParser.h"
#include "PRFParser.h"
#include "SCDParser.h"
#include "StandardEvent.h"

#define MAX_APPS        10
#define MAX_APP_FACS    10
#define MAX_DEV_MGRS    10

class OSSIECF_API DomainManager_impl;

#ifdef HAVE_OMNIEVENTS
class OSSIECF_API DomainManagerEventHandler:public virtual
        POA_CosEventComm::PushConsumer
{
public:
    DomainManagerEventHandler (DomainManager_impl * _dmn);
    void
    push (const CORBA::Any & _any)
    throw (CosEventComm::Disconnected, CORBA::SystemException);
    void
    disconnect_push_consumer ()
    throw (CORBA::SystemException);

private:
    DomainManager_impl *
    _dmnMgr;

};
#endif

class OSSIECF_API
        DomainManager_impl:
        public
        virtual
        POA_CF::DomainManager,
        public
        PropertySet_impl
{
public:
    DomainManager_impl (const char*, const char*);
    ~DomainManager_impl ();

    char *
    identifier (void)
    throw (CORBA::SystemException);
    char *
    domainManagerProfile (void)
    throw (CORBA::SystemException);
    void
    registerDevice (CF::Device_ptr registeringDevice,
                    CF::DeviceManager_ptr registeredDeviceMgr)
    throw (CF::DomainManager::RegisterError,
           CF::DomainManager::DeviceManagerNotRegistered, CF::InvalidProfile,
           CF::InvalidObjectReference, CORBA::SystemException);
    void
    registerDeviceManager (CF::DeviceManager_ptr deviceMgr)
    throw (CF::DomainManager::RegisterError, CF::InvalidProfile,
           CF::InvalidObjectReference, CORBA::SystemException);
    void
    unregisterDeviceManager (CF::DeviceManager_ptr deviceMgr)
    throw (CF::DomainManager::UnregisterError, CF::InvalidObjectReference,
           CORBA::SystemException);
    void
    unregisterDevice (CF::Device_ptr unregisteringDevice)
    throw (CF::DomainManager::UnregisterError, CF::InvalidObjectReference,
           CORBA::SystemException);
    void
    installApplication (const char *profileFileName)
    throw (CF::DomainManager::ApplicationInstallationError, CF::InvalidFileName,
           CF::InvalidProfile, CORBA::SystemException);
    void
    uninstallApplication (const char *applicationId)
    throw (CF::DomainManager::ApplicationUninstallationError,
           CF::DomainManager::InvalidIdentifier, CORBA::SystemException);
    void
    registerService (CORBA::Object_ptr registeringService,
                     CF::DeviceManager_ptr registeredDeviceMgr,
                     const char *name)
    throw (CF::DomainManager::RegisterError,
           CF::DomainManager::DeviceManagerNotRegistered,
           CF::InvalidObjectReference, CORBA::SystemException);
    void
    unregisterService (CORBA::Object_ptr unregisteringService, const char *name)
    throw (CF::DomainManager::UnregisterError, CF::InvalidObjectReference,
           CORBA::SystemException);
    void
    registerWithEventChannel (CORBA::Object_ptr registeringObject,
                              const char *registeringId,
                              const char *eventChannelName)
    throw (CF::DomainManager::AlreadyConnected,
           CF::DomainManager::InvalidEventChannelName,
           CF::InvalidObjectReference, CORBA::SystemException);
    void
    unregisterFromEventChannel (const char *unregisteringId,
                                const char *eventChannelName)
    throw (CF::DomainManager::NotConnected,
           CF::DomainManager::InvalidEventChannelName, CORBA::SystemException);
    CF::FileManager_ptr
    fileMgr (void)
    throw (CORBA::SystemException);
    CF::DomainManager::ApplicationFactorySequence *
    applicationFactories (void)
    throw (CORBA::SystemException);
    CF::DomainManager::ApplicationSequence *
    applications (void)
    throw (CORBA::SystemException);
    CF::DomainManager::DeviceManagerSequence *
    deviceManagers (void)
    throw (CORBA::SystemException);

#ifdef HAVE_OMNIEVENTS
    friend void
    DomainManagerEventHandler::push (const CORBA::Any &);
    friend void
    DomainManagerEventHandler::disconnect_push_consumer ();
#endif

protected:
    struct DeviceNode {
        CF::Device_ptr  devicePtr;
        char*   softwareProfile;
        char*   label;

        CF::DeviceManager_ptr   devMgrPtr;

        CF::Properties  properties;

        DeviceNode():softwareProfile(NULL), label(NULL) {}

        ~DeviceNode() {
            if (softwareProfile!=NULL) delete []softwareProfile;    // technically, you can safely delete NULL ptr -TP
            if (label!=NULL) delete []label;

            //    CORBA::release(devicePtr);
            CORBA::release(devMgrPtr);
        }
    };

    struct ServiceNode {
        CORBA::Object_ptr       objectPtr;
        char*                   name;
        CF::DeviceManager_ptr   devMgrPtr;

        ServiceNode():name(NULL) {}

        ~ServiceNode() { // should we call CORBA::release() for those _ptr(s)? -TP
            if (name!=NULL) delete []name;
            CORBA::release(objectPtr);
            CORBA::release(devMgrPtr);
        }
    };
    enum ErrorCode {
        NULL_ERROR,
        FILE_NOT_FOUND,
        NOT_SCA_COMPLIANT,
        NO_PROPERTIES
    };
    std::string _identifier;
    std::string _domainManagerProfile;
    vector <
    DeviceNode * >
    _registeredDevices;
    vector <
    ServiceNode * >
    _registeredServices;
    vector <
    Connection * >
    _pendingConnections;

    CF::DomainManager::DeviceManagerSequence
    _deviceManagers;
    CF::DomainManager::ApplicationSequence
    _applications;
    int
    _numApps;
    int
    _numDevMgrs;

    CF::FileManager_ptr
    _fileMgr;

    CF::FileSystem_ptr
    _fileSys;

    void
    validate (const char *_profile);
    void
    validateSPD (const char *_spdProfile, int _cnt = 0);
    void
    removeSPD (const char *_spdProfile, int _cnt = 0);
    DomainManager_impl::ErrorCode
    storeDeviceInDomainMgr (CF::Device_ptr, CF::DeviceManager_ptr);
    void
    storeServiceInDomainMgr (CORBA::Object_ptr, CF::DeviceManager_ptr, const char *);
    DomainManager_impl::ErrorCode
    getDeviceProperties (DeviceNode &);
    void
    removeDeviceFromDomainMgr (CF::Device_ptr);
    void
    removeServiceFromDomainMgr (CORBA::Object_ptr, const char *);
    bool
    deviceMgrIsRegistered (CF::DeviceManager_ptr);
    bool
    deviceIsRegistered (const CF::Device_ptr);
    bool
    serviceIsRegistered (const char *);
    void
    establishServiceConnections (CF::Device_ptr, CF::DeviceManager_ptr);
    void
    establishPendingServiceConnections (const char *);
    void
    disconnectThisService (const char *);
    void
    disconectEventService ();
    void
    sendEventToOutgoingChannel (CORBA::Any & _event);
    void
    addDeviceMgr (CF::DeviceManager_ptr deviceMgr);
    void
    addDeviceMgrDevices (CF::DeviceManager_ptr deviceMgr);
    void
    addDeviceMgrServices (CF::DeviceManager_ptr deviceMgr);
    void
    removeDeviceMgrDevices (CF::DeviceManager_ptr deviceMgr);
    void
    removeDeviceMgrServices (CF::DeviceManager_ptr deviceMgr);
    void
    removeDeviceMgr (CF::DeviceManager_ptr deviceMgr);

private:
    DomainManager_impl(); // no default constructor
    DomainManager_impl(const DomainManager_impl &); // No copying

    vector <ApplicationFactory_impl *> appFact_servants;
    CF::DomainManager::ApplicationFactorySequence _applicationFactories;

// Event Channel objects
    void createEventChannels (void);

#ifdef HAVE_OMNIEVENTS
/// todo Move all Event channel support into OSSIE support
    CosEventChannelAdmin::EventChannel_var ODM_channel;
    CosEventChannelAdmin::EventChannel_var IDM_channel;

    CosEventChannelAdmin::ProxyPushConsumer_var ODM_Channel_consumer;

    CosEventChannelAdmin::ProxyPushConsumer_var connectEventChannel(CosEventChannelAdmin::EventChannel_var eventChannel);
#endif

};                                            /* END CLASS DEFINITION DomainManager */


#endif                                            /* __DOMAINMANAGER_IMPL__ */
