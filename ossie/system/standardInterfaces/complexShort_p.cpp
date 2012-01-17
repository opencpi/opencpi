/****************************************************************************

Copyright 2006, Virginia Polytechnic Institute and State University

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

****************************************************************************/

#include <iostream>

#include <ossie/ossieSupport.h>

#include <standardinterfaces/complexShort_p.h>

standardInterfaces_i::complexShort_p::complexShort_p(const char* _name, unsigned int bufLen) : portName(_name), bufferLength(bufLen), rdPtr(0), wrPtr(0)
{

    data_servant = new complexShort::providesPort(this);
    data_servant_var = data_servant->_this();

    PortTypes::ShortSequence a;
    I_buf.assign(bufferLength, a);
    Q_buf.assign(bufferLength, a);

    ready_for_input = new omni_semaphore(bufferLength);
    data_ready = new omni_semaphore(0);
}

standardInterfaces_i::complexShort_p::complexShort_p(const char* _name, const char* _domain, unsigned int bufLen) : portName(_name), bufferLength(bufLen), rdPtr(0), wrPtr(0)
{
    ossieSupport::ORB orb;

    data_servant = new complexShort::providesPort(this);
    data_servant_var = data_servant->_this();

    PortTypes::ShortSequence a;
    I_buf.assign(bufferLength, a);
    Q_buf.assign(bufferLength, a);

    ready_for_input = new omni_semaphore(bufferLength);
    data_ready = new omni_semaphore(0);

    std::string objName;
    objName = _domain;
    objName += "/";
    objName += _name;

    orb.bind_object_to_name((CORBA::Object_ptr) data_servant_var, objName.c_str());
}

standardInterfaces_i::complexShort_p::~complexShort_p()
{

}

CORBA::Object_ptr standardInterfaces_i::complexShort_p::getPort(const char* _portName)
{
    if (portName == _portName)
        return CORBA::Object::_duplicate(data_servant_var);
    else
        return CORBA::Object::_nil();
}

void standardInterfaces_i::complexShort_p::bufferEmptied()
{
    ready_for_input->post();
}


void standardInterfaces_i::complexShort_p::getData(PortTypes::ShortSequence* &I, PortTypes::ShortSequence* &Q)
{
    data_ready->wait();

    I = &I_buf[rdPtr];
    Q = &Q_buf[rdPtr];
    rdPtr = (++rdPtr) % bufferLength;
}

complexShort::providesPort::providesPort(standardInterfaces_i::complexShort_p* _base) : base(_base)
{

}

complexShort::providesPort::~providesPort()
{

}

void complexShort::providesPort::pushPacket(const PortTypes::ShortSequence &I, const PortTypes::ShortSequence &Q)
{
    base->ready_for_input->wait();

    (base->I_buf)[base->wrPtr] = I;
    (base->Q_buf)[base->wrPtr] = Q;
    base->wrPtr = (++(base->wrPtr)) % base->bufferLength;

    base->data_ready->post();

}
