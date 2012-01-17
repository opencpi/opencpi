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

#include <iostream>
#include <sstream>

#include "ossie/debug.h"
#include "ossie/ossieSupport.h"
#include "ossie/portability.h"
#include "ossie/ApplicationFactory_impl.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

ApplicationFactory_impl::ApplicationFactory_impl (const char *_softProfile, CF::DomainManager::ApplicationSequence *_appseq)
{
    orb = new ossieSupport::ORB();

// Get application factory data for private variables
    _softwareProfile = _softProfile;
    appseq = _appseq;

// Get an object reference for the domain manager
    CORBA::Object_ptr obj;

        obj = orb->get_object_from_name("DomainName1/DomainManager");



        dmnMgr = CF::DomainManager::_narrow (obj);



        fileMgr = dmnMgr->fileMgr();


    CF::File_var _sad;

        _sad = fileMgr->open( _softwareProfile.c_str(), true );


    _sadParser = new SADParser ( _sad );


        _sad->close();


    _name = _sadParser->getName();
    _identifier = _sadParser->getID();

#ifdef HAVE_OMNIEVENTS

    ApplicationFactoryEventHandler *_evtHandler =
        new ApplicationFactoryEventHandler (this);

    dmnMgr->registerWithEventChannel (_evtHandler->_this (),
                                      CORBA::string_dup (_identifier),
                                      CORBA::string_dup ("ODM_Channel"));
#endif
}


ApplicationFactory_impl::~ApplicationFactory_impl ()
{
    delete orb;
}


/// \TODO Comment/Rewrite this function. It's painful.
CF::Application_ptr ApplicationFactory_impl::create (const char *name,
        const CF::Properties & initConfiguration,
        const CF::DeviceAssignmentSequence & deviceAssignments)
