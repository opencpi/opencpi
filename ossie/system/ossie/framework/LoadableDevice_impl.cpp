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

LOADABLEDEVICE_IMPL.CPP
Functions for class LoadableDevice_impl. Provides a mechanism to load and unload
software on the device.

Craig Neely, Carlos Aguayo, 12 December 2003

****************************************************************************/

#include "ossie/LoadableDevice_impl.h"

/* LoadableDevice_impl ****************************************************************************
    - constructor 1: no capacities defined
************************************************************************************************ */
LoadableDevice_impl::LoadableDevice_impl (char *id, char *lbl, char *sftwrPrfl):
        Device_impl (id, lbl, sftwrPrfl)
{
}


/* LoadableDevice_impl ****************************************************************************
    - constructor 2: capacities defined
************************************************************************************************ */
LoadableDevice_impl::LoadableDevice_impl (char *id, char *lbl, char *sftwrPrfl,
        CF::Properties capacities):
        Device_impl (id, lbl, sftwrPrfl, capacities)
{
}


/* LoadableDevice_impl ****************************************************************************
    - destructor
************************************************************************************************ */
LoadableDevice_impl::~LoadableDevice_impl ()
{
    for (unsigned int i = 0; i < loadedFiles.size (); i++) {
//remove node from loadedFiles
        delete loadedFiles[i].fileId;
    }
    loadedFiles.erase (loadedFiles.begin (), loadedFiles.end ());

}


/* load *******************************************************************************************
    - loads a file into the device
************************************************************************************************ */
void
LoadableDevice_impl::load (CF::FileSystem_ptr fs, const char *fileName,
                           CF::LoadableDevice::LoadType loadKind)
throw (CORBA::SystemException, CF::Device::InvalidState,
       CF::LoadableDevice::InvalidLoadKind, CF::InvalidFileName,
       CF::LoadableDevice::LoadFail)
{

//CF::File_ptr fileToLoad;                      // file pointer
    bool readOnly = true;                         // read only flag

// verify that the device is in a valid state for loading
    if (!isUnlocked () || isDisabled ()) {
        throw (CF::Device::
               InvalidState
               ("Cannot load. System is either LOCKED, SHUTTING DOWN or DISABLED."));
    }

// verify that the loadKind is supported (only executable is supported by this version)
    if (loadKind != CF::LoadableDevice::EXECUTABLE) {
        throw CF::LoadableDevice::InvalidLoadKind ();
    }

// verify the file name exists in the file system and get a pointer to it
// NOTE: in this context, this step is redundant; the 'installApplication' method
// already performs this existence check
    /*
    if (!fs->exists ((char *) fileName))
        throw (CF::InvalidFileName (CF::CFENOENT, "Cannot load. File name is invalid."));
    */

// pass fileToLoad to API (device specific, not currently implemented)

// check if file loaded correctly
//if (API unsuccessful)
//{
//      throw(CF::LoadableDevice::LoadFail("Cannot load. Attempt to load to device unsuccessful."));
//      return;
//}

// add filename to loadedfiles. If it's been already loaded, then increment its counter
    incrementFile (fileName);

//CORBA::release (fileToLoad);

}


void
LoadableDevice_impl::incrementFile (const char *fileName)
{
    for (unsigned int i = 0; i < loadedFiles.size (); i++) {
        if (strcmp (loadedFiles[i].fileId, fileName) == 0) {
            loadedFiles[i].counter++;
            return;
        }
    }
//If the file is not loaded yet, then add a new element to loadedFiles
    fileCount tempNode;
    char *tempName = new char[strlen (fileName) + 1];
    strcpy (tempName, fileName);
    tempNode.fileId = tempName;
    tempNode.counter = 1;
    loadedFiles.push_back (tempNode);
}


/* unload *****************************************************************************************
    - removes one instance of a file from the device
************************************************************************************************ */
void
LoadableDevice_impl::unload (const char *fileName)
throw (CORBA::SystemException, CF::Device::InvalidState, CF::InvalidFileName)
{

// verify that the device is in a valid state for loading
    if (isLocked () || isDisabled ()) {
        throw (CF::Device::
               InvalidState
               ("Cannot unload. System is either LOCKED or DISABLED."));
    }

// decrement the list entry counter for this file
    decrementFile (fileName);

}


void
LoadableDevice_impl::decrementFile (const char *fileName)
{
    for (unsigned int i = 0; i < loadedFiles.size (); i++) {
        if (strcmp (loadedFiles[i].fileId, fileName) == 0) {
            if (--loadedFiles[i].counter == 0) {

// unload fileName using API (device specific, not currently implemented)

//remove node from loadedFiles
                delete loadedFiles[i].fileId;
//delete loadedFiles[i];
                loadedFiles.erase (loadedFiles.begin () + i);
            }
            return;
        }
    }

//If it gets to this point the file was not loaded previously
    throw (CF::
           InvalidFileName (CF::CFENOENT,
                            "Cannot unload. File was not already loaded."));
}


/* isFileLoaded ***********************************************************************************
    - looks for a given file name in the loaded files list
************************************************************************************************ */
bool LoadableDevice_impl::isFileLoaded (const char *fileName)
{
    for (unsigned int i = 0; i < loadedFiles.size (); i++) {
        if (strcmp (loadedFiles[i].fileId, fileName) == 0) {
            return true;
        }
    }
    return false;
}


void LoadableDevice_impl ::configure (const CF::Properties & capacities)
throw (CF::PropertySet::PartialConfiguration, CF::PropertySet::
       InvalidConfiguration, CORBA::SystemException)
{
    Device_impl::configure(capacities);
}

