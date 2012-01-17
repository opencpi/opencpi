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

Nov/10/03	C. Neely	Created
C. Aguayo

****************************************************************************/
#include <iostream>

#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else  /// \todo change else to ifdef windows var
#include <process.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <errno.h>

#include "ossie/debug.h"
#include "ossie/ossieSupport.h"
#include "ossie/DeviceManager_impl.h"
#include "ossie/DPDParser.h"
#include "ossie/portability.h"

DeviceManager_impl::~DeviceManager_impl ()
{
}


DeviceManager_impl::DeviceManager_impl(const char *DCDInput, const char* _rootfs)
{
    _fsroot = _rootfs;
    _deviceConfigurationProfile = DCDInput;
}

//Parsing constructor
void DeviceManager_impl::post_constructor (CF::DeviceManager_var my_object_var) throw (CORBA::SystemException)

{
    orb_obj = new ossieSupport::ORB();

    myObj = my_object_var;

    _registeredDevices.length(0);
    _registeredServices.length(0);

    fs_servant = new FileSystem_impl(_fsroot.c_str());
    _fileSys = fs_servant->_this();


        _fileSys->exists(_deviceConfigurationProfile.c_str());


//Get Device Manager attributes (deviceConfigurationProfile, identifier and label)
//from DCD file
    CF::File_var _dcd;

        _dcd = _fileSys->open( _deviceConfigurationProfile.c_str(), true );


    DCDParser _DCDParser ( _dcd );

        _dcd->close();


    _identifier = _DCDParser.getID();
    _label = _DCDParser.getName();

    std::string _devmgrsoftpkg = _DCDParser.getDeviceManagerSoftPkg();

    CF::File_var _devmgrspd;

        _devmgrspd = _fileSys->open( _devmgrsoftpkg.c_str(), true );


    SPDParser _devmgrspdparser( _devmgrspd );

        _devmgrspd->close();


// parse all DPD files
    std::vector <DPDParser*> _dpd;
    parseDPDFiles(_dpd,_DCDParser);

//get DomainManager reference
    getDomainManagerReference ((char *)_DCDParser.getDomainManagerName ());

//Register DeviceManager with DomainManager
    DEBUG(2, DevMgr, "Registering with DomainManager" );

        _dmnMgr->registerDeviceManager (my_object_var);


    _adminState = DEVMGR_REGISTERED;

//parse filesystem names

//Parse local componenents from DCD files
    std::vector <componentPlacement> componentPlacements = _DCDParser.getComponentPlacements ();
    if ( componentPlacements.size() == 0 ) {
        for ( unsigned int i = 0; i < _DCDParser.getDeployOnComponents()->size(); i++ ) {
            componentPlacements.push_back(componentPlacement(
                                              (*_DCDParser.getDeployOnComponents())[i]->getFileRefId(),
                                              (*_DCDParser.getDeployOnComponents())[i]->getInstantiationId(),
                                              (*_DCDParser.getDeployOnComponents())[i]->getUsageName() ));
        }
    }

    DEBUG(2, DevMgr, "ComponentPlacement size is" << componentPlacements.size());
    for (unsigned int i = 0; i < componentPlacements.size(); i++) {
        //get spd reference
        //parse spd file
        CF::File_var _spd;
        DEBUG(2, DevMgr, "Parsing Device SPD");

            _spd = _fileSys->open( _DCDParser.getFileNameFromRefId(componentPlacements[i].refId()), true );


        SPDParser _SPDParser ( _spd );

            _spd->close();


        //get code file name from implementation
        DEBUG(2, DevMgr, "Searching for matching implementation");

        // first find the correct implementation of the GPP from the Device Manager
        // implementation. second, determine if the Device Manager has any
        // <dependency> tags, and if they exist, get a reference to these devices
        //
        // (this algorithm assumes only a single GPP is being deployed in the node, and
        // it also assumes that there is a single device manager implementation)

        // get Device Manager implementation
        std::vector <SPDImplementation*> *_allDevManImpls = _devmgrspdparser.getImplementations();
        if (_allDevManImpls->size() > 1) {
            DEBUG(1, DevMgr, "WARNING: the current node SPD file has more than one implementation. Multiple node implementations is not currently supported. Only the first implementation will be used.");
        }

        SPDImplementation* _devManImpl = (*_allDevManImpls)[0];
        SPDImplementation *matchedDevice = NULL;

        std::vector <SPDImplementation*> *deviceImpls = _SPDParser.getImplementations();
        bool isGPP = checkImplForGPP(deviceImpls);

        // match the GPP or dependency
        if (isGPP) {
            matchedDevice = matchGPP(deviceImpls, _devManImpl);
        } else {
            matchedDevice = matchOtherDevice(deviceImpls,_devManImpl);
        }

        if (matchedDevice == NULL) {
            std::cout << "[DeviceManager::post_ctor] Could not match a device implementation to ";
            std::cout << "the <dependency> tag within the node SPD file.\n";
            exit(EXIT_FAILURE);
        }


        // store location of implementation specific PRF file
        std::vector <std::string> PRFPaths;

        if ( matchedDevice->getPRFFile()) {
            if ( strlen(matchedDevice->getPRFFile()->getLocalFile().c_str()) > 0) {
                PRFPaths.push_back(matchedDevice->getPRFFile()->getLocalFile());
            }
        }

        // get generic device PRF
        if ( strlen(_SPDParser.getPRFFile()) > 0) {
            PRFPaths.push_back(_SPDParser.getPRFFile());
        }

        // record the mapping of the component instantiation id to the matched implementation id
        ComponentMapping _cm;
        _cm._instantiationid = componentPlacements[i].id();
        _cm._implementationid = matchedDevice->getID();
        _componentImplMap.push_back(_cm);

        //spawn device
#ifdef HAVE_WORKING_FORK
        DEBUG(2, DevMgr, "Launching Device file " << matchedDevice->getCodeFile () << " Usage name " << componentPlacements[i].usageName());

        if ((m_pid = fork()) < 0) std::cout << "Fork Error" << std::endl;
        if (m_pid == 0) {
            // in child
            if (getenv("VALGRIND")) {
                std::string logFile = "--log-file=";
                logFile += matchedDevice->getCodeFile();
                char *val = "/usr/local/bin/valgrind";
                execl(val, val, logFile.c_str(), matchedDevice->getCodeFile (), componentPlacements[i].id(), componentPlacements[i].usageName() , _DCDParser.getFileNameFromRefId(componentPlacements[i].refId()), NULL);
            } else {
                // NOTE: it's not pretty, but it's readable
                std::string _devexecparam[10];
                _devexecparam[0] = "DEVICE_MGR_IOR";
                _devexecparam[1] = ossieSupport::ORB::orb->object_to_string(_this());
                _devexecparam[2] = "PROFILE_NAME";
                _devexecparam[3] = _DCDParser.getFileNameFromRefId(componentPlacements[i].refId());
                _devexecparam[4] = "DEVICE_ID";
                _devexecparam[5] = componentPlacements[i].id();
                _devexecparam[6] = "DEVICE_LABEL";
                _devexecparam[7] = componentPlacements[i].usageName();
                // NOTE: NAMING_CONTEXT_IOR is not a required parameter per the specification,
                // but its inclusion enables distributed nodes within the OSSIE universe
                _devexecparam[8] = "NAMING_CONTEXT_IOR";
                _devexecparam[9] = ossieSupport::ORB::orb->object_to_string(ossieSupport::ORB::inc);

                execl(
                    matchedDevice->getCodeFile (),
                    matchedDevice->getCodeFile (),
                    _devexecparam[0].c_str(),
                    _devexecparam[1].c_str(),
                    _devexecparam[2].c_str(),
                    _devexecparam[3].c_str(),
                    _devexecparam[4].c_str(),
                    _devexecparam[5].c_str(),
                    _devexecparam[6].c_str(),
                    _devexecparam[7].c_str(),
                    _devexecparam[8].c_str(),
                    _devexecparam[9].c_str(),
                    NULL);
            }
            /// \todo if execl fails in child, parent should be notified
            std::cout << "[DeviceManager::execute] Device did not execute : " << strerror(errno) << std::endl;
            exit (EXIT_FAILURE);
        }
#endif
        ossieSupport::nsleep(0, 1000*1000);
        CORBA::Object_var _obj = CORBA::Object::_nil();
        char nameStr[255];
        sprintf( nameStr, "DomainName1/%s", componentPlacements[i].usageName() );
        DEBUG(3, DevMgr, "searching for "<< nameStr);
        do {
            /// \todo sleep prevents system from beating Name Service to death, Fix better
            ossieSupport::nsleep(0, 50*1000);
            try {
                _obj = orb_obj->get_object_from_name(nameStr);
            } catch (CosNaming::NamingContext::NotFound) {
                ossieSupport::nsleep(0, 100*1000);
            }
        } while (CORBA::is_nil (_obj));
        DEBUG(3, DevMgr, "found "<< nameStr);

        CF::Device_var tempDevice = CF::Device::_narrow (_obj);

            tempDevice->initialize ();


        // tally all properties
        CF::Properties allProperties;

        //Get properties from PRF
        for (int k = 0; k < PRFPaths.size(); k++) {
            CF::File_var _prf;

                DEBUG(7, DevMgr, "opening PRF File: " << PRFPaths[k]);
                _prf = _fileSys->open( PRFPaths[k].c_str(), true );


            PRFParser _PRFparser ( _prf );

                _prf->close();


            // get properties stored from device PRF and
            // implemenation PRF (if either exist)
            std::vector <PRFProperty *> *configProps = _PRFparser.getConfigureProperties ();
            int oldLen = allProperties.length();
            int newLen = allProperties.length() + configProps->size();

            if (newLen > oldLen) {
                allProperties.length(newLen);

                DEBUG(7, DevMgr, "Number of Configure Properties: " << configProps->size());
                for (unsigned int p = oldLen; p < newLen; p++) {
                    DEBUG(7, DevMgr, "adding configure property: " << (*configProps)[p]->getName());
                    allProperties[p] = *((*configProps)[p]->getDataType ());
                }
            }

            // get initial capacities on device
            std::vector <PRFProperty*> *allocProps = _PRFparser.getCapacityProperties();
            oldLen = allProperties.length();
            newLen = allProperties.length() + allocProps->size();

            if (newLen > oldLen) {
                allProperties.length(newLen);

                DEBUG(7, DevMgr, "Number of Allocation Properties: " << allocProps->size());
                for (unsigned int p = oldLen; p < newLen; p++) {
                    DEBUG(7, DevMgr, "adding allocation property: " << (*allocProps)[p]->getName());
                    allProperties[p] = *((*allocProps)[p]->getDataType());
                }

            }
        }


            tempDevice->configure(allProperties);


        DEBUG(3, DevMgr, "Registering device");

            registerDevice (CF::Device::_duplicate(tempDevice));


        DEBUG(3, DevMgr, "Device Registered");
    }

// print device information from DPD
    printDPD(_dpd);

}