throw (CORBA::SystemException, CF::ApplicationFactory::CreateApplicationError,
       CF::ApplicationFactory::CreateApplicationRequestError,
       CF::ApplicationFactory::InvalidInitConfiguration)
{

    DEBUG(1, AppFact, "entering appFactory->create")

// Establish naming context for this waveform
    CORBA::Object_var obj_DN;

        obj_DN = orb->get_object_from_name("DomainName1");


    CosNaming::NamingContext_var DomainContext = CosNaming::NamingContext::_nil();

        DomainContext = CosNaming::NamingContext::_narrow(obj_DN);


    short WaveformCount = 0;
    string waveform_context_name("");
    CORBA::Object_var obj_WaveformContext;
    CosNaming::NamingContext_var WaveformContext = CosNaming::NamingContext::_nil();

    bool found_empty = false;

    do {
        WaveformCount++;
        waveform_context_name = "";
        waveform_context_name.append(name);
        waveform_context_name.append("_");
        ostringstream number_str;
        number_str << WaveformCount;
        waveform_context_name.append(number_str.str());
        string temp_waveform_context("DomainName1/");
        temp_waveform_context.append(waveform_context_name);
        CosNaming::Name_var cosName = orb->string_to_CosName(temp_waveform_context.c_str());
        try {
            obj_WaveformContext = orb->inc->resolve(cosName);
        } catch (const CosNaming::NamingContext::NotFound &) {
            found_empty = true;
        }
    } while (!found_empty);

    CosNaming::Name WaveformContextName;
    WaveformContextName.length(1);
    WaveformContextName[0].id = CORBA::string_dup(waveform_context_name.c_str());


        DomainContext->bind_new_context(WaveformContextName);


    string base_naming_context("DomainName1/");
    base_naming_context.append(waveform_context_name);

// Populate registeredDevices vector
    if (registeredDevices.size()==0) getRegisteredDevices();

// The reason why it needs to be re-parsed is to populate the vector with the new waveform naming context
    if (requiredComponents.size()!=0) requiredComponents.clear();

    getRequiredComponents(base_naming_context.c_str(), deviceAssignments);

// If there is a device assignment sequence, verify its validity;
// otherwise generate deployment information automatically
/// \TODO Write dynamic component deployment routine
    if (deviceAssignments.length() > 0) {
        verifyDeviceAssignments(deviceAssignments);
    } else {
        std::cerr << "No Device Assignment Sequence, write dynamic deployment code" << endl;
        throw CF::ApplicationFactory::CreateApplicationRequestError();
    }

// Allocate space for device assignment
/// \TODO (assume maximum length of 10 - will make dynamic in later version)
    CF::DeviceAssignmentSequence* _availableDevs =
        new CF::DeviceAssignmentSequence(deviceAssignments);

    PROC_ID_SEQ* _pidSeq = new PROC_ID_SEQ (30);
    _pidSeq->length (0);

    loadAndExecuteComponents(_pidSeq);

/// \TODO Move this code into loadAndExecuteComponents

    CF::Resource_ptr _assemblyController = NULL;

    ELEM_SEQ* _namingCtxSeq = new ELEM_SEQ ();
    _namingCtxSeq->length (requiredComponents.size ());

    ELEM_SEQ* _implSeq = new ELEM_SEQ ();
    _implSeq->length (requiredComponents.size ());

// Install the different components in the system
    for (unsigned int rc_idx = 0; rc_idx < requiredComponents.size (); rc_idx++) {
        ossieSupport::ComponentInfo *component = requiredComponents[rc_idx];

        // This makes sure that the resource factory is not handled like a resource
        if (!component->getIsResource ()) {
            continue;
        }

        (*_namingCtxSeq)[rc_idx].componentId =
            CORBA::string_dup (component->getIdentifier ());

        (*_implSeq)[rc_idx].componentId =
            CORBA::string_dup (component->getIdentifier ());

        // Assuming 1 instantiation for each componentplacement
        if (component->getNamingService ()) {
            // This is for the naming-service based configuration,
            // it assumes that the component already exists (like a device)
            const char* _lookupName = component->getNamingServiceName ();

            (*_namingCtxSeq)[rc_idx].elementId = CORBA::string_dup (_lookupName);

            CORBA::Object_ptr _obj = CORBA::Object::_nil ();

            // Wait for component to start
            do {
                try {
                    _obj = orb->get_object_from_name(_lookupName);
                } catch (CosNaming::NamingContext::NotFound) {
                };

                ///\todo Check the name not found exceptions and make certain this is correct
                ossieSupport::nsleep(0,50 * 1000);
            } while (CORBA::is_nil (_obj));

            // Check to see if the resource is the assembly controller
            // either way, the resource is initialized and configured
            CF::Resource_ptr _rsc;

                _rsc = CF::Resource::_narrow (_obj);

            component->setResourcePtr(_rsc);

            if (component->getIsAssemblyController()) {
                _assemblyController = _rsc;

                    _assemblyController->initialize ();


                    _assemblyController->configure (initConfiguration);


                    _assemblyController->configure (component->getConfigureProperties());

            } else {
                if (component->getIsResource () && component->getIsConfigurable ()) {

                        _rsc->initialize ();


                        _rsc->configure (component->getConfigureProperties());

                }
            }
        }

#if 0 ///\todo Add support for resource factories 
        else if ((*ComponentInstantiationVector)[0]->isResourceFactoryRef ()) {
            // resource-factory based component instantiation
            SADComponentInstantiation* _resourceFacInstantiation = NULL;

            int tmpCnt = 0;

            // figure out which resource factory is used
            while (_resourceFacInstantiation == NULL) {
                _resourceFacInstantiation =
                    (*ComponentsVector)[tmpCnt]->
                    getSADInstantiationById ((*ComponentInstantiationVector)
                                             [0]->getResourceFactoryRefId ());
                tmpCnt++;
            }

            (*_namingCtxSeq)[i].elementId =
                CORBA::string_dup (_resourceFacInstantiation->
                                   getFindByNamingServiceName ());

            CORBAOBJ _obj =
                orb->get_object_from_name (_resourceFacInstantiation->
                                           getFindByNamingServiceName ());

            CF::ResourceFactory_ptr _resourceFactory =
                CF::ResourceFactory::_narrow (_obj);

            // configure factory
            /*std::vector < InstantiationProperty * >*_factoryInstantiationProps
            	= (*ComponentInstantiationVector)[0]->getFactoryProperties ();

            CF::Properties _factoryProperties (_factoryInstantiationProps->
            	size ());

            for (unsigned int j = 0; j < _factoryInstantiationProps->size ();
            	j++)
            {
            	_factoryProperties[j].id
            		=
            CORBA::string_dup ((*_factoryInstantiationProps)[j]->
            getID ());
            _factoryProperties[j].
            value <<= CORBA::
            string_dup ((*_factoryInstantiationProps)[j]->getValue ());
            }*/

            // unused PJB                   char *value = LoadInfoVector[vector_access]->name;

            // instantiate resource
            // (mechanism for this is left up to the factory's implementation)

            CF::Resource_ptr _resourceCreated =
                _resourceFactory->
                createResource (LoadInfoVector[vector_access]->name,
                                LoadInfoVector[vector_access]->factoryParameters);

            if (CORBA::is_nil (_resourceCreated))
                throw CF::ApplicationFactory::CreateApplicationError ();

            // check to see if the resource is the assembly controller
            //      either way, the resource is initialized and configured
            if (strcmp (_assemblyControllerProfile->getID (),
                        (*ComponentInstantiationVector)[0]->getID ()) == 0) {
                _assemblyController = _resourceCreated;
                _assemblyController->initialize ();
                _assemblyController->configure (initConfiguration);
                _assemblyController->
                configure (LoadInfoVector[vector_access]->configureProperties);
            } else {
                _resourceCreated->initialize ();
                _resourceCreated->
                configure (LoadInfoVector[vector_access]->configureProperties);
            }
            vector_access++;
        } else {
            const char* _id, *_usagename;

            _id = LoadInfoVector[i]->implId;

            _usagename = LoadInfoVector[i]->name;

            char* _lookup = new char[strlen (_usagename) + strlen (_id) + 1];

            strcpy (_lookup, _usagename);
            strcat (_lookup, "_");
            strcat (_lookup, _id);

            (*_namingCtxSeq)[i].elementId = CORBA::string_dup (_lookup);

            CORBA::Object_ptr _obj = orb->get_object_from_name(_lookup);

            if (_assemblyControllerProfile == (*ComponentInstantiationVector)[0]) {
                _assemblyController = CF::Resource::_narrow (_obj);
                _assemblyController->initialize ();
                _assemblyController->configure (initConfiguration);
                _assemblyController->configure (LoadInfoVector[i]->
                                                configureProperties);
            } else {
                CF::Resource_ptr _rsc = CF::Resource::_narrow (_obj);
                _rsc->initialize ();
                _rsc->configure (LoadInfoVector[i]->configureProperties);
            }

            delete _lookup;
            _lookup = NULL;
        }
#endif // End resource factory support
    }                                         // end for()

    std::vector < Connection * >*_connection = _sadParser->getConnections ();
    if (connectionData.size()!=0) {
        connectionData.clear();
    }

// Create all resource connections
    for (int c_idx = _connection->size () - 1; c_idx >= 0; c_idx--) {
        Connection *connection = (*_connection)[c_idx];

        DEBUG(3, AppFact, "Processing connection " << connection->getID())

        // Process provides port
        CORBA::Object_var provides_port_ref;

        ProvidesPort* _providesPortParser = connection->getProvidesPort ();
        FindBy * _findProvidesPortBy;
        CORBA::Object_var _providesObj;

        if (connection->isProvidesPort ()) {
            DEBUG(3, AppFact, "Provides Port is provides port")
            _providesPortParser = connection->getProvidesPort ();

            if (_providesPortParser->isFindBy()) {
                DEBUG(3, AppFact, "Provides port is find by component name")
                FindBy* _findProvidesPortBy = _providesPortParser->getFindBy ();

                if (_findProvidesPortBy->isFindByNamingService ()) {
                    string findbyname(base_naming_context);
                    findbyname.append("/");

                    // This initial string compare is here for legacy reasons
                    if (!strncmp("DomainName1", _findProvidesPortBy->getFindByNamingServiceName (), 11)) {
                        findbyname.append((_findProvidesPortBy->getFindByNamingServiceName ()+12));
                    } else {
                        findbyname.append(_findProvidesPortBy->getFindByNamingServiceName ());
                    }


                        _providesObj = orb->get_object_from_name(findbyname.c_str());

                }
            } else if (_providesPortParser->isComponentInstantiationRef()) {
                DEBUG(3, AppFact, "Provides port is find by componentinstantiationref");

                for (unsigned int i=0; i < requiredComponents.size(); i++) {
                    ossieSupport::ComponentInfo *component = requiredComponents[i];
                    DEBUG(3, AppFact, "Looking for provides port component ID " << component->getIdentifier() \
                          << " ID from SAD " << _providesPortParser->getComponentInstantiationRefID());

                    if (strcmp(component->getIdentifier(), _providesPortParser->getComponentInstantiationRefID()) == 0) {
                        _providesObj = component->getResourcePtr();
                        break;
                    }

                    if (i == (requiredComponents.size() - 1)) {
                        std::cerr << "Provides port component not found" << std::endl;
                        throw CF::ApplicationFactory::CreateApplicationError();
                    }
                }
            }
        } else if (connection->isFindBy ()) {
            DEBUG(3, AppFact, "Provides Port is FindBy port name") \
            _findProvidesPortBy = connection->getFindBy ();

            if (_findProvidesPortBy->isFindByNamingService ()) {
                string findbyname(base_naming_context);
                findbyname.append("/");

                // This initial string compare is here for legacy reasons
                if (!strncmp("DomainName1", _findProvidesPortBy->getFindByNamingServiceName (), 11)) {
                    findbyname.append((_findProvidesPortBy->getFindByNamingServiceName ()+12));
                } else {
                    findbyname.append(_findProvidesPortBy->getFindByNamingServiceName ());
                }

                // The name is not found because it is hardware
                DEBUG(4, AppFact, "The findname that I'm using is: " << findbyname);
                try {
                    provides_port_ref = orb->get_object_from_name (findbyname.c_str());
                } catch (CosNaming::NamingContext::NotFound) {
                    string my_component_name = findbyname;
                    size_t location_first_slash = my_component_name.find('/', 0);
                    size_t location_second_slash = my_component_name.find('/', location_first_slash+1);
                    findbyname = "DomainName1/";
                    findbyname.append(my_component_name, location_second_slash+1, \
                                      my_component_name.length()-location_second_slash-1);
                    DEBUG(4, AppFact, "The findname that I'm using is: " << findbyname);
                }


                    provides_port_ref = orb->get_object_from_name (findbyname.c_str());

            } else {
                std::cerr << "Cannot find naming service name for FindBy provides port" << std::endl;
                throw CF::ApplicationFactory::CreateApplicationError();
                /// \todo throw an exception?
            }
        } else {
            std::cerr << "Cannot find port information for provides port" << std::endl;
            throw CF::ApplicationFactory::CreateApplicationError();
            /// \todo throw an exception?
        }

        // Find object ref for uses port
        // Process uses port
        UsesPort* _usesPortParser = connection->getUsesPort ();
        DEBUG(3, AppFact, "Uses port Identifier - " << _usesPortParser->getID());
        CORBA::Object_var _usesObj;

        if (_usesPortParser->isFindBy()) {
            FindBy* _findUsesPortBy = _usesPortParser->getFindBy ();

            if (_findUsesPortBy->isFindByNamingService ()) {
                string findbyname(base_naming_context);
                findbyname.append("/");

                // This initial string compare is here for legacy reasons
                if (!strncmp("DomainName1", _findUsesPortBy->getFindByNamingServiceName (), 11)) {
                    findbyname.append((_findUsesPortBy->getFindByNamingServiceName ()+12));
                } else {
                    findbyname.append(_findUsesPortBy->getFindByNamingServiceName ());
                }
                try {
                    _usesObj = orb->get_object_from_name (findbyname.c_str());
                } catch (CosNaming::NamingContext::NotFound) {
                    string my_component_name = findbyname;
                    size_t location_first_slash = my_component_name.find('/', 0);
                    size_t location_second_slash = my_component_name.find('/', location_first_slash+1);
                    findbyname = "DomainName1/";
                    findbyname.append(my_component_name, location_second_slash+1, \
                                      my_component_name.length()-location_second_slash-1);
                    DEBUG(4, AppFact, "The findname that I'm using is: " << findbyname);
                }


                    _usesObj = orb->get_object_from_name (findbyname.c_str());

            }
        } else if (_usesPortParser->isComponentInstantiationRef()) {
            for (unsigned int i=0; i < requiredComponents.size(); i++) {
                ossieSupport::ComponentInfo *component = requiredComponents[i];
                if (strcmp(component->getIdentifier(), _usesPortParser->getComponentInstantiationRefID()) == 0) {
                    _usesObj = component->getResourcePtr();
                    break;
                }
                if (i == requiredComponents.size()) {
                    std::cerr << "[ApplicationFactory::create] Uses port component not found" << std::endl;
                    throw CF::ApplicationFactory::CreateApplicationError();
                    ///\todo throw exception
                }
            }
        } else {
            std::cerr << "[ApplicationFactory::create] Did not find method to get uses port" << std::endl;
            throw CF::ApplicationFactory::CreateApplicationError();
            ///\todo throw exception
        }

        /**************************************/
        /*                                    */
        /* Connection Establishment Rewritten */
        /*                                    */
        /**************************************/

        // CORBA object reference
        CORBA::Object_var uses_port_ref;

        // Output Uses Port Name
        const char* portName = _usesPortParser->getID();
        DEBUG(3, AppFact, "Getting Uses Port " << portName);

        // Get Uses Port
        CF::Resource_var _usesComp;

            _usesComp = CF::Resource::_narrow(_usesObj);



            uses_port_ref = _usesComp->getPort (_usesPortParser->getID());


        DEBUG(3, AppFact, "back from getport");
        CF::Port_ptr usesPort;

            usesPort = CF::Port::_narrow (uses_port_ref);

        DEBUG(3, AppFact, "result from getport narrowed");

        if (CORBA::is_nil (usesPort)) {
            std::cerr << "[ApplicationFactory::create] getPort returned nil reference for uses port ID " << portName << std::endl;
            throw CF::ApplicationFactory::CreateApplicationError();
        }

        // Output Provides Port Name
        DEBUG(3, AppFact, "Done with uses port");
        DEBUG(3, AppFact, "Getting Provides Port ");

        // Get Provides Port
        // Note: returned object reference is NOT narrowed to a CF::Port
        if (!connection->isFindBy ()) {
            CF::Resource_ptr _providesResource;
            DEBUG(3, AppFact, "Narrowing provides resource");

                _providesResource = CF::Resource::_narrow (_providesObj);

            DEBUG(3, AppFact, "Getting provides port with id - " << _providesPortParser->getID());

                provides_port_ref = _providesResource->getPort (_providesPortParser->getID());

        }

        if (CORBA::is_nil (provides_port_ref)) {
            std::cerr << "[ApplicationFactory::create] getPort returned nil or non-port reference for provides port ID " << _providesPortParser->getID() << std::endl;
            throw CF::ApplicationFactory::CreateApplicationError();
        }

        // Output ConnectionID
        DEBUG(3, AppFact, "Creating Connection " << connection->getID());

        // Create connection
        try {
            usesPort->connectPort (provides_port_ref, connection->getID());
        } catch ( ... ) {
            std::cout << "[ApplicationFactory::create] \"usesPort->connectPort\" failed with Unknown Exception for connection ID " << connection->getID() << "\n";
            exit(EXIT_FAILURE);
        }

        connectionData.push_back(new ossieSupport::ConnectionInfo(usesPort, connection->getID()));
    }

// Check to make sure _assemblyController was initialized
    if (_assemblyController == NULL) {
        std::cerr << "[ApplicationFactory_impl::create] assembly controller was never initialized" << std::endl;

        // Throw exception
        CF::ApplicationFactory::CreateApplicationError create_app_error;
        create_app_error.msg = "assembly controller was never initialized";
        create_app_error.errorNumber = CF::CFNOTSET;
        throw create_app_error;
    }

// We are assuming that all components and their resources are collocated.
// This means that we assume the SAD <partitioning> element contains the
// <hostcollocation> element.
    Application_impl* _application = new Application_impl (_identifier.c_str(), name, _softwareProfile.c_str(), \
            _availableDevs, _implSeq, _namingCtxSeq, _pidSeq, connectionData, \
            CF::Resource::_duplicate (_assemblyController), appseq);

    char* _appid = _application->identifier();
    char* _appname = _application->name();
/// \todo Pass object ref for application when application servant/ref stuff sorted out
    ossieSupport::sendObjAdded_event(_identifier.c_str(), _appid, _appname, \
                                     (CORBA::Object_var) NULL, StandardEvent::APPLICATION);

    CORBA::string_free( _appid );
    CORBA::string_free( _appname );

// Add a reference to the new application to the ApplicationSequence in DomainManager
    int old_length = appseq->length();
    appseq->length(old_length+1);
    (*appseq)[old_length] = CF::Application::_duplicate (_application->_this ());

    return CF::Application::_duplicate (_application->_this ());
}
; /* END ApplicationFactory_impl::create() */


