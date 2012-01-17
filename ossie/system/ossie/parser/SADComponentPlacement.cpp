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

#include <iostream>

#include "ossie/SADComponentPlacement.h"
#include "ossie/debug.h"

#include "tinyxml.h"

SADComponentPlacement::SADComponentPlacement(TiXmlElement *elem) : ComponentPlacement(elem)
{
    DEBUG(4, SADParser, "In SADComponentPlacement constructor.");
    parseElement(elem);

    for (unsigned int i = 0; i < instantiations.size(); i++)
        sadComp.push_back((SADComponentInstantiation* ) instantiations[i]);

    ComponentPlacement::parseElement(elem);
}


SADComponentPlacement::~SADComponentPlacement()
{
    for (unsigned int i=0; i < instantiations.size(); i++) {
        delete instantiations[i];
    }
}

void SADComponentPlacement::parseElement(TiXmlElement *elem)
{
    parseFileRef(elem);
    parseInstantiations(elem);
}


void SADComponentPlacement::parseInstantiations(TiXmlElement *elem)
{
    DEBUG(4, SADComponentPlacement, "In parseInstantiations.");

    TiXmlElement *instance = elem->FirstChildElement("componentinstantiation");
    for (; instance; instance = instance->NextSiblingElement()) {
        DEBUG(6, SADComponentPlacement, "Found componentinstantiation.");

        SADComponentInstantiation* SADInstance = new SADComponentInstantiation(instance);
        instantiations.push_back((ComponentInstantiation* ) SADInstance);
    }
}


std::vector <SADComponentInstantiation*> *SADComponentPlacement::getSADInstantiations()
{
    return &sadComp;
}


SADComponentInstantiation* SADComponentPlacement::getSADInstantiationById(char* _id) const
{
    for (unsigned int i = 0; i < instantiations.size(); i++) {
        if (strcmp(instantiations[i]->getID(), _id) == 0)
            return (SADComponentInstantiation* ) instantiations[i];
    }

    return NULL;
}
