/*******************************************************************************

Copyright 2004, 2006, 2007, 2008 Virginia Polytechnic Institute and State University

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

#include <assert.h>

#include "ossie/debug.h"
#include "ossie/PRFSimpleProperty.h"

#include "tinyxml.h"

PRFProperty::PRFProperty (TiXmlElement *elem) : dataType(NULL)
{
    DEBUG(4, PRFProperty, "In constructor.");
    dataType = new CF::DataType();
    parseElement(elem);
}


PRFProperty::~PRFProperty()
{
    DEBUG(4, PRFProperty, "In destructor.");

    delete dataType;
}


void PRFProperty::parseElement(TiXmlElement *elem)
{
    DEBUG(5, PRFProperty, "In parseElement.");

    id = elem->Attribute("id");

    dataType->id = CORBA::string_dup(id.c_str());

    type = elem->Attribute("type");

    name = elem->Attribute("name");

    mode = elem->Attribute("mode");

    parseKind(elem);
    parseAction(elem);
}

void PRFProperty::parseKind(TiXmlElement *elem)
{
    DEBUG(5, PRFProperty, "In parseKind.");

    TiXmlElement *kind = elem->FirstChildElement("kind");
    assert(kind);

    DEBUG(9, PRFProperty, "Looping through kind tags.");

    for ( ; kind; kind = kind->NextSiblingElement("kind")) {
        const char *str = kind->Attribute("kindtype");
        assert(str);
        simpleKinds.push_back(str);
    }
    DEBUG(8, PRFProperty, "Leaving parseKind.");
}

void PRFProperty::parseAction(TiXmlElement *elem)
{
    DEBUG(5, PRFProperty, "In parseAction.");

    TiXmlElement *act = elem->FirstChildElement("action");

    if (act) {
        action = act->Attribute("type");
    }
}

bool PRFProperty::isAllocation()
{
    DEBUG(5, PRFProperty, "In isAllocation.");

    for (unsigned int i = 0; i < simpleKinds.size (); i++) {
        if (simpleKinds[i] == "allocation")
            return true;
    }

    return false;
}

bool PRFProperty::isConfigure()
{
    DEBUG(5, PRFProperty, "In isConfigure.");

    for (unsigned int i = 0; i < simpleKinds.size (); i++) {
        if (simpleKinds[i] == "configure")
            return true;
    }

    return false;
}

bool PRFProperty::isTest()
{
    DEBUG(5, PRFProperty, "In isTest.");

    for (unsigned int i = 0; i < simpleKinds.size (); i++) {
        if (simpleKinds[i] == "test")
            return true;
    }

    return false;
}

bool PRFProperty::isExecParam()
{
    DEBUG(5, PRFProperty, "In isExecParam.");

    for (unsigned int i = 0; i < simpleKinds.size (); i++) {
        if (simpleKinds[i] == "execparam")
            return true;
    }

    return false;
}

bool PRFProperty::isFactoryParam()
{
    DEBUG(5, PRFProperty, "In isFactoryParam.");

    for (unsigned int i = 0; i < simpleKinds.size (); i++) {
        if (simpleKinds[i] == "factoryparam")
            return true;
    }

    return false;
}

const char* PRFProperty::getID()
{
    return id.c_str();
}

const char* PRFProperty::getType()
{
    return type.c_str();
}

const char* PRFProperty::getName()
{
    return name.c_str();
}

const char* PRFProperty::getMode()
{
    return mode.c_str();
}

std::vector<std::string> PRFProperty::getValue()
{
    return value;
}

const char* PRFProperty::getAction()
{
    return action.c_str();
}

CF::DataType* PRFProperty::getDataType() const
{
    return dataType;
}

std::vector <std::string> PRFProperty::getKinds()
{
    return simpleKinds;
}

bool PRFProperty::isBoolean()
{
    return (type == "boolean");
}

bool PRFProperty::isChar()
{
    return (type == "char");
}

bool PRFProperty::isDouble()
{
    return (type == "double");
}

bool PRFProperty::isFloat()
{
    return (type == "float");
}

bool PRFProperty::isShort()
{
    return (type == "short");
}

bool PRFProperty::isUShort()
{
    return (type == "ushort");
}

bool PRFProperty::isLong()
{
    return (type == "long");
}

bool PRFProperty::isObjref()
{
    return (type == "objref");
}

bool PRFProperty::isOctet()
{
    return (type == "octet");
}

bool PRFProperty::isString()
{
    return (type == "string");
}

bool PRFProperty::isULong()
{
    return (type == "ulong");
}

bool PRFProperty::isUshort()
{
    return (type == "ushort");
}

bool PRFProperty::isReadOnly()
{
    return (type == "readonly");
}

bool PRFProperty::isReadWrite()
{
    return (type == "readwrite");
}

bool PRFProperty::isWriteOnly()
{
    return (type == "writeonly");
}

bool PRFProperty::isEqual()
{
    return (action == "eq");
}

bool PRFProperty::isNotEqual()
{
    return (action == "ne");
}


bool PRFProperty::isGreaterThan()
{
    return (action == "gt");
}

bool PRFProperty::isLessThan()
{
    return (action == "lt");
}

bool PRFProperty::isGreaterThanOrEqual()
{
    return (action == "ge");
}


bool PRFProperty::isLessThanOrEqual()
{
    return (action == "le");
}

bool PRFProperty::isExternal()
{
    return (action == "external");
}