void DeviceManager_impl::printDPD(std::vector <DPDParser*> DPD)
{
    if (DPD.size() > 0) {
        std::cout << "\n";
        DEBUG(1, DevMgr, "Printing Device Deployment Information");

        for (int i = 0; i < DPD.size(); i++) {
            DevicePkg *devPkg = DPD[i]->getDevicePkg();

            std::cout << "  Device Package Descriptor " << i+1 << " of " << DPD.size() << "\n";
            std::cout << "  Device Package: " << devPkg->getTitle() << "\n";
            std::cout << "  Description: " << devPkg->getDescription() << "\n";

            HWDeviceRegistration *parentHW = devPkg->getHWDeviceRegistration();
            std::cout << "  Parent Device: " << parentHW->getName() << "\n";
            std::cout << "  Parent Description: " << parentHW->getDescription() << "\n";

            std::vector <ChildHWDevice*> childHW = parentHW->getChildHWDevice();

            for (int i = 0; i < childHW.size(); i++) {
                printChildHWRecursive(childHW[i], 4);
            }
        }
        std::cout << "\n";
    } else {
        DEBUG(1, DevMgr, "No DPD File present.");
    }
}

bool DeviceManager_impl::checkImplForGPP(std::vector <SPDImplementation*> *impls)
{
    DEBUG(4, DevMgr, "in checkImplForGPP");
// check all SPD implementations for OS name

    for (unsigned int i = 0; i < impls->size(); i++) {
        DEBUG(7, DevMgr, "Checking implementation " << i);

        // check for OS name
        // NOTE: This is an insufficient test of a GPP
        std::string osname = (*impls)[i]->getOperatingSystem().getOSName();
        if (osname == "") {
            DEBUG(7, DevMgr, "OS Name is blank, device is not a GPP");
            return false;
        }
    }

    DEBUG(7, DevMgr, "Implementation is for a GPP");
    return true;
}

