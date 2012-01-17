/****************************************************************************

Copyright 2007 Virginia Polytechnic Institute and State University

This file is part of the OSSIE TxDemo.

OSSIE TxDemo is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

OSSIE TxDemo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OSSIE TxDemo; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/


#include <string>
#include <iostream>
#include "TxDemo.h"

TxDemo_i::TxDemo_i(const char *uuid, omni_condition *condition) :
        Resource_impl(uuid), component_running(condition)
{
    dataOutPort = new standardInterfaces_i::complexShort_u("symbols_out");

// initialize packet delay to be 5 seconds
    packet_delay = 5000;
}

TxDemo_i::~TxDemo_i(void)
{
    delete dataOutPort;
}

// Static function for omni thread
void TxDemo_i::Run( void * data )
{
    ((TxDemo_i*)data)->ProcessData();
}

CORBA::Object_ptr TxDemo_i::getPort( const char* portName ) throw (
    CORBA::SystemException, CF::PortSupplier::UnknownPort)
{
    DEBUG(3, TxDemo, "getPort() invoked with " << portName)

    CORBA::Object_var p;

    p = dataOutPort->getPort(portName);

    if (!CORBA::is_nil(p))
        return p._retn();

    /*exception*/
    throw CF::PortSupplier::UnknownPort();
}

void TxDemo_i::start() throw (CORBA::SystemException,
                              CF::Resource::StartError)
{
    DEBUG(3, TxDemo, "start() invoked")
    omni_mutex_lock l(processing_mutex);
    if (false == thread_started) {
        thread_started = true;    //initialize
        //Create the thread for the writer's processing function
        processing_thread = new omni_thread(Run, (void *) this);

        //Start the thread containing the writer's processing function
        processing_thread->start();
    }
}

void TxDemo_i::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    DEBUG(3, TxDemo, "stop() invoked")
    omni_mutex_lock l(processing_mutex);
    thread_started=false;
}

void TxDemo_i::releaseObject() throw (CORBA::SystemException,
                                      CF::LifeCycle::ReleaseError)
{
    DEBUG(3, TxDemo, "releaseObject() invoked")

    component_running->signal();
}

void TxDemo_i::initialize() throw (CF::LifeCycle::InitializeError,
                                   CORBA::SystemException)
{
    DEBUG(3, TxDemo, "initialize() invoked")
}

void
TxDemo_i::query (CF::Properties & configProperties)
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
                if ( strcmp(configProperties[i].id, propertySet[i].id) == 0 ) {
                    configProperties[i].value = propertySet[i].value;
                }
            }
        }
    }
}


void TxDemo_i::configure(const CF::Properties& props)
throw (CORBA::SystemException,
       CF::PropertySet::InvalidConfiguration,
       CF::PropertySet::PartialConfiguration)
{
    static int init = 0;
    DEBUG(3, TxDemo, "configure() invoked")

    if (init == 0) {
        if ( props.length() == 0  )	{
            std::cout << "TxDemo: configure called with invalid props.length() - " << props.length() << std::endl;
            return;
        }

        propertySet.length(props.length());
        for (unsigned int i=0; i < props.length(); i++) {
            std::cout << "Property Id : " << props[i].id << std::endl;
            propertySet[i].id = CORBA::string_dup(props[i].id);
            propertySet[i].value = props[i].value;
        }
        init = 1;
    }

    for (unsigned int i = 0; i <props.length(); i++) {
        if (strcmp(props[i].id, "DCE:1b9ca208-8242-11dc-9ba3-00123f63025f") == 0) {
            // packet_delay_ms
            CORBA::Short simple_temp;
            props[i].value >>= simple_temp;
            packet_delay = simple_temp;
            std::cout << "Property id : " << props[i].id << std::endl;
            // Update value of this property in propertySet also
            for (unsigned int j=0; j < propertySet.length(); j++ ) {
                if ( strcmp(propertySet[j].id, props[i].id) == 0 ) {
                    propertySet[j].value = props[i].value;
                    break;
                }
            }
        } else {
            // unknown property
            std::cerr << "ERROR: TxDemo_i::configure() unknown property \""
                      << props[i].id << "\"" << std::endl;
            throw CF::PropertySet::InvalidConfiguration();
        }
    }
}

void TxDemo_i::ProcessData()
{
    DEBUG(3, TxDemo, "ProcessData() invoked")

// Generate output data
    PortTypes::ShortSequence I_out, Q_out;
    I_out.length(512);
    Q_out.length(512);
    for (unsigned int i=0; i<512; i++) {
        I_out[i] = (data_i[i]==0) ? -7000 : 7000;
        Q_out[i] = (data_q[i]==0) ? -7000 : 7000;
    }

    while (continue_processing()) {
        // push data to output
        dataOutPort->pushPacket(I_out, Q_out);

        // wait
        usleep(packet_delay*1000);
    }
}
bool TxDemo_i::continue_processing(void)
{
    omni_mutex_lock l(processing_mutex);
    return thread_started;
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


