/*******************************************************************************

Copyright 2004, 2007 Virginia Polytechnic Institute and State University

This file is part of the OSSIE Parser.

OSSIE Parser is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Parser is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Parser; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Even though all code is original, the architecture of the OSSIE Parser is based
on the architecture of the CRC's SCA Reference Implementation (SCARI)
see: http://www.crc.ca/en/html/rmsc/home/sdr/projects/scari

*********************************************************************************/

#include "ossie/SPDUsesDevice.h"

SPDUsesDevice::SPDUsesDevice (const char *_id, const char *_type,
                              CF::Properties _propertyRefs)
{
    DEBUG(4, SPDUsesDevice, "in constructor.");

    id = _id;

    type = _type;

    propertyRefs = _propertyRefs;
}

#if 0 ///\todo delete this code
SPDUsesDevice::SPDUsesDevice():
        id(NULL), type(NULL)
{}


SPDUsesDevice::SPDUsesDevice(const SPDUsesDevice & _copy)
{
    this->id = new char[strlen (_copy.id) + 1];
    strcpy (this->id, _copy.id);

    this->type = new char[strlen (_copy.type) + 1];
    strcpy (this->type, _copy.type);

    for (unsigned int i = 0; i < _copy.propertyRefs.length(); i++)
        this->propertyRefs[i] = _copy.propertyRefs[i];
}
#endif

SPDUsesDevice::~SPDUsesDevice()
{
}


const char* SPDUsesDevice::getID()
{
    return id.c_str();
}


const char* SPDUsesDevice::getType()
{
    return type.c_str();
}


CF::Properties SPDUsesDevice::getPropertyRefs() const
{
    return propertyRefs;
}
