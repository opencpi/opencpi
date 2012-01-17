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

#ifndef REALSHORT_P_H
#define REALSHORT_P_H

#include <string>

#include <standardinterfaces/realShort.h>

namespace realShort
{
class providesPort;
}

namespace standardInterfaces_i
{
class realShort_p
{
    friend class realShort::providesPort;

public:
    realShort_p(const char* portName);
    ~realShort_p();

    CORBA::Object_ptr getPort(const char* portName);

    void getData(PortTypes::ShortSequence* &I);
    void bufferEmptied() {
        ready_for_input->post();
    };

private:
    realShort_p();
    realShort_p(const realShort_p &);

    std::string portName;

// Provides port
    realShort::providesPort *data_servant;
    standardInterfaces::realShort_var data_servant_var;

// Buffer storage
    PortTypes::ShortSequence I_in;

// Semaphores for synchronization
    omni_semaphore *data_ready;  // Ready to process data
    omni_semaphore *ready_for_input; // Ready to receive more data

};

}

namespace realShort
{

class providesPort : public POA_standardInterfaces::realShort
{
public:
    providesPort(standardInterfaces_i::realShort_p* _base);
    ~providesPort();

    void pushPacket(const PortTypes::ShortSequence &I);

private:
    standardInterfaces_i::realShort_p* base;
};
}


#endif
