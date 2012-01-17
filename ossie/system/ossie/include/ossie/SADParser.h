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

#ifndef SADPARSER_H
#define SADPARSER_H

#include <vector>

#include "ossie/ComponentAssemblyParser.h"
#include "ossie/SADComponentPlacement.h"
#include "ossie/SADComponentFile.h"
#include "ossie/SADHostCollocation.h"
#include "ossie/ExternalPort.h"

#include "tinyxml.h"

class OSSIEPARSER_API SADParser:public ComponentAssemblyParser
{
public:
    SADParser(CF::File_ptr file);
    ~SADParser();

    const char* getSADFilename();
    SADComponentInstantiation* getAssemblyController();
    const char *getAssemblyControllerRefId();
    std::vector <SADComponentPlacement* >* getComponents();
    std::vector <SADHostCollocation* >* getHostCollocations();
    std::vector <ExternalPort* >* getExternalPorts();
    const char *getSPDById(const char *refid);

protected:
    SADComponentInstantiation* assemblyControllerInstantiation;
    std::vector <SADComponentPlacement*> components;
    std::vector <SADComponentFile*> componentFiles;
    std::vector <SADHostCollocation*> hostCollocations;
    std::vector <ExternalPort*> externalPorts;

    void parseFile(TiXmlHandle docHandle);
    void parseComponentFiles(TiXmlHandle docHandle);
    void parseComponents(TiXmlHandle docHandle);
    void parseHostCollocation(TiXmlHandle docHandle);
    void parseAssemblyController(TiXmlHandle docHandle);
    void parseExternalPorts(TiXmlHandle docHandle);

private:
    SADParser();  // No default constructor
    SADParser(const SADParser& aSADParser);  // No copying

    std::string assemblyControllerRefId;
};
#endif
