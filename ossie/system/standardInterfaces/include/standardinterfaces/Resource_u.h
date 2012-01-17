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

#ifndef RESOURCE_U_H
#define RESOURCE_U_H

#include <string>
#include <vector>

#ifdef HAVE_OMNIORB4
#include "omniORB4/CORBA.h"
#endif

#include <omnithread.h>

#include <ossie/cf.h>

// Forward declaration
namespace Resource
{

class usesPort;
}

namespace standardInterfaces_i
{

class Resource_u
{
    friend class Resource::usesPort;

public:
    Resource_u(const char* portName);
    virtual ~Resource_u();

    CORBA::Object_ptr getPort(const char* portName);

    CF::Resource_ptr getRef() {
        return CF::Resource::_duplicate(dest_port);
    };


private:
    Resource_u();
    Resource_u(const Resource_u &);

    std::string portName;

// Uses port
    Resource::usesPort *data_servant;
    CF::Port_var data_servant_var;
    std::string connectionID;
    CF::Resource_var dest_port;
    omni_mutex port_mutex;

};

}

namespace Resource
{

class usesPort : public virtual POA_CF::Port
{
public:
    usesPort(standardInterfaces_i::Resource_u *_base);
    ~usesPort();

    void connectPort(CORBA::Object_ptr connection, const char* connectionID);
    void disconnectPort(const char* connectionID);

private:
    standardInterfaces_i::Resource_u *base;
};

}
#endif
