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

#ifndef COMPLEXFLOAT_P_H
#define COMPLEXFLOAT_P_H

#include <string>

#include <standardinterfaces/complexFloat.h>

namespace complexFloat
{
class providesPort;
}

namespace standardInterfaces_i
{
class complexFloat_p
{
    friend class complexFloat::providesPort;

public:
    complexFloat_p(const char* portName);
    ~complexFloat_p();

    CORBA::Object_ptr getPort(const char* portName);

    void getData(PortTypes::FloatSequence* &I, PortTypes::FloatSequence* &Q);
    void bufferEmptied() {
        ready_for_input->post();
    };

private:
    complexFloat_p();
    complexFloat_p(const complexFloat_p &);

    std::string portName;

// Provides port
    complexFloat::providesPort *data_servant;
    standardInterfaces::complexFloat_var data_servant_var;

// Buffer storage
    PortTypes::FloatSequence I_in;
    PortTypes::FloatSequence Q_in;

// Semaphores for synchronization
    omni_semaphore *data_ready;  // Ready to process data
    omni_semaphore *ready_for_input; // Ready to receive more data

};

}

namespace complexFloat
{

class providesPort : public POA_standardInterfaces::complexFloat
{
public:
    providesPort(standardInterfaces_i::complexFloat_p* _base);
    ~providesPort();

    void pushPacket(const PortTypes::FloatSequence &I, const PortTypes::FloatSequence &Q);

private:
    standardInterfaces_i::complexFloat_p* base;
};
}


#endif
