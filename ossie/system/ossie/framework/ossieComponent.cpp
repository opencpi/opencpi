/*******************************************************************************

Copyright 2008, Virginia Polytechnic Institute and State University

This file is part of the OSSIE.

OSSIE is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Parser; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <iostream>

#include <string.h>
#include <stdlib.h>

#include "ossie/cf.h"
#include "ossie/ossieSupport.h"
#include "ossie/debug.h"

ossieSupport::ossieComponent::ossieComponent(ossieSupport::ORB *_orb, int argc, char *argv[]) : namingContextIORFound(false), uuidFound(false), nameBindingFound(false), scheduler(-1), orb(_orb), numArgs(0)
{
    DEBUG(3, ossieComponent, "In ossieComponent Constructor.");

    p.sched_priority = 1;

///\bug this is broken if the framework starts this with valgrind.
    for (unsigned int i(1); i < argc-1; i+=2) {

        if (strcmp(argv[i], "NAMING_CONTEXT_IOR") == 0) {
            namingContextIOR = argv[i+1];
            CORBA::Object_var obj = orb->orb->string_to_object(namingContextIOR.c_str());
            nc = CosNaming::NamingContext::_narrow(obj);
            namingContextIORFound = true;

        } else if (strcmp(argv[i], "NAME_BINDING") == 0) {
            nameBinding = argv[i+1];
            nameBindingFound = true;

        } else if (strcmp(argv[i], "COMPONENT_IDENTIFIER") == 0) {
            uuid = argv[i+1];
            uuidFound = true;

        } else if (strcmp(argv[i], "OS_SCHEDULER") == 0) {
            if (strcmp(argv[i+1], "SCHED_RR") == 0) {
                scheduler = SCHED_RR;
            } else if (strcmp(argv[i+1], "SCHED_FIFO") == 0) {
                scheduler = SCHED_FIFO;
            }

        } else if (strcmp(argv[i], "OS_PRIORITY") == 0) {
            p.sched_priority = atoi(argv[i+1]);

        } else {
            CF::DataType arg;
            arg.id = argv[i];
            arg.value <<= argv[i+1];

            CLArgs.length(++numArgs);
            CLArgs[numArgs-1] = arg;
        }

    }

    if (!(namingContextIORFound && uuidFound && nameBindingFound)) {
        DEBUG(1, ossieComponent, "Not enough command line arguments found.");
        throw;
    }



#if (_POSIX_PRIORITY_SCHEDULING - 0) >=  200112L
    if (scheduler >= 0) {
        // Check macro
        if (sched_setscheduler(0, scheduler, &p) > 0) {
            std::cerr << nameBinding
                      << " : Unable to set OS scheduler."
                      << std::endl;
        }
    }
#endif

}

ossieSupport::ossieComponent::~ossieComponent()
{
    DEBUG(3, ossieComponent, "In ossieComponent Destructor.");

}

void ossieSupport::ossieComponent::bind(CF::Resource_ptr res)
{
    DEBUG(3, ossieComponent, "In ossieComponent bind.");

    orb->bind_object_to_name((CORBA::Object_ptr) res, nc, nameBinding.c_str());
}

void ossieSupport::ossieComponent::unbind()
{
    DEBUG(3, ossieComponent, "In ossieComponent unbind.");

    orb->unbind_name(nc, nameBinding.c_str());
}

const char *ossieSupport::ossieComponent::getUuid()
{
    DEBUG(3, ossieComponent, "In ossieComponent getUuid.");

    return uuid.c_str();
}

void ossieSupport::ossieComponent::getCLArgs(CF::Properties &props)
{
    DEBUG(3, ossieComponent, "In ossieComponent getCLArgs.");

    props = CLArgs;
}
