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


#ifndef AMPLIFIER_IMPL_H
#define AMPLIFIER_IMPL_H

#include <stdlib.h>
#include "ossie/cf.h"


#include "ossie/PortTypes.h"
#include "standardinterfaces/complexShort.h"
#include "standardinterfaces/complexShort_u.h"
#include "standardinterfaces/complexShort_p.h"

#include "ossie/Resource_impl.h"
class amplifier_i;

void process_data(void *data);

class amplifier_i : public virtual Resource_impl
{

    friend void process_data(void *data);

public:
    amplifier_i(const char *uuid, omni_condition *sem);
    ~amplifier_i(void);

    void start() throw (CF::Resource::StartError, CORBA::SystemException);
    void stop() throw (CF::Resource::StopError, CORBA::SystemException);

    CORBA::Object_ptr getPort( const char* portName ) throw (CF::PortSupplier::UnknownPort, CORBA::SystemException);

    void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

    void initialize() throw (CF::LifeCycle::InitializeError, CORBA::SystemException);
    void query(CF::Properties &configProperties)
    throw (CF::UnknownProperties, CORBA::SystemException);
    void configure(const CF::Properties&) throw (CORBA::SystemException, CF::PropertySet::InvalidConfiguration, CF::PropertySet::PartialConfiguration);


private:
    amplifier_i();
    amplifier_i(amplifier_i&);

    omni_condition *component_running;  //for component shutdown
    omni_thread *processing_thread;     //for component writer function

    CORBA::Float simple_0_value;
    CORBA::Float simple_1_value;


//list components provides and uses ports
    standardInterfaces_i::complexShort_u *dataOut_0;
    standardInterfaces_i::complexShort_p *dataIn_0;

};
#endif
