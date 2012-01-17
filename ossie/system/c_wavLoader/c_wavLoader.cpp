
/*******************************************************************************

Copyright 2006, 2007 Virginia Polytechnic Institute and State University

This file is part of the OSSIE  waveform loader.

OSSIE Waveform Loader is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

OSSIE Sample Waveform is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OSSIE Waveform loader; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


*******************************************************************************/

#include <iostream>
#include <string>
#include <cstdlib>

#include <ossie/debug.h>
#include <ossie/cf.h>
#include <ossie/DASParser.h>

static CF::FileManager_ptr _DM_fm;

static void display_menu(int state, CF::FileSystem::FileInformationSequence* files)

{

    std::cout << std::endl;

    switch (state) {
    case 0:   // No installed applications
        for (unsigned int i=0; i<files->length(); i++) {
            std::cout << "  " << i+1 << ":  " << (*files)[i].name << std::endl;
        }
        std::cout << "n - Install application number n" << std::endl;
        std::cout << "x - exit" << std::endl;
        break;

    case 1:   // Application installed and not running
        std::cout << "s - start application" << std::endl;
        std::cout << "u - uninstall application" << std::endl;
        break;

    case 2:   // Application running
        std::cout << "t - stop application" << std::endl;
        break;
    }

    std::cout << "Selection: " << std::flush;
}

static CF::DeviceAssignmentSequence *read_das(const char *name_SAD)
{
    std::string inStr = name_SAD;

    std::string::size_type pos = inStr.find(".sad.xml");
    std::string::size_type begin = inStr.find_last_of("/");

//std::cout << "pos = " << pos <<  " Base name - " << inStr.substr(begin+1,pos-begin-1) << std::endl;

    std::string DAS_name = inStr.substr(0,pos) + "_DAS.xml";
    std::cout << "DAS name: " << DAS_name << std::endl;

    CF::File_var das_file;
    try {
        das_file = _DM_fm->open(DAS_name.c_str(), true);
    } catch ( CF::InvalidFileName &_ex ) {
        std::cout << _ex.errorNumber << " " << _ex.msg << std::endl;
        exit(-1);
    } catch ( CF::FileException &_ex ) {
        std::cout << _ex.errorNumber << " " << _ex.msg << std::endl;
        exit(-1);
    } catch ( ... ) {
        std::cout << "Unknown error while opening DAS file" << std::endl;
        exit(-1);
    }

    DASParser das(das_file);

    try {
        das_file->close();
    } catch ( CORBA::SystemException& se ) {
        std::cout << "[c_wavLoader::read_das] \"das_file->close\" failed with CORBA::SystemException\n";
        exit(EXIT_FAILURE);
    } catch ( ... ) {
        std::cout << "[c_wavLoader::read_das] \"das_file->close\" failed with Unknown Exception\n";
        exit(EXIT_FAILURE);
    }

    return das.das();
}


static CF::Application_ptr install_app (int app_no, CF::FileSystem::FileInformationSequence* files, CF::DomainManager_var domMgr)
{
    CF::Properties _appFacProps( 0 );
    CF::Application_ptr app = CF::Application::_nil();

    std::cout << "In install_app with app_no: " << app_no << std::endl;

// find the application associated with requested SAD

    std::string name_SAD( (*files)[app_no].name );
// NOTE: this is kind of a hack that assumes that the first
// directory in the string is the root, which should be removed.
// This may or may not be permanent.
    size_t found = name_SAD.find_first_of("/");
    name_SAD = name_SAD.substr(found);

// install application in DM")

    try {
        domMgr->installApplication( name_SAD.c_str() );
        std::cout << name_SAD << " successfully installed onto Domain Manager\n";

        //Specify what component should be deployed on particular devices

        CF::DeviceAssignmentSequence *das;

        das = read_das(name_SAD.c_str());

        // local variable for list of Application Factories returned by DM

        CF::DomainManager::ApplicationFactorySequence *_applicationFactories;
        // get the list of available Application Factories from DM
        _applicationFactories = domMgr->applicationFactories();

        // create Application from Application Factory
        std::cout << "App fact seq length = " << _applicationFactories->length() << std::endl;
        std::cout << "_applicationFactories[0]->name()" << std::endl;
        char* _appfactname = (*_applicationFactories)[0]->name();
        std::cout << _appfactname << std::endl;
        CORBA::string_free( _appfactname );
        std::cout << "Calling appfactory->create now" << std::endl;

        app = (*_applicationFactories)[ 0 ] ->create( (*_applicationFactories)[ 0 ] ->name(), _appFacProps, *das );

        std::cout << "Application created.  Ready to run or uninstall\n";

        return(app);
    } catch (CF::DomainManager::ApplicationInstallationError ex) {
        std::cout << "[c_wavLoader::install_app] Install application failed with message: " << ex.msg << std::endl;

        return NULL;
    } catch ( ... ) {
        std::cout << "[c_wavLoader::install_app] Install application failed with Unknown Exception\n";

        return NULL;
    }
}