void DeviceManager_impl::parseDPDFiles(std::vector <DPDParser*> &_DPD, DCDParser &_DCD)
{
// this function gets references to all the DPD files, and then parses them

    DEBUG(4, DevMgr, "in parseDPDFiles");

    std::vector <std::string> DPDList;
    std::vector <DCDComponentPlacement*> *compPlaces = _DCD.getDeployOnComponents();

// get all DPD paths
    for (int k = 0; k < compPlaces->size(); k++) {
        if (strlen((*compPlaces)[k]->getDPDFile().c_str()) > 0) {
            DPDList.push_back((*compPlaces)[k]->getDPDFile());
        }
    }

// parse all DPD files
    CF::File_var _dpdFile;
    for (int k = 0; k < DPDList.size(); k++) {

            DEBUG(7, DevMgr, "opening DPD File: " << DPDList[k]);
            _dpdFile = _fileSys->open(DPDList[k].c_str(), true);


        _DPD.push_back(new DPDParser(_dpdFile));


            _dpdFile->close();

    }

    DEBUG(7, DevMgr, "leaving parseDPDFile");
}

void DeviceManager_impl::printChildHWRecursive(ChildHWDevice *child, int indent)
{
// this function recursively prints DPD files

// check for hw dev reg
    HWDeviceRegistration *hwDev = child->getHWDeviceRegistration();
    if (hwDev != NULL) {

        for (int k = 0; k < indent; k++)
            std::cout << " ";

        std::cout << "Child Device: " << hwDev->getName() << " (" << hwDev->getDescription() << ")\n";

        std::vector <ChildHWDevice*> childDevices = hwDev->getChildHWDevice();
        for (int i = 0; i < childDevices.size(); i++) {
            // additional child devices found
            printChildHWRecursive(childDevices[i], indent+4);
        }
    }

}

