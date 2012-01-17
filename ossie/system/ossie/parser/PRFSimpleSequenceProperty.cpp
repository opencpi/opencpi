/*******************************************************************************

Copyright 2004, 2006, 2007 Virginia Polytechnic Institute and State University

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

#include <ossie/prop_helpers.h>
#include "ossie/PRFSimpleSequenceProperty.h"
#include "ossie/debug.h"

#include "tinyxml.h"

PRFSimpleSequenceProperty::PRFSimpleSequenceProperty (TiXmlElement *elem) : PRFProperty(elem)
{
    DEBUG(4, PRFSimpleSequenceProperty, "In Constructor");

    TiXmlElement *vals = elem->FirstChildElement("values");
    extract_strings_from_element(vals, value);

// The sequences should be freed when the datatype goes away
    if (isBoolean()) {
        dataType->value <<= ossieSupport::strings_to_boolean_sequence(value);
    } else if (isChar()) {
        dataType->value <<= ossieSupport::strings_to_char_sequence(value);
    } else if (isDouble()) {
        dataType->value <<= ossieSupport::strings_to_double_sequence(value);
    } else if (isFloat()) {
        dataType->value <<= ossieSupport::strings_to_float_sequence(value);
    } else if (isShort()) {
        dataType->value <<= ossieSupport::strings_to_short_sequence(value);
    } else if (isLong()) {
        dataType->value <<= ossieSupport::strings_to_long_sequence(value);
    } else if (isOctet()) {
        dataType->value <<= ossieSupport::strings_to_octet_sequence(value);
    } else if (isUShort()) {
        dataType->value <<= ossieSupport::strings_to_unsigned_short_sequence(value);
    } else if (isULong()) {
        dataType->value <<= ossieSupport::strings_to_unsigned_long_sequence(value);
    } else if (isString()) {
        dataType->value <<= ossieSupport::strings_to_string_sequence(value);
    }
}


PRFSimpleSequenceProperty::~PRFSimpleSequenceProperty()
{

}

void PRFSimpleSequenceProperty::extract_strings_from_element(TiXmlElement *elem, std::vector<std::string> &value)
{
    DEBUG(5, PRFSimpleSequenceProperty, "Looking for values.");

    TiXmlElement *val = elem->FirstChildElement("value");

    DEBUG(8, PRFSimpleSequenceProperty, "Found a value.");
    for ( ; val; val = val->NextSiblingElement("value")) {
        std::string str = val->GetText();
        DEBUG(8, PRFSimpleSequenceProperty, "Found value: " << str);
        value.push_back(str);
    }
}