int main( int argc, char** argv )
{
    ossieDebugLevel = 0;

    for ( int i = 0; i < argc; i++ ) {
        if ( strcmp( "-debug", argv[i] ) == 0 )
            ossieDebugLevel = atoi(argv[i+1]);
    }

    CORBA::ORB_var orb = CORBA::ORB_init( argc, argv );

    CosNaming::NamingContext_var rootContext;

    try {
        CORBA::Object_var obj;
        obj = orb->resolve_initial_references("NameService");

        rootContext = CosNaming::NamingContext::_narrow(obj);

        if (CORBA::is_nil(rootContext)) {
            std::cerr << "Failed to narrow the root naming context." << std::endl;
        }
    } catch (CORBA::ORB::InvalidName & ) {
        std::cerr << "This shouldn't happen" << std::endl;
        exit (EXIT_FAILURE);
    } catch ( ... ) {
        std::cout << "Unknown error while resolving NamingContext" << std::endl;
        exit(EXIT_FAILURE);
    }

    CORBA::Object_ptr obj;
    CosNaming::Name name;

    name.length(2);

// Set the radio context first
    name[0].id = (const char *) "DomainName1";


// Find DomainManager
    name[1].id = (const char *) "DomainManager";
    try {
        obj = rootContext->resolve(name);
    } catch ( ... ) {
        // error message goes here
    }

    CF::DomainManager_var domMgr;

    try {
        domMgr = CF::DomainManager::_narrow(obj);
    } catch ( ... ) {
        std::cout << "[c_wavLoader] \"CF::DomainManager::_narrow\" failed with Unknown Exception\n";
        exit(EXIT_FAILURE);
    }

    try {
        // access to the file manager is needed to figure out what applications are available
        _DM_fm = domMgr->fileMgr();
    } catch ( CORBA::SystemException& se ) {
        std::cout << "[c_wavLoader] \"domMgr->fileMgr\" failed with CORBA::SystemException\n";
        exit(EXIT_FAILURE);
    } catch ( ... ) {
        std::cout << "[c_wavLoader] \"domMgr->fileMgr\" failed with Unknown Exception\n";
        exit(EXIT_FAILURE);
    }

// Generate a list of available waveforms by findind all the _SAD files
    CF::FileSystem::FileInformationSequence* _fileSeq;
    try {
        _fileSeq = _DM_fm->list( "/*.sad.xml" );
    } catch ( CF::FileException &_ex ) {
        std::cout << _ex.errorNumber << " " << _ex.msg << std::endl;
        exit(-1);
    } catch ( CF::InvalidFileName &_ex ) {
        std::cout << _ex.errorNumber << " " << _ex.msg << std::endl;
        exit(-1);
    } catch ( ... ) {
        std::cout << "Unknown error while listing available applications" << std::endl;
        exit(-1);
    }

    int maximum_number_applications = _fileSeq->length();

    if ( maximum_number_applications == 0 )
        std::cout << "No Applications found." << std::endl;
    else
        std::cout << "Found " << maximum_number_applications << " available applications\n";

// create handle to application
    CF::Application_ptr app = CF::Application::_nil();


// interface control loop

    int state = 0;

    char ch_hit = 0;

    do {

        if (ch_hit != 10) {
            display_menu(state, _fileSeq);
        }

        std::string inputString;
        std::cin >> inputString;
        int inputInt = atoi(inputString.c_str());
        ch_hit = inputString[0];

        switch (state) {
        case 0:
            if (ch_hit == 'x') {
                state = 99;
            } else if ((inputInt >= 1) && (inputInt <= (1 + maximum_number_applications - 1))) {
                if ((app = install_app(inputInt - 1, _fileSeq, domMgr))) state = 1;
            }
            break;

        case 1:
            if (ch_hit == 'u') {
                char* _appid = NULL;
                try {
                    _appid = app->identifier();
                } catch ( CORBA::SystemException& ) {
                    std::cout << "[c_wavLoader] \"app->identifier\" failed with CORBA::SystemException\n";
                    exit(EXIT_FAILURE);
                } catch ( ... ) {
                    std::cout << "[c_wavLoader] \"app->identifier\" failed with Unknown Exception\n";
                    exit(EXIT_FAILURE);
                }
                try {
                    domMgr->uninstallApplication( _appid );
                } catch ( CORBA::SystemException& se ) {
                    std::cout << "[c_wavLoader] \"domMgr->uninstallApplication\" failed with CORBA::SystemException\n";
                    exit(EXIT_FAILURE);
                } catch ( ... ) {
                    std::cout << "[c_wavLoader] \"domMgr->uninstallApplication\" failed with Unknown Exception\n";
                    exit(EXIT_FAILURE);
                }

                std::cout << "Application uninstalled from Domain Manager" << std::endl;
                CORBA::string_free( _appid );

                // destroy all components associated with Application

                try {
                    app->releaseObject();
                } catch ( CORBA::SystemException& se ) {
                    std::cout << "[c_wavLoader] \"app->releaseObject\" failed with CORBA::SystemException\n";
                    exit(EXIT_FAILURE);
                } catch ( ... ) {
                    std::cout << "[c_wavLoader] \"app->releaseObject\" failed with Unknown Exception\n";
                    exit(EXIT_FAILURE);
                }

                app = CF::Application::_nil();
                std::cout << "Application destroyed" << std::endl;
                state = 0;

            } else if (ch_hit == 's') {
                try {
                    app->start();
                } catch ( CORBA::SystemException& se ) {
                    std::cout << "[c_wavLoader] \"app->start\" failed with CORBA::SystemException\n";
                    exit(EXIT_FAILURE);
                } catch ( ... ) {
                    std::cout << "[c_wavLoader] \"app->start\" failed with Unknown Exception\n";
                    exit(EXIT_FAILURE);
                }

                state = 2;
            }
            break;

        case 2:
            if (ch_hit == 't') {
                try {
                    app->stop();
                } catch ( CORBA::SystemException& se ) {
                    std::cout << "[c_wavLoader] \"app->stop\" failed with CORBA::SystemException\n";
                    exit(EXIT_FAILURE);
                } catch ( ... ) {
                    std::cout << "[c_wavLoader] \"app->stop\" failed with Unknown Exception\n";
                    exit(EXIT_FAILURE);
                }

                state = 1;
            }
            break;

        default:
            std::cerr << "Hit default in keypress switch, this should never happen" << std::endl;
        }
    } while (state != 99);

    return 0;
};

