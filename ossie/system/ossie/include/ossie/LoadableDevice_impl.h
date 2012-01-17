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

LOADABLEDEVICE_IMPL.H
Class defintion for LoadableDevice_impl.
ExecutableDevice_impl
Craig Neely, Carlos Aguayo, 12 December 2003

****************************************************************************/

#ifndef LOADABLE_DEVICE_IMPL_H
#define LOADABLE_DEVICE_IMPL_H

#include <vector>

#include "Resource_impl.h"
#include "Device_impl.h"
#include "FileSystem_impl.h"
#include "cf.h"
using namespace std;


/* DATA TYPE DEFINITIONS **************************************************************************
 ************************************************************************************************ */
struct fileCount {
    char *fileId;
    int counter;
};

/* CLASS DEFINITION *******************************************************************************
 ************************************************************************************************ */
class OSSIECF_API LoadableDevice_impl:public virtual
        POA_CF::LoadableDevice,
        public
        Device_impl
{
protected:
    vector <
    fileCount >
    loadedFiles;
    void
    incrementFile (const char *);
    void
    decrementFile (const char *);
public:
    LoadableDevice_impl (char *, char *, char *);
    LoadableDevice_impl (char *, char *, char *, CF::Properties capacities);
    ~LoadableDevice_impl ();
    void
    load (CF::FileSystem_ptr fs, const char *fileName,
          CF::LoadableDevice::LoadType loadKind)
    throw (CF::LoadableDevice::LoadFail, CF::InvalidFileName,
           CF::LoadableDevice::InvalidLoadKind, CF::Device::InvalidState,
           CORBA::SystemException);
    void
    unload (const char *fileName)
    throw (CF::InvalidFileName, CF::Device::InvalidState,
           CORBA::SystemException);
    bool
    isFileLoaded (const char *fileName);

    void configure (const CF::Properties & configProperties)
    throw (CF::PropertySet::PartialConfiguration,
           CF::PropertySet::InvalidConfiguration, CORBA::SystemException);

private:
    LoadableDevice_impl(); // No default constructor
    LoadableDevice_impl(LoadableDevice_impl&); // No copying
};

#endif

