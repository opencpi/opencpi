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

#include <iostream>

#include "ossie/cf.h"
#include "ossie/SADParser.h"
#include "ossie/debug.h"

#include "tinyxml.h"

SADParser::SADParser (CF::File_ptr file) : ComponentAssemblyParser(file), assemblyControllerInstantiation(NULL)

{
    TiXmlHandle docHandle(root);

    parseFile(docHandle);
}

SADParser::~SADParser ()
{
    DEBUG(9, SADParser, "Entering destructor");
    unsigned int i;
    DEBUG(9, SADParser, "Deleting hostCollocations");
    for (i=0; i < hostCollocations.size(); i++) {
        delete hostCollocations[i];
    }

    DEBUG(9, SADParser, "Deleting externalPorts");
    for (i=0; i < externalPorts.size(); i++) {
        delete externalPorts[i];
    }
    /*
     * NOTE: this section fails for some reason
    	DEBUG(9, SADParser, "Deleting components");
        for (i=0; i<components.size(); i++)
        {
            delete components[i];
        }
    */
    DEBUG(9, SADParser, "Leaving destructor");
}


void SADParser::parseFile(TiXmlHandle docHandle)
{

    parseComponentFiles(docHandle);
    parseComponents(docHandle);
    parseHostCollocation(docHandle);
    parseAssemblyController(docHandle);
    parseExternalPorts(docHandle);
}

void SADParser::parseComponentFiles(TiXmlHandle docHandle)
{
    DEBUG(6, SADParser, "About to parse component files");

    TiXmlElement *cf = docHandle.FirstChild("componentfiles").FirstChild("componentfile").Element();

    for (; cf; cf = cf->NextSiblingElement()) {
        componentFiles.push_back (new SADComponentFile (cf));
    }
}

void SADParser::parseComponents(TiXmlHandle docHandle)
{
    DEBUG(6, SADParser, "About to parse components.");

    TiXmlElement *cp = docHandle.FirstChild("partitioning").FirstChild("componentplacement").Element();

    if (!cp)
        std::cout << "No component placements found" << std::endl;

    for (; cp; cp = cp->NextSiblingElement()) {
        DEBUG(7, SADParser, "Found component, about to create placement " << cp->Value() << " node type " << cp->Type() );
        components.push_back (new SADComponentPlacement (cp));
    }

}

void SADParser::parseHostCollocation(TiXmlHandle docHandle)
{
    DEBUG(4, SADParser, "Parsing Host Collocation");

    TiXmlElement *col = docHandle.FirstChild("partitioning").FirstChild("hostcollocation").Element();

    if (col) {
        const char *id = col->Attribute("id");
        const char *name = col->Attribute("name");

        std::vector <SADComponentPlacement* > collocatedComponents;

        TiXmlElement *place = col->FirstChildElement("componentplacement");

        for (; place; place = place->NextSiblingElement()) {
            DEBUG(6, SADParser, "Found placement.");
            collocatedComponents.push_back(new SADComponentPlacement (place));

        }

        DEBUG(6, SADParser, "Done parsing placements.");

        hostCollocations.push_back (new SADHostCollocation (id, name, collocatedComponents));
    }
}


void SADParser::parseAssemblyController (TiXmlHandle docHandle)
{
    DEBUG(4, SADParser, "In parseAssemblyController.");

    TiXmlElement *ac = docHandle.FirstChild("assemblycontroller").FirstChild("componentinstantiationref").Element();

    if (ac) {
        assemblyControllerRefId = ac->Attribute("refid");
        DEBUG(7, SADParser, "Assembly controller refid = " << assemblyControllerRefId);
    } else {
        DEBUG(1, SADParser, "Assembly Controller Ref ID not found.");
    }
}

void SADParser::parseExternalPorts(TiXmlHandle docHandle)
{
    DEBUG(4, SADParser, "In parseExternalPorts.");

    TiXmlElement *ep = docHandle.FirstChild("externalports").Element();

    if (ep) {
        TiXmlElement *port = ep->FirstChildElement("port");

        for (; port; port = port->NextSiblingElement()) {
            externalPorts.push_back(new ExternalPort (port));
        }
    }
}


const char* SADParser::getSADFilename()
{
    return fileName.c_str();
}

const char* SADParser::getAssemblyControllerRefId()
{
    return assemblyControllerRefId.c_str();
}

SADComponentInstantiation* SADParser::getAssemblyController()
{
    return assemblyControllerInstantiation;
}


std::vector <SADComponentPlacement*>*
SADParser::getComponents()
{
    return &components;
}


std::vector <SADHostCollocation*>*
SADParser::getHostCollocations()
{
    return &hostCollocations;
}


std::vector <ExternalPort*>*
SADParser::getExternalPorts()
{
    return &externalPorts;
}

const char *SADParser::getSPDById(const char *_refId)
{
    std::string refId(_refId);

    for (unsigned int i(0); i < componentFiles.size(); ++i) {
        std::string testId = componentFiles[i]->getId();

        if (refId == testId)
            return componentFiles[i]->getFileName();
    }

    return NULL;
}
