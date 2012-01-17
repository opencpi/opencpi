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

#include "ossie/DCDComponentPlacement.h"

#include "tinyxml.h"

// the ifDeployOn initialization has been set from FALSE to true
DCDComponentPlacement::DCDComponentPlacement(TiXmlElement *elem) : ComponentPlacement(elem), ifDeployOn(false), ifCompositePartOf(false), ifDomainManager(false)
{
    DEBUG(4, DCDComponentPlacement, "In constructor.");

    parseDeployOnDevice(elem);
    parseCompositePartOfDevice(elem);
    parseDPDFileName(elem);
    parseInstantiations(elem);

    DEBUG(4, DCDComponentPlacement, "Leaving constructor.");
}



DCDComponentPlacement::~DCDComponentPlacement()
{
}


void DCDComponentPlacement::parseDeployOnDevice(TiXmlElement *elem)
{
    DEBUG(4, DCDComponentPlacement, "In parseDeployOnDevice.");

    TiXmlElement *deploy = elem->FirstChildElement("deployondevice");

    if (deploy) {
        ifDeployOn = true;
        deployOnDeviceID = deploy->Attribute("refid");
    } else {
        DEBUG(4, DCDComponentPlacement, "deployondevice NOT found")
    }
}


void DCDComponentPlacement::parseCompositePartOfDevice(TiXmlElement *elem)
{
    DEBUG(4, DCDComponentPlacement, "In parseCompositePartOfDevice.");

    TiXmlElement *composite = elem->FirstChildElement("compositepartofdevice");

    if (composite) {
        ifCompositePartOf = true;
        compositePartOfDeviceID = composite->Attribute("refid");
    }
}


void DCDComponentPlacement::parseDPDFileName(TiXmlElement *elem)
{
    DEBUG(4, DCDComponentPlacement, "In DPDFileName.");

    TiXmlElement *DPD = elem->FirstChildElement("devicepkgfile");

    if (DPD) {
        TiXmlElement *local = DPD->FirstChildElement("localfile");
        DPDFile = local->Attribute("name");
    } else {
        DEBUG(4, DCDComponentPlacement, "devicepkgfile tag not found, DPD not referenced");
    }
}


void DCDComponentPlacement::parseInstantiations(TiXmlElement *elem)
{
    DEBUG(4, DCDComponentPlacement, "In parseInstantiations.");

    TiXmlElement *instance = elem->FirstChildElement("componentinstantiation");

    std::vector <ComponentInstantiation* > _instantiations;

    for (; instance; instance = instance->NextSiblingElement()) {

        _instantiations.push_back((ComponentInstantiation* ) new DCDComponentInstantiation(instance));
        _instantiationId = _instantiations.back()->getID();
        _usageName = _instantiations.back()->getUsageName();
    }
}


const char* DCDComponentPlacement::getDMDFile()
{
    return DMDFile.c_str();
}


const char* DCDComponentPlacement::getDeployOnDeviceID()
{
    return deployOnDeviceID.c_str();
}


const char* DCDComponentPlacement::getCompositePartOfDeviceID()
{
    return compositePartOfDeviceID.c_str();
}

std::string DCDComponentPlacement::getDPDFile()
{
    return DPDFile.c_str();
}


bool DCDComponentPlacement::isDeployOn()
{
    return ifDeployOn;
}


bool DCDComponentPlacement::isCompositePartOf()
{
    return ifCompositePartOf;
}


bool DCDComponentPlacement::isDomainManager()
{
    return ifDomainManager;
}


const char *DCDComponentPlacement::getFileRefId()
{
    return _fileRefId.c_str();
}

const char *DCDComponentPlacement::getInstantiationId()
{
    return _instantiationId.c_str();
}

const char *DCDComponentPlacement::getUsageName()
{
    return _usageName.c_str();
}
