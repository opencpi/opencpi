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

#ifndef SCDPARSER_H
#define SCDPARSER_H

#include <string>

#include "ossie/ossieparser.h"
#include "ossie/cf.h"
#include "ossie/debug.h"

#include "tinyxml.h"


class OSSIEPARSER_API SCDParser
{
private:
    SCDParser();  // No default Constructor
    SCDParser(SCDParser &); // No copying

    TiXmlDocument XMLDoc;

    std::string componentType;
    std::string SCDFile;
    std::string PRFFile;

protected:
    void parseFile(TiXmlElement *elem);
    void parseComponentType(TiXmlElement *elem);
    void parsePRFRef(TiXmlElement *elem);

public:
    SCDParser(CF::File_ptr file);
    ~SCDParser();

    const char* getComponentType();
    const char* getPRFFile();
    bool isDevice();
    bool isResource();
    bool isApplication();
    bool isDomainManager();
    bool isDeviceManager();
    bool isService();
    bool isConfigurable();
};
#endif
