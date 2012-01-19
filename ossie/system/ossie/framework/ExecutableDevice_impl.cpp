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

EXECUTABLEDEVICE_IMPL.CPP
Functions for class ExecutableDevice_impl. Provides a mechanism to execute and terminate
processes on the device.

Craig Neely, Carlos Aguayo, 12 December 2003

****************************************************************************/

#include <iostream>
#include <vector>
#include <algorithm>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#include "ossie/ExecutableDevice_impl.h"

//vector<CF::ExecutableDevice::ProcessID_Type> ExecutableDevice_impl::PID;

/* ExecutableDevice_impl ****************************************
    - constructor 1: no capacities defined
****************************************************************** */
ExecutableDevice_impl::ExecutableDevice_impl (char *id, char *lbl, char *sftwrPrfl):
        LoadableDevice_impl (id, lbl, sftwrPrfl)
{
}


/* ExecutableDevice_impl ************************************************
    - constructor 2: capacities defined
******************************************************************** */
ExecutableDevice_impl::ExecutableDevice_impl (char *id, char *lbl, char *sftwrPrfl,
        CF::Properties capacities):
        LoadableDevice_impl (id, lbl, sftwrPrfl, capacities)
{
}


/* execute *****************************************************************
    - executes a process on the device
************************************************************************* */
CF::ExecutableDevice::ProcessID_Type ExecutableDevice_impl::execute (const char *name, const CF::Properties & options, const CF::Properties & parameters) throw (CORBA::SystemException, CF::Device::InvalidState, CF::ExecutableDevice::InvalidFunction, CF::ExecutableDevice::InvalidParameters, CF::ExecutableDevice::InvalidOptions, CF::InvalidFileName, CF::ExecutableDevice::ExecuteFail)
{
    CORBA::TypeCode_var tc;                       // CORBA type code
    const char *tempStr;                          // temporaray character string
    CF::ExecutableDevice::ProcessID_Type PID;     // process ID
    int size = 2 * parameters.length () + 2;      // length of the POSIX argv arguments
    char **argv = new char *[size];               // POSIX arguments
    CORBA::ULong stackSize, priority;             // CORBA unsigned longs for options storage

// verify device is in the correct state
    if (!isUnlocked () || isDisabled ()) {
        std::cout <<"Cannot execute. System is either LOCKED, SHUTTING DOWN or DISABLED." << std::endl;;
        throw (CF::Device::InvalidState("Cannot execute. System is either LOCKED, SHUTTING DOWN or DISABLED."));
    }

// validate if function or file is loaded on the device
// IMPORTANT NOTE: the validation for correct function has not been implemented yet
    if (!isFileLoaded (name)) {
        std::cout << "Cannot Execute. File was not already loaded." << std::endl;;
        throw (CF::
               InvalidFileName (CF::CFENOENT,
                                "Cannot Execute. File was not already loaded."));
    }

// validate parameters to be strings
    {
        for (unsigned int i = 0; i < parameters.length (); i++) {
            tc = parameters[i].value.type ();

            if (tc->kind () != CORBA::tk_string) {
                std::cout << "Invalid Parameters." << std::endl;;
                throw CF::ExecutableDevice::InvalidParameters();
            }
        }
    }

    priority=0;                                   // this is the default value for the priority (it's actually meaningless in this version)
    stackSize=4096;                               // this is the default value for the stacksize (it's actually meaningless in this version)

    {
// verify valid options, both STACK_SIZE_ID and PRIORITY_ID must have unsigned-long types
        for (unsigned i = 0; i < options.length (); i++) {
            tc = options[i].value.type ();

            if (tc->kind () != CORBA::tk_ulong) {
                std::cout << "Invalid Options." << std::endl;;
                throw CF::ExecutableDevice::InvalidOptions();
            }

// extract priority and stack size from the options list
            if (strcmp (options[i].id, CF::ExecutableDevice::PRIORITY_ID))
                options[i].value >>= priority;
            if (strcmp (options[i].id, CF::ExecutableDevice::STACK_SIZE))
                options[i].value >>= stackSize;
        }
    }

// convert input parameters to the POSIX standard argv format
    char *name_temp = new char[strlen (name) + 1];
    strcpy (name_temp, name);
    argv[0] = name_temp;
    {
        for (unsigned int i = 0; i < parameters.length (); i++) {
            name_temp = new char[strlen (parameters[i].id) + 1];
            strcpy (name_temp, parameters[i].id);
            argv[2 * i + 1] = name_temp;

            // NOTE: I'm not very proud of this hack
            // but the test for the NAMING_CONTEXT_IOR
            // supports the goal of distributed apps
            // The IOR provided by the CF::AppFact is
            // relative to the CF::DomainManager.
            // To support distribution, the IOR must be 
            // provided relative to the CF::DeviceManager.
            // This test guarantees the IOR is relative to the node.
            if( strcmp(name_temp, "NAMING_CONTEXT_IOR") == 0 )
                tempStr = ossieSupport::ORB::orb->object_to_string(ossieSupport::ORB::inc);
            else
                parameters[i].value >>= tempStr;
            name_temp = new char[strlen (tempStr) + 1];
            strcpy (name_temp, tempStr);
            argv[2 * i + 2] = name_temp;
        }
        argv[size-1] = NULL;
    }

// execute file or function, pass arguments and options and execute, handler is returned
#ifdef HAVE_WORKING_FORK
    CF::ExecutableDevice::ProcessID_Type new_pid;

    if ((new_pid = fork()) < 0) {
        std::cout << "error executing fork()"<< std::endl;
        return((CF::ExecutableDevice::ProcessID_Type) -1);
    }

    if (new_pid > 0) {

// in parent process
        PID = new_pid;
    } else {
// in child process
        if (getenv("VALGRIND")) {
            char *new_argv[20];
            new_argv[0] =  "/usr/local/bin/valgrind";
            string logFile = "--log-file=";
            logFile += argv[0];
            new_argv[1] = (char *)logFile.c_str();
            unsigned int i;
            for (i=2; argv[i-2]; i++) {
                new_argv[i] = argv[i-2];
            }
            new_argv[i]=NULL;
            execv(new_argv[0], new_argv);
        } else {
            execv(argv[0], argv);
        }

        std::cout << strerror(errno) << std::endl;
        exit(-1);                                 // If we ever get here the program we tried to start and failed
    }
#endif

    {
        for (int i = 0; i < size; i++) {          /// \bug Does this leak memory when executeProcess Fails?
            delete argv[i];
        }
    }
    delete argv;

// create a PID and return it
    return (PID);
}