// Verify each component from the SAD exits in the device assignment sequence
void ApplicationFactory_impl::verifyDeviceAssignments (const CF::DeviceAssignmentSequence& _deviceAssignments)
{
    DEBUG(2, AppFact, "Entering verifyDeviceAssignments");
    CF::DeviceAssignmentSequence badAssignments;
    badAssignments.length(requiredComponents.size());
    unsigned int notFound = 0;

    DEBUG(4, AppFact, "No. of required components: " << requiredComponents.size());
    for (unsigned int i = 0; i < requiredComponents.size (); i++) {
        bool found = false;

        for (unsigned int j = 0; j < _deviceAssignments.length (); j++) {
            DEBUG(4, AppFact, "ComponentID - " << _deviceAssignments[j].componentId << "   Component - " \
                  << requiredComponents[i]->getIdentifier());

            if (strcmp (_deviceAssignments[j].componentId, requiredComponents[i]->getIdentifier()) == 0) {
                found = true;
                requiredComponents[i]->setAssignedDeviceId(_deviceAssignments[j].assignedDeviceId);
                break;
            }
        }

        if (!found) {
            badAssignments[notFound++].componentId = CORBA::string_dup(requiredComponents[i]->getName());
        }
    }

    if (notFound > 0) {
        std::cerr << "Device Assignment Sequence does not validate against the .sad file" << std::endl;
        throw CF::ApplicationFactory::CreateApplicationRequestError(badAssignments);
    }
    DEBUG(2, AppFact, "Leaving verifyDeviceAssignents");
}

