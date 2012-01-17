/*******************************************************************************
 Copyright 2009 Virginia Polytechnic Institute and State University

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

#include "ossie/DevicePkg.h"

#include <iostream>

DevicePkg::DevicePkg(TiXmlElement *elem)
{
    DEBUG(4, DevicePkg, "In constructor.");

    parseElement(elem); // parse elements within devicepkg
    hwDev = new HWDeviceRegistration(elem->FirstChildElement("hwdeviceregistration"));
}

DevicePkg::~DevicePkg()
{
    delete hwDev;
}

void DevicePkg::parseElement(TiXmlElement *elem)
{
    DEBUG(4, DevicePkg, "in parseElement");
// get attributes from devicepkg
    parseAttributes(elem);

// parse info from author, title and description tags
    parseChildElements(elem);
}

void DevicePkg::parseAttributes(TiXmlElement *elem)
{
    parseAttributeID(elem);
    parseAttributeName(elem);
    parseAttributeVersion(elem);
}

void DevicePkg::parseAttributeID(TiXmlElement *elem)
{
    DEBUG(4, DevicePkg, "in parseAttributeID");

    if (elem->Attribute("id")) {
        id = elem->Attribute("id");
    } else {
        std::cout << "ERROR: Could not find attrbute 'id' within devicepkg tag" << std::endl;
        exit(-1);
    }
}

void DevicePkg::parseAttributeName(TiXmlElement *elem)
{
    DEBUG(4, DevicePkg, "in parseAttributeName");

    if (elem->Attribute("name")) {
        name = elem->Attribute("name");
    } else {
        std::cout << "ERROR: Could not find attribute 'name' within devicepkg tag" << std::endl;
        exit(-1);
    }
}

void DevicePkg::parseAttributeVersion(TiXmlElement *elem)
{
    DEBUG(4, DevicePkg, "in parseAttributeVersion");

    if (elem->Attribute("version")) {
        version = elem->Attribute("version");
    } else {
        std::cout << "ERROR: Could not find attribute 'version' within devicepkg tag" << std::endl;
        exit(-1);
    }
}

void DevicePkg::parseChildElements(TiXmlElement *elem)
{
    parseAuthor(elem);
    parseTitle(elem);
    parseDescription(elem);
}

void DevicePkg::parseAuthor(TiXmlElement *elem)
{
    DEBUG(4, DevicePkg, "in parseAuthor");

    TiXmlElement *auth = elem->FirstChildElement("author");
    if (auth) {
        author = auth->GetText();
    } else {
        std::cout << "ERROR: Could not find 'author' element within devicepkg tag" << std::endl;
        exit(-1);
    }
}

void DevicePkg::parseTitle(TiXmlElement *elem)
{
    DEBUG(4, DevicePkg, "in parseTitle");

    TiXmlElement *titleElem = elem->FirstChildElement("title");
    if (titleElem) {
        title = titleElem->GetText();
    } else {
        std::cout << "ERROR: Could not find 'title' element within devicepkg tag" << std::endl;
        exit(-1);
    }
}

void DevicePkg::parseDescription(TiXmlElement *elem)
{
    DEBUG(4, DevicePkg, "in parseDescription");

    TiXmlElement *descrip = elem->FirstChildElement("description");
    if (descrip) {
        description = descrip->GetText();
    } else {
        std::cout << "ERROR: Could not find 'description' element within devicepkg tag" << std::endl;
    }
}

std::string DevicePkg::getID()
{
    return id;
}

std::string DevicePkg::getName()
{
    return name;
}

std::string DevicePkg::getVersion()
{
    return version;
}

std::string DevicePkg::getTitle()
{
    return title;
}

std::string DevicePkg::getAuthor()
{
    return author;
}

std::string DevicePkg::getDescription()
{
    return description;
}

HWDeviceRegistration* DevicePkg::getHWDeviceRegistration()
{
    return hwDev;
}
