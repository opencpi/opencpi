/*******************************************************************************

Copyright 2006,2007, Virginia Polytechnic Institute and State University

This file is the OSSIE Node Booter.

OSSIE nodebooter is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

OSSIE nodebooter is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OSSIE nodebooter; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

nodebooter.cpp

*******************************************************************************/

#include <iostream>
#include <string>
#include <cctype>
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <ossie/debug.h>
#include <ossie/cf.h>
#include <ossie/DomainManager_impl.h>
#include <ossie/DeviceManager_impl.h>
#include <ossie/ossieSupport.h>

using namespace ossieSupport;

/** \mainpage

\section Nodebooter

what it does

\section Installation

How to install

*/

/** \file
    A description of the nodebooter.cpp file
*/


CF::DomainManager_var DomainManager_objref;
CF::DeviceManager_var DeviceManager_objref;

int startDeviceManager = 0;
int startDomainManager = 0;

void usage()
{
    std::cout << "nodeBooter -D <optional dmd file> -d ddd file <optional flags>" << std::endl;
    std::cout << "Example: nodeBooter -d DeviceManager.dcd.xml" << std::endl;
    std::cout << "Example: nodeBooter -D -d DeviceManager.dcd.xml" << std::endl;
    std::cout << "Example: nodeBooter -D DomainManager.dmd.xml -d DeviceManager.dcd.xml" << std::endl;
    std::cout << "Example: nodeBooter -D -d DeviceManager.dcd.xml -debug 9" << std::endl;
    std::cout << "Example: nodeBooter -D -d DeviceManager.dcd.xml -ORBInitRef NameService=corbaname::<IP>" << std::endl;
    std::cout << "Example: nodeBooter --help" << std::endl;
}

