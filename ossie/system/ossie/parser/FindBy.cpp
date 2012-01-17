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

#include "ossie/FindBy.h"
#include "ossie/debug.h"

#include "tinyxml.h"

FindBy::FindBy(TiXmlElement *element):
        ifFindByNamingService(false),
        ifFindByStringifiedObjectRef(false),
        ifFindByDomainFinder(false)
{
    DEBUG(4, FindBy, "In constructor.");

    parseElement(element);
}


FindBy::FindBy(const FindBy& _fb):
        ifFindByNamingService(false),
        ifFindByStringifiedObjectRef(false),
        ifFindByDomainFinder(false)
{
    findByNamingService = _fb.findByNamingService;
    findByStringifiedObjectRef = _fb.findByStringifiedObjectRef;
    findByDomainFinderType = _fb.findByDomainFinderType;
    findByDomainFinderName = _fb.findByDomainFinderName;

    ifFindByNamingService = _fb.ifFindByNamingService;
    ifFindByStringifiedObjectRef = _fb.ifFindByStringifiedObjectRef;
    ifFindByDomainFinder = _fb.ifFindByDomainFinder;
}

FindBy::~FindBy()
{
}


void FindBy::parseElement(TiXmlElement *elem)
{
    parseFindByNamingService(elem);
    if (isFindByNamingService())
        return;

    parseFindByStringifiedObjectRef(elem);
    if (isFindByStringifiedObjectRef())
        return;

    parseFindByDomainFinder (elem);
    if (!isFindByDomainFinder ())
        return;

    std::cout << "Did not find method to locate port in FindBy.cpp" << std::endl;
/// \todo implement exception throwing in  parseElement
//throw an InvalidProfile here

}


void FindBy::parseFindByDomainFinder(TiXmlElement *elem)
{
    DEBUG(5, FindBy, "In parseFindByDomainFinder.");

    TiXmlElement *df = elem->FirstChildElement("domainfinder");

    if (df) {
        findByDomainFinderType = df->Attribute("type");
        if (df->Attribute("name"))
            findByDomainFinderName = df->Attribute("name");
        ifFindByDomainFinder = true;
    }
}

void FindBy::parseFindByNamingService(TiXmlElement *elem)
{
    DEBUG(5, FindBy, "In parseFindByNamingService.");

    TiXmlElement *ns = elem->FirstChildElement("namingservice");

    if (ns) {
        findByNamingService = ns->Attribute("name");
        ifFindByNamingService = true;
    }
}


void FindBy::parseFindByStringifiedObjectRef(TiXmlElement *elem)
{
    DEBUG(5, FindBy, "In parseFindByStringifiedObjectReference.");

    TiXmlElement *sior = elem->FirstChildElement("stringifiedobjectref");

    if (sior) {
        findByStringifiedObjectRef = sior->ValueStr();
        ifFindByStringifiedObjectRef = true;
    }
}

const char* FindBy::getFindByDomainFinderName()
{
    return findByDomainFinderName.c_str();
}


const char* FindBy::getFindByDomainFinderType()
{
    return findByDomainFinderType.c_str();
}


const char* FindBy::getFindByNamingServiceName()
{
    return findByNamingService.c_str();
}


const char* FindBy::getFindByStringifiedObjectRef()
{
    return findByStringifiedObjectRef.c_str();
}


bool FindBy::isFindByDomainFinder()
{
    return ifFindByDomainFinder;
}


bool FindBy::isFindByNamingService()
{
    return ifFindByNamingService;
}


bool FindBy::isFindByStringifiedObjectRef()
{
    return ifFindByStringifiedObjectRef;
}