SPDImplementation* DeviceManager_impl::matchOtherDevice(std::vector <SPDImplementation*> *impls, SPDImplementation *devMan)
{
// device implementation is not a GPP, need to search against
// <dependency> tag in Dev Man

    DEBUG(4, DevMan, "in matchOtherDevice.");

// device is not a GPP, checking against uses device
    std::vector <SPDDependency*> devManDevices = devMan->getDependencies();
// loop through device manager <dependency>
    for (unsigned int k = 0; k < devManDevices.size(); k++) {
        // loop through spd implemenatations of device
        for (unsigned int p = 0; p < impls->size(); p++) {
            std::string devID = (*impls)[p]->getID();
            std::string devManID = devManDevices[k]->getSoftPkgRef();
            // check IDs
            if (devID == devManID) {
                DEBUG(7, DevMan, "Implementation matched in DevMan SPD and device impl.");
                return (*impls)[p];
            }
        }
    }

    DEBUG(7, DevMan, "Could not pair DevMan SPD and Device impl");
    return NULL;
}

SPDImplementation* DeviceManager_impl::matchGPP(std::vector <SPDImplementation*> *impls, SPDImplementation *devMgr)
{
    DEBUG(4, DevMgr, "in matchGPP");

// Go through devices, and search through the current component
// SPD implementations for matching impls. Once a matching implementation
// has been found, set the appropriate parameters on the component
// to be executed

// search impls for matching implementation details from device,
// stop once the first matching implementation is found
// (this checking section will need to be expanded if other
// options need to be verified: compiler, etc.)

// (it is currently assumed in SPDImplementation.h,
// DeviceManager.cpp, and other places that there is only
// a single OS. this is not compliant and needs to be fixed.)
    DEBUG(4, DevMgr, "Matching device manager implementation to candidate device impls.");

// flip through all component implementations
    unsigned int i = 0;
    while ( i < impls->size() ) {
        DEBUG(4, DevMgr, "Attempting to match Device ID: " << (*impls)[i]->getID());

        // verify that the operating system name and version match
        bool osmatch = matchOperatingSystem((*impls)[i], devMgr);
        if (osmatch) {
            // verify that the processors are compatible
            bool matchproc = matchProcessor((*impls)[i], devMgr);
            if (matchproc) {
                // all parameters match
                DEBUG(4, DevMgr, "found matching processing device implementation to the Device Manager's implementation.");

                return (*impls)[i];
            }
        }
        i++;
    }

// if a component implementation cannot be matched to the device, error out
    DEBUG(4, DevMgr,"Could not match an implementation.");
    return NULL;
}