// System Signal Interrupt Handler will allow proper ORB shutdown
void signal_catcher( int sig )
{
    if ( sig == SIGINT ) {
        std::cout << "[nodeBooter] Shutting down ..." << std::endl;
        if ( startDeviceManager ) DeviceManager_objref->shutdown();
        if ( startDomainManager ) CORBA::release(DomainManager_objref);
        ossieSupport::ORB::orb->shutdown(0);
//  exit(EXIT_SUCCESS);
    }

    if ( sig == SIGQUIT ) {
        std::cout << "SIGQUIT caught" << std::endl;
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[])
{
// Set debug verbosity 0 - no debug messages 3 for typical noisy level
    ossieDebugLevel = 0;

    DomainManager_impl *DomainManager_servant = NULL;
    DeviceManager_impl *DeviceManager_servant = NULL;

// parse command line options
    string *dmdFile = NULL;
    string *dmdRoot = NULL;
    string *dcdFile = NULL;
    string *dcdRoot = NULL;

    for ( unsigned int i = 1; i < argc; i++ ) {
        if ( strcmp( argv[i], "-D" ) == 0 ) {
            if ( i+1 >= argc )
                dmdFile = new string("dom/domain/DomainManager.dmd.xml");
            else {
                string tmpdmdfile = argv[i+1];
                if ( tmpdmdfile.find(".dmd.xml") == string::npos )
                    dmdFile = new string("dom/domain/DomainManager.dmd.xml");
                else
                    dmdFile = new string(tmpdmdfile);
            }
            startDomainManager = 1;
        }

        if ( strcmp( argv[i], "-d" ) == 0 ) {
            string tmpdcdfile = argv[i+1];
            if ( tmpdcdfile.find_first_of(".dcd.xml") != string::npos )
                dcdFile = new string(tmpdcdfile);
            else {
                std::cout << "[nodeBooter] ERROR: Illegal or no DCD profile given\n";
                usage();
                exit(EXIT_FAILURE);
            }
            startDeviceManager = 1;
        }

        if ( strcmp( argv[i], "-debug" ) == 0 ) {
            if ( isdigit( *argv[i+1] ) ) {
                ossieDebugLevel = atoi( argv[i+1] );

                if ( ossieDebugLevel < 0 || ossieDebugLevel > 9 ) {
                    std::cout << "[nodeBooter] ERROR: Debug Level must be 0 - 9\n";
                    exit(EXIT_FAILURE);
                }
            } else {
                std::cout << "[nodeBooter] ERROR: Illegal or no Debug Level given\n";
                usage();
                exit(EXIT_FAILURE);
            }
        }

        if ( strcmp( argv[i], "--help" ) == 0 ) {
            usage();
            exit(EXIT_SUCCESS);
        }
    }


// Create signal handler to catch system interrupts SIGINT and SIGQUIT

    struct sigaction sa;
    sa.sa_handler = signal_catcher;
    sa.sa_flags = 0;

// Associate SIGINT to signal_catcher interrupt handler

    if ( sigaction( SIGINT, &sa, NULL ) == -1 ) {
        perror("SIGINT");
        exit(EXIT_FAILURE);
    }

// Associate SIGQUIT to signal_catcher interrupt handler

    if ( sigaction( SIGQUIT, &sa, NULL ) == -1 ) {
        perror("SIGQUIT");
        exit(EXIT_FAILURE);
    }

// Start CORBA
    ORB *orb_obj = new ORB(argc, argv);

// Check that there is work to do
    if (!(startDeviceManager || startDomainManager)) {
        usage();
        exit (0);
    }

// Start Domain Manager if requested

    if (startDomainManager) {
        DEBUG(1, NB, "Starting Domain Manager");

        // Create naming context for Domain. Must be done before servant
        // instantiation so the event channels can find their context
        ///\todo Figure out how to make DomainName1 run time configurable
        ///\todo and name context stuff to ORB class

        CosNaming::Name_var base_context = orb_obj->string_to_CosName("DomainName1");
        CosNaming::NamingContext_var rootContext;

        try { ///\todo review this code and see what alternative solutions exist
            orb_obj->inc->bind_new_context (base_context);
        } catch (CosNaming::NamingContext::AlreadyBound &) {
            rootContext = CosNaming::NamingContext::_narrow(orb_obj->inc->resolve(base_context));
            orb_obj->unbind_all_from_context(rootContext);
        } catch ( CORBA::SystemException& se ) {
            std::cout << "[nodeBooter] \"bind_new_context\" failed with CORBA::SystemException\n";
            exit(EXIT_FAILURE);
        } catch ( CORBA::UserException& se ) {
            std::cout << "[nodeBooter] \"bind_new_context\" failed with CORBA::UserException\n";
            exit(EXIT_FAILURE);
        } catch ( ... ) {
            std::cout << "[nodeBooter] \"bind_new_context\" failed with Unknown Exception\n";
            exit(EXIT_FAILURE);
        }

        // Create Domain Manager servant and object

        int idx = dmdFile->find_first_of('/');
        dmdRoot = new string( dmdFile->substr(0, idx+1) );
        std::cout <<  "Root of DomainManager FileSystem set to " << dmdRoot->c_str() << "\n";
        try {
            DomainManager_servant = new DomainManager_impl(dmdFile->substr(idx, dmdFile->length()).c_str(), dmdRoot->c_str());
        } catch ( CORBA::UserException& se ) {
            std::cout << "[nodeBooter] \"DomainManager_impl\" failed with CORBA::UserException\n";
            exit(EXIT_FAILURE);
        } catch ( ... ) {
            std::cout << "[nodeBooter] \"DomainManager_impl\" failed with Unknown Exception\n";
            exit(EXIT_FAILURE);
        }

        DomainManager_objref = DomainManager_servant->_this();
        try {
            orb_obj->bind_object_to_name((CORBA::Object_ptr) DomainManager_objref, "DomainName1/DomainManager");
        } catch ( CORBA::SystemException& se ) {
            std::cout << "[nodeBooter] \"bind_object_to_name\" failed with CORBA::SystemException\n";
            exit(EXIT_FAILURE);
        } catch ( CORBA::UserException& se ) {
            std::cout << "[nodeBooter] \"bind_object_to_name\" failed with CORBA::UserException\n";
            exit(EXIT_FAILURE);
        } catch ( ... ) {
            std::cout << "[nodeBooter] \"bind_object_to_name\" failed with Unknown Exception\n";
            exit(EXIT_FAILURE);
        }
    }

// Start Device Manager if requested
    if (startDeviceManager) {
        int idx = dcdFile->find_first_of('/');
        dcdRoot = new string( dcdFile->substr(0, idx+1) );
        DEBUG(1, NB, "Starting Device Manager with " << *dcdFile);

        DeviceManager_servant = new DeviceManager_impl(dcdFile->substr(idx, dcdFile->length()).c_str(), dcdRoot->c_str());
        DeviceManager_objref = DeviceManager_servant->_this();

        // finish initializing the Device Manager

        try {
            DeviceManager_servant->post_constructor(DeviceManager_objref);
        } catch ( CORBA::UserException& se ) {
            std::cout << "[nodeBooter] \"DeviceManager->post_constructor\" failed with CORBA::UserException:\n";
            exit(EXIT_FAILURE);
        } catch ( ... ) {
            std::cout << "[nodeBooter] \"DeviceManager->post_constructor\" failed with Unknown Exception\n";
            exit(EXIT_FAILURE);
        }
    }

    orb_obj->orb->run();
    try {
        orb_obj->orb->destroy();
    } catch (...) {
        std::cout << "[nodeBooter] \"orb->destroy\" threw an Unknown Exception\n";
    }

//CORBA::release(orb_obj->orb);
    delete orb_obj;
    std::cout << "[nodeBooter] Exiting ...\n";
//    exit(EXIT_SUCCESS);
    return 0;
}

