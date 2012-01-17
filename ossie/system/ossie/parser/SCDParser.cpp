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

#include <string>
#include <iostream>

#include "ossie/cf.h"
#include "ossie/SCDParser.h"

#include "tinyxml.h"

SCDParser::SCDParser(CF::File_ptr file)
{
    DEBUG(4, SCDParser, "In constructor.");

    unsigned long fileSize = file->sizeOf();
    CF::OctetSequence_var fileData;

    file->read(fileData, fileSize);

    unsigned char *fileBuffer = fileData->get_buffer();

    XMLDoc.Parse((const char *)fileBuffer);

    TiXmlElement *elem = XMLDoc.FirstChildElement("softwarecomponent");
    if (!elem) {
        DEBUG(1, SCDParser, "Bad SCD file.");
    }

    parseFile(elem);
}


SCDParser::~SCDParser ()
{
}


void SCDParser::parseFile(TiXmlElement *elem)
{
    DEBUG(4, SCDParser, "In parseFile.");

    parseComponentType(elem);
    parsePRFRef(elem);
}


void SCDParser::parseComponentType(TiXmlElement *elem)
{
    TiXmlElement *type = elem->FirstChildElement("componenttype");

    if (type) {
        componentType = type->GetText();;
    }
}


void SCDParser::parsePRFRef(TiXmlElement *elem)
{
    TiXmlElement *prf = elem->FirstChildElement("propertyfile");

    if (prf) {
        TiXmlElement *local = elem->FirstChildElement("localfile");
        if (local)
            PRFFile = local->Attribute("name");;
    }
}


bool SCDParser::isDevice()
{
    if (componentType == "device")
        return true;
    else
        return false;
}


bool SCDParser::isResource()
{
    if (componentType == "resource")
        return true;
    else
        return false;
}


bool SCDParser::isApplication()
{
    if (componentType == "application")
        return true;
    else
        return false;
}


bool SCDParser::isDomainManager()
{
    if (componentType == "domainmanager")
        return true;
    else
        return false;
}


bool SCDParser::isDeviceManager()
{
    if (componentType == "devicemanager")
        return true;
    else
        return false;
}


bool SCDParser::isService()
{
    if ((componentType == "logger") ||
            (componentType == "filemanager") ||
            (componentType == "filesystem")) {
        return true;
    } else
        return false;
}


bool SCDParser::isConfigurable()
{
    if ((componentType == "resource") ||
            (componentType == "application") ||
            (componentType == "devicemanager") ||
            (isDevice ()) || (isDomainManager ())) {
        return true;
    } else
        return false;
}

const char* SCDParser::getComponentType()
{
    return componentType.c_str();
}


const char* SCDParser::getPRFFile()
{
    return PRFFile.c_str();
}
