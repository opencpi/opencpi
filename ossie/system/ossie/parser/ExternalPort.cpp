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
on the architecture of the CRCs SCA Reference Implementation(SCARI)
see: http://www.crc.ca/en/html/rmsc/home/sdr/projects/scari

*********************************************************************************/

#include "ossie/ExternalPort.h"

#include "tinyxml.h"

ExternalPort::ExternalPort(TiXmlElement *elem):
        ifUsesIdentifier(false),
        ifProvidesIdentifier(false),
        ifSupportedIdentifier(false)
{
    parseElement(elem);
}

ExternalPort::~ExternalPort()
{
}


void ExternalPort::parseElement(TiXmlElement *elem)
{
    parseUsesIdentifier(root);

    if (!isUsesIdentifier()) {
        parseProvidesIdentifier(root);
    }
    if (!isProvidesIdentifier()) {
        parseSupportedIdentifier(root);
    }

    parseComponentInstantiationRefId(root);
}


void ExternalPort::parseUsesIdentifier(TiXmlElement *elem)
{
    TiXmlElement *usesId = elem->FirstChildElement("usesidentifier");

    if (!usesId)
        return;

    usesIdentifier = usesId->Value();
    ifUsesIdentifier = true;
}


void ExternalPort::parseProvidesIdentifier(TiXmlElement *elem)
{
    TiXmlElement *providesId = elem->FirstChildElement("providesidentifier");

    if (!providesId)
        return;

    providesIdentifier = providesId->Value();
    ifProvidesIdentifier = true;

}


void ExternalPort::parseSupportedIdentifier(TiXmlElement *elem)
{
    TiXmlElement *supportedId = elem->FirstChildElement("supportedidentifier");

    if (!supportedId)
        return;

    supportedIdentifier = elem->Value();
    ifSupportedIdentifier = true;

}


void ExternalPort::parseComponentInstantiationRefId(TiXmlElement *elem)
{
    TiXmlElement *refId = elem->FirstChildElement("componentinstantiationref");

    if (!refId)
        return;

    componentInstantiationRefId = elem->Attribute("refid");
}


bool ExternalPort::isUsesIdentifier()
{
    return ifUsesIdentifier;
}


bool ExternalPort::isProvidesIdentifier()
{
    return ifProvidesIdentifier;
}


bool ExternalPort::isSupportedIdentifier()
{
    return ifSupportedIdentifier;
}


const char* ExternalPort::getUsesIdentifier()
{
    return usesIdentifier.c_str();
}


const char* ExternalPort::getProvidesIdentifier()
{
    return providesIdentifier.c_str();
}


const char* ExternalPort::getSupportedIdentifier()
{
    return supportedIdentifier.c_str();
}


const char* ExternalPort::getComponentInstantiationRefId()
{
    return componentInstantiationRefId.c_str();
}
