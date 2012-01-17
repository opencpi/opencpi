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

#ifndef RADIO_CONTROL_P_H
#define RADIO_CONTROL_P_H

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

class providesPort;
}

namespace standardInterfaces_i
{

class RX_Control_p
{
    friend class RX_Control::providesPort;

public:
    RX_Control_p(const char* portName);
    RX_Control_p(const char* portName, const char* domain);
    virtual ~RX_Control_p() = 0;

    CORBA::Object_ptr getPort(const char* portName);

/// Set the number of active channels
    virtual void set_number_of_channels(CORBA::ULong num) = 0;
    virtual void get_number_of_channels(CORBA::ULong& num) = 0;

/// Gain functions
    virtual void get_gain_range(CORBA::ULong channel, float& gmin, float& gmax, float& gstep) = 0;
    virtual void set_gain(CORBA::ULong channel, float gain) = 0;
    virtual void get_gain(CORBA::ULong channel, float& gain) = 0;

/// Frequency tuning functions
    virtual void get_frequency_range(CORBA::ULong channel, float& fmin, float& fmax, float& fstep) = 0;
    virtual void set_frequency(CORBA::ULong channel, float f) = 0;
    virtual void get_frequency(CORBA::ULong channel, float& f) = 0;

// Start/Stop Control
    virtual void start(CORBA::ULong channel) = 0;
    virtual void stop(CORBA::ULong channel) = 0;

// Set properties
    virtual void set_values(const CF::Properties& values) = 0;

/// Set the receiver decimation rate
    virtual void set_decimation_rate(CORBA::ULong channel, CORBA::ULong M) = 0;
    virtual void get_decimation_range(CORBA::ULong channel, CORBA::ULong& dmin,
                                      CORBA::ULong& dmax, CORBA::ULong& dstep) = 0;

/// Set data output size
    virtual void set_data_packet_size(CORBA::ULong channel, CORBA::ULong N) = 0;



private:
    RX_Control_p();
    RX_Control_p(const RX_Control_p &);

    std::string portName;

// Provides port
    RX_Control::providesPort *data_servant;
    standardInterfaces::RX_Control_var data_servant_var;


};

}

namespace RX_Control
{

class providesPort : public virtual POA_standardInterfaces::RX_Control
{
public:
    providesPort(standardInterfaces_i::RX_Control_p *_base);
    ~providesPort();

/// Set the number of active channels
    void set_number_of_channels(CORBA::ULong num) {
        base->set_number_of_channels(num);
    };
    void get_number_of_channels(CORBA::ULong& num) {
        base->get_number_of_channels(num);
    };

/// Gain functions
    void get_gain_range(CORBA::ULong channel, float& gmin, float& gmax, float& gstep) {
        base->get_gain_range(channel, gmin, gmax, gstep);
    };
    void set_gain(CORBA::ULong channel, float gain) {
        base->set_gain(channel, gain);
    };
    void get_gain(CORBA::ULong channel, float& gain) {
        base->get_gain(channel, gain);
    };

/// Frequency tuning functions
    void get_frequency_range(CORBA::ULong channel, float& fmin, float& fmax, float& fstep) {
        base->get_frequency_range(channel, fmin, fmax, fstep);
    };
    void set_frequency(CORBA::ULong channel, float f) {
        base->set_frequency(channel, f);
    };
    void get_frequency(CORBA::ULong channel, float& f) {
        base->get_frequency(channel, f);
    };

// Start/Stop Control
    void start(CORBA::ULong channel) {
        base->start(channel);
    };
    void stop(CORBA::ULong channel) {
        base->stop(channel);
    };

// Set properties
    void set_values(const CF::Properties& values) {
        base->set_values(values);
    };

/// Set the receiver decimation rate
    void set_decimation_rate(CORBA::ULong channel, CORBA::ULong M) {
        base->set_decimation_rate(channel, M);
    };
    void get_decimation_range(CORBA::ULong channel, CORBA::ULong& dmin,
                              CORBA::ULong& dmax, CORBA::ULong& dstep) {
        base->get_decimation_range(channel, dmin, dmax, dstep);
    };

/// Set data output size
    void set_data_packet_size(CORBA::ULong channel, CORBA::ULong N) {
        base->set_data_packet_size(channel, N);
    };

private:
    standardInterfaces_i::RX_Control_p *base;
};

}