/* terminate ***********************************************************
    - terminates a process on the device
******************************************************************* */
void
ExecutableDevice_impl::terminate (CF::ExecutableDevice::ProcessID_Type processId) throw (CORBA::SystemException, CF::ExecutableDevice::InvalidProcess, CF::Device::InvalidState)
{

// validate device state
    if (isLocked () || isDisabled ()) {
        printf ("Cannot terminate. System is either LOCKED or DISABLED.");
        throw (CF::Device::
               InvalidState
               ("Cannot terminate. System is either LOCKED or DISABLED."));
    }

// validate processId
#ifdef HAVE_WORKING_FORK

//    ExecutableDevice_impl::PID.clear(); //::iterator p = find(PID.begin(), PID.end());

//vector<CF::ExecutableDevice::ProcessID_Type>::iterator p;

// p = find(PID.begin(), PID.end(), processId);

    /*if (p == PID.end())
    {
        cout <<"Cannot terminate. Process ID does not exist.";
        throw (CF::ExecutableDevice::
            InvalidProcess (CF::CFENOENT,
            "Cannot terminate. Process ID does not exist."));
        }*/
#endif

// go ahead and terminate the process
#ifdef HAVE_WORKING_FORK
    int status;
//kill(processId, SIGTERM);
    kill(processId, SIGKILL);
//throw CF::ExecutableDevice::InvalidProcess when processId does not exists
    waitpid(processId, &status, 0);
// PID.erase(p);
#endif
}


void  ExecutableDevice_impl::configure (const CF::Properties & capacities)
throw (CF::PropertySet::PartialConfiguration, CF::PropertySet::
       InvalidConfiguration, CORBA::SystemException)
{
    Device_impl::configure(capacities);
}