bool DeviceManager_impl::matchOperatingSystem(SPDImplementation *devMan, SPDImplementation *device)
{
// match the operating system name and version from the
// device manager to the device

    DEBUG(4, DevMgr, "in matchOperatingSystem.");

// device manager OS
    std::string deviceOSName = devMan->getOperatingSystem().getOSName();
    std::string deviceOSVersion = device->getOperatingSystem().getOSVersion();

// device OS
    OSAttributes osattr = device->getOperatingSystem();
    std::string devManOSName = osattr.getOSName();
    std::string devManOSVersion = osattr.getOSVersion();

// perform matching
    if (deviceOSName == devManOSName) {
        DEBUG(7, DevMgr, "Found matching Operating System Name");

        if (deviceOSVersion == devManOSVersion) {
            DEBUG(7, DevMgr, "Found matching Operating System Version");
            return true;
        } else {
            DEBUG(7, DevMgr, "Could not match Operating System Version");
            return false;
        }
    } else {
        DEBUG(7, DevMgr, "Could not match Operating System name");
        return false;
    }

// catch all
    DEBUG(7, DevMgr, "match OS logic failed somehow, this should not happen");
    exit(EXIT_FAILURE);
}

bool DeviceManager_impl::matchProcessor(SPDImplementation *devMan, SPDImplementation *device)
{
// check all of the compatible processors in the component impl
// against the processor in the device

// (realistically the algorithm needs to check against the single
// processor that the component will need to deploy to)

    DEBUG(4, DevMgr, "in matchProcessor");

    unsigned int k = 0;
    unsigned int p = 0;
    std::vector <std::string> devManProcs = devMan->getProcessors();
    std::vector<std::string> deviceProclist = device->getProcessors();

    while ( k < devManProcs.size() ) {
        while ( p < deviceProclist.size() ) {
            if (devManProcs[k] == deviceProclist[p]) {
                DEBUG(4, DevMgr, "Found processor match.");
                return true;
            }
            p++;
        }
        k++;
    }

    DEBUG(4, DevMgr, "Could not match processor.");
    return false;

}


void
DeviceManager_impl::init ()
{

    _adminState = DEVMGR_UNREGISTERED;
}


void
DeviceManager_impl::getDomainManagerReference (char *domainManagerName)
{
    CORBA::Object_var obj = CORBA::Object::_nil();

/// \todo sleep prevents system from beating Name Service to death, Fix better
    do {

            obj = orb_obj->get_object_from_name (domainManagerName);

        usleep(1000);
    } while (CORBA::is_nil(obj));


        _dmnMgr = CF::DomainManager::_narrow (obj);

}


