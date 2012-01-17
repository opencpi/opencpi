/*******************************************************************************

Copyright 2004, 2005, 2006, 2007 Virginia Polytechnic Institute and State University

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

#include "ossie/PRFSimpleProperty.h"
#include "ossie/prop_helpers.h"

PRFSimpleProperty::PRFSimpleProperty (TiXmlElement *elem) : PRFProperty(elem)
{

    extract_strings_from_element(elem, value);

    if (isBoolean()) {
    	CORBA::Boolean b = ossieSupport::strings_to_boolean(value);
    	dataType->value <<= CORBA::Any::from_boolean(b);
    	//dataType->value <<= ossieSupport::strings_to_boolean(CORBA::Any::from_boolean(value));
    } else if (isChar()) {
    	CORBA::Char c = ossieSupport::strings_to_char(value);
    	dataType->value <<= CORBA::Any::from_char(c);
        //dataType->value <<= ossieSupport::strings_to_char(value);
    } else if (isDouble()) {
        dataType->value <<= ossieSupport::strings_to_double(value);
    } else if (isFloat()) {
        dataType->value <<= ossieSupport::strings_to_float(value);
    } else if (isShort()) {
        dataType->value <<= ossieSupport::strings_to_short(value);
    } else if (isLong()) {
        dataType->value <<= ossieSupport::strings_to_long(value);
    } else if (isOctet()) {
		CORBA::Octet o = ossieSupport::strings_to_octet(value);
		dataType->value <<= CORBA::Any::from_octet(o);
    	//dataType->value <<= ossieSupport::strings_to_octet(value);
    } else if (isUShort()) {
        dataType->value <<= ossieSupport::strings_to_unsigned_short(value);
    } else if (isULong()) {
        dataType->value <<= ossieSupport::strings_to_unsigned_long(value);
    } else if (isString()) {
        //dataType->value <<= CORBA::string_dup(value[0].c_str());
        dataType->value <<= ossieSupport::strings_to_string(value);
    }

}



PRFSimpleProperty::~PRFSimpleProperty()
{

}

void PRFSimpleProperty::extract_strings_from_element(TiXmlElement *elem, std::vector<std::string> &value)
{
    TiXmlElement * val = elem->FirstChildElement("value");

    if (val) {
        std::string str = val->GetText();
        value.push_back(str);
    }
}
