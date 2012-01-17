/*******************************************************************************
 Copyright 2009 Virginia Polytechnic Institute and State University

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

#include "ossie/DevicePkgRef.h"

#include <iostream>

DevicePkgRef::DevicePkgRef(TiXmlElement *elem)
{
    DEBUG(4, DevicePkgRef, "In constructor.");
    parseElement(elem);
}

DevicePkgRef::~DevicePkgRef()
{
}

void DevicePkgRef::parseElement(TiXmlElement *elem)
{
    DEBUG(4, DevicePkgRef, "in parseElement");

    parseAttributes(elem); // store type from devicepkgref tag
    parseChildElements(elem);

}

void DevicePkgRef::parseAttributes(TiXmlElement *elem)
{
    DEBUG(4, DevicePkgRef, "in parseAttributes");

    if (elem->Attribute("type")) {
        type = elem->Attribute("type");
    } else {
        std::cout << "ERROR: Could not find 'type' attribute in devicepkgref tag" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void DevicePkgRef::parseChildElements(TiXmlElement *elem)
{
    DEBUG(4, DevicePkgRef, "in parseChildElements");

    TiXmlElement *local = elem->FirstChildElement("localfile");
    if (local) {
        localfile = local->Attribute("name");
    } else {
        std::cout << "ERROR: Could not find 'name' attribute in localfile tag" << std::endl;
        exit(EXIT_FAILURE);
    }
}

std::string DevicePkgRef::getType()
{
    return type;
}

std::string DevicePkgRef::getLocalFile()
{
    return localfile;
}


