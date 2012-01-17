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

#include <standardinterfaces/Resource_u.h>
#include <ossie/debug.h>

standardInterfaces_i::Resource_u::Resource_u(const char* _name) : portName(_name)
{
    data_servant = new Resource::usesPort(this);
    data_servant_var = data_servant->_this();
}


standardInterfaces_i::Resource_u::~Resource_u()
{

}

CORBA::Object_ptr standardInterfaces_i::Resource_u::getPort(const char* _portName)
{
    if (portName == _portName)
        return CORBA::Object::_duplicate(data_servant_var);
    else
        return CORBA::Object::_nil();
}


Resource::usesPort::usesPort(standardInterfaces_i::Resource_u *_base) :
        base(_base)
{
}

Resource::usesPort::~usesPort()
{
}

void Resource::usesPort::connectPort(CORBA::Object_ptr connection, const char* _connectionID)
{
    omni_mutex_lock l(base->port_mutex);

    base->dest_port = CF::Resource::_narrow(connection);
    if (CORBA::is_nil(base->dest_port)) {
        DEBUG(5, StandardInterfaces, "Port is not RX Control");
        return;
    }

    base->connectionID = _connectionID;

}

void Resource::usesPort::disconnectPort(const char* _connectionID)
{
    omni_mutex_lock l(base->port_mutex);
    if (base->connectionID == _connectionID) {
        base->dest_port = CF::Resource::_nil();
        base->connectionID.clear();
    }

    DEBUG(5, StandardInterfaces, "Attempted to disconnect non-existent port: " << _connectionID);


}

