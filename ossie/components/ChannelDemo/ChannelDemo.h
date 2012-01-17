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


#ifndef CHANNELDEMO_IMPL_H
#define CHANNELDEMO_IMPL_H

#include <stdlib.h>
#include "ossie/cf.h"
#include "ossie/PortTypes.h"
#include "ossie/Resource_impl.h"
#include "ossie/debug.h"

#include "standardinterfaces/complexShort_u.h"
#include "standardinterfaces/complexShort_p.h"

#include <math.h>

/// Uniform random number generator, (0,1]
float randf();

/// Gaussian random number generator w/ zero mean, unit variance, N(0,1)
void randnf(float * i, float * q);

/// Converts float to short, clipping as necessary
short float2short(float x);

/// rotates a complex vector by theta radians
void rotate(short I_in, short Q_in, float theta, short &I_out, short &Q_out);


/** \brief
 *
 *
 */
class ChannelDemo_i : public virtual Resource_impl
{
public:
/// Initializing constructor
    ChannelDemo_i(const char *uuid, omni_condition *sem);

/// Destructor
    ~ChannelDemo_i(void);

/// Static function for omni thread
    static void Run( void * data );

///
    void start() throw (CF::Resource::StartError, CORBA::SystemException);

///
    void stop() throw (CF::Resource::StopError, CORBA::SystemException);

///
    CORBA::Object_ptr getPort( const char* portName )
    throw (CF::PortSupplier::UnknownPort, CORBA::SystemException);

///
    void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

///
    void initialize() throw (CF::LifeCycle::InitializeError, CORBA::SystemException);

    void query(CF::Properties &configProperties)
    throw (CF::UnknownProperties, CORBA::SystemException);

/// Configures properties read from .prf.xml
    void configure(const CF::Properties&)
    throw (CORBA::SystemException,
           CF::PropertySet::InvalidConfiguration,
           CF::PropertySet::PartialConfiguration);


private:
/// Disallow default constructor
    ChannelDemo_i();

/// Disallow copy constructor
    ChannelDemo_i(ChannelDemo_i&);

/// Main signal processing method
    void ProcessData();

    omni_condition *component_running;  ///< for component shutdown
    omni_thread *processing_thread;     ///< for component writer function

// list components provides and uses ports
    standardInterfaces_i::complexShort_p *dataInPort;   // "samples_in"
    standardInterfaces_i::complexShort_u *dataOutPort;  // "samples_out"

// Properties
    short noise_std_dev;
    float phase_offset;

};
#endif