void ApplicationFactory_impl::getRequiredComponents(const char * incomingNamingContext, const CF::DeviceAssignmentSequence& _deviceAssignments)
{
    DEBUG(4, AppFact, "in getRequiredComponents.");

    std::vector <SADComponentPlacement*> *componentsFromSAD = _sadParser->getComponents();
    std::vector <SPDImplementation*> _devspdimpls;

    const char *assemblyControllerRefId = _sadParser->getAssemblyControllerRefId();
// get all device implementations deployed by each DevMgr
    getDeviceImplementations(_devspdimpls, _deviceAssignments);

    for (unsigned int i = 0; i < componentsFromSAD->size(); i++) {
        SADComponentPlacement *component = (*componentsFromSAD)[i];
        ossieSupport::ComponentInfo *newComponent = new ossieSupport::ComponentInfo();

        // Extract required data from SPD file
        CF::File_var _spd, _scd;

            _spd = fileMgr->open( _sadParser->getSPDById( component->getFileRefId() ), true );


        SPDParser spd( _spd );

            _spd->close();


        // match component implementations in SPD file to the current device
        matchDeviceToComponent(&spd, _devspdimpls, newComponent);

        std::vector <std::string> pathToPRF; // vector to hold all PRF paths
        pathToPRF.push_back(spd.getPRFFile()); // get generic PRF for component

        // get implementation specific PRF
        if (strlen(newComponent->getImplPRFFile()) > 0) {
            DEBUG(7, AppFact, "Found component implementation PRF: " << newComponent->getImplPRFFile());
            pathToPRF.push_back(newComponent->getImplPRFFile());
        }


            _scd = fileMgr->open( spd.getSCDFile(), true );


        SCDParser scd( _scd );

            _scd->close();


        // set new component properties from SPD
        newComponent->setIsResource(scd.isResource());
        newComponent->setIsConfigurable(scd.isConfigurable());

        for (unsigned int k = 0; k < pathToPRF.size(); k++) {

            // Extract Properties
            CF::File_var _prf;

                DEBUG(7, AppFact, "opening PRF: " << pathToPRF[k]);
                _prf = fileMgr->open( pathToPRF[k].c_str(), true );


            PRFParser prf( _prf );

                _prf->close();


            std::vector <PRFProperty *> *prop = prf.getFactoryParamProperties();

            for (unsigned int i=0; i < prop->size(); i++) {
                DEBUG(8, AppFact, "adding factory parameter: " << (*prop)[i]->getName() );
                newComponent->addFactoryParameter((*prop)[i]->getDataType());
            }

            prop = prf.getExecParamProperties();

            for (unsigned int i=0; i < prop->size(); i++) {
                DEBUG(8, AppFact, "adding exec parameter: " << (*prop)[i]->getName() );
                newComponent->addExecParameter((*prop)[i]->getDataType());
            }

            prop = prf.getMatchingProperties();

            for (unsigned int i=0; i < prop->size(); i++) {
                DEBUG(8, AppFact, "adding allocation capacity: " << (*prop)[i]->getName() );
                newComponent->addAllocationCapacity((*prop)[i]->getDataType());
            }

            prop = prf.getConfigureProperties();

            for (unsigned int i=0; i < prop->size(); i++) {
                DEBUG(8, AppFact, "adding configure property: " << (*prop)[i]->getName() );
                newComponent->addConfigureProperty((*prop)[i]->getDataType());
            }

            prop = prf.getCapacityProperties();

            for (unsigned int i=0; i < prop->size(); i++) {
                DEBUG(8, AppFact, "adding allocation property: " << (*prop)[i]->getName() );
                newComponent->addAllocationCapacity((*prop)[i]->getDataType());
            }
        }

        // Extract Instantiation data from SAD
        // This is wrong, there can be more than one instantiation per placement
        // Basic fix, iterate over instantiations
        ///\todo Fix for multiple instantiations per component
        vector <SADComponentInstantiation *> *instantiations = component->getSADInstantiations();

        SADComponentInstantiation *instance = (*instantiations)[0];

        newComponent->setIdentifier(instance->getID());

        if (strcmp(newComponent->getIdentifier(), assemblyControllerRefId) == 0) {
            newComponent->setIsAssemblyController(true);
        }

        newComponent->setNamingService(instance->isNamingService());

        if (newComponent->getNamingService()) {
            string initial_name(incomingNamingContext);
            initial_name.append("/");
            // this initial string compare is here for legacy reasons
            if (!strncmp("DomainName1", instance->getFindByNamingServiceName(), 11)) {
                initial_name.append((instance->getFindByNamingServiceName()+12));
            } else {
                initial_name.append(instance->getFindByNamingServiceName());
            }

            //newComponent->setNamingServiceName(instance->getFindByNamingServiceName());
            newComponent->setNamingServiceName(initial_name.c_str());
        }

        newComponent->setUsageName(instance->getUsageName());
        std::vector <InstantiationProperty *> *ins_prop = instance->getProperties();

        for (unsigned int i = 0; i < ins_prop->size(); ++i) {
            DEBUG(3, AppFact, "Instantiation property id = " << (*ins_prop)[i]->getID());
            vector <string> ins_values = (*ins_prop)[i]->getValues();
            newComponent->overrideProperty((*ins_prop)[i]->getID(), (*ins_prop)[i]->getValues());
        }

        requiredComponents.push_back(newComponent);
    }

    DEBUG(7, AppFact, "leaving getRequiredComponents.");
}


