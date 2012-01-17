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

#include <ossie/debug.h>

#include <standardinterfaces/Radio_Control_u.h>

standardInterfaces_i::RX_Control_u::RX_Control_u(const char* _name) : portName(_name)
{
    data_servant = new RX_Control::usesPort(this);
    data_servant_var = data_servant->_this();
}


standardInterfaces_i::RX_Control_u::~RX_Control_u()
{

}

CORBA::Object_ptr standardInterfaces_i::RX_Control_u::getPort(const char* _portName)
{
    if (portName == _portName)
        return CORBA::Object::_duplicate(data_servant_var);
    else
        return CORBA::Object::_nil();
}

void standardInterfaces_i::RX_Control_u::set_number_of_channels(CORBA::ULong num)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_number_of_channels(num);
}

void standardInterfaces_i::RX_Control_u::get_number_of_channels(CORBA::ULong& num)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_number_of_channels(num);
}

void standardInterfaces_i::RX_Control_u::get_gain_range(CORBA::ULong channel, float& gmin, float& gmax, float& gstep)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_gain_range(channel, gmin, gmax, gstep);
}

void standardInterfaces_i::RX_Control_u::set_gain(CORBA::ULong channel, float gain)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_gain(channel, gain);
}

void standardInterfaces_i::RX_Control_u::get_gain(CORBA::ULong channel, float& gain)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_gain(channel, gain);
}

void standardInterfaces_i::RX_Control_u::get_frequency_range(CORBA::ULong channel, float& fmin, float& fmax, float& fstep)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_frequency_range(channel, fmin, fmax, fstep);
}

void standardInterfaces_i::RX_Control_u::set_frequency(CORBA::ULong channel, float f)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_frequency(channel, f);
}

void standardInterfaces_i::RX_Control_u::get_frequency(CORBA::ULong channel, float& f)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_frequency(channel, f);
}

void standardInterfaces_i::RX_Control_u::start(CORBA::ULong channel)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->start(channel);
}

void standardInterfaces_i::RX_Control_u::stop(CORBA::ULong channel)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->stop(channel);
}

void standardInterfaces_i::RX_Control_u::set_values(CF::Properties values)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_values(values);
}

void standardInterfaces_i::RX_Control_u::set_decimation_rate(CORBA::ULong channel, CORBA::ULong M)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_decimation_rate(channel, M);
}

void standardInterfaces_i::RX_Control_u::get_decimation_range(CORBA::ULong channel, CORBA::ULong& dmin,
        CORBA::ULong& dmax, CORBA::ULong& dstep)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_decimation_range(channel, dmin, dmax, dstep);
}

void standardInterfaces_i::RX_Control_u::set_data_packet_size(CORBA::ULong channel, CORBA::ULong N)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_data_packet_size(channel, N);
}

RX_Control::usesPort::usesPort(standardInterfaces_i::RX_Control_u *_base) :
        base(_base)
{
}

RX_Control::usesPort::~usesPort()
{
}

void RX_Control::usesPort::connectPort(CORBA::Object_ptr connection, const char* _connectionID)
{
    omni_mutex_lock l(base->port_mutex);

    base->dest_port = standardInterfaces::RX_Control::_narrow(connection);
    if (CORBA::is_nil(base->dest_port)) {
        std::cout << "Port is not RX COntrol" << std::endl;
        return;
    }

    base->connectionID = _connectionID;
}

void RX_Control::usesPort::disconnectPort(const char* _connectionID)
{
    omni_mutex_lock l(base->port_mutex);
    if (base->connectionID == _connectionID) {
        base->dest_port = standardInterfaces::RX_Control::_nil();
        base->connectionID.clear();
        return;
    }

    DEBUG(2, StandardInterfaces, "Attempted to disconnect non-existent port: " << _connectionID);


}

standardInterfaces_i::TX_Control_u::TX_Control_u(const char* _name) : portName(_name)
{
    data_servant = new TX_Control::usesPort(this);
    data_servant_var = data_servant->_this();
}


standardInterfaces_i::TX_Control_u::~TX_Control_u()
{

}

CORBA::Object_ptr standardInterfaces_i::TX_Control_u::getPort(const char* _portName)
{
    if (portName == _portName)
        return CORBA::Object::_duplicate(data_servant_var);
    else
        return CORBA::Object::_nil();
}

void standardInterfaces_i::TX_Control_u::set_number_of_channels(CORBA::ULong num)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_number_of_channels(num);
}

void standardInterfaces_i::TX_Control_u::get_number_of_channels(CORBA::ULong& num)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_number_of_channels(num);
}

void standardInterfaces_i::TX_Control_u::get_gain_range(CORBA::ULong channel, float& gmin, float& gmax, float& gstep)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_gain_range(channel, gmin, gmax, gstep);
}

void standardInterfaces_i::TX_Control_u::set_gain(CORBA::ULong channel, float gain)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_gain(channel, gain);
}

void standardInterfaces_i::TX_Control_u::get_gain(CORBA::ULong channel, float& gain)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_gain(channel, gain);
}

void standardInterfaces_i::TX_Control_u::get_frequency_range(CORBA::ULong channel, float& fmin, float& fmax, float& fstep)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_frequency_range(channel, fmin, fmax, fstep);
}

void standardInterfaces_i::TX_Control_u::set_frequency(CORBA::ULong channel, float f)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_frequency(channel, f);
}

void standardInterfaces_i::TX_Control_u::get_frequency(CORBA::ULong channel, float& f)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_frequency(channel, f);
}

void standardInterfaces_i::TX_Control_u::start(CORBA::ULong channel)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->start(channel);
}

void standardInterfaces_i::TX_Control_u::stop(CORBA::ULong channel)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->stop(channel);
}

void standardInterfaces_i::TX_Control_u::set_values(CF::Properties values)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_values(values);
}

void standardInterfaces_i::TX_Control_u::set_interpolation_rate(CORBA::ULong channel, CORBA::ULong M)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->set_interpolation_rate(channel, M);
}

void standardInterfaces_i::TX_Control_u::get_interpolation_range(CORBA::ULong channel, CORBA::ULong& dmin, CORBA::ULong& dmax, CORBA::ULong& dstep)
{
    omni_mutex_lock l(port_mutex);
    if (!CORBA::is_nil(dest_port))
        dest_port->get_interpolation_range(channel, dmin, dmax, dstep);
}

TX_Control::usesPort::usesPort(standardInterfaces_i::TX_Control_u *_base) :
        base(_base)
{
}

TX_Control::usesPort::~usesPort()
{
}

void TX_Control::usesPort::connectPort(CORBA::Object_ptr connection, const char* _connectionID)
{
    omni_mutex_lock l(base->port_mutex);

    base->dest_port = standardInterfaces::TX_Control::_narrow(connection);
    if (CORBA::is_nil(base->dest_port)) {
        std::cout << "Port is not RX COntrol" << std::endl;
        return;
    }

    base->connectionID = _connectionID;

}

void TX_Control::usesPort::disconnectPort(const char* _connectionID)
{
    omni_mutex_lock l(base->port_mutex);
    if (base->connectionID == _connectionID) {
        base->dest_port = standardInterfaces::TX_Control::_nil();
        base->connectionID.clear();
        return;
    }

    DEBUG(2, StandardInterfaces, "Attempt to disconnect non-existent connection: " << _connectionID);


}
