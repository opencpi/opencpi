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

#ifndef REALFLOAT_P_H
#define REALFLOAT_P_H

#include <string>

#include <standardinterfaces/realFloat.h>

namespace realFloat
{
class providesPort;
}

namespace standardInterfaces_i
{
class realFloat_p
{
    friend class realFloat::providesPort;

public:
    realFloat_p(const char* portName);
    ~realFloat_p();

    CORBA::Object_ptr getPort(const char* portName);

    void getData(PortTypes::FloatSequence* &I);
    void bufferEmptied() {
        ready_for_input->post();
    };

private:
    realFloat_p();
    realFloat_p(const realFloat_p &);

    std::string portName;

// Provides port
    realFloat::providesPort *data_servant;
    standardInterfaces::realFloat_var data_servant_var;

// Buffer storage
    PortTypes::FloatSequence I_in;

// Semaphores for synchronization
    omni_semaphore *data_ready;  // Ready to process data
    omni_semaphore *ready_for_input; // Ready to receive more data

};

}

namespace realFloat
{

class providesPort : public POA_standardInterfaces::realFloat
{
public:
    providesPort(standardInterfaces_i::realFloat_p* _base);
    ~providesPort();

    void pushPacket(const PortTypes::FloatSequence &I);

private:
    standardInterfaces_i::realFloat_p* base;
};
}


#endif
