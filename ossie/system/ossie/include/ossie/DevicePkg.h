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

#ifndef DEVICEPKG_H
#define DEVICEPKG_H

#include "ossie/ossieparser.h"
#include "ossie/debug.h"

#include <string>
#include <vector>

#include "HWDeviceRegistration.h"

#include "tinyxml.h"

class OSSIEPARSER_API DevicePkg
{
public:
    DevicePkg(TiXmlElement *elem);
    ~DevicePkg();

    std::string getID();
    std::string getName();
    std::string getVersion();
    std::string getTitle();
    std::string getAuthor();
    std::string getDescription();

    HWDeviceRegistration* getHWDeviceRegistration();

protected:
    void parseElement(TiXmlElement *elem);
    void parseAttributes(TiXmlElement *elem);
    void parseAttributeID(TiXmlElement *elem);
    void parseAttributeName(TiXmlElement *elem);
    void parseAttributeVersion(TiXmlElement *elem);
    void parseChildElements(TiXmlElement *elem);
    void parseTitle(TiXmlElement *elem);
    void parseAuthor(TiXmlElement *elem);
    void parseDescription(TiXmlElement *elem);

private:
    DevicePkg(); // no default constructor
    DevicePkg(const DevicePkg& aDevicePkg); // no copying

    std::string id;
    std::string name;
    std::string version;
    std::string title;
    std::string author;
    std::string description;

    HWDeviceRegistration *hwDev;
};

#endif


