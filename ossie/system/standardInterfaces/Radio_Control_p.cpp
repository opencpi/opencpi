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

#include <standardinterfaces/Radio_Control_p.h>

standardInterfaces_i::RX_Control_p::RX_Control_p(const char* _name) : portName(_name)
{
    data_servant = new RX_Control::providesPort(this);
    data_servant_var = data_servant->_this();
}

standardInterfaces_i::RX_Control_p::RX_Control_p(const char* _name, const char* _domain) : portName(_name)
{
    ossieSupport::ORB orb;

    data_servant = new RX_Control::providesPort(this);
    data_servant_var = data_servant->_this();

    std::string objName;
    objName = _domain;
    objName += "/";
    objName += _name;

    orb.bind_object_to_name((CORBA::Object_ptr) data_servant_var, objName.c_str());
}

standardInterfaces_i::RX_Control_p::~RX_Control_p()
{

}

CORBA::Object_ptr standardInterfaces_i::RX_Control_p::getPort(const char* _portName)
{
    if (portName == _portName)
        return CORBA::Object::_duplicate(data_servant_var);
    else
        return CORBA::Object::_nil();
}

RX_Control::providesPort::providesPort(standardInterfaces_i::RX_Control_p *_base) :
        base(_base)
{
}

RX_Control::providesPort::~providesPort()
{
}


standardInterfaces_i::TX_Control_p::TX_Control_p(const char* _name) : portName(_name)
{
    data_servant = new TX_Control::providesPort(this);
    data_servant_var = data_servant->_this();
}

standardInterfaces_i::TX_Control_p::TX_Control_p(const char* _name, const char* _domain) : portName(_name)
{
    ossieSupport::ORB orb;

    data_servant = new TX_Control::providesPort(this);
    data_servant_var = data_servant->_this();

    std::string objName;
    objName = _domain;
    objName += "/";
    objName += _name;

    orb.bind_object_to_name((CORBA::Object_ptr) data_servant_var, objName.c_str());
}

standardInterfaces_i::TX_Control_p::~TX_Control_p()
{

}

CORBA::Object_ptr standardInterfaces_i::TX_Control_p::getPort(const char* _portName)
{
    if (portName == _portName)
        return CORBA::Object::_duplicate(data_servant_var);
    else
        return CORBA::Object::_nil();
}

TX_Control::providesPort::providesPort(standardInterfaces_i::TX_Control_p *_base) :
        base(_base)
{
}

TX_Control::providesPort::~providesPort()
{
}


