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

#ifndef REALCHAR_P_H
#define REALCHAR_P_H

#include <string>
#include <vector>

#include <standardinterfaces-metadata/realCharMD.h>
#include "standardinterfaces-metadata/RadioMetaData_impl.h"

namespace realChar
{
class providesPort;
}

namespace standardInterfacesMD_i
{
class realChar_p
{
    friend class realChar::providesPort;

public:
    realChar_p(const char* portName, unsigned int bufLen = 5);
    ~realChar_p();

    CORBA::Object_ptr getPort(const char* portName);

    void getData(
        PortTypes::CharSequence* &I);

    void getData(
        PortTypes::CharSequence* &I,
        standardInterfacesMD::MetaData* &packet_data);

    void bufferEmptied();

private:
    realChar_p();
    realChar_p(const realChar_p &);

    std::string portName;

// Provides port
    realChar::providesPort *data_servant;
    standardInterfacesMD::realCharMD_var data_servant_var;

// Buffer storage
    unsigned int bufferLength;
    unsigned int rdPtr;
    unsigned int wrPtr;
    std::vector <PortTypes::CharSequence> I_buf;
    std::vector <standardInterfacesMD::MetaData> metadata_buf;

// Semaphores for synchronization
    omni_semaphore *data_ready;  // Ready to process data
    omni_semaphore *ready_for_input; // Ready to receive more data

// packet meta data
    standardInterfacesMD::MetaData metadata;



};

}

namespace realChar
{

class providesPort : public POA_standardInterfacesMD::realCharMD
{
public:
    providesPort(standardInterfacesMD_i::realChar_p* _base);
    ~providesPort();

    void pushPacket(const PortTypes::CharSequence &I);

    void pushPacketMetaData(
        const PortTypes::CharSequence &I,
        const standardInterfacesMD::MetaData &packet_data);

private:
    standardInterfacesMD_i::realChar_p* base;
};
}


#endif
