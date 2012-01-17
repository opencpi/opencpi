/*******************************************************************************

Copyright 2004, 2007, 2008 Virginia Polytechnic Institute and State University

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
#include <string>
#include <sstream>

#include "ossie/cf.h"
#include "ossie/PRFParser.h"
#include "ossie/PRFSimpleProperty.h"
#include "ossie/PRFSimpleSequenceProperty.h"
#include "ossie/debug.h"

#include "tinyxml.h"

PRFParser::PRFParser(CF::File_ptr file)
{
    DEBUG(4, PRFParser, "In constructor.");

    unsigned long fileSize = file->sizeOf();
    CF::OctetSequence_var fileData;

    file->read(fileData, fileSize);

    unsigned char *fileBuffer = fileData->get_buffer();

    XMLDoc.Parse((const char *)fileBuffer);

    TiXmlElement *elem = XMLDoc.FirstChildElement("properties");
    if (!elem) {
        DEBUG(1, PRFParser, "Bad PRF file.");
    }

    parseFile(elem);
}

PRFParser::~PRFParser()
{
    DEBUG(4, PRFParser, "In destructor.");

    for (unsigned int i=0; i< allProperties.size(); i++)
        delete allProperties[i];
}


void PRFParser::parseFile (TiXmlElement *elem)
{
    DEBUG(4, PRFParser, "In parseFile.");

    TiXmlElement *prop = elem->FirstChildElement();


    DEBUG(8, PRFParser, "Process property entries");
    for ( ; prop; prop = prop->NextSiblingElement()) {
        DEBUG(8, PRFParser, "Found property " << prop->ValueStr());
        std::string str = prop->ValueStr();

        if (str == "simple") addProperty (new PRFSimpleProperty(prop));
        if (str == "simplesequence") addProperty (new PRFSimpleSequenceProperty(prop));

    }
    DEBUG(9, PRFParser, "Done processing property entries");

    for (unsigned int i = 0; i < configProperties.size(); i++)
        allProperties.push_back (configProperties[i]);

    for (unsigned int i = 0; i < capacityProperties.size (); i++)
        allProperties.push_back (capacityProperties[i]);

    for (unsigned int i = 0; i < matchingProperties.size (); i++)
        allProperties.push_back (matchingProperties[i]);

    for (unsigned int i = 0; i < execProperties.size (); i++)
        allProperties.push_back (execProperties[i]);

    for (unsigned int i = 0; i < factoryProperties.size (); i++)
        allProperties.push_back (factoryProperties[i]);

}


void PRFParser::addProperty(PRFProperty* _sp)
{

    if (_sp->isAllocation ()) {
        if (_sp->isExternal ()) {
            DEBUG(7, PRFParser, "storing capacity property: " << _sp->getName());
            capacityProperties.push_back (_sp);
        } else {
            DEBUG(7, PRFParser, "storing matching property: " << _sp->getName());
            matchingProperties.push_back (_sp);
        }
    } else {
        if (_sp->isConfigure ()) {
            DEBUG(7, PRFParser, "storing configure property: " << _sp->getName());
            configProperties.push_back (_sp);
        } else {
            if (_sp->isExecParam ()) {
                DEBUG(7, PRFParser, "storing executive property: " << _sp->getName());
                execProperties.push_back (_sp);
            } else {
                if (_sp->isFactoryParam ()) {
                    DEBUG(7, PRFParser, "storing factory property: " << _sp->getName());
                    factoryProperties.push_back (_sp);
                } else {
                    DEBUG(7, PRFParser, "could not find the appropriate property type, deleting.");
                    delete _sp;    // this is unlikely --TTP
                }
            }
        }
    }
}


std::vector<PRFProperty *> *PRFParser::getProperties ()
{
    return &allProperties;
}


std::vector <PRFProperty *> *PRFParser::getConfigureProperties()
{
    return &configProperties;
}


std::vector <PRFProperty *> *PRFParser::getCapacityProperties()
{
    return &capacityProperties;
}


std::vector <PRFProperty *> *PRFParser::getMatchingProperties()
{
    return &matchingProperties;
}


std::vector <PRFProperty *> *PRFParser::getExecParamProperties()
{
    return &execProperties;
}


std::vector <PRFProperty *> *PRFParser::getFactoryParamProperties()
{
    return &factoryProperties;
}
