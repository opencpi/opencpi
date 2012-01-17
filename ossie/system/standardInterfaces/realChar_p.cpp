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

#include <standardinterfaces/realChar_p.h>

standardInterfaces_i::realChar_p::realChar_p(const char* _name, unsigned int bufLen) : portName(_name), bufferLength(bufLen), rdPtr(0), wrPtr(0)
{

    data_servant = new realChar::providesPort(this);
    data_servant_var = data_servant->_this();

    PortTypes::CharSequence a;
    I_buf.assign(bufferLength, a);

    ready_for_input = new omni_semaphore(bufferLength);
    data_ready = new omni_semaphore(0);
}

standardInterfaces_i::realChar_p::~realChar_p()
{

}

CORBA::Object_ptr standardInterfaces_i::realChar_p::getPort(const char* _portName)
{
    if (portName == _portName)
        return CORBA::Object::_duplicate(data_servant_var);
    else
        return CORBA::Object::_nil();
}

void standardInterfaces_i::realChar_p::bufferEmptied()
{
    ready_for_input->post();
}

void standardInterfaces_i::realChar_p::getData(PortTypes::CharSequence* &I)
{
    data_ready->wait();

    I = &I_buf[rdPtr];
    rdPtr = ++rdPtr % bufferLength;
}

realChar::providesPort::providesPort(standardInterfaces_i::realChar_p* _base) : base(_base)
{

}

realChar::providesPort::~providesPort()
{

}

void realChar::providesPort::pushPacket(const PortTypes::CharSequence &I)
{
    base->ready_for_input->wait();

    base->I_buf[base->wrPtr] = I;
    base->wrPtr= ++base->wrPtr % base->bufferLength;

    base->data_ready->post();
}
