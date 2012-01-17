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

#ifndef RADIO_CONTROL_U_H
#define RADIO_CONTROL_U_H

#include <string>
#include <vector>

#ifdef HAVE_OMNIORB4
#include "omniORB4/CORBA.h"
#endif

#include <omnithread.h>

#include <ossie/PortTypes.h>
#include <ossie/cf.h>

#include <standardinterfaces/Radio_Control.h>

// Forward declaration
namespace RX_Control
{

class usesPort;
}

namespace standardInterfaces_i
{

class RX_Control_u
{
    friend class RX_Control::usesPort;

public:
    RX_Control_u(const char* portName);
    virtual ~RX_Control_u();

    CORBA::Object_ptr getPort(const char* portName);

/// Set the number of active channels
    void set_number_of_channels(CORBA::ULong num);
    void get_number_of_channels(CORBA::ULong& num);

/// Gain functions
    void get_gain_range(CORBA::ULong channel, float& gmin, float& gmax, float& gstep);
    void set_gain(CORBA::ULong channel, float gain);
    void get_gain(CORBA::ULong channel, float& gain);

/// Frequency tuning functions
    void get_frequency_range(CORBA::ULong channel, float& fmin, float& fmax, float& fstep);
    void set_frequency(CORBA::ULong channel, float f);
    void get_frequency(CORBA::ULong channel, float& f);

// Start/Stop Control
    void start(CORBA::ULong channel);
    void stop(CORBA::ULong channel);

// Set properties
    void set_values(CF::Properties values);

/// Set the receiver decimation rate
    void set_decimation_rate(CORBA::ULong channel, CORBA::ULong M);
    void get_decimation_range(CORBA::ULong channel, CORBA::ULong& dmin,
                              CORBA::ULong& dmax, CORBA::ULong& dstep);

/// Set data output size
    void set_data_packet_size(CORBA::ULong channel, CORBA::ULong N);



private:
    RX_Control_u();
    RX_Control_u(const RX_Control_u &);

    std::string portName;

// Uses port
    RX_Control::usesPort *data_servant;
    CF::Port_var data_servant_var;
    std::string connectionID;
    standardInterfaces::RX_Control_var dest_port;
    omni_mutex port_mutex;

};

}

namespace RX_Control
{

class usesPort : public virtual POA_CF::Port
{
public:
    usesPort(standardInterfaces_i::RX_Control_u *_base);
    ~usesPort();

    void connectPort(CORBA::Object_ptr connection, const char* connectionID);
    void disconnectPort(const char* connectionID);

private:
    standardInterfaces_i::RX_Control_u *base;
};

}

// Forward declaration
namespace TX_Control
{

class usesPort;
}

namespace standardInterfaces_i
{

class TX_Control_u
{
    friend class TX_Control::usesPort;

public:
    TX_Control_u(const char* portName);
    ~TX_Control_u();

    CORBA::Object_ptr getPort(const char* portName);

/// Set the number of active channels
    void set_number_of_channels(CORBA::ULong num);
    void get_number_of_channels(CORBA::ULong& num);

/// Gain functions
    void get_gain_range(CORBA::ULong channel, float& gmin, float& gmax, float& gstep);
    void set_gain(CORBA::ULong channel, float gain);
    void get_gain(CORBA::ULong channel, float& gain);

/// Frequency tuning functions
    void get_frequency_range(CORBA::ULong channel, float& fmin, float& fmax, float& fstep);
    void set_frequency(CORBA::ULong channel, float f);
    void get_frequency(CORBA::ULong channel, float& f);

// Start/Stop Control
    void start(CORBA::ULong channel);
    void stop(CORBA::ULong channel);

// Set properties
    void set_values(CF::Properties values);

/// Set the transmitter interpolation rate
    void set_interpolation_rate(CORBA::ULong channel, CORBA::ULong M);
    void get_interpolation_range(CORBA::ULong channel, CORBA::ULong& dmin, CORBA::ULong& dmax, CORBA::ULong& dstep);



private:
    TX_Control_u();
    TX_Control_u(const RX_Control_u &);

    std::string portName;

// Uses port
    TX_Control::usesPort *data_servant;
    CF::Port_var data_servant_var;
    standardInterfaces::TX_Control_var dest_port;
    std::string connectionID;
    omni_mutex port_mutex;

};

}

namespace TX_Control
{

class usesPort : public virtual POA_CF::Port
{
public:
    usesPort(standardInterfaces_i::TX_Control_u *_base);
    ~usesPort();

    void connectPort(CORBA::Object_ptr connection, const char* connectionID);
    void disconnectPort(const char* connectionID);

private:
    standardInterfaces_i::TX_Control_u *base;
};


}



#endif
