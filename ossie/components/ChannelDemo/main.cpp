/****************************************************************************

Copyright 2006 Virginia Polytechnic Institute and State University

This file is part of the OSSIE ChannelDemo.

OSSIE ChannelDemo is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

OSSIE ChannelDemo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OSSIE ChannelDemo; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/

#include <iostream>

#ifndef HAVE_LEGACY
#include <omniORB4/omniURI.h>
#endif

#include "ossie/ossieSupport.h"
#include "ossie/debug.h"

#include "ChannelDemo.h"

using namespace std;
using namespace standardInterfaces;  // For standard OSSIE interface classes


int main(int argc, char* argv[])
{
    ossieDebugLevel = 0;
#ifdef HAVE_LEGACY
    if (argc != 3) {
        cout << argv[0] << " <id> <usage name> " << endl;
        exit (-1);
    }

    ossieSupport::ORB *orb = new ossieSupport::ORB;

    char *uuid = argv[1];
    char *label = argv[2];

    cout << "Identifier - " << uuid << "  Label - " << label << endl;
#else
    if ( argc < 7 ) {
        cout << argv[0] << " NAMING_CONTEXT_IOR <string> NAME_BINDING <string> COMPONENT_IDENTIFIER <string> " << endl;
        exit(EXIT_FAILURE);
    }

    int i = 0;
    char *uuid = NULL;
    char *label = NULL;
    char *ior = NULL;

    while ( i < argc ) {
        if ( strcmp("NAMING_CONTEXT_IOR", argv[i]) == 0 ) ior = argv[i+1];
        if ( strcmp("NAME_BINDING", argv[i]) == 0 ) label = argv[i+1];
        if ( strcmp("COMPONENT_IDENTIFIER", argv[i]) == 0 ) uuid = argv[i+1];
        ++i;
    }

    if ( uuid == NULL ) {
        cout << "ERROR: " << argv[0] << " missing COMPONENT_IDENTIFIER argument" << endl;
        exit(EXIT_FAILURE);
    }

    if ( label == NULL ) {
        cout << "ERROR: " << argv[0] << " missing NAME_BINDING argument" << endl;
        exit(EXIT_FAILURE);
    }

    if ( ior == NULL ) {
        cout << "ERROR: " << argv[0] << " missing NAMING_CONTEXT_IOR argument" << endl;
        exit(EXIT_FAILURE);
    }

    cout << argv[0] << " NAMING_CONTEXT_IOR " << ior << " NAME_BINDING " << label << " COMPONENT_IDENTIFIER " << uuid << endl;

    ossieSupport::ORB::orb = CORBA::ORB_init( argc, argv );

    try {
        CORBA::Object_ptr _ncobj = ossieSupport::ORB::orb->string_to_object(ior);
        ossieSupport::ORB::inc = CosNaming::NamingContext::_narrow(_ncobj);
        CORBA::release(_ncobj);
    } catch ( ... ) {
        cout << "ERROR: " << argv[0] << " Failed to narrow NamingContext from IOR" << endl;
        exit(EXIT_FAILURE);
    }

    try {
        CORBA::Object_ptr _poaobj = ossieSupport::ORB::orb->resolve_initial_references("RootPOA");
        ossieSupport::ORB::poa = PortableServer::POA::_narrow(_poaobj);
        ossieSupport::ORB::pman = ossieSupport::ORB::poa->the_POAManager();
        ossieSupport::ORB::pman->activate();
    } catch ( ... ) {
        cout << "ERROR: " << argv[0] << " Failed to initialize POA" << endl;
        exit(EXIT_FAILURE);
    }
#endif
    omni_mutex component_running_mutex;
    omni_condition *component_running = new omni_condition(&component_running_mutex);

    ChannelDemo_i* channeldemo_servant;
    CF::Resource_var channeldemo_var;

// Create the channeldemo component servant and object reference

    channeldemo_servant = new ChannelDemo_i(uuid, component_running);
    channeldemo_var = channeldemo_servant->_this();

#ifdef HAVE_LEGACY
    orb->bind_object_to_name((CORBA::Object_ptr) channeldemo_var, label);
#else
    CosNaming::Name_var _bindingname = omni::omniURI::stringToName(label);
    try {
        ossieSupport::ORB::inc->rebind(_bindingname, (CORBA::Object_ptr) channeldemo_var);
    } catch ( ... ) {
        cout << "ERROR: " << argv[0] << " Failed to rebind reference to NameService" << endl;
        exit(EXIT_FAILURE);
    }
#endif

// This bit is ORB specific
// omniorb is threaded and the servants are running at this point
// so we block on the condition
// The releaseObject method clear the condition and the component exits

    component_running->wait();
#ifdef HAVE_LEGACY
    orb->unbind_name(label);
    orb->orb->shutdown(0);
#else
    ossieSupport::ORB::inc->unbind(_bindingname);
    ossieSupport::ORB::orb->destroy();
#endif
    return 0;
}
