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

#include <standardinterfaces-metadata/RadioMetaData_p.h>

standardInterfacesMD_i::RadioMetaData_p::RadioMetaData_p(const char* _name, unsigned int bufLen) : portName(_name), bufferLength(bufLen), rdPtr(0), wrPtr(0)
{

    data_servant = new RadioMetaData::providesPort(this);
    data_servant_var = data_servant->_this();

// Initialize MetaData
    standardInterfacesMD::MetaData metadata;
    InitializeMetaData( metadata );
    metadata_buf.assign(bufferLength, metadata);

    ready_for_input = new omni_semaphore(bufferLength);
    data_ready = new omni_semaphore(0);


}

standardInterfacesMD_i::RadioMetaData_p::RadioMetaData_p(const char* _name, const char* _domain, unsigned int bufLen) : portName(_name), bufferLength(bufLen), rdPtr(0), wrPtr(0)
{
    ossieSupport::ORB orb;

    data_servant = new RadioMetaData::providesPort(this);
    data_servant_var = data_servant->_this();

// Initialize MetaData
    standardInterfacesMD::MetaData metadata;
    InitializeMetaData( metadata );
    metadata_buf.assign(bufferLength, metadata);

    ready_for_input = new omni_semaphore(bufferLength);
    data_ready = new omni_semaphore(0);

    std::string objName;
    objName = _domain;
    objName += "/";
    objName += _name;

    orb.bind_object_to_name((CORBA::Object_ptr) data_servant_var, objName.c_str());
}

standardInterfacesMD_i::RadioMetaData_p::~RadioMetaData_p()
{

}

CORBA::Object_ptr standardInterfacesMD_i::RadioMetaData_p::getPort(const char* _portName)
{
    if (portName == _portName)
        return CORBA::Object::_duplicate(data_servant_var);
    else
        return CORBA::Object::_nil();
}

void standardInterfacesMD_i::RadioMetaData_p::getMetaData(
    standardInterfacesMD::MetaData* &packet_data)
{
    data_ready->wait();

    packet_data = &metadata_buf[rdPtr];
    rdPtr = (++rdPtr) % bufferLength;
}

RadioMetaData::providesPort::providesPort(standardInterfacesMD_i::RadioMetaData_p* _base) : base(_base)
{

}

RadioMetaData::providesPort::~providesPort()
{

}

void RadioMetaData::providesPort::pushMetaData(
    const standardInterfacesMD::MetaData &packet_data)
{
    base->ready_for_input->wait();

// Deep copy meta data to RadioMetaData_p base and add it to the buffer
    (base->metadata_buf)[base->wrPtr] = packet_data;

    base->wrPtr = (++(base->wrPtr)) % base->bufferLength;

    base->data_ready->post();
}
