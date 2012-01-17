/****************************************************************************

Copyright 2006 Virginia Polytechnic Institute and State University

This file is part of the OSSIE amplifier.

OSSIE amplifier is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

OSSIE amplifier is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OSSIE amplifier; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/


#include <string>
#include <iostream>
#include "amplifier.h"

amplifier_i::amplifier_i(const char *uuid, omni_condition *condition) : Resource_impl(uuid), component_running(condition)
{
    dataOut_0 = new standardInterfaces_i::complexShort_u("dataOut");
    dataIn_0 = new standardInterfaces_i::complexShort_p("dataIn");


    processing_thread = new omni_thread(process_data, (void *) this);   //Create the thread for the writer's processing function
    processing_thread->start();    //Start the thread containing the writer's processing function

}

amplifier_i::~amplifier_i(void)
{
    delete dataOut_0;
    delete dataIn_0;
}

CORBA::Object_ptr amplifier_i::getPort( const char* portName ) throw (CORBA::SystemException, CF::PortSupplier::UnknownPort)
{
    std::cout << "amplifier_i getPort called with : " << portName << std::endl;

    CORBA::Object_var p;

    p = dataOut_0->getPort(portName);

    if (!CORBA::is_nil(p))
        return p._retn();

    p = dataIn_0->getPort(portName);

    if (!CORBA::is_nil(p))
        return p._retn();

    /*exception*/
    throw CF::PortSupplier::UnknownPort();
}

void amplifier_i::start() throw (CORBA::SystemException, CF::Resource::StartError)
{
    std::cout << "start called on amplifier" << std::endl;
}

void amplifier_i::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    std::cout << "stop called on amplifier" << std::endl;
}

void amplifier_i::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{
    std::cout << "releaseObject called on amplifier" << std::endl;

    component_running->signal();
}

void amplifier_i::initialize() throw (CF::LifeCycle::InitializeError, CORBA::SystemException)
{
    std::cout << "initialize called on amplifier" << std::endl;
}

void
amplifier_i::query (CF::Properties & configProperties)
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

void amplifier_i::configure(const CF::Properties& props) throw (CORBA::SystemException, CF::PropertySet::InvalidConfiguration, CF::PropertySet::PartialConfiguration)
{
    static int init = 0;

    std::cout << "Component -AMPLIFIER" << std::endl;
    std::cout << " Props length = " << props.length() << std::endl;

    if (init == 0) {
        if ( props.length() <= 0 ) {
            std::cout << "amplifier: configure called with invalid props.length() - " << props.length() << std::endl;
            return;
        }
        propertySet.length(props.length());
        for (unsigned int i=0; i < props.length(); i++) {
            propertySet[i].id = CORBA::string_dup(props[i].id);
            propertySet[i].value = props[i].value;
        }
        init++;
    }

    for (unsigned int i=0; i < props.length(); i++) {
        if (strcmp(props[i].id, "DCE:06b88d4f-dd38-44e6-bc49-82db0eba5bc6") == 0) {
            CORBA::Float simple_temp;
            props[i].value >>= simple_temp;
            simple_0_value = simple_temp;
            for (unsigned int j=0; j < propertySet.length(); j++ ) {
                if ( strcmp(propertySet[j].id, props[i].id) == 0 ) {
                    propertySet[j].value = props[i].value;
                    break;
                }
            }
        }

        else if (strcmp(props[i].id, "DCE:df91b1a8-9c83-44b4-bf2c-0dbeacb2b6f4") == 0) {
            CORBA::Float simple_temp;
            props[i].value >>= simple_temp;
            simple_1_value = simple_temp;
            // Update value of this property in propertySet also
            for (unsigned int j=0; j < propertySet.length(); j++ ) {
                if ( strcmp(propertySet[j].id, props[i].id) == 0 ) {
                    propertySet[j].value = props[i].value;
                    break;
                }
            }
        } else {
            // unknown property
            std::cerr << "ERROR: amplifier_i::configure() unknown property \""
                      << props[i].id << "\"" << std::endl;
            throw CF::PropertySet::InvalidConfiguration();
        }
    }
}

void process_data(void *data)
{
    std::cout << "amplifier's process_data thread started" << std::endl;

    amplifier_i *channel = (amplifier_i *) data;

    PortTypes::ShortSequence I_out_0, Q_out_0;

    float I_gain, Q_gain;
    float I_tmp, Q_tmp;

    PortTypes::ShortSequence *I_in_0(NULL), *Q_in_0(NULL);
    CORBA::UShort I_in_0_length, Q_in_0_length;

    while (1) {
        channel->dataIn_0->getData(I_in_0, Q_in_0);

        I_in_0_length = I_in_0->length();
        Q_in_0_length = Q_in_0->length();

        I_out_0.length(I_in_0_length);
        Q_out_0.length(Q_in_0_length);

        /*begin amplifier algorithm*/
        I_gain = (float) channel->simple_0_value;
        Q_gain = (float) channel->simple_1_value;

        for (int i = 0; i < I_in_0_length; ++i) {
            I_tmp = (*I_in_0)[i] * I_gain;
            Q_tmp = (*Q_in_0)[i] * Q_gain;

            (I_out_0)[i] = (short) I_tmp;
            (Q_out_0)[i] = (short) Q_tmp;
        } //close the for loop

        channel->dataIn_0->bufferEmptied();
        channel->dataOut_0->pushPacket(I_out_0, Q_out_0);
    }
}


