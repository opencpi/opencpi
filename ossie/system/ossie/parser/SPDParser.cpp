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
on the architecture of the CRCs SCA Reference Implementation(SCARI)
see: http://www.crc.ca/en/html/rmsc/home/sdr/projects/scari

*********************************************************************************/

#include <iostream>
#include <string>

#include "ossie/SPDParser.h"
#include "ossie/SPDImplementation.h"
#include "ossie/cf.h"
#include "ossie/debug.h"

SPDParser::SPDParser(CF::File_ptr file)
{
    DEBUG(4, SPDParser, "In constructor.");

    unsigned long fileSize = file->sizeOf();
    CF::OctetSequence_var fileData;

    file->read(fileData, fileSize);

    unsigned char *fileBuffer = fileData->get_buffer();

    XMLDoc.Parse((const char *)fileBuffer);

// Handle SAD or DCD files
    TiXmlElement *elem = XMLDoc.FirstChildElement("softpkg");
    if (!elem) {
        std::cerr << "Invalid SPD file (insert-name-here): "
                  << "cannot read <softpkg> tag" << std::endl;
        throw CF::FileException();
    }

    parseFile(elem);
}

SPDParser::~SPDParser()
{
    DEBUG(4, SPDParser, "In destructor.");

    unsigned int i;

    for (i=0; i<authors.size(); i++) {
        delete authors[i];
    }

    DEBUG(5, SPDParser, "Deleted authors.");


    /*
        for (i=0; i<implementations.size(); i++)
        {
            mdel(implementations[i]);
        }
    */

    for (i=0; i<usesDevice.size(); i++) {
        delete usesDevice[i];
    }

    DEBUG(5, SPDParser, "Deleted usesDevices.");
}


void SPDParser::parseFile(TiXmlElement *elem)
{
    DEBUG(4, SPDParser, "In parseFile.");

    parseSoftPkgAttributes(elem);
    parseSoftPkgTitle(elem);
    parseSoftPkgDescription(elem);
    parseSoftPkgAuthor(elem);
    parsePRFRef(elem);
    parseSCDRef(elem);
    parseImplementations(elem);
    parseUsesDevices(elem);
}


void SPDParser::parseSoftPkgAttributes(TiXmlElement *elem)
{
    DEBUG(4, SPDParser, "In parseSoftPkgAttributes.");

    const char *id = elem->Attribute("id");
    softPkgID = id;

    const char *name = elem->Attribute("name");
    softPkgName = name;

    const char *type = elem->Attribute("type");
    if (type) {
        softPkgType = type;
    } else {
        softPkgType = "sca_compliant";
    }

    const char *ver = elem->Attribute("version");
    if (ver)
        softPkgVersion = ver;
}


void SPDParser::parseSoftPkgTitle(TiXmlElement *elem)
{
    DEBUG(4, SPDParser, "In parseSoftPkgTitle.");

    TiXmlElement *title = elem->FirstChildElement("title");

    if (title) {
        const char *str = title->GetText();
        if (str)
            softPkgTitle = str;
    }
}


void SPDParser::parseSoftPkgDescription(TiXmlElement *elem)
{
    DEBUG(4, SPDParser, "In parseSoftPkgDescription.");

    TiXmlElement *title = elem->FirstChildElement("description");

    if (title && title->GetText()) { // Check that tag exists and contains text
        softPkgDescription = title->GetText();
    }
}


void SPDParser::parseSoftPkgAuthor(TiXmlElement *elem)
{
    DEBUG(4, SPDParser, "In parseSoftPkgAuthor.");

    TiXmlElement *author = elem->FirstChildElement("author");

    if (!author) {
        ///\todo output file name here
        std::cerr << "Invalid SPD file (insert-name-here): "
                  << "must include at least one <author>" << std::endl;
        throw CF::FileException();
    }

    for (; author; author = author->NextSiblingElement("author")) {
        SPDAuthor* spdAuthor = new SPDAuthor(author);
        authors.push_back(spdAuthor);
    }
}


