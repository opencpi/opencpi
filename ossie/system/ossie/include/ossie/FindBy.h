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

#ifndef FINDBY_H
#define FINDBY_H

#include <string>

#include "ossieparser.h"

#include "tinyxml.h"

class OSSIEPARSER_API FindBy
{
public:
    FindBy(TiXmlElement *element);
    FindBy(const FindBy & _fb);
    ~FindBy();

    const char* getFindByDomainFinderName();
    const char* getFindByDomainFinderType();
    const char* getFindByNamingServiceName();
    const char* getFindByStringifiedObjectRef();
    bool isFindByDomainFinder();
    bool isFindByNamingService();
    bool isFindByStringifiedObjectRef();

private:
    FindBy();  // No default constructor

    bool ifFindByNamingService;
    bool ifFindByStringifiedObjectRef;
    bool ifFindByDomainFinder;
    std::string findByNamingService;
    std::string findByStringifiedObjectRef;
    std::string findByDomainFinderType;
    std::string findByDomainFinderName;
    void parseElement(TiXmlElement *elem);
    void parseFindByDomainFinder(TiXmlElement *elem);
    void parseFindByNamingService(TiXmlElement *elem);
    void parseFindByStringifiedObjectRef(TiXmlElement *elem);

};
#endif