namespace TX_Control
{

class providesPort;
}

namespace standardInterfaces_i
{

class TX_Control_p
{
    friend class TX_Control::providesPort;

public:
    TX_Control_p(const char* portName);
    TX_Control_p(const char* portName, const char* domain);
    virtual ~TX_Control_p() = 0;

    CORBA::Object_ptr getPort(const char* portName);

/// Set the number of active channels
    virtual void set_number_of_channels(CORBA::ULong num) = 0;
    virtual void get_number_of_channels(CORBA::ULong& num) = 0;

/// Gain functions
    virtual void get_gain_range(CORBA::ULong channel, float& gmin, float& gmax, float& gstep) = 0;
    virtual void set_gain(CORBA::ULong channel, float gain) = 0;
    virtual void get_gain(CORBA::ULong channel, float& gain) = 0;

/// Frequency tuning functions
    virtual void get_frequency_range(CORBA::ULong channel, float& fmin, float& fmax, float& fstep) = 0;
    virtual void set_frequency(CORBA::ULong channel, float f) = 0;
    virtual void get_frequency(CORBA::ULong channel, float& f) = 0;

// Start/Stop Control
    virtual void start(CORBA::ULong channel) = 0;
    virtual void stop(CORBA::ULong channel) = 0;

// Set properties
    virtual void set_values(const CF::Properties& values) = 0;

/// Set the receiver decimation rate
    virtual void set_interpolation_rate(CORBA::ULong channel, CORBA::ULong M) = 0;
    virtual void get_interpolation_range(CORBA::ULong channel, CORBA::ULong& dmin,
                                         CORBA::ULong& dmax, CORBA::ULong& dstep) = 0;



private:
    TX_Control_p();
    TX_Control_p(const TX_Control_p &);

    std::string portName;

// Provides port
    TX_Control::providesPort *data_servant;
    standardInterfaces::TX_Control_var data_servant_var;


};

}

namespace TX_Control
{

class providesPort : public virtual POA_standardInterfaces::TX_Control
{
public:
    providesPort(standardInterfaces_i::TX_Control_p *_base);
    ~providesPort();

/// Set the number of active channels
    void set_number_of_channels(CORBA::ULong num) {
        base->set_number_of_channels(num);
    };
    void get_number_of_channels(CORBA::ULong& num) {
        base->get_number_of_channels(num);
    };

/// Gain functions
    void get_gain_range(CORBA::ULong channel, float& gmin, float& gmax, float& gstep) {
        base->get_gain_range(channel, gmin, gmax, gstep);
    };
    void set_gain(CORBA::ULong channel, float gain) {
        base->set_gain(channel, gain);
    };
    void get_gain(CORBA::ULong channel, float& gain) {
        base->get_gain(channel, gain);
    };

/// Frequency tuning functions
    void get_frequency_range(CORBA::ULong channel, float& fmin, float& fmax, float& fstep) {
        base->get_frequency_range(channel, fmin, fmax, fstep);
    };
    void set_frequency(CORBA::ULong channel, float f) {
        base->set_frequency(channel, f);
    };
    void get_frequency(CORBA::ULong channel, float& f) {
        base->get_frequency(channel, f);
    };

// Start/Stop Control
    void start(CORBA::ULong channel) {
        base->start(channel);
    };
    void stop(CORBA::ULong channel) {
        base->stop(channel);
    };

// Set properties
    void set_values(const CF::Properties& values) {
        base->set_values(values);
    };

/// Set the receiver decimation rate
    void set_interpolation_rate(CORBA::ULong channel, CORBA::ULong M) {
        base->set_interpolation_rate(channel, M);
    };
    void get_interpolation_range(CORBA::ULong channel, CORBA::ULong& dmin,
                                 CORBA::ULong& dmax, CORBA::ULong& dstep) {
        base->get_interpolation_range(channel, dmin, dmax, dstep);
    };

private:
    standardInterfaces_i::TX_Control_p *base;
};

}



#endif
