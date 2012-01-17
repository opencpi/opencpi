/****************************************************************************

Copyright 2008, Virginia Polytechnic Institute and State University

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

EXECUTABLEDEVICE_IMPL.H
Class defintion for ExecutableDevice_impl.

Craig Neely, Carlos Aguayo, 12 December 2003

****************************************************************************/

#ifndef EXECUTABLE_DEVICE_IMPL_H
#define EXECUTABLE_DEVICE_IMPL_H

#include <vector>

#include <sys/types.h>

#include "Resource_impl.h"
#include "Device_impl.h"
#include "LoadableDevice_impl.h"
#include "cf.h"
using namespace std;


class OSSIECF_API ExecutableDevice_impl:public virtual
        POA_CF::ExecutableDevice,
        public
        LoadableDevice_impl
{

public:

    ExecutableDevice_impl (char *, char *, char *);
    ExecutableDevice_impl (char *, char *, char *, CF::Properties capacities);
    ~ExecutableDevice_impl () {
    };
    CF::ExecutableDevice::ProcessID_Type execute (const char *name, const CF::Properties & options,
            const CF::Properties & parameters) throw (CF::ExecutableDevice::ExecuteFail,
                    CF::InvalidFileName, CF::ExecutableDevice::InvalidOptions,
                    CF::ExecutableDevice::InvalidParameters,
                    CF::ExecutableDevice::InvalidFunction, CF::Device::InvalidState,
                    CORBA::SystemException);

    void terminate (CF::ExecutableDevice::ProcessID_Type processId) throw
    (CF::Device::InvalidState, CF::ExecutableDevice::InvalidProcess,
     CORBA::SystemException);

    void configure (const CF::Properties & configProperties)
    throw (CF::PropertySet::PartialConfiguration,
           CF::PropertySet::InvalidConfiguration, CORBA::SystemException);

private:

    ExecutableDevice_impl();
    ExecutableDevice_impl(ExecutableDevice_impl&);

    CF::ExecutableDevice::ProcessID_Type PID;
};

#endif

