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

#ifndef COMPLEXLONG_P_H
#define COMPLEXLONG_P_H

#include <string>

#include <standardinterfaces/complexLong.h>

#include <fstream>

namespace complexLong
{
class providesPort;
}

namespace standardInterfaces_i
{
class complexLong_p
{
    friend class complexLong::providesPort;

public:
    complexLong_p(const char* portName);
    complexLong_p(const char* portName, const char* domainName);
    ~complexLong_p();

    CORBA::Object_ptr getPort(const char* portName);

    void getData(PortTypes::LongSequence* &I, PortTypes::LongSequence* &Q);
    void bufferEmptied() {
        ready_for_input->post();
    };

private:
    complexLong_p();
    complexLong_p(const complexLong_p &);

    std::string portName;

// Provides port
    complexLong::providesPort *data_servant;
    standardInterfaces::complexLong_var data_servant_var;

// Buffer storage
    PortTypes::LongSequence I_in;
    PortTypes::LongSequence Q_in;

// Semaphores for synchronization
    omni_semaphore *data_ready;  // Ready to process data
    omni_semaphore *ready_for_input; // Ready to receive more data

};

}

namespace complexLong
{

class providesPort : public POA_standardInterfaces::complexLong
{
public:
    providesPort(standardInterfaces_i::complexLong_p* _base);
    ~providesPort();

    void pushPacket(const PortTypes::LongSequence &I, const PortTypes::LongSequence &Q);

private:
    standardInterfaces_i::complexLong_p* base;

// File for data latency measurements
    std::ofstream latencyFile;
};
}


#endif
