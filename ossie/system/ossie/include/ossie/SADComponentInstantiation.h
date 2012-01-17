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

#ifndef SADCOMPONENTINSTANTIATION_H
#define SADCOMPONENTINSTANTIATION_H

#include <string>
#include <vector>

#include "ossie/ComponentInstantiation.h"

#include "tinyxml.h"

class OSSIEPARSER_API SADComponentInstantiation:public ComponentInstantiation
{
public:
    SADComponentInstantiation(TiXmlElement *elem);
    virtual ~SADComponentInstantiation();

    bool isResourceFactoryRef();
    bool isNamingService();
    const char* getResourceFactoryRefId();
    const char* getFindByNamingServiceName();
    std::vector <InstantiationProperty*>* getFactoryProperties();

protected:
    void parseElement();
    void parseFindComponent(TiXmlElement * _elem);

private:
    SADComponentInstantiation();
    SADComponentInstantiation(const SADComponentInstantiation & _compInst);

    bool ifResourceFactoryRef;
    bool ifNamingService;
    std::string resourceFactoryRefId;
    std::string findByNamingServiceName;
    std::vector <InstantiationProperty*> factoryProperties;
};
#endif
