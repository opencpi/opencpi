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

#include <iostream>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "ossie/cf.h"
#include "ossie/Port.h"

///\todo Figure out why this is used because it probably does not work
Port::Port()
{}

Port::Port (TiXmlElement* element):
        findBy(NULL),
        ifComponentInstantiationRef(false),
        ifDeviceThatLoadedThisComponentRef(false),
        ifDeviceUsedByThisComponentRef(false),
        ifFindBy(false)
{
    parsePort(element);
}

Port::Port (const Port &aPort):
        findBy(NULL),
        ifComponentInstantiationRef(aPort.ifComponentInstantiationRef),
        ifDeviceThatLoadedThisComponentRef(aPort.ifDeviceThatLoadedThisComponentRef),
        ifDeviceUsedByThisComponentRef(aPort.ifDeviceUsedByThisComponentRef),
        ifFindBy(aPort.ifFindBy)
{
    std::cout << "In Port copy constructor" << std::endl;


    componentInstantiationRefId = aPort.componentInstantiationRefId;

    deviceThatLoadedThisComponentRefId =aPort.deviceThatLoadedThisComponentRefId;

    deviceUsedByThisComponentRefId = aPort.deviceUsedByThisComponentRefId;

    deviceUsedByThisComponentRefUsesRefId = aPort.deviceUsedByThisComponentRefUsesRefId;
}


Port::~Port()
{
}


// \todo implement exception handling for parsePort
void Port::parsePort (TiXmlElement *port)
{

    TiXmlElement *elem;
    if (elem = port->FirstChildElement("componentinstantiationref")) {
        parseComponentInstantiationRef (elem);

    } else if (elem = port->FirstChildElement("devicethatloadedthiscomponentref")) {
        parseDeviceThatLoadedThisComponentRef (elem);

    } else if (elem = port->FirstChildElement("findby")) {
        parseFindBy (elem);

    } else if (elem = port->FirstChildElement("deviceusedbythiscomponentref")) {
        parseDeviceUsedByThisComponentRef (elem);

    } else if (!(ifFindBy || ifComponentInstantiationRef || ifDeviceThatLoadedThisComponentRef || ifDeviceUsedByThisComponentRef)) {
        throw CF::ApplicationFactory::CreateApplicationError(CF::CFNOTSET , "Bad port data.");

    }

}


void Port::parseComponentInstantiationRef(TiXmlElement *elem)
{
    componentInstantiationRefId = elem->Attribute("refid");
    ifComponentInstantiationRef = true;
}


void Port::parseDeviceThatLoadedThisComponentRef (TiXmlElement *elem)
{
    deviceThatLoadedThisComponentRefId = elem->Attribute("refid");
    ifDeviceThatLoadedThisComponentRef = true;
}


void Port::parseFindBy(TiXmlElement *elem)
{
    findBy = new FindBy(elem);
    ifFindBy = true;
}


void Port::parseDeviceUsedByThisComponentRef(TiXmlElement *elem)
{
    deviceUsedByThisComponentRefId = elem->Attribute("refid");
    deviceUsedByThisComponentRefUsesRefId = elem->Attribute("usesrefid");
    ifDeviceUsedByThisComponentRef = true;
}
