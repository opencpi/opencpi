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

#ifdef HAVE_STRING_H
#include <string.h>    // using POSIX strcpy
#endif

#include "ossie/DCDParser.h"
#include "ossie/debug.h"

componentFile::componentFile(const char *id, const char *fileName)

{

    _fileName = fileName;
    _id = id;

}

const char* componentFile::fileName()

{
    return _fileName.c_str();
}

const char* componentFile::id()

{
    return _id.c_str();
}


componentPlacement::componentPlacement(const char *refId, const char *id, const char *usageName)
{

    _refId = refId;
    _id = id;
    _usageName = usageName;
}

const char *componentPlacement::refId()
{

    return _refId.c_str();
}

const char *componentPlacement::id()
{

    return _id.c_str();
}

const char *componentPlacement::usageName()
{

    return _usageName.c_str();
}

DCDParser::DCDParser(CF::File_ptr file):ComponentAssemblyParser(file)
{
    DEBUG(6, DCDParser, "Entering constructor");

    TiXmlHandle docHandle(root);

    parseDeviceManagerSoftPkg(docHandle);
    parseComponentPlacement(docHandle);
    parseDomainManager(docHandle);
    parseLocalComponents(docHandle);
}


DCDParser::~DCDParser()
{
}


void DCDParser::parseDeviceManagerSoftPkg(TiXmlHandle docHandle)
{
    DEBUG(6, DCDParser, "In parseDeviceManagerSoftPkg.");

    TiXmlElement *dmspd = docHandle.FirstChild("devicemanagersoftpkg").Element();

    if (dmspd) {
        TiXmlElement *local = dmspd->FirstChildElement("localfile");
        deviceManagerSoftPkg = local->Attribute("name");
        DEBUG(7, DCDParser, "Found device mananager software package " << deviceManagerSoftPkg);
    } else {
        DEBUG(1, DCDParser, "Device Manager Software Package not found.");
    }
}


void DCDParser::parseLocalComponents(TiXmlHandle docHandle)
{
    DEBUG(6, DCDParser, "In parseLocalComponents.");

    TiXmlElement *cfile = docHandle.FirstChild("componentfiles").FirstChild("componentfile").Element();

    for (; cfile; cfile = cfile->NextSiblingElement()) {
        const char *id = cfile->Attribute("id");

        TiXmlElement *local = cfile->FirstChildElement("localfile");
        const char *name = local->Attribute("name");

        DEBUG(8, DCDParser, "Found component " << id << " with file name " << name);

        componentFiles.push_back(componentFile(id, name));

    }

}

void DCDParser::parseComponentPlacement(TiXmlHandle docHandle)
{
    DEBUG(6, DCDParser, "In parseComponentPlacement.");

    TiXmlElement *cp = docHandle.FirstChild("partitioning").FirstChild("componentplacement").Element();

    for (; cp; cp = cp->NextSiblingElement()) {
        DEBUG(8, DCDParser, "Found component placement.");

        DCDComponentPlacement* dcdComponent = new DCDComponentPlacement(cp);

        if (dcdComponent->isDomainManager()) {
            domainManagerComponent = dcdComponent;
        } else if (dcdComponent->isDeployOn()) {
            deployOnComponents.push_back(dcdComponent);
        } else {
            componentPlacements.push_back(componentPlacement(dcdComponent->getFileRefId(), dcdComponent->getInstantiationId(), dcdComponent->getUsageName()));
        }
    }
}


void DCDParser::parseDomainManager(TiXmlHandle docHandle)
{
    DEBUG(6, DCDParser, "In parseDomainManager.");

    TiXmlElement *nms = docHandle.FirstChild("domainmanager").FirstChild("namingservice").Element();

    if (nms) {
        domainManagerName = nms->Attribute("name");
    } else if (TiXmlElement *sior = docHandle.FirstChild("domainmanager").FirstChild("stringifiedobjectref").Element()) {
        domainManagerIOR = sior->Value();
    } else {
        std::cerr << "DCDParser::parseDomainManager No Domain Manager reference dound" << std::endl;
    }
}


const char* DCDParser::getDCDFilename()
{
    return fileName.c_str();
}


const char* DCDParser::getDeviceManagerSoftPkg()
{
    return deviceManagerSoftPkg.c_str();
}


const char* DCDParser::getDomainManagerName()
{
    return domainManagerName.c_str();
}


const char* DCDParser::getDomainManagerIOR()
{
    return domainManagerIOR.c_str();
}


DCDComponentPlacement* DCDParser::getDomainManagerComponent() const
{
    return domainManagerComponent;
}


std::vector <DCDComponentPlacement*>*
DCDParser::getDeployOnComponents()
{
    return &deployOnComponents;
}

std::vector <componentFile> DCDParser::getComponentFiles()

{

    return componentFiles;
}

std::vector <componentPlacement> DCDParser::getComponentPlacements()
{

    return componentPlacements;
}

const char *DCDParser::getFileNameFromRefId(const char *refid)
{
    for (unsigned int i=0; i<componentFiles.size(); i++) {
        if (strcmp(refid, componentFiles[i].id()) == 0)
            return componentFiles[i].fileName();
    }
    return NULL;
}
