/*******************************************************************************

Copyright 2004,2006, 2007 Virginia Polytechnic Institute and State University

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

#ifndef PRFPROPERTY_H
#define PRFPROPERTY_H

#include <vector>
#include <string>

#include "ossie/ossieparser.h"
#include "ossie/cf.h"

#include "tinyxml.h"

class OSSIEPARSER_API PRFProperty
{
public:
    PRFProperty(TiXmlElement *elem);
    virtual ~PRFProperty();

    bool isBoolean();
    bool isChar();
    bool isDouble();
    bool isFloat();
    bool isShort();
    bool isUShort();
    bool isLong();
    bool isObjref();
    bool isOctet();
    bool isString();
    bool isULong();
    bool isUshort();
    bool isReadOnly();
    bool isReadWrite();
    bool isWriteOnly();
    bool isAllocation();
    bool isConfigure();
    bool isTest();
    bool isExecParam();
    bool isFactoryParam();
    bool isEqual();
    bool isNotEqual();
    bool isGreaterThan();
    bool isLessThan();
    bool isGreaterThanOrEqual();
    bool isLessThanOrEqual();
    bool isExternal();

    const char* getPropType();
    const char* getID();
    const char* getType();
    const char* getName();
    const char* getMode();
    std::vector <std::string> getValue();
    const char* getAction();

    CF::DataType* getDataType() const;
    std::vector <std::string> getKinds();


protected:
    CF::DataType* dataType;
    std::vector<std::string> value;


private:
    PRFProperty();
    PRFProperty(const PRFProperty & _prfSimpleProp); // No copying

    void parseElement(TiXmlElement *elem);
    void parseKind(TiXmlElement *elem);
    void parseAction (TiXmlElement *elem);

    std::string prop_type;
    std::string id;
    std::string type;
    std::string name;
    std::string mode;
    std::string action;
    std::vector <std::string> simpleKinds;

};
#endif
