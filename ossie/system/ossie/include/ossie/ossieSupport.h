/*******************************************************************************

Copyright 2008, Virginia Polytechnic Institute and State University

This file is part of the OSSIE Parser.

OSSIE Parser is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Parser is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Parser; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef ORBSUPPORT_H
#define ORBSUPPORT_H

#include <string>
#include <vector>

#include <sched.h>

#ifdef HAVE_OMNIORB4_CORBA_H
#include "omniORB4/CORBA.h"
#endif

#include "ossie/cf.h"
#include "ossie/StandardEvent.h"

#include "ossieparser.h" // For OSSIEPARSER_API which is needed for windows shared libraries, for other targets the definition is empty

/**
The ossieSupport namespace contains useful functions used throughout the
OSSIE framework. The classes in this namespace are also useful for SCA
component developers.
*/

namespace ossieSupport
{

/**
The ORB class provides access to a CORBA orb variable across multiple classes.

*/
class OSSIEPARSER_API ORB
{
public:
    /**
       The first time the constructor runs, a CORBA orb variable is
       initialized. Additionally, variables for the POA, POA manager, and
       initial naming context are created. Creating additional ORB objects
       provides access to these variables.
     */
    ORB();

/// This constructor privides argc and argv to the CORBA orb when it is created.
    ORB(int argc, char *argv[]);
    /**
       The destructor decrements a reference count, when the last reference
       to the orb is destroyed, the destructor releases the orb variable.
    */
    ~ORB();


///\todo Define exceptions for these methods to through

/// Return an object reference for the object name contained in name.
    CORBA::Object_ptr get_object_from_name(const char *name);

    CORBA::Object_ptr get_object_from_name(const CosNaming::NamingContext_ptr nc, const char *name);

/// Return a pointer to a CosName from a string. Free the returned pointer when you are finished with it.
    CosNaming::Name_var string_to_CosName(const char *name);

/// Create an entry, name, in the Naming Service for the object reference obj.
    void bind_object_to_name(CORBA::Object_ptr obj, const char *name);
    void bind_object_to_name(CORBA::Object_ptr obj, const CosNaming::NamingContext_ptr nc, const char *name);

/// Remove a name from the Naming Service
    void unbind_name(const char *name);
    void unbind_name(const CosNaming::NamingContext_ptr nc, const char *name);

/// Remove all names from a Context
    void unbind_all_from_context(const CosNaming::NamingContext_ptr nc);

// ORB variables
    static CORBA::ORB_var orb;
    static PortableServer::POA_var poa;
    static PortableServer::POAManager_var pman;
    static CosNaming::NamingContext_var inc;

private:
    void init(void);
    static unsigned int ref_cnt; //Count the number of class instances


};


// Event Channel Support routines ///\todo Add real event support

void sendObjAdded_event(const char *producerId, const char *sourceId, const char *sourceName, CORBA::Object_ptr sourceIOR, StandardEvent::SourceCategoryType sourceCategory);




// Application support routines

// Base class to contain data for the components required to
// create an application

class ComponentInfo

{
public:
    ComponentInfo ();

    void setName(const char *name);
    void setIdentifier(const char *identifier);
    void setAssignedDeviceId(const char *name);
///\todo make this work for multiple implementations
    void setImplId(const char *implId);
    void setCodeType(CF::LoadableDevice::LoadType codeType);
    void setLocalFileName(const char *localFileName);
    void setImplPRFFile(const char *PRFFile);
    void setEntryPoint(const char *entryPoint);
    void setNamingService(const bool isNamingService);
    void setNamingServiceName(const char *NamingServiceName);
    void setUsageName(const char *usageName);
    void setIsResource(bool isResource);
    void setIsConfigurable(bool isConfigurable);
    void setIsAssemblyController(bool isAssemblyController);

    void setDeployedOnResourceFactory(bool _deployedOnResourceFactory);
    void setDeployedOnExecutableDevice(bool _deployedOnExecutableDevice);
    void setPID(CF::ExecutableDevice::ProcessID_Type _pid);
    void setDeployedOnLoadableDevice(bool _deployedOnLoadableDevice);