void ApplicationFactory_impl::getRegisteredDevices ()
{
    DEBUG(4, AppFact, "in getRegisteredDevices");

    CF::DomainManager::DeviceManagerSequence* devMgrs;

        devMgrs = dmnMgr->deviceManagers ();


    for (unsigned int i=0; i<devMgrs->length(); i++) {
        CF::DeviceSequence* devices;

            devices = (*devMgrs)[i]->registeredDevices();


        for (unsigned int j=0; j<devices->length(); j++) {
            char* _devlabel = NULL;

                _devlabel = (*devices)[j]->label();

            DEBUG(4, AppFact, "storing device " << _devlabel);
            registeredDevices.push_back((*devices)[j]);
            CORBA::string_free( _devlabel );
        }
    }
}

void ApplicationFactory_impl::getDeviceImplementations( std::vector<SPDImplementation*>& _devspdimpls, const CF::DeviceAssignmentSequence& _deviceAssignments )
{
    DEBUG(2, AppFact, "Entering getDeviceImplementations");
    CF::DomainManager::DeviceManagerSequence* _devmgrseq;

        _devmgrseq = dmnMgr->deviceManagers();

    std::vector <SPDImplementation*>* _devspdimpl;
    std::string _devimplid;
//char* _olddevimplid = NULL;
    bool found = false;
    for (unsigned int j = 0; j < _deviceAssignments.length(); j++) {
        found = false;
        for (unsigned int k = 0; k < _devmgrseq->length(); k++) {
            DEBUG(4, AppFact, "getting device implementation id from the DeviceManager");

                _devimplid = (*_devmgrseq)[k]->getComponentImplementationId(_deviceAssignments[j].assignedDeviceId);


            if (!_devimplid.empty()) {
                found = true;
                DEBUG(4, AppFact, "found the device implementation id: " << _devimplid);
                // if the device implementation has already been searched, don't search it again
                //if( strcmp( _olddevimplid, _devimplid) == 0 ) break;
                //else _olddevimplid = _devimplid;
                DEBUG(4, AppFact, "searching registered devices");
                for (unsigned int i = 0; i < registeredDevices.size(); i++) {
                    std::string _regdevid;

                        _regdevid = registeredDevices[i]->identifier();


                    if (strcmp(_deviceAssignments[j].assignedDeviceId, _regdevid.c_str()) == 0) {
                        std::string _regdevsoftprf;

                            _regdevsoftprf = registeredDevices[i]->softwareProfile();


                        DEBUG(4, AppFact, "opening device SPD to obtain additional implementation info");
                        CF::File_var _devspdfile;

                            _devspdfile = fileMgr->open(_regdevsoftprf.c_str(), true);


                        SPDParser _devspd(_devspdfile);

                            _devspdfile->close();

                        _devspdimpl = _devspd.getImplementations();
                        DEBUG(4, AppFact, "storing the matching device implementation");
                        for (unsigned int m = 0; m < _devspdimpl->size(); m++) {
                            if ( strcmp(_devimplid.c_str(), (*_devspdimpl)[m]->getID()) == 0 )
                                _devspdimpls.push_back((*_devspdimpl)[m]);
                        } // end for m
                    } // fi strcmp
                } // end for i
            } // fi !empty
            if (found) break; // NOTE: we break here because we found the device manager that hosts the device
        } // end for k
        if (!found) {
            DEBUG(1, AppFact, "Bad Device Assignment Identifier: see assignedDeviceId");
            CF::DeviceAssignmentSequence badassignment;
            badassignment.length(1);
            badassignment[0].assignedDeviceId = CORBA::string_dup(_deviceAssignments[j].assignedDeviceId);
            throw CF::ApplicationFactory::CreateApplicationRequestError(badassignment);
        } // fi !found
    } // end for j
    DEBUG(2, AppFact, "Leaving getDeviceImplementations");
}

