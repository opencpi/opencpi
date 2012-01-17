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

#include "ossie/ComponentPlacement.h"
#include "ossie/debug.h"

ComponentPlacement::ComponentPlacement (TiXmlElement *elem) : root(elem)
{
    DEBUG(4, ComponentPlacement, "In constructor.");
    parseElement(elem);
}

ComponentPlacement::~ComponentPlacement()
{
    for (unsigned int i=0; i < instantiations.size(); i++) {
        delete instantiations[i];
    }
}



void ComponentPlacement::parseElement(TiXmlElement *elem)
{
    parseFileRef(elem);
}


void ComponentPlacement::parseFileRef (TiXmlElement *elem)
{
    DEBUG(4, ComponentPlacement, "In parseFileRef.");

    if (elem) {
        TiXmlElement *fileRef = elem->FirstChildElement("componentfileref");
        _fileRefId = fileRef->Attribute("refid");
    } else {
        //Invalid Profile
        std::cerr<<"Invalid profile cannot parse the refid in componentplacement"<<std::endl;
    }
}

ComponentInstantiation*
ComponentPlacement::getInstantiationById (const char *_id)
{
    for (unsigned int i = 0; i < instantiations.size(); i++) {
        if (strcmp (instantiations[i]->getID (), _id) == 0)
            return instantiations[i];
    }

    return NULL;
}


const char *ComponentPlacement::getFileRefId()
{
    return _fileRefId.c_str();
}

std::vector <ComponentInstantiation*>*
ComponentPlacement::getInstantiations()
{
    return &instantiations;
}
