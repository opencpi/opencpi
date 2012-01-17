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

#ifndef EXTERNALPORT_H
#define EXTERNALPORT_H

#include <string>

#include "ossie/ossieparser.h"

#include "tinyxml.h"

class OSSIEPARSER_API ExternalPort
{
protected:
    TiXmlElement* root;
    std::string usesIdentifier;
    std::string providesIdentifier;
    std::string supportedIdentifier;
    std::string componentInstantiationRefId;

    bool ifUsesIdentifier;
    bool ifProvidesIdentifier;
    bool ifSupportedIdentifier;
    void parseElement(TiXmlElement *elem);
    void parseUsesIdentifier(TiXmlElement *elem);
    void parseProvidesIdentifier(TiXmlElement *elem);
    void parseSupportedIdentifier(TiXmlElement *elem);
    void parseComponentInstantiationRefId(TiXmlElement *elem);

public:
    ExternalPort(TiXmlElement *elem);
    ~ExternalPort();

    bool isUsesIdentifier();
    bool isProvidesIdentifier();
    bool isSupportedIdentifier();
    const char* getUsesIdentifier();
    const char* getProvidesIdentifier();
    const char* getSupportedIdentifier();
    const char* getComponentInstantiationRefId();

private:
    ExternalPort(); // No default constructor
    ExternalPort(const ExternalPort & _EP); // No copying

};
#endif
