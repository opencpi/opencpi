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

#include <string>
#include <iostream>

#include "ossie/ComponentInstantiation.h"
#include "ossie/debug.h"

#include "tinyxml.h"

ComponentInstantiation::ComponentInstantiation (TiXmlElement *element):
        root(element)
{
    DEBUG(4, ComponentInstantiation, "In constructor.");
    parseElement();
}

ComponentInstantiation::~ComponentInstantiation()
{

    for (unsigned int i=0; i< properties.size(); i++) {
        delete properties[i];
    }
}


void ComponentInstantiation::parseElement()
{
    parseID(root);
    parseName(root);
    parseProperties(root);
}

void ComponentInstantiation::parseID(TiXmlElement *elem)
{
    DEBUG(4, ComponentInstantiation, "In parseID.");

    instantiationId = elem->Attribute("id");

    DEBUG(8, ComponentInstantiation, "Found instantiation ID " << instantiationId);
}


void ComponentInstantiation::parseName(TiXmlElement *elem)
{
    DEBUG(4, ComponentInstantiation, "In parseName.");

    TiXmlElement *usage = elem->FirstChildElement("usagename");
    if (usage)
        usageName = usage->GetText();

    DEBUG(8, ComponentInstantiation, "Found usage name " << usageName);
}


void ComponentInstantiation::parseProperties(TiXmlElement *elem)
{
    DEBUG(4, ComponentInstantiation, "In parseProperties.");

    TiXmlElement *cp = elem->FirstChildElement("componentproperties");
    if (cp) {
        TiXmlElement *prop = cp->FirstChildElement();

        for (; prop; prop = prop->NextSiblingElement()) {
            std::string str = prop->ValueStr();
            DEBUG(8, ComponentInstantiation, "Found prop: " << str);

            if (str == "simpleref") {
                DEBUG(8, ComponentInstantiation, "Found simpleref.");
                InstantiationProperty *i_prop = parseSimpleRef (prop);
                properties.push_back(i_prop);

            } else if (str == "simplesequenceref") {
                DEBUG(8, ComponentInstantiation, "Found simplesequence.");
                InstantiationProperty *i_prop = parseSimpleSequenceRef (prop);
                properties.push_back(i_prop);
            }
        }
    }
    DEBUG(4, ComponentInstantiation, "Leaving parseProperties.");
}


InstantiationProperty* ComponentInstantiation::parseSimpleRef(TiXmlElement *elem)
{
    DEBUG(4, ComponentInstantiation, "In parseSimpleRef.");

    const char *id = elem->Attribute("refid");
    const char *value = elem->Attribute("value");

    InstantiationProperty* property = new InstantiationProperty(id, value);
    return property;
}

InstantiationProperty* ComponentInstantiation::parseSimpleSequenceRef(TiXmlElement *elem)
{
    DEBUG(4, ComponentInstantiation, "In parseSimpleSequence.");

    const char *id = elem->Attribute("refid");

    DEBUG(8, ComponentInstantiation, "Found refid: " << id);

    InstantiationProperty* property = new InstantiationProperty(id);

    TiXmlElement *values = elem->FirstChildElement("values");
    TiXmlElement *value = values->FirstChildElement("value");
    DEBUG(8, ComponentInstantiation, "Found value.");

    for (; value; value = value->NextSiblingElement("value")) {
        DEBUG(9, ComponentInstantiation, "Found value: " << value->GetText());
        property->setValue(value->GetText());
    }

    return property;
}


const char* ComponentInstantiation::getID()
{
    return instantiationId.c_str();
}


const char* ComponentInstantiation::getUsageName()
{
    return usageName.c_str();
}


std::vector <InstantiationProperty*>* ComponentInstantiation::getProperties()
{
    return &properties;
}