    void addFactoryParameter(CF::DataType *dt);
    void addExecParameter(CF::DataType *dt);
    void addAllocationCapacity(CF::DataType *dt);
    void addConfigureProperty(CF::DataType *dt);
    void overrideProperty(const char *id, std::vector <std::string> values);

    void setResourcePtr(CF::Resource_ptr);
    void setDevicePtr(CF::Device_ptr);

    const char* getName();
    const char* getIdentifier();
    const char* getAssignedDeviceId();
    const char* getImplId();
    CF::LoadableDevice::LoadType getCodeType();
    const char* getLocalFileName();
    const char* getImplPRFFile();
    const char* getEntryPoint();
    const bool  getNamingService();
    const char* getUsageName();
    const char* getNamingServiceName();
    const bool  getIsResource();
    const bool  getIsConfigurable();
    const bool  getIsAssemblyController();

    const bool  getDeployedOnResourceFactory();
    const bool  getDeployedOnExecutableDevice();
    const CF::ExecutableDevice::ProcessID_Type getPID();
    const bool  getDeployedOnLoadableDevice();

    CF::Properties getConfigureProperties();
    CF::Properties getAllocationCapacities();
    CF::Properties getOptions();
    CF::Properties getExecParameters();

    CF::Resource_ptr getResourcePtr();
    CF::Device_ptr getDevicePtr();

private:
    ComponentInfo (const ComponentInfo &);

    void process_overrides(CF::Properties *props, const char *id, std::vector <std::string> values);
    bool isAssemblyController;
    bool isResource;
    bool isConfigurable;
    bool isNamingService;
    bool deployedOnResourceFactory;
    bool deployedOnLoadableDevice;
    bool deployedOnExecutableDevice;

    std::string name;    // Component name from SPD File
    std::string assignedDeviceId;  // Device to deploy component on from DAS.

///\todo Support multiple implementations in SPD File
// The implementation stuff should be vectorized to support more than
// one implementation. The implemenation parser may not be consistent
// how it returns values.
    std::string implId;  // Implementation ID from SPD File
    CF::LoadableDevice::LoadType codeType;  // Implementation code type from SPD File
    std::string localFileName;  // Implementation local file name from SPD File
    std::string implPRF; // PRF path for specific implementation
    std::string entryPoint; // Implementation entry point from SPD File
    std::string usageName;
    std::string identifier;
    std::string namingServiceName;

// PJB    CF::LoadableDevice::LoadType codeType;
//    std::vector <SPDUsesDevice *>* usesDevices;

    void addProperty(CF::DataType *dt, CF::Properties &prop);

    CF::Properties allocationCapacities;
    CF::Properties configureProperties;
    CF::Properties options;
    CF::Properties factoryParameters;
    CF::Properties execParameters;

    CF::ExecutableDevice::ProcessID_Type PID;

    CF::Resource_var rsc;
    CF::Device_var devicePtr;
};

class ConnectionInfo
{
public:
    ConnectionInfo(CF::Port_ptr, const char*);

    CF::Port_ptr getPort();
    const char *getID();

private:
    ConnectionInfo();  // No default constructor
    ConnectionInfo(ConnectionInfo &);  // No copying

    CF::Port_var port_obj;
    std::string identifier;
};

// Miscellaneous helper functions

void createProfileFromFileName(std::string fileName, std::string &profile);
std::string getFileNameFromProfile(std::string profile);
bool isValidFileName(const char* fileName);
const char *spd_rel_file(const char *spdfile, const char *name, std::string &fileName);


class ossieComponent
{

public:
    ossieComponent(ossieSupport::ORB *orb, int argc, char *argv[]);
    ~ossieComponent();

    void bind(CF::Resource_ptr res);
    void unbind();

    const char *getUuid();
    void getCLArgs(CF::Properties &props);

private:
    ossieComponent();

    std::string namingContextIOR;
    std::string uuid;
    std::string nameBinding;
    bool namingContextIORFound;
    bool uuidFound;
    bool nameBindingFound;

    int scheduler;
    sched_param p;

    ossieSupport::ORB *orb;
    CosNaming::NamingContext_var nc;

    unsigned int numArgs;
    CF::Properties CLArgs;
};

}  // Close ossieSupport Namespace
#endif
