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

#ifndef COMPONENTINSTANTIATION_H
#define COMPONENTINSTANTIATION_H

#include <string>
#include <vector>

#include "ossie/InstantiationProperty.h"

#include "tinyxml.h"

class OSSIEPARSER_API ComponentInstantiation
{
public:
    ComponentInstantiation(TiXmlElement *);
    virtual ~ComponentInstantiation();

    const char* getID();
    const char* getUsageName();
    std::vector <InstantiationProperty*>* getProperties();

protected:
    TiXmlElement *root;
    void parseElement();
    void parseID(TiXmlElement *);
    void parseName(TiXmlElement *);
    void parseProperties(TiXmlElement *);
    InstantiationProperty* parseSimpleRef(TiXmlElement *);
    InstantiationProperty* parseSimpleSequenceRef(TiXmlElement *);

private:
    ComponentInstantiation();
    ComponentInstantiation(const ComponentInstantiation &); // No copying

    std::string instantiationId;
    std::string usageName;
    std::vector <InstantiationProperty*> properties;
};
#endif
