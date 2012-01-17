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
#include <string>

#include "ossie/cf.h"

#include "ossie/ComponentAssemblyParser.h"
#include "ossie/debug.h"

ComponentAssemblyParser::ComponentAssemblyParser(CF::File_ptr file)
{
    DEBUG(6, ComponentAssemblyParser, "Entering constructor with");

    unsigned long fileSize = file->sizeOf();
    CF::OctetSequence_var fileData;

    file->read(fileData, fileSize);

    unsigned char *fileBuffer = fileData->get_buffer();

    XMLDoc.Parse((const char *)fileBuffer);

// Handle SAD or DCD files
    root = XMLDoc.FirstChild("softwareassembly");
    if (!root)
        root = XMLDoc.FirstChild("deviceconfiguration");

    TiXmlHandle docHandle(root);

    parseIdAndName (docHandle);
    parseConnections (docHandle);
}


ComponentAssemblyParser::~ComponentAssemblyParser ()
{
}


void ComponentAssemblyParser::parseIdAndName(TiXmlHandle &docHandle)
{
    DEBUG(4, ComponentAssemblyParser, "In parseIdAndName.");

    TiXmlElement *SWAssembly = docHandle.Element();
    if (!SWAssembly) {
        DEBUG(1, ComponentAssemblyParser, "Failed to retrieve root element tag.");
        throw CF::ApplicationFactory::CreateApplicationError(CF::CFNOTSET , "XML tag software assembly not found.");
    }

    id = SWAssembly->Attribute("id");
    name = SWAssembly->Attribute("name");
}


void ComponentAssemblyParser::parseConnections(TiXmlHandle &docHandle)
{
    DEBUG(4, ComponentAssemblyParser, "In parseConnections.");

    TiXmlElement* connection = docHandle.FirstChild("connections").FirstChild("connectinterface").ToElement();

    for (; connection; connection = connection->NextSiblingElement()) {
        DEBUG(9, ComponentAssemblyParser, "Found connectinterface.");

        connections.push_back(new Connection(connection));
    }
}

std::vector <Connection*>* ComponentAssemblyParser::getConnections()
{
    return &connections;
}

const char* ComponentAssemblyParser::getFileName()
{
    return fileName.c_str();
}

const char* ComponentAssemblyParser::getID()
{
    return id.c_str();
}

const char* ComponentAssemblyParser::getName()
{
    return name.c_str();
}
