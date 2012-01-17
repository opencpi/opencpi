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

#include "ossie/ComponentSupportedInterface.h"
#include "ossie/debug.h"

ComponentSupportedInterface::ComponentSupportedInterface(TiXmlElement *element):
        identifier(""),
        componentInstantiationRefId(""),
        ifComponentInstantiationRef(false),
        ifFindBy(false),
        theFindBy(NULL)
{
    DEBUG(9, ComponentSupportedInterface, "In constructor.")
    parseID(element);
    parseComponentInstantiationRef(element);
}


ComponentSupportedInterface::~ComponentSupportedInterface ()
{
}


void ComponentSupportedInterface::parseID (TiXmlElement *elem)
{
    DEBUG(9, ComponentSupportedInterface, "In parseID")
    TiXmlElement *id = elem->FirstChildElement("supportedidentifier");
    DEBUG(9, ComponentSupportedInterface, "Parsed ID")
    identifier = id->GetText();
}

void ComponentSupportedInterface::parseComponentInstantiationRef(TiXmlElement *elem)
{
    DEBUG(9, ComponentSupportedInterface, "In parseComponentInstantiationRef")
    TiXmlElement *id = elem->FirstChildElement("componentinstantiationref");
    DEBUG(9, ComponentSupportedInterface, "Parsed ID")
    componentInstantiationRefId = id->Attribute("refid");
    DEBUG(9, ComponentSupportedInterface, "Parsed Reference ID")
}


bool ComponentSupportedInterface::isComponentInstantiationRef()
{
    return ifComponentInstantiationRef;
}


bool ComponentSupportedInterface::isFindBy()
{
    return ifFindBy;
}


const char* ComponentSupportedInterface::getID()
{
    return identifier.c_str();
}


const char* ComponentSupportedInterface::getComponentInstantiationRefId()
{
    return componentInstantiationRefId.c_str();
}


FindBy* ComponentSupportedInterface::getFindBy() const
{
    return theFindBy;
}
