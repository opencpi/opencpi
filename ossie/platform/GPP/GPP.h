/****************************************************************************

Copyright 2005,2006 Virginia Polytechnic Institute and State University

This file is part of the OSSIE GPP.

OSSIE GPP is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

OSSIE GPP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OSSIE GPP; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


****************************************************************************/

#include "ossie/cf.h"

#include "ossie/ExecutableDevice_impl.h"

class GPP_i : public virtual ExecutableDevice_impl

{
public:
    GPP_i(char *id, char* label, char* profile);

    void load( CF::FileSystem_ptr fs, const char* filename, CF::LoadableDevice::LoadType loadkind )
    throw (CORBA::SystemException, CF::Device::InvalidState, CF::LoadableDevice::InvalidLoadKind, CF::InvalidFileName, CF::LoadableDevice::LoadFail);

    void releaseObject()
    throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

private:
    GPP_i();
    GPP_i(const GPP_i &);
};