char *DeviceManager_impl::deviceConfigurationProfile ()
throw (CORBA::SystemException)
{
    return CORBA::string_dup(_deviceConfigurationProfile.c_str());
}


CF::FileSystem_ptr DeviceManager_impl::fileSys ()throw (CORBA::
        SystemException)
{
    CF::FileSystem_var result = _fileSys;
    return result._retn();
}


char *DeviceManager_impl::identifier ()
throw (CORBA::SystemException)
{
    return CORBA::string_dup (_identifier.c_str());
}


char *DeviceManager_impl::label ()
throw (CORBA::SystemException)
{
    return CORBA::string_dup (_label.c_str());
}


CF::DeviceSequence *
DeviceManager_impl::registeredDevices ()throw (CORBA::SystemException)
{
    CF::DeviceSequence* result = new CF::DeviceSequence(_registeredDevices);
    return result;
}


CF::DeviceManager::ServiceSequence *
DeviceManager_impl::registeredServices ()throw (CORBA::SystemException)
{
    CF::DeviceManager::ServiceSequence_var result = new CF::DeviceManager::ServiceSequence(_registeredServices);
    return result._retn();
}


void
DeviceManager_impl::registerDevice (CF::Device_ptr registeringDevice)
throw (CORBA::SystemException, CF::InvalidObjectReference)
{
    if (CORBA::is_nil (registeringDevice)) {
        //writeLogRecord(FAILURE_ALARM,invalid reference input parameter.)
        throw (CF::
               InvalidObjectReference
               ("[DeviceManager::registerDevice] Cannot register Device. registeringDevice is a nil reference."));
    }

// Register the device with the Device manager, unless it is already
// registered
    if (!deviceIsRegistered (registeringDevice)) {
        _registeredDevices.length (_registeredDevices.length () + 1);
        _registeredDevices[_registeredDevices.length () - 1] =
            registeringDevice;
    }

// If this Device Manager is registered with a Domain Manager, register
// the new device with the Domain Manager
    if (_adminState == DEVMGR_REGISTERED) {

            _dmnMgr->registerDevice (registeringDevice, myObj);

    }

//The registerDevice operation shall write a FAILURE_ALARM log record to a
//DomainManagers Log, upon unsuccessful registration of a Device to the DeviceManagers
//registeredDevices.
}


//This function returns TRUE if the input registeredDevice is contained in the _registeredDevices list attribute
bool DeviceManager_impl::deviceIsRegistered (CF::Device_ptr registeredDevice)
{
//Look for registeredDevice in _registeredDevices
    for (unsigned int i = 0; i < _registeredDevices.length (); i++) {
        if (strcmp (_registeredDevices[i]->label (), registeredDevice->label ()) == 0) {
            return true;
        }
    }
    return false;
}


//This function returns TRUE if the input serviceName is contained in the _registeredServices list attribute
bool DeviceManager_impl::serviceIsRegistered (const char *serviceName)
{
//Look for registeredDevice in _registeredDevices
    for (unsigned int i = 0; i < _registeredServices.length (); i++) {
        if (strcmp (_registeredServices[i].serviceName, serviceName)  == 0) {
            return true;
        }
    }
    return false;
}


