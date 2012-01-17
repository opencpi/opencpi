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

#include <standardinterfaces/complexChar_p.h>

standardInterfaces_i::complexChar_p::complexChar_p(const char* _name) : portName(_name)
{

    data_servant = new complexChar::providesPort(this);
    data_servant_var = data_servant->_this();

    ready_for_input = new omni_semaphore(1);
    data_ready = new omni_semaphore(0);
}

standardInterfaces_i::complexChar_p::~complexChar_p()
{

}

CORBA::Object_ptr standardInterfaces_i::complexChar_p::getPort(const char* _portName)
{
    if (portName == _portName)
        return CORBA::Object::_duplicate(data_servant_var);
    else
        return CORBA::Object::_nil();
}

void standardInterfaces_i::complexChar_p::getData(PortTypes::CharSequence* &I, PortTypes::CharSequence* &Q)
{
    data_ready->wait();

    I = &I_in;
    Q = &Q_in;
}

complexChar::providesPort::providesPort(standardInterfaces_i::complexChar_p* _base) : base(_base)
{

}

complexChar::providesPort::~providesPort()
{

}

void complexChar::providesPort::pushPacket(const PortTypes::CharSequence &I, const PortTypes::CharSequence &Q)
{
    base->ready_for_input->wait();

    base->I_in = I;
    base->Q_in = Q;

    base->data_ready->post();
}
