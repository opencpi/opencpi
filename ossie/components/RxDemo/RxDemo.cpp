/****************************************************************************

Copyright 2007 Virginia Polytechnic Institute and State University

This file is part of the OSSIE RxDemo.

OSSIE RxDemo is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

OSSIE RxDemo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OSSIE RxDemo; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/


#include <string>
#include <iostream>
#include "RxDemo.h"

RxDemo_i::RxDemo_i(const char *uuid, omni_condition *condition) :
        Resource_impl(uuid), component_running(condition)
{
    dataInPort = new standardInterfaces_i::complexShort_p("symbols_in");


//Create the thread for the writer's processing function
    processing_thread = new omni_thread(Run, (void *) this);

//Start the thread containing the writer's processing function
    processing_thread->start();

}

RxDemo_i::~RxDemo_i(void)
{
    delete dataInPort;
}

// Static function for omni thread
void RxDemo_i::Run( void * data )
{
    ((RxDemo_i*)data)->ProcessData();
}

CORBA::Object_ptr RxDemo_i::getPort( const char* portName ) throw (
    CORBA::SystemException, CF::PortSupplier::UnknownPort)
{
    DEBUG(3, RxDemo, "getPort() invoked with " << portName)

    CORBA::Object_var p;

    p = dataInPort->getPort(portName);

    if (!CORBA::is_nil(p))
        return p._retn();

    /*exception*/
    throw CF::PortSupplier::UnknownPort();
}

void RxDemo_i::start() throw (CORBA::SystemException,
                              CF::Resource::StartError)
{
    DEBUG(3, RxDemo, "start() invoked")
}

void RxDemo_i::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    DEBUG(3, RxDemo, "stop() invoked")
}

void RxDemo_i::releaseObject() throw (CORBA::SystemException,
                                      CF::LifeCycle::ReleaseError)
{
    DEBUG(3, RxDemo, "releaseObject() invoked")

    component_running->signal();
}

void RxDemo_i::initialize() throw (CF::LifeCycle::InitializeError,
                                   CORBA::SystemException)
{
    DEBUG(3, RxDemo, "initialize() invoked")
}

void
RxDemo_i::query (CF::Properties & configProperties)
throw (CORBA::SystemException, CF::UnknownProperties)
{
// for queries of zero length, return all id/value pairs in propertySet
    if (configProperties.length () == 0) {
        configProperties.length (propertySet.length ());
        for (unsigned int i = 0; i < propertySet.length (); i++) {
            configProperties[i].id = CORBA::string_dup (propertySet[i].id);
            configProperties[i].value = propertySet[i].value;
        }

        return ;
    } else {
        for (unsigned int i = 0; i < configProperties.length(); i++) {
            for (unsigned int j=0; j < propertySet.length(); j++) {
                if ( strcmp(configProperties[i].id, propertySet[j].id) == 0 ) {
                    configProperties[i].value = propertySet[j].value;
                }
            }
        }
    }
}



void RxDemo_i::configure(const CF::Properties& props)
throw (CORBA::SystemException,
       CF::PropertySet::InvalidConfiguration,
       CF::PropertySet::PartialConfiguration)
{
    DEBUG(3, RxDemo, "configure() invoked")

    std::cout << "props length : " << props.length() << std::endl;

    for (unsigned int i = 0; i <props.length(); i++) {
        std::cout << "Property id : " << props[i].id << std::endl;

    }
}

void RxDemo_i::ProcessData()
{
    DEBUG(3, RxDemo, "ProcessData() invoked")

    PortTypes::ShortSequence *I_in(NULL), *Q_in(NULL);
    CORBA::UShort I_in_length, Q_in_length;

    unsigned int num_errors;
    unsigned char bit;

    while (true) {
        dataInPort->getData(I_in, Q_in);

        I_in_length = I_in->length();
        Q_in_length = Q_in->length();

        if (I_in_length != 512 || Q_in_length != 512)
            std::cout << "WARNING! RxDemo expecting 512 samples!" << std::endl;

        // count errors
        num_errors = 0;
        for (unsigned int i=0; i<512; i++) {
            // count errors in in-phase channel
            bit = ( (*I_in)[i] > 0 ) ? 1 : 0;
            num_errors += (bit==data_i[i]) ? 0 : 1;

            // count errors in quadrature channel
            bit = ( (*Q_in)[i] > 0 ) ? 1 : 0;
            num_errors += (bit==data_q[i]) ? 0 : 1;
        }
        std::cout << "RxDemo errors: " << num_errors << " / 1024" << std::endl;

        dataInPort->bufferEmptied();
    }
}

unsigned char data_i[512] = {
    1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1,
    1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0,
    1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1,
    0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1,
    0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1,
    1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0,
    1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1,
    0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0,
    0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1,
    0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0,
    1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1,
    1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0,
    1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1,
    0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0,
    0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
    0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1,
    0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1,
    0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1,
    1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1,
    1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0,
    1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0,
    1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0
};

unsigned char data_q[512] = {
    1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0,
    1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1,
    1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1,
    0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1,
    1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
    1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1,
    0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0,
    1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0,
    1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0,
    0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1,
    0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1,
    1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0,
    0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0,
    1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1,
    0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1,
    0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1,
    1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1,
    1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0,
    1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0,
    0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1,
    0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0,
    0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1
};