void
DeviceManager_impl::unregisterDevice (CF::Device_ptr registeredDevice)
throw (CORBA::SystemException, CF::InvalidObjectReference)
{
    bool deviceFound = false;
    if (CORBA::is_nil (registeredDevice)) {       //|| !deviceIsRegistered(registeredDevice) )
//The unregisterDevice operation shall write a FAILURE_ALARM log record, when it cannot
//successfully remove a registeredDevice from the DeviceManagers registeredDevices.

//The unregisterDevice operation shall raise the CF InvalidObjectReference when the input
//registeredDevice is a nil CORBA object reference or does not exist in the DeviceManagers
//registeredDevices attribute.
        /*writeLogRecord(FAILURE_ALARM,invalid reference input parameter.); */
        throw (CF::
               InvalidObjectReference
               ("Cannot unregister Device. registeringDevice is a nil reference."));

        return;
    }

//The unregisterDevice operation shall remove the input registeredDevice from the
//DeviceManagers registeredDevices attribute.

//Look for registeredDevice in _registeredDevices
    for (unsigned int i = 0; i < _registeredDevices.length (); i++) {
//        if (strcmp (_registeredDevices[i]->label (), registeredDevice->label ())== 0)
        if (registeredDevice->_is_equivalent(_registeredDevices[i])) {
//when the appropiater device is found, remove it from the _registeredDevices sequence
            deviceFound = true;
            if (_adminState == DEVMGR_REGISTERED) {

                    _dmnMgr->unregisterDevice (CF::Device::_duplicate (registeredDevice));

                CORBA::release (registeredDevice);
            }
            for (unsigned int j = i; j < _registeredDevices.length () - 1; j++) {
//The unregisterDevice operation shall unregister
//the input registeredDevice from the DomainManager when the input registeredDevice is
//registered with the DeviceManager and the DeviceManager is not shutting down.
                _registeredDevices[j] = _registeredDevices[j + 1];
            }
//_registeredDevices[_registeredDevices.length() - 1] = 0;
            _registeredDevices.length (_registeredDevices.length () - 1);
//TO DO: Avoid memory leaks by reducing the length of the sequence _registeredDevices
            break;
        }
    }
    if (!deviceFound) {
        /*writeLogRecord(FAILURE_ALARM,invalid reference input parameter.); */

        throw (CF::
               InvalidObjectReference
               ("Cannot unregister Device. registeringDevice was not registered."));
        return;
    }

}


void
DeviceManager_impl::shutdown ()
throw (CORBA::SystemException)
{
    _adminState = DEVMGR_SHUTTING_DOWN;

//The shutdown operation shall perform releaseObject on all of the DeviceManagers registered
//Devices (DeviceManagers registeredDevices attribute).

    for (int i = _registeredDevices.length () - 1; i >= 0; i--) {
//Important Note: It is necessary to manage the lenght of the _registeredDevices sequence
//otherwise, some elements in the sequence will be null.
        /*
             CF::LifeCycle_var _dev2release;
             try
             {
              CORBA::Object_var _obj = orb_obj->get_object_from_name("DomainName1/GPP1");
              if( CORBA::is_nil(_obj) ) exit(EXIT_FAILURE);
              _dev2release = CF::LifeCycle::_narrow(_obj);
             } catch(...) {
              DEBUG(1, DevMgr, "Failed to resolve reference to a Device");
              exit(EXIT_FAILURE);
             }

             try
             {
              _dev2release->releaseObject();
              //_registeredDevices[i]->releaseObject ();
             } catch( CORBA::SystemException& se ) {
              std::cout << "[DeviceManager::shutdown] \"registeredDevices[" << i << "]->releaseObject\" failed with CORBA::SystemException\n";
              exit(EXIT_FAILURE);
             } catch( ... ) {
              std::cout << "[DeviceManager::shutdown] \"registeredDevices[" << i << "]->releaseObject\" failed with Unknown Exception\n";
              exit(EXIT_FAILURE);
             }
        */
        unregisterDevice (_registeredDevices[i]);
    }
//The shutdown operation shall unregister the DeviceManager from the DomainManager.

        _dmnMgr->unregisterDeviceManager (CF::DeviceManager::_duplicate(this->_this())); ///\bug This looks wrong.

}


void
DeviceManager_impl::registerService (CORBA::Object_ptr registeringService,
                                     const char *name)
