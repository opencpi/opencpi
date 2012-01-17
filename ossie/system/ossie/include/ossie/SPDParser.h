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

#ifndef SPDPARSER_H
#define SPDPARSER_H

#include <string>
#include <vector>

#include "ossie/SPDAuthor.h"
#include "ossie/SPDUsesDevice.h"
#include "ossie/cf.h"

#include "tinyxml.h"

class OSSIEPARSER_API SPDImplementation;

class OSSIEPARSER_API SPDParser
{
protected:

    std::string SPDFile;
    std::string PRFFile;
    std::string SCDFile;
    std::string softPkgID;
    std::string softPkgName;
    std::string softPkgType;
    std::string softPkgVersion;
    std::string softPkgTitle;
    std::string softPkgDescription;

    std::vector <SPDAuthor*> authors;
    std::vector <SPDImplementation*> implementations;
    std::vector <SPDUsesDevice*> usesDevice;

    char* getTextNode(TiXmlElement *elem);

    void parseFile(TiXmlElement *elem);
    void parseSoftPkgAttributes(TiXmlElement *elem);
    void parseSoftPkgTitle(TiXmlElement *elem);
    void parseSoftPkgDescription(TiXmlElement *elem);
    void parseSoftPkgAuthor(TiXmlElement *elem);
    void parsePRFRef(TiXmlElement *elem);
    void parseSCDRef(TiXmlElement *elem);
    void parseImplementations(TiXmlElement *elem);
    void parseUsesDevices(TiXmlElement *elem);

public:
    SPDParser(CF::File_ptr file);
    ~SPDParser();

    bool isScaCompliant();
    bool isScaNonCompliant();

    const char* getSoftPkgID();
    const char* getSoftPkgName();
    const char* getSoftPkgType();
    const char* getSoftPkgVersion();
    const char* getSoftPkgTitle();
    const char* getDescription();
    const char* getSPDFile();
    const char* getPRFFile();
    const char* getSCDFile();

    std::vector <SPDAuthor*>* getAuthors();
    std::vector <SPDImplementation*>* getImplementations();
    std::vector <SPDUsesDevice*>* getUsesDevices();

    static char* SCA_COMPLIANT;
    static char* SCA_NON_COMPLIANT;
private:
//	SPDParser();  //  No default constructor
    SPDParser(const SPDParser & _spdParser); // No copying

    TiXmlDocument XMLDoc;
};
#endif
