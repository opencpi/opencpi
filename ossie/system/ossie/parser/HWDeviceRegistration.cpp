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

#include "ossie/HWDeviceRegistration.h"

#include <iostream>

HWDeviceRegistration::HWDeviceRegistration(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in constructor.");

    parseElement(elem); // parse tags in hwdeviceregistration, with exception of propertyfile, deviceclass, childhwdevice

    propFile = new PropertyFile(elem->FirstChildElement("propertyfile"));
    devClass = new DeviceClass(elem->FirstChildElement("deviceclass"));

    constructChildHWDevice(elem->FirstChildElement("childhwdevice"));
}

HWDeviceRegistration::~HWDeviceRegistration()
{
    for (int i =0; i < childHWDevices.size(); i++) {
        delete childHWDevices[i];
        childHWDevices[i] = NULL;
    }

    delete propFile;
    propFile = NULL;
    delete devClass;
    devClass = NULL;

}

void HWDeviceRegistration::constructChildHWDevice(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in constructChildHWDevice");

// parse childhwdevice tag(s)
    for (; elem; elem=elem->NextSiblingElement("childhwdevice")) {
        // create child device object
        ChildHWDevice *cDevice;
        cDevice = new ChildHWDevice(elem);

        // push new child device onto vector
        childHWDevices.push_back(cDevice);
    }
}

// parse attributes within hwdeviceregistration tag
void HWDeviceRegistration::parseElement(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in parseElement");

// pull info from hwdeviceregistration tag
    parseAttributes(elem);

// description, manufacturer, modelnumber tags
    parseChildElements(elem);
}

void HWDeviceRegistration::parseAttributes(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in parseAttributes");

    parseAttributeID(elem);
    parseAttributeVersion(elem);
    parseAttributeName(elem);
}

void HWDeviceRegistration::parseAttributeID(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in parseAttributeID");

// verify attribute exists to prevent seg fault
    if (elem->Attribute("id")) {
        id = elem->Attribute("id");
    } else {
        std::cout << "ERROR: Could not find attribute 'id' within hwdeviceregistration tag" << std::endl;
        exit(-1);
    }
}

void HWDeviceRegistration::parseAttributeVersion(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in parseAttributeVersion");

// verify attribute exists to prevent seg fault
    if (elem->Attribute("version")) {
        version = elem->Attribute("version");
    } else {
        std::cout << "ERROR: Could not find attribute 'version' within hwdeviceregistration tag" << std::endl;
        exit(-1);
    }
}

void HWDeviceRegistration::parseAttributeName(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in parseAttributeName");

// verify attribute exists to prevent seg fault
    if (elem->Attribute("name")) {
        name = elem->Attribute("name");
    } else {
        std::cout<< "ERROR: Could not find attribute 'name' wtihin hwdeviceregistration tag" << std::endl;
        exit(-1);
    }
}

void HWDeviceRegistration::parseChildElements(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in parseChildElements");

    parseDescription(elem);
    parseManufacturer(elem);
    parseModelNumber(elem);
}

void HWDeviceRegistration::parseDescription(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in parseDescription");

    TiXmlElement *descrip = elem->FirstChildElement("description");
    if (descrip) {
        description = descrip->GetText();
    } else {
        std::cout << "ERROR: Could not find description tag in hwdeviceregistration" << std::endl;
        exit(-1);
    }
}

void HWDeviceRegistration::parseManufacturer(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in parseManufacturer");

    TiXmlElement *manufac = elem->FirstChildElement("manufacturer");
    if (manufac) {
        manufacturer = manufac->GetText();
    } else {
        std::cout << "ERROR: Could not find manufacturer tag in hwdeviceregistration" << std::endl;
        exit(-1);
    }
}

void HWDeviceRegistration::parseModelNumber(TiXmlElement *elem)
{
    DEBUG(4, HWDeviceRegistration, "in parseModelNumber");

    TiXmlElement *modNum = elem->FirstChildElement("modelnumber");
    if (modNum) {
        modelnumber = modNum->GetText();
    } else {
        std::cout << "ERROR: Could not find modelnumber tag in hwdeviceregistration" << std::endl;
        exit(-1);
    }
}

ChildHWDevice::ChildHWDevice(TiXmlElement *elem)
{
    DEBUG(4, ChildHWDevice, "in constructor.");

    hwDeviceReg = new HWDeviceRegistration(elem->FirstChildElement("hwdeviceregistration"));
    if (elem->FirstChildElement("devicepkgref")) {
        devPkgRef = new DevicePkgRef(elem->FirstChildElement("devicepkgref"));
    }
}

ChildHWDevice::~ChildHWDevice()
{
    delete hwDeviceReg;
}

std::vector <ChildHWDevice*> HWDeviceRegistration::getChildHWDevice()
{
    return childHWDevices;
}

PropertyFile* HWDeviceRegistration::getPropertyFile()
{
    return propFile;
}

DeviceClass* HWDeviceRegistration::getDeviceClass()
{
    return devClass;
}

std::string HWDeviceRegistration::getID()
{
    return id;
}

std::string HWDeviceRegistration::getVersion()
{
    return version;
}

std::string HWDeviceRegistration::getName()
{
    return name;
}

std::string HWDeviceRegistration::getDescription()
{
    return description;
}

std::string HWDeviceRegistration::getManufacturer()
{
    return manufacturer;
}

std::string HWDeviceRegistration::getModelNumber()
{
    return modelnumber;
}

HWDeviceRegistration* ChildHWDevice::getHWDeviceRegistration()
{
    return hwDeviceReg;
}

DevicePkgRef* ChildHWDevice::getDevicePkgRef()
{
    return devPkgRef;
}



