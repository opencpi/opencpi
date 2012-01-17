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
on the architecture of the CRCs SCA Reference Implementation (SCARI)
see: http://www.crc.ca/en/html/rmsc/home/sdr/projects/scari

*********************************************************************************/

#include "ossie/UsesPort.h"
#include "ossie/debug.h"

UsesPort::UsesPort(TiXmlElement *root) : Port(root)
{
    DEBUG(9, UsesPort, "In constructor.");

    Port::parsePort(root);    // call the base class first
    parseID(root);
}


// copy constructor
UsesPort::UsesPort(const UsesPort & _up)
{
    this->ifComponentInstantiationRef = _up.ifComponentInstantiationRef;
    this->ifDeviceThatLoadedThisComponentRef =
        _up.ifDeviceThatLoadedThisComponentRef;
    this->ifDeviceUsedByThisComponentRef = _up.ifDeviceUsedByThisComponentRef;
    this->ifFindBy = _up.ifFindBy;

    identifier = _up.identifier;
}

UsesPort::~UsesPort()
{
}


void UsesPort::parseID(TiXmlElement *elem)
{
    TiXmlElement *usesId = elem->FirstChildElement("usesidentifier");
    identifier = usesId->GetText();

    DEBUG(9, UsesPort, "Found uses port " << identifier);
}

const char* UsesPort::getID()
{
    return identifier.c_str();
}
