/****************************************************************************

Copyright 2005, 2006, 2007 Virginia Polytechnic Institute and State University

This file is part of the OSSIE GPP.

OSSIE GPP is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

OSSIE GPP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OSSIE GPP; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/

#include <iostream>
#include <string>
#include <cstdlib>

#ifndef HAVE_LEGACY
#include <omniORB4/omniURI.h>
#endif

#include "ossie/debug.h"
#include "ossie/ossieSupport.h"

#include "GPP.h"

int main(int argc, char *argv[])

{
    ossieDebugLevel = 3;
#ifdef HAVE_LEGACY
    ossieSupport::ORB* orbsup = new ossieSupport::ORB();

    char *id, *profile, *label;

    if (argc != 4) {
        std::cout << argv[0] << " : <identifier> <name> <SPD Profile>" << std::endl;
        exit(-1);
    }

    id = argv[1];
    label = argv[2];
    profile = argv[3];

    DEBUG(1, GPP, "Identifier = " << id << "Label = " << label << " Profile = " << profile);
#else
    if ( argc < 9 ) {
        std::cout << argv[0] << " DEVICE_MGR_IOR <string> PROFILE_NAME <string> DEVICE_ID <string> DEVICE_LABEL <string> " << std::endl;
        exit(EXIT_FAILURE);
    }

    int i = 0;
    char* devmgrior = NULL;
    char* profile = NULL;
    char* id = NULL;
    char* label = NULL;
    char* ncior = NULL;

    while ( i < argc ) {
        if ( strcmp( "DEVICE_MGR_IOR", argv[i] ) == 0 ) devmgrior = argv[i+1];
        if ( strcmp( "PROFILE_NAME", argv[i] ) == 0 ) profile = argv[i+1];
        if ( strcmp( "DEVICE_ID", argv[i] ) == 0 ) id = argv[i+1];
        if ( strcmp( "DEVICE_LABEL", argv[i] ) == 0 ) label = argv[i+1];
        if ( strcmp( "NAMING_CONTEXT_IOR", argv[i] ) == 0 ) ncior = argv[i+1];
        ++i;
    }

    if ( devmgrior == NULL ) {
        std::cout << "ERROR: " << argv[0] << " missing DEVICE_MGR_IOR argument" << std::endl;
        exit(EXIT_FAILURE);
    }

    if ( profile == NULL ) {
        std::cout << "ERROR: " << argv[0] << " missing PROFILE_NAME argument" << std::endl;
        exit(EXIT_FAILURE);
    }

    if ( id == NULL ) {
        std::cout << "ERROR: " << argv[0] << " missing DEVICE_ID argument" << std::endl;
        exit(EXIT_FAILURE);
    }

    if ( label == NULL ) {
        std::cout << "ERROR: " << argv[0] << " missing DEVICE_LABEL argument" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << argv[0]
              << " DEVICE_MGR_IOR " << devmgrior
              << " PROFILE_NAME " << profile
              << " DEVICE_ID " << id
              << " DEVICE_LABEL " << label;
    if ( ncior != NULL ) std::cout << " NAMING_CONTEXT_IOR " << ncior;
    std::cout << std::endl;

    ossieSupport::ORB::orb = CORBA::ORB_init( argc, argv );

    try {
        CORBA::Object_ptr _ncobj;
        if ( ncior == NULL )
            _ncobj = ossieSupport::ORB::orb->resolve_initial_references("NameService");
        else
            _ncobj = ossieSupport::ORB::orb->string_to_object(ncior);
        ossieSupport::ORB::inc = CosNaming::NamingContext::_narrow(_ncobj);
        CORBA::release(_ncobj);
    } catch ( ... ) {
        std::cout << "ERROR: " << argv[0] << " Failed to narrow NamingContext" << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        CORBA::Object_ptr _poaobj = ossieSupport::ORB::orb->resolve_initial_references("RootPOA");
        ossieSupport::ORB::poa = PortableServer::POA::_narrow(_poaobj);
        ossieSupport::ORB::pman = ossieSupport::ORB::poa->the_POAManager();
        ossieSupport::ORB::pman->activate();
    } catch ( ... ) {
        std::cout << "ERROR: " << argv[0] << " Failed to initialize the POA" << std::endl;
        exit(EXIT_FAILURE);
    }
#endif

// Create Executable device servant and object reference
    GPP_i* GPP_servant;
    CF::ExecutableDevice_var GPP_var;

    GPP_servant = new GPP_i(id, label, profile);
    GPP_var = GPP_servant->_this();

// Add the object to the Naming Service
    std::string objName = "DomainName1/";
    objName += label;
#ifdef HAVE_LEGACY
    orbsup->bind_object_to_name((CORBA::Object_ptr) GPP_var, objName.c_str());
#else
    CosNaming::Name_var _bindingname = omni::omniURI::stringToName(objName.c_str());
    try {
        ossieSupport::ORB::inc->rebind(_bindingname, (CORBA::Object_ptr) GPP_var);
    } catch ( ... ) {
        std::cout << "ERROR: " << argv[0] << " Failed to rebind reference to NameService" << std::endl;
        exit(EXIT_FAILURE);
    }
#endif

// Start handling CORBA requests
#ifdef HAVE_LEGACY
    orbsup->orb->run();
#else
    ossieSupport::ORB::orb->run();
    ossieSupport::ORB::orb->destroy();
#endif
    return 0;
}
