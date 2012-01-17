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

#ifndef PROPHELPERS_H
#define PROPHELPERS_H

#include <string>
#include <vector>

#ifdef HAVE_OMNIORB4_CORBA_H
#include "omniORB4/CORBA.h"
#endif


/**
The ossieSupport namespace contains useful functions used throughout the
OSSIE framework. The classes in this namespace are also useful for SCA
component developers.
*/

namespace ossieSupport
{
CORBA::Boolean strings_to_boolean(std::vector<std::string> &values);
CORBA::Char strings_to_char(std::vector<std::string> &values);
CORBA::Double strings_to_double(std::vector<std::string> &values);
CORBA::Float strings_to_float(std::vector<std::string> &values);
CORBA::Short strings_to_short(std::vector<std::string> &values);
CORBA::Long strings_to_long(std::vector<std::string> &values);
CORBA::Octet strings_to_octet(std::vector<std::string> &values);
CORBA::UShort strings_to_unsigned_short(std::vector<std::string> &values);
CORBA::ULong strings_to_unsigned_long(std::vector<std::string> &values);
CORBA::String_var strings_to_string(std::vector<std::string> &values);

CORBA::BooleanSeq *strings_to_boolean_sequence(std::vector<std::string> &values);
CORBA::CharSeq *strings_to_char_sequence(std::vector<std::string> &values);
CORBA::DoubleSeq *strings_to_double_sequence(std::vector<std::string> &values);
CORBA::FloatSeq *strings_to_float_sequence(std::vector<std::string> &values);
CORBA::ShortSeq *strings_to_short_sequence(std::vector<std::string> &values);
CORBA::LongSeq *strings_to_long_sequence(std::vector<std::string> &values);
CORBA::OctetSeq *strings_to_octet_sequence(std::vector<std::string> &values);
CORBA::UShortSeq *strings_to_unsigned_short_sequence(std::vector<std::string> &values);
CORBA::ULongSeq *strings_to_unsigned_long_sequence(std::vector<std::string> &values);
CORBA::StringSeq *strings_to_string_sequence(std::vector<std::string> &values);

}

#endif
