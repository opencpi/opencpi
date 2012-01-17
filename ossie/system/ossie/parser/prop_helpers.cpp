/*******************************************************************************

Copyright 2008, Virginia Polytechnic Institute and State University

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

*/

#include <string>
#include <vector>
#include <cstdlib>
#ifdef HAVE_OMNIORB4_CORBA_H
#include "omniORB4/CORBA.h"
#endif

#include <ossie/prop_helpers.h>

static std::vector<std::string> trueValues;

CORBA::Boolean ossieSupport::strings_to_boolean(std::vector<std::string> &values)
{
    trueValues.push_back("true");
    trueValues.push_back("t");
    trueValues.push_back("on");
    trueValues.push_back("yes");
    trueValues.push_back("1");

    std::string localValue = values[0];

    for(int i = 0; localValue[i] != '\0'; i++)
        localValue[i] = tolower(localValue[i]);

    for(int i = 0; i < 5; i++)
    {
    	if(trueValues[i] == localValue)
    		return CORBA::Boolean(true);
    }

    return CORBA::Boolean(false);
}

CORBA::Char ossieSupport::strings_to_char(std::vector<std::string> &values)
{
    CORBA::Char result(' ');

    result = values[0][0];

    return result;
}

CORBA::Double ossieSupport::strings_to_double(std::vector<std::string> &values)
{
    CORBA::Double result(0);

    result = strtod(values[0].c_str(), NULL);

    return result;
}

CORBA::Float ossieSupport::strings_to_float(std::vector<std::string> &values)
{
    CORBA::Float result(0);

    result = strtof(values[0].c_str(), NULL);

    return result;
}

CORBA::Short ossieSupport::strings_to_short(std::vector<std::string> &values)
{
    CORBA::Short result(0);

    result = (short) atoi(values[0].c_str());

    return result;
}

CORBA::Long ossieSupport::strings_to_long(std::vector<std::string> &values)
{
    CORBA::Long result(0);

    result = strtol(values[0].c_str(), NULL, 0);

    return result;
}

CORBA::Octet ossieSupport::strings_to_octet(std::vector<std::string> &values)
{
    CORBA::Octet result(0);

    result = (short) atoi(values[0].c_str());

    return result;
}

CORBA::UShort ossieSupport::strings_to_unsigned_short(std::vector<std::string> &values)
{
    CORBA::UShort result(0);

    result = (unsigned short) atoi(values[0].c_str());

    return result;
}

CORBA::ULong ossieSupport::strings_to_unsigned_long(std::vector<std::string> &values)
{
    CORBA::ULong result(0);

    result = (unsigned long) atoll(values[0].c_str());

    return result;
}

CORBA::String_var ossieSupport::strings_to_string(std::vector<std::string> &values)
{
    CORBA::String_var result;

    result = CORBA::string_dup(values[0].c_str());

    return result;
}

CORBA::BooleanSeq *ossieSupport::strings_to_boolean_sequence(std::vector<std::string> &values)
{
    CORBA::BooleanSeq_var result = new CORBA::BooleanSeq;

    result->length(values.size());
    for (unsigned int i = 0; i < values.size(); ++i) {

        if (values[i] == "true") {
            result[i] = true;
        } else if (values[i] == "false") {
            result[i] = false;
        }
    }
    return result._retn();
}

CORBA::CharSeq *ossieSupport::strings_to_char_sequence(std::vector<std::string> &values)
{
    CORBA::CharSeq_var result = new CORBA::CharSeq;

    result->length(values.size());
    for (unsigned int i = 0; i < values.size(); ++i) {

        result[i] = values[i][0];
    }
    return result._retn();
}

CORBA::DoubleSeq *ossieSupport::strings_to_double_sequence(std::vector<std::string> &values)
{
    CORBA::DoubleSeq_var result = new CORBA::DoubleSeq;

    result->length(values.size());
    for (unsigned int i = 0; i < values.size(); ++i) {
        result[i] = strtod(values[i].c_str(), NULL);
    }

    return result._retn();
}

CORBA::FloatSeq *ossieSupport::strings_to_float_sequence(std::vector<std::string> &values)
{
    CORBA::FloatSeq_var result = new CORBA::FloatSeq;

    result->length(values.size());

    for (unsigned int i = 0; i < values.size(); ++i) {
        result[i] = strtof(values[i].c_str(), NULL);
    }

    return result._retn();
}

CORBA::ShortSeq *ossieSupport::strings_to_short_sequence(std::vector<std::string> &values)
{
    CORBA::ShortSeq_var result = new CORBA::ShortSeq;

    result->length(values.size());

    for (unsigned int i = 0; i < values.size(); ++i) {
        result[i] = (short) atoi(values[i].c_str());
    }

    return result._retn();
}

CORBA::LongSeq *ossieSupport::strings_to_long_sequence(std::vector<std::string> &values)
{
    CORBA::LongSeq_var result = new CORBA::LongSeq;

    result->length(values.size());

    for (unsigned int i = 0; i < values.size(); ++i) {
        result[i] = strtol(values[i].c_str(), NULL, 0);
    }

    return result._retn();
}

CORBA::OctetSeq *ossieSupport::strings_to_octet_sequence(std::vector<std::string> &values)
{
    CORBA::OctetSeq_var result = new CORBA::OctetSeq;

    result->length(values.size());

    for (unsigned int i = 0; i < values.size(); ++i) {
        result[i] = (short) atoi(values[i].c_str());
    }

    return result._retn();
}

CORBA::UShortSeq *ossieSupport::strings_to_unsigned_short_sequence(std::vector<std::string> &values)
{
    CORBA::UShortSeq_var result = new CORBA::UShortSeq;

    result->length(values.size());

    for (unsigned int i = 0; i < values.size(); ++i) {
        result[i] = (unsigned short) atoi(values[i].c_str());
    }

    return result._retn();
}

CORBA::ULongSeq *ossieSupport::strings_to_unsigned_long_sequence(std::vector<std::string> &values)
{
    CORBA::ULongSeq_var result = new CORBA::ULongSeq;

    result->length(values.size());

    for (unsigned int i = 0; i < values.size(); ++i) {
        result[i] = (unsigned long) atoll(values[i].c_str());
    }

    return result._retn();
}

CORBA::StringSeq *ossieSupport::strings_to_string_sequence(std::vector<std::string> &values)
{
    CORBA::StringSeq_var result = new CORBA::StringSeq;

    result->length(values.size());

    for (unsigned int i = 0; i < values.size(); ++i) {
        result[i] = CORBA::string_dup(values[i].c_str());
    }

    return result._retn();
}
