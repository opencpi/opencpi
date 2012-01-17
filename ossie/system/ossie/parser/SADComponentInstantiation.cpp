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

#include <sstream>

#include "ossie/SADComponentInstantiation.h"
#include "ossie/debug.h"

#include "tinyxml.h"

SADComponentInstantiation::SADComponentInstantiation(TiXmlElement *elem):
        ComponentInstantiation(elem),
        ifResourceFactoryRef(false), ifNamingService(false)
{
    parseElement();
}


SADComponentInstantiation::~SADComponentInstantiation ()
{

    for (unsigned int i=0; i < factoryProperties.size(); i++) {
        delete factoryProperties[i];
    }

}


void SADComponentInstantiation::parseElement()
{
    ComponentInstantiation::parseElement();
    parseFindComponent(root);
}


void SADComponentInstantiation::parseFindComponent(TiXmlElement *elem)
{
    DEBUG(9, SADComponentInstantiation, "In parseFindComponent");
    TiXmlElement * findcomponent = elem->FirstChildElement("findcomponent");
    if (findcomponent) {
        TiXmlElement *tag = findcomponent->FirstChildElement();

        DEBUG(9, SADComponentInstantiation, "Found tag:" << tag->ValueStr());

        if (tag->ValueStr() == "namingservice") {
            findByNamingServiceName = tag->Attribute("name");
            ifNamingService = true;
            DEBUG(9, SADComponentInstantiation, "Found naming service name: " << findByNamingServiceName);


        } else if (tag->ValueStr() == "componentresourcefactoryref") {
            resourceFactoryRefId = tag->Attribute("refid");
            ///\todo Parse resource factory properties
            DEBUG(1, SADComponentInstantiation, "Trying to parse resource factory properties, need to write some more code.");
        }
    }

#if 0
    tmpXMLStr = XMLString::transcode ("resourcefactoryproperties");
    nodeList = tmpElement->getElementsByTagName(tmpXMLStr);
    DELARRAY(tmpXMLStr);

    if (nodeList->getLength() != 0) {
        tmpElement = (DOMElement *) nodeList->item (0);
        tmpXMLStr = XMLString::transcode("simpleref");
        nodeList = tmpElement->getElementsByTagName(tmpXMLStr);
        DELARRAY(tmpXMLStr);

        for (unsigned int i = 0; i < nodeList->getLength(); i++)
            factoryProperties.push_back(parseSimpleRef((DOMElement *) nodeList->item (i)));
    }
}
}
#endif
}



bool SADComponentInstantiation::isResourceFactoryRef()
{
    return ifResourceFactoryRef;
}


bool SADComponentInstantiation::isNamingService()
{
    return ifNamingService;
}


const char* SADComponentInstantiation::getResourceFactoryRefId()
{
    return resourceFactoryRefId.c_str();
}


const char* SADComponentInstantiation::getFindByNamingServiceName()
{
    DEBUG(9, SADComponentInstantiation, "getFindByNamingServiceName returning: " << findByNamingServiceName);
    return findByNamingServiceName.c_str();
}


std::vector <InstantiationProperty* >* SADComponentInstantiation::getFactoryProperties()
{
    return &factoryProperties;
}
