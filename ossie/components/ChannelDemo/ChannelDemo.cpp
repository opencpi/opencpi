/****************************************************************************

Copyright 2007 Virginia Polytechnic Institute and State University

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


#include <string>
#include <iostream>
#include "ChannelDemo.h"

// Uniform random number generator, (0,1]
float randf()
{
    float x = (float) rand();
    return x / (float) RAND_MAX;
}

// Gaussian random number generator w/ zero mean, unit variance, N(0,1)
void randnf(float * i, float * q)
{
// generate two uniform random numbers
    float u1, u2;

// ensure u1 does not equal zero
    do {
        u1 = randf();
    } while (u1 == 0.0f);

    u2 = randf();

    float x = sqrt(-2*logf(u1));
    *i = x * sinf(6.283185307*u2);
    *q = x * cosf(6.283185307*u2);
}
/// Converts float to short, clipping as necessary
short float2short(float x)
{
    short y;
    if (x > SHRT_MAX) {
        y = SHRT_MAX;
    } else if (x < SHRT_MIN) {
        y = SHRT_MIN;
    } else {
        y = (short) x;
    }
    return y;
}

// rotates a complex vector by theta radians
void rotate(short I_in, short Q_in, float theta, short &I_out, short &Q_out)
{
    float c = cosf(theta);
    float s = sinf(theta);

    I_out = (short)  ( (float) (I_in*c) - (float) (Q_in*s)  );
    Q_out = (short)  ( (float) (I_in*s) + (float) (Q_in*c)  );
}

ChannelDemo_i::ChannelDemo_i(const char *uuid, omni_condition *condition) :
        Resource_impl(uuid), component_running(condition)
{
    dataInPort = new standardInterfaces_i::complexShort_p("samples_in");
    dataOutPort = new standardInterfaces_i::complexShort_u("samples_out");

// Initialize noise standard deviation
    noise_std_dev = 1000;

// Initialize phase offset
    phase_offset = 0.0f;

//Create the thread for the writer's processing function
    processing_thread = new omni_thread(Run, (void *) this);

//Start the thread containing the writer's processing function
    processing_thread->start();

}

ChannelDemo_i::~ChannelDemo_i(void)
{
    delete dataInPort;
    delete dataOutPort;
}

// Static function for omni thread
void ChannelDemo_i::Run( void * data )
{
    ((ChannelDemo_i*)data)->ProcessData();
}

CORBA::Object_ptr ChannelDemo_i::getPort( const char* portName ) throw (
    CORBA::SystemException, CF::PortSupplier::UnknownPort)
{
    DEBUG(3, ChannelDemo, "getPort() invoked with " << portName)

    CORBA::Object_var p;

    p = dataInPort->getPort(portName);

    if (!CORBA::is_nil(p))
        return p._retn();

    p = dataOutPort->getPort(portName);

    if (!CORBA::is_nil(p))
        return p._retn();

    /*exception*/
    throw CF::PortSupplier::UnknownPort();
}

void ChannelDemo_i::start() throw (CORBA::SystemException,
                                   CF::Resource::StartError)
{
    DEBUG(3, ChannelDemo, "start() invoked")
}

void ChannelDemo_i::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    DEBUG(3, ChannelDemo, "stop() invoked")
}

void ChannelDemo_i::releaseObject() throw (CORBA::SystemException,
        CF::LifeCycle::ReleaseError)
{
    DEBUG(3, ChannelDemo, "releaseObject() invoked")

    component_running->signal();
}

void ChannelDemo_i::initialize() throw (CF::LifeCycle::InitializeError,
                                        CORBA::SystemException)
{
    DEBUG(3, ChannelDemo, "initialize() invoked")
}

void
ChannelDemo_i::query (CF::Properties & configProperties)
throw (CORBA::SystemException, CF::UnknownProperties)
{
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

void ChannelDemo_i::configure(const CF::Properties& props)
throw (CORBA::SystemException,
       CF::PropertySet::InvalidConfiguration,
       CF::PropertySet::PartialConfiguration)
{
    static int init = 0;

    DEBUG(3, ChannelDemo, "configure() invoked")

    if (init == 0) {
        if ( props.length() <= 0 ) {
            std::cout << "ChannelDemo: configure called with invalid props.length() - " << props.length() << std::endl;
            return;
        }

        propertySet.length(props.length());
        for (unsigned int i=0; i < props.length(); i++) {
            propertySet[i].id = CORBA::string_dup(props[i].id);
            propertySet[i].value = props[i].value;
        }
        init++;
    }

    for (unsigned int i = 0; i <props.length(); i++) {

        if (strcmp(props[i].id, "DCE:a337c5f0-8245-11dc-860f-00123f63025f") == 0) {
            // noise_std_dev (standard deviation of noise)
            CORBA::Short simple_temp;
            props[i].value >>= simple_temp;
            // Test: setting propertySet[] to the given value.
            noise_std_dev = simple_temp;
            for (unsigned int j=0; j < propertySet.length(); j++ ) {
                if ( strcmp(propertySet[j].id, props[i].id) == 0 ) {
                    propertySet[j].value = props[i].value;
                    break;
                }
            }
            DEBUG(1, ChannelDemo, "Setting noise standard deviation to " << noise_std_dev);
        } else if (strcmp(props[i].id, "DCE:1c4a3eb9-9e3a-4c20-849a-90c6eaef9e5a") == 0) {
            // phase offset (degrees)
            CORBA::Float simple_temp;
            props[i].value >>= simple_temp;
            // Test: Setting propertySet[i] to the given value
            phase_offset = simple_temp;
            DEBUG(1, ChannelDemo, "Setting phase offset to " << phase_offset << " degrees");
            // convert to radians
            phase_offset *= M_PI/180.0f;
            for (unsigned int j=0; j < propertySet.length(); j++ ) {
                if ( strcmp(propertySet[j].id, props[i].id) == 0 ) {
                    propertySet[j].value = props[i].value;
                    break;
                }
            }

        } else {
            // unknown property
            std::cerr << "ERROR: ChannelDemo_i::configure() unknown property \""
                      << props[i].id << "\"" << std::endl;
            throw CF::PropertySet::InvalidConfiguration();
        }
    }
}

void ChannelDemo_i::ProcessData()
{
    DEBUG(3, ChannelDemo, "ProcessData() invoked")

    PortTypes::ShortSequence I_out, Q_out;


    PortTypes::ShortSequence *I_in(NULL), *Q_in(NULL);
    CORBA::UShort I_in_length, Q_in_length;

// input/output data length
    unsigned int N(0);

    short I_rotated, Q_rotated;

//
    float noise_i(0.0f);
    float noise_q(0.0f);

    while (true) {
        dataInPort->getData(I_in, Q_in);

        I_in_length = I_in->length();
        Q_in_length = Q_in->length();

        if (I_in_length == Q_in_length) {
            N = I_in_length;
        } else {
            std::cout << "ERROR! Input I/Q data are not the same length!" << std::endl;
            throw 0;
        }

        I_out.length(N);
        Q_out.length(N);

        // add channel impairments
        for (unsigned int i=0; i<N; i++) {
            // Rotate input by phase_offset radians
            rotate((*I_in)[i], (*Q_in)[i], phase_offset, I_rotated, Q_rotated);

            // Add noise
            randnf(&noise_i, &noise_q);
            noise_i *= noise_std_dev;
            noise_q *= noise_std_dev;
            I_out[i] = I_rotated + float2short(noise_i);
            Q_out[i] = Q_rotated + float2short(noise_q);
        }

        dataInPort->bufferEmptied();
        dataOutPort->pushPacket(I_out, Q_out);
    }
}