void ApplicationFactory_impl::matchDeviceToComponent(SPDParser*  spd,
        const std::vector<SPDImplementation*>& _devspdimpls, ossieSupport::ComponentInfo *comp)
{
    DEBUG(4, AppFact, "in matchDeviceToComponent");

// Go through devices, and search through the current component
// SPD implementations for matching impls. Once a matching implementation
// has been found, set the appropriate parameters on the component
// to be executed

// this code is mirrored in DeviceManager_impl.cpp device manager
// implementatio and device matching scheme. when either of the
// matching routines change, the other needs to be changed as well.

// store implementation info from SPD file
    std::vector <SPDImplementation*> *compImpl = spd->getImplementations();

// check all device implementations
    bool implFound = false; // flag to determine if an implementation has been found
    unsigned int m = 0;
    while (!implFound && m < _devspdimpls.size()) {
        std::string deviceID = _devspdimpls[m]->getID();

        // search compImpl for matching implementation details from device,
        // stop once the first matching implementation is found
        // (this checking section will need to be expanded if other
        // options need to be verified: compiler, etc.). first check
        // the <dependency> tag to deploy the component to the FPGA
        // if an implementation is available. if not, match it to the
        // GPP if that implementation is possible. if that cannot be
        // found, error out.

        // (it is currently assumed in SPDImplementation.h,
        // DeviceManager.cpp, and other places that there is only
        // a single OS. this is not compliant and needs to be fixed.)

        DEBUG(4, AppFact, "Matching device implementation to candidate components.");

        unsigned int i = 0; // counter variable for looping through components

        // flip through all component implementations
        while (!implFound && i < compImpl->size()) {
            DEBUG(7, AppFact, "matching Device " << deviceID << " to Component "
                  << (*compImpl)[i]->getID());

            // loop through dependencies
            unsigned int d = 0;
            std::vector <SPDDependency*> compDep = (*compImpl)[i]->getDependencies();
            while (!implFound && d < compDep.size()) {
                std::string compDepID = compDep[d]->getSoftPkgRef();
                if (compDepID == deviceID) {
                    DEBUG(7, AppFact, "found device dependency");

                    comp->setName(spd->getSoftPkgName());
                    comp->setImplId((*compImpl)[i]->getID());
                    comp->setCodeType((*compImpl)[i]->getCodeType());
                    comp->setLocalFileName((*compImpl)[i]->getCodeFile());
                    comp->setEntryPoint((*compImpl)[i]->getEntryPoint());

                    // check prf within implementation tag
                    if ((*compImpl)[i]->getPRFFile()) {
                        DEBUG(7, AppFact, "getting path to implementation specific PRF");
                        comp->setImplPRFFile((*compImpl)[i]->getPRFFile()->getLocalFile().c_str());
                    } else {
                        std::cout << "[ApplicationFactory::matchDeviceToComponent] The matched component has a device dependency, but does not reference a propertyfile. Allocation properties need to be added so a true allocateCapacity() call can be made on the assigned device.\n";
                        throw CF::ApplicationFactory::CreateApplicationError();
                    }

                    implFound = true;
                }
                d++;
            }
            i++;
        }

        unsigned int k = 0;
        while ( !implFound && (k < compImpl->size()) ) {
            DEBUG(4, AppFact, "Attempting to match Component ID: " << (*compImpl)[k]->getID()
                  << " with GPP implementation");

            // verify that the operating system name and version match
            bool osmatch = matchOperatingSystem((*compImpl)[k], _devspdimpls[m]);
            if (osmatch) {

                // verify that the processors are compatible
                bool matchproc = matchProcessor((*compImpl)[k], _devspdimpls[m]);
                if (matchproc) {
                    // all parameters match
                    implFound = true;

                    // set parameters in component to be executed
                    comp->setName(spd->getSoftPkgName());
                    comp->setImplId((*compImpl)[k]->getID());
                    comp->setCodeType((*compImpl)[k]->getCodeType());
                    comp->setLocalFileName((*compImpl)[k]->getCodeFile());
                    comp->setEntryPoint((*compImpl)[k]->getEntryPoint());

                    // check properyfile within implementation
                    if ((*compImpl)[k]->getPRFFile()) {
                        DEBUG(7, AppFact, "getting path to implementation specific PRF");
                        comp->setImplPRFFile((*compImpl)[k]->getPRFFile()->getLocalFile().c_str());
                    } else {
                        DEBUG(7, AppFact, "no implementation specific PRF");
                    }


                }
            }
            k++;
        }
        m++;
    }

// if a component implementation cannot be matched to the device, error out
    if (!implFound) {
        std::cout <<  "[ApplicationFactory::matchDeviceToComponent] Could not match an implementation of " << spd->getSoftPkgName() << " to the current device\n";
        exit(EXIT_FAILURE);
    }
}