throw (CORBA::SystemException, CF::InvalidObjectReference)
{
//This release does not support services
    if (CORBA::is_nil (registeringService)) {
        /*writeLogRecord(FAILURE_ALARM,invalid reference input parameter.); */

        throw (CF::
               InvalidObjectReference
               ("Cannot register Device. registeringDevice is a nil reference."));
        return;
    }

//The registerService operation shall add the input registeringService to the DeviceManagers
//registeredServices attribute when the input registeringService does not already exist in the
//registeredServices attribute. The registeringService is ignored when duplicated.
    if (!serviceIsRegistered (name)) {
        _registeredServices.length (_registeredServices.length () + 1);
        _registeredServices[_registeredServices.length () - 1].serviceObject = registeringService;
        _registeredServices[_registeredServices.length () - 1].serviceName = name;
    }

//The registerService operation shall register the registeringService with the DomainManager
//when the DeviceManager has already registered to the DomainManager and the
//registeringService has been successfully added to the DeviceManagers registeredServices
//attribute.
    if (_adminState == DEVMGR_REGISTERED) {

            _dmnMgr->registerService (registeringService, this->_this (), name);

    }

//The registerService operation shall write a FAILURE_ALARM log record, upon unsuccessful
//registration of a Service to the DeviceManagers registeredServices.
//The registerService operation shall raise the CF InvalidObjectReference exception when the
//input registeringService is a nil CORBA object reference.

}


void
DeviceManager_impl::unregisterService (CORBA::Object_ptr registeredService,
                                       const char *name)
throw (CORBA::SystemException, CF::InvalidObjectReference)
{
    if (CORBA::is_nil (registeredService)) {
        /*writeLogRecord(FAILURE_ALARM,invalid reference input parameter.); */

        throw (CF::
               InvalidObjectReference
               ("Cannot unregister Service. registeringService is a nil reference."));
        return;
    }

//The unregisterService operation shall remove the input registeredService from the
//DeviceManagers registeredServices attribute. The unregisterService operation shall unregister
//the input registeredService from the DomainManager when the input registeredService is
//registered with the DeviceManager and the DeviceManager is not in the shutting down state.

//Look for registeredService in _registeredServices
    for (unsigned int i = 0; i < _registeredServices.length (); i++) {
        if (strcmp (_registeredServices[i].serviceName, name) == 0) {
//when the appropiater device is found, remove it from the _registeredDevices sequence
            if (_adminState == DEVMGR_REGISTERED) {

                    _dmnMgr->unregisterService (registeredService, name);

            }

            for (unsigned int j = i; j < _registeredServices.length ()-1; j++) {

                CORBA::release (registeredService);
                _registeredServices[j] = _registeredServices[j+1];
            }
            _registeredServices.length (_registeredServices.length () - 1);
            return;
        }
    }

//If it didn't find registeredDevice, then throw an exception
    /*writeLogRecord(FAILURE_ALARM,invalid reference input parameter.);*/
    throw (CF::
           InvalidObjectReference
           ("Cannot unregister Service. registeringService was not registered."));
//The unregisterService operation shall write a FAILURE_ALARM log record, when it cannot
//successfully remove a registeredService from the DeviceManagers registeredServices.
//The unregisterService operation shall raise the CF InvalidObjectReference when the input
//registeredService is a nil CORBA object reference or does not exist in the DeviceManagers
//registeredServices attribute.
}


char *
DeviceManager_impl::
getComponentImplementationId (const char *componentInstantiationId)
throw (CORBA::SystemException)
{
    DEBUG(5, DevMgr, "Entering getComponentImplementationId");
//The getComponentImplementationId operation shall return the SPD implementation elements
//ID attribute that matches the SPD implementation element used to create the component
//identified by the input componentInstantiationId parameter.

    char* _id;
    for ( int i = 0; i < _componentImplMap.size(); i++ ) {
        if ( strcmp( componentInstantiationId, _componentImplMap[i]._instantiationid.c_str() ) == 0 ) {
            DEBUG(9, DevMgr, "Found matching instantiation ID");
            _id = CORBA::string_dup( _componentImplMap[i]._implementationid.c_str() );
            DEBUG(5, DevMgr, "Leaving getComponentImplementationId");
            return _id;
        }
    }
    _id = CORBA::string_dup("");
    DEBUG(9, DevMgr, "NO matching instantiation ID");
    DEBUG(5, DevMgr, "Leaving getComponentImplementationId");
    return _id;

//The getComponentImplementationId operation shall return an empty string when the input
//componentInstantiationId parameter does not match the ID attribute of any SPD implementation
//element used to create the component.
}

