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

#ifndef COMPONENTASSEMBLYPARSER_H
#define COMPONENTASSEMBLYPARSER_H

#include <vector>
#include <string>

#include "ossie/cf.h"
#include "ossie/Connection.h"

#include "tinyxml.h"

class OSSIEPARSER_API ComponentAssemblyParser
{
public:
    ComponentAssemblyParser(CF::File_ptr file);
    virtual ~ComponentAssemblyParser();

    std::vector <Connection*>* getConnections();
    const char* getFileName();
    const char* getID();
    const char* getName();

protected:
    std::string fileName;  // the file name for a given DCD or SAD Profile
    std::string id;        // the id of the root node
    std::string name;      // the name of the root node

    TiXmlDocument XMLDoc;

    std::vector <Connection*> connections;
    void parseIdAndName(TiXmlHandle &docHandle);
    void parseConnections(TiXmlHandle &docHandle);

    TiXmlNode *root;

private:
    ComponentAssemblyParser(); // No default constructor
    ComponentAssemblyParser(const ComponentAssemblyParser&); // No Copying

};
#endif