bool ApplicationFactory_impl::matchOperatingSystem(SPDImplementation *compImpl, SPDImplementation *devImpl)
{
// match the operating system name and version from the
// device to the component

    DEBUG(4, AppFact, "in matchOperatingSystem.");

// component OS
    std::string compImplOSName = compImpl->getOperatingSystem().getOSName();
    std::string compImplOSVersion = compImpl->getOperatingSystem().getOSVersion();

// device OS
    OSAttributes osattr = devImpl->getOperatingSystem();
    std::string devOSName = osattr.getOSName();
    std::string devOSVersion = osattr.getOSVersion();

// perform matching
    if (compImplOSName == devOSName) {
        DEBUG(7, AppFact, "Found matching Operating System Name");

        if (compImplOSVersion == devOSVersion) {
            DEBUG(7, AppFact, "Found matching Operating System Version");
            return true;
        } else {
            DEBUG(7, AppFact, "Could not match Operating System Version");
            return false;
        }
    } else {
        DEBUG(7, AppFact, "Could not match Operating System name");
        return false;
    }

// catch all
    DEBUG(7, AppFact, "match OS logic failed somehow, this should not happen");
    exit(EXIT_FAILURE);
}

bool ApplicationFactory_impl::matchProcessor(SPDImplementation *compImpl, SPDImplementation *devImpl)
{
// check all of the compatible processors in the component impl
// against the processor in the device

// (realistically the algorithm needs to check against the single
// processor that the component will need to deploy to)

    DEBUG(4, AppFact, "in matchProcessor");

    unsigned int k = 0;
    unsigned int p = 0;
    std::vector <std::string> compatibleProcs = compImpl->getProcessors();
    std::vector<std::string> proclist = devImpl->getProcessors();

    while ( k < compatibleProcs.size() ) {
        while ( p < proclist.size() ) {
            if (compatibleProcs[k] == proclist[p]) {
                DEBUG(4, AppFact, "Found processor match.");
                return true;
            }
        }
    }

    DEBUG(4, AppFact, "Could not match processor.");
    return false;

}

CF::Device_ptr ApplicationFactory_impl::find_device_from_id(const char *device_id)
{
    for (unsigned int i=0; i<registeredDevices.size(); i++) {
        char* _regdevid = NULL;

            _regdevid = registeredDevices[i]->identifier();

        if (strcmp(_regdevid, device_id) == 0) {
            CORBA::string_free( _regdevid );
            return CF::Device::_narrow(registeredDevices[i]); // NOTE: This is DANGEROUS!!
        }
        CORBA::string_free( _regdevid );
    }

    std::cerr << "Device not found, this should never happen. (Verify that nodeBooter has started the correct node.)" << std::endl;
    exit(EXIT_FAILURE);

    return 0;
}

