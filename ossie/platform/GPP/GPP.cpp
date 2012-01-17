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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "GPP.h"

GPP_i::GPP_i (char *id, char* label, char* profile) : ExecutableDevice_impl(id, label, profile)

{

}

void GPP_i::load( CF::FileSystem_ptr fs, const char* filename, CF::LoadableDevice::LoadType loadkind ) throw (CORBA::SystemException, CF::Device::InvalidState, CF::LoadableDevice::InvalidLoadKind, CF::InvalidFileName, CF::LoadableDevice::LoadFail)
{
    DEBUG( 6, GPP, "Entering load " << filename );

    /* add the file name to the loaded files using the default load implementation */
    LoadableDevice_impl::load( fs, filename, loadkind );

    /* check if you can open the file locally */
    int fd = open(filename, O_RDONLY);

    if ( fd == -1 ) throw CF::InvalidFileName();

// NOTE: this resolves the lack of changing execute permissions within the framework;
// however, we do not make local copy of the binary because the teardown features
// of OSSIE are not very good; therefore, if we made a local copy of the executable,
// we currently do not have a way of deleting it when we are done with it; well, we do,
// but it doesn't really work very well
    /* chmod 755 filename */
    int ch = fchmod( fd, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );

    if ( ch == -1 ) throw CF::LoadableDevice::LoadFail();

    close(fd);

    DEBUG( 6, GPP, "Leaving load " << filename );
}

void GPP_i::releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException)
{
    DEBUG(6, GPP, "Entering releaseObject");

    try {
        ossieSupport::ORB::orb->destroy();
    } catch ( ... ) {
        throw CF::LifeCycle::ReleaseError();
    }

    DEBUG(6, GPP, "ORB Destroyed");

// delete this;
}