void SPDParser::parsePRFRef(TiXmlElement *elem)
{
    DEBUG(4, SPDParser, "In parsePRFRef.");

    TiXmlElement *prf = elem->FirstChildElement("propertyfile");

    if (prf) {
        TiXmlElement *local = prf->FirstChildElement("localfile");
        PRFFile = local->Attribute("name");;
    }
}


void SPDParser::parseSCDRef(TiXmlElement *elem)
{
    DEBUG(4, SPDParser, "In parseSCDRef.");

    TiXmlElement *scd = elem->FirstChildElement("descriptor");

    if (scd) {
        TiXmlElement *local = scd->FirstChildElement("localfile");
        if (!local) {
            std::cerr << "Invalid SPD file (insert-name-here): "
                      << "cannot read <localfile> tag for SCD ref" << std::endl;
            throw CF::FileException();
        }
        SCDFile = local->Attribute("name");
    } else {
        std::cerr << "Invalid SPD file (insert-name-here): "
                  << "cannot read <descriptor> tag for SCD ref" << std::endl;
        throw CF::FileException();
    }
}


void SPDParser::parseImplementations(TiXmlElement *elem)
{
    DEBUG(4, SPDParser, "In parseImplementations.");

    TiXmlElement *imp = elem->FirstChildElement("implementation");

    if (!imp) {
        ///\todo output file name here
        std::cerr << "Invalid SPD file (insert-name-here): "
                  << "must include at least one <implementation>" << std::endl;
        throw CF::FileException();
    }

    for (; imp; imp = imp->NextSiblingElement("implementation")) {
        implementations.push_back(new SPDImplementation(imp));
    }
}


void SPDParser::parseUsesDevices(TiXmlElement *elem)
{
    DEBUG(4, SPDParser, "In parseUsesDevices.");

    TiXmlElement *uses = elem->FirstChildElement("usesdevice");

    for (; uses; uses = uses->NextSiblingElement("usesDevice")) {

        const char *id = uses->Attribute("id");
        const char *type = uses->Attribute("type");

        CF::Properties props;
        unsigned int i(0);

        TiXmlElement *prop = uses->FirstChildElement("propertyref");

        if (!prop) {
            ///\todo output file name here
            std::cerr << "Invalid SPD file (insert-name-here): "
                      << "must include at least one <propertyref> in <usesdevice>" << std::endl;
            throw CF::FileException();
        }

        for (; prop; prop = prop->NextSiblingElement("propertyref")) {
            const char *refid = prop->Attribute("refid");
            const char *value = prop->Attribute("value");

            props.length(i+1);
            props[i].id = CORBA::string_dup(refid);
            props[i].value <<= value;
            i++;
        }

        usesDevice.push_back(new SPDUsesDevice(id, type, props));
    }
}


bool SPDParser::isScaCompliant()
{

    if (softPkgType == "sca_compliant")
        return true;
    else
        return false;
}


bool SPDParser::isScaNonCompliant()
{
    if (softPkgType == "sca_non_compliant")
        return true;
    else
        return false;
}


const char* SPDParser::getSoftPkgID()
{
    return softPkgID.c_str();
}


const char* SPDParser::getSoftPkgName()
{
    return softPkgName.c_str();
}


const char* SPDParser::getSoftPkgType()
{
    return softPkgType.c_str();
}


const char* SPDParser::getSoftPkgVersion()
{
    return softPkgVersion.c_str();
}


const char* SPDParser::getSoftPkgTitle()
{
    return softPkgTitle.c_str();
}


const char* SPDParser::getDescription()
{
    return softPkgDescription.c_str();
}


const char* SPDParser::getSPDFile()
{
    return SPDFile.c_str();
}


const char* SPDParser::getPRFFile()
{
    return PRFFile.c_str();
}


const char* SPDParser::getSCDFile()
{
    return SCDFile.c_str();
}


std::vector <SPDAuthor*>* SPDParser::getAuthors()
{
    return &authors;
}


std::vector <SPDImplementation*>* SPDParser::getImplementations()
{
    return &implementations;
}


std::vector <SPDUsesDevice*>* SPDParser::getUsesDevices()
{
    return &usesDevice;
}