void ApplicationFactory_impl::loadAndExecuteComponents(PROC_ID_SEQ* pid)
{

    DEBUG(2, AppFact, "requiredComponents size - " << requiredComponents.size());

    for (unsigned int rc_idx = 0; rc_idx < requiredComponents.size (); rc_idx++) {
        ossieSupport::ComponentInfo *component = requiredComponents[rc_idx];

        DEBUG(1, AppFact, "Component - " << component->getName() << "   Assigned device - " << component->getAssignedDeviceId());

        CF::Device_ptr device;

            device = find_device_from_id(component->getAssignedDeviceId());


        DEBUG(1, AppFact, "Host is " << device->label () << " Local file name is " << component->getLocalFileName());

        // Now we have a pointer to the required device
        // Get allocation properties

            device->allocateCapacity (component->getAllocationCapacities());



        // now that capacity has been allocated, need to execute the component.
        // check devices until an executable device has been found, set up
        // the pointer to the device, and then execute. this assumes that
        // only a single GPP is being used.


        CF::LoadableDevice_ptr loadabledev;

            loadabledev = CF::LoadableDevice::_narrow(device);

        if (CORBA::is_nil(loadabledev)) {
            DEBUG(7, AppFact, "component assigned to Device, searching for Loadable device");

            unsigned int l = 0;
            bool loadFound = false;
            // loop through and find a loadable device
            while (!loadFound && l < registeredDevices.size()) {

                    loadabledev = CF::LoadableDevice::_narrow(registeredDevices[l]);

                if (!CORBA::is_nil(loadabledev)) {
                    DEBUG(4, AppFact, "found a Loadable device");
                    loadFound = true;
                }
            }

            // can't find loadable device, error out
            if (!loadFound) {
                std::cout << "[ApplicationFactory::loadAndExecuteComponents] Could not find a loadable device in the system.\n";
                throw CF::ApplicationFactory::CreateApplicationError();
            }
        }

        // Get file name, load if it is not empty
        if (strlen (component->getLocalFileName()) >  0) {
            /// \TODO Validate that the intended device supports the LoadableDevice interface

            DEBUG(1, AppFact, "loading " << component->getLocalFileName());

            int idx = 0;

            CF::FileManager_var fileMgr;

                fileMgr = dmnMgr->fileMgr();


            CF::FileManager::MountSequence_var mts;

                mts = fileMgr->getMounts();


            for ( unsigned int i = 0; i < mts->length(); i++ ) {
                try {
                    if ( mts[i].fs->exists( component->getLocalFileName() ) )
                        idx = i;
                } catch ( CF::InvalidFileName &_ex ) {
                    std::cout << "[ApplicationFactory::create] Searching for FileSys containing " << component->getLocalFileName() << "\n";
                } catch ( CF::FileException &_ex ) {
                    std::cout << "[ApplicationFactory::create] Searching for FileSys containing " << component->getLocalFileName() << "\n";
                } catch ( ... ) {
                }
            }

            std::string binaryFileName = CORBA::string_dup( mts[idx].mountPoint );
            binaryFileName += component->getLocalFileName();


                loadabledev->load (mts[idx].fs, /*component->getLocalFileName()*/ binaryFileName.c_str(), component->getCodeType());


            // execute the component
            CF::ExecutableDevice_ptr execdev;

                execdev = CF::ExecutableDevice::_narrow(loadabledev);

            if (CORBA::is_nil(execdev)) {
                DEBUG(7, AppFact, "component is assigned to device that is only Loadable, searching for Executable");

                unsigned int l = 0;
                bool execFound = false;
                // loop through and find a executable device
                while (!execFound && l < registeredDevices.size()) {

                        execdev = CF::ExecutableDevice::_narrow(registeredDevices[l]);

                    if (!CORBA::is_nil(execdev)) {
                        DEBUG(7, AppFact, "found an executable device");
                        execFound = true;
                    }
                }

                // can't find executable device, error out
                if (!execFound) {
                    std::cout << "[ApplicationFactory::loadAndExecuteComponents] Could not find a executable device in the system.\n";
                    throw CF::ApplicationFactory::CreateApplicationError();
                }
            }

            // Execute when necesary
            if ((component->getCodeType() == CF::LoadableDevice::EXECUTABLE) || (component->getCodeType() == CF::LoadableDevice::SHARED_LIBRARY) && (strcmp (component->getEntryPoint(), "") != 0)) {
                /// \TODO: Validate that the intended device supports the LoadableDevice interface

                DEBUG(1, AppFact, "executing-> " << binaryFileName /*component->getLocalFileName()*/);
#ifdef HAVE_LEGACY
                CF::DataType dt;
                dt.id = component->getIdentifier();

                string initial_name("");

                if (!strncmp("DomainName1", component->getNamingServiceName(), 11)) {
                    initial_name.append((component->getNamingServiceName()+12));
                } else {
                    initial_name.append(component->getNamingServiceName());
                }

                dt.value <<= component->getNamingServiceName();

                component->addExecParameter(&dt);
#else
                CF::DataType namebinding;
                namebinding.id = CORBA::string_dup("NAME_BINDING");
                namebinding.value <<= component->getNamingServiceName();

                component->addExecParameter(&namebinding);

                CF::DataType componentid;
                componentid.id = CORBA::string_dup("COMPONENT_IDENTIFIER");
                componentid.value <<= component->getIdentifier();

                component->addExecParameter(&componentid);

                CF::DataType namingctxior;
                namingctxior.id = CORBA::string_dup("NAMING_CONTEXT_IOR");
                namingctxior.value <<= ossieSupport::ORB::orb->object_to_string(ossieSupport::ORB::inc);

                component->addExecParameter(&namingctxior);
#endif
                CF::ExecutableDevice::ProcessID_Type tempPid;


                    tempPid = execdev->execute (/*component->getLocalFileName()*/ binaryFileName.c_str(), component->getOptions(), component->getExecParameters());


                if (tempPid < 0) {
                    /// \TODO throw exception here
                } else {
                    pid->length (pid->length() + 1);
                    (* pid)[pid->length() - 1].processId = tempPid;
                    (* pid)[pid->length() - 1].componentId =
                        CORBA::string_dup(component->getIdentifier());
                }
                // NOTE: The PID returned by execute is not handled
                // An exception shall be thrown when PID = -1
            }
        }
    }
}

#ifdef HAVE_OMNIEVENTS
ApplicationFactoryEventHandler::ApplicationFactoryEventHandler (ApplicationFactory_impl * _appFac)
{
    appFactory = _appFac;
}


voidApplicationFactoryEventHandler::push (const CORBA::Any & _any)
throw (CORBA::SystemException, CosEventComm::Disconnected)
{}


void ApplicationFactoryEventHandler::disconnect_push_consumer ()
throw (CORBA::SystemException)
{}
#endif

