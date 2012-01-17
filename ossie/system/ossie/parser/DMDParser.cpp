/*******************************************************************************

Copyright 2004, 2007, Virginia Polytechnic Institute and State University

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
#include "ossie/DMDParser.h"
#include "ossie/debug.h"

DMDParser::DMDParser(CF::File_ptr file)
{
    DEBUG(4, DMDParser, "In constructor.");

    unsigned long fileSize = file->sizeOf();
    CF::OctetSequence_var fileData;

    file->read(fileData, fileSize);

    unsigned char *fileBuffer = fileData->get_buffer();

    XMLDoc.Parse((const char *)fileBuffer);

    TiXmlElement *elem = XMLDoc.FirstChildElement("domainmanagerconfiguration");
    if (!elem) {
        DEBUG(1, DMDParser, "Bad DMD file.");
    }

    parseFile(elem);
    parseDomainManagerSoftPkg (elem);
}


DMDParser::~DMDParser ()
{
    DEBUG(4, DMDParser, "In destructor.");

}


void DMDParser::parseFile(TiXmlElement *elem)
{
    DEBUG(4, DMDParser, "In parseFile.");

    _dmdId = elem->Attribute("id");
    _dmdName = elem->Attribute("name");
}


void DMDParser::parseDomainManagerSoftPkg(TiXmlElement *elem)
{
    DEBUG(4, DMDParser, "In parseDomainManagerSoftPkg.");

    TiXmlElement *dmSoft = elem->FirstChildElement("domainmanagersoftpkg");

    if (dmSoft) {
        TiXmlElement *local = dmSoft->FirstChildElement("localfile");
        domainManagerSoftPkg = local->Attribute("name");
        DEBUG(8, DMDParser, "Found DM soft pkg " << domainManagerSoftPkg);
    }
}

const char* DMDParser::getDomainManagerSoftPkg()
{
    return domainManagerSoftPkg.c_str();
}
