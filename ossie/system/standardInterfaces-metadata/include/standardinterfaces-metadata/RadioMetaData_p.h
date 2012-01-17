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

#ifndef RADIOMETADATA_P_H
#define RADIOMETADATA_P_H

#include <string>
#include <vector>

#include <standardinterfaces-metadata/RadioMetaData.h>
#include "standardinterfaces-metadata/RadioMetaData_impl.h"

namespace RadioMetaData
{
class providesPort;
}

namespace standardInterfacesMD_i
{
class RadioMetaData_p
{
    friend class RadioMetaData::providesPort;

public:
    RadioMetaData_p(const char* portName, unsigned int bufLen = 5);
    RadioMetaData_p(const char* portName, const char* domainName, unsigned int bufLen = 5);
    ~RadioMetaData_p();

    CORBA::Object_ptr getPort(const char* portName);

    void getMetaData(standardInterfacesMD::MetaData* &packet_data);

    void bufferEmptied() {
        ready_for_input->post();
    };

private:
    RadioMetaData_p();
    RadioMetaData_p(const RadioMetaData_p &);

    std::string portName;

// Provides port
    RadioMetaData::providesPort *data_servant;
    standardInterfacesMD::RadioMetaData_var data_servant_var;

// Buffer storage
    unsigned int bufferLength;
    unsigned int rdPtr;
    unsigned int wrPtr;
    std::vector <standardInterfacesMD::MetaData> metadata_buf;

// Semaphores for synchronization
    omni_semaphore *data_ready;  // Ready to process data
    omni_semaphore *ready_for_input; // Ready to receive more data

};

}

namespace RadioMetaData
{

class providesPort : public POA_standardInterfacesMD::RadioMetaData
{
public:
    providesPort(standardInterfacesMD_i::RadioMetaData_p* _base);
    ~providesPort();

    void pushMetaData(const standardInterfacesMD::MetaData &packet_data);

private:
    standardInterfacesMD_i::RadioMetaData_p* base;
};
}


#endif
