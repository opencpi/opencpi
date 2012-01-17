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

#ifndef HWDEVICEREGISTRATION_H
#define HWDEVICEREGISTRATION_H

#include "ossie/ossieparser.h"

#include "PropertyFile.h"
#include "DeviceClass.h"
#include "DevicePkgRef.h"

#include <string>
#include <vector>

#include "tinyxml.h"

// pre-definition
class ChildHWDevice;

class OSSIEPARSER_API HWDeviceRegistration
{
public:
    HWDeviceRegistration(TiXmlElement *elem);
    ~HWDeviceRegistration();

    std::string getID();
    std::string getVersion();
    std::string getName();
    std::string getDescription();
    std::string getManufacturer();
    std::string getModelNumber();

    std::vector <ChildHWDevice*> getChildHWDevice();
    PropertyFile* getPropertyFile();
    DeviceClass* getDeviceClass();

protected:
    void parseElement(TiXmlElement *elem);
    void parseAttributes(TiXmlElement *elem);
    void parseAttributeID(TiXmlElement *elem);
    void parseAttributeVersion(TiXmlElement *elem);
    void parseAttributeName(TiXmlElement *elem);
    void parseChildElements(TiXmlElement *elem);
    void parseDescription(TiXmlElement *elem);
    void parseManufacturer(TiXmlElement *elem);
    void parseModelNumber(TiXmlElement *elem);
    void constructChildHWDevice(TiXmlElement *elem);

private:
    HWDeviceRegistration(); // no default constructor
    HWDeviceRegistration(const HWDeviceRegistration& aHWDeviceRegistration); // no copying

    std::string id;
    std::string version;
    std::string name;
    std::string description;
    std::string manufacturer;
    std::string modelnumber;

    std::vector <ChildHWDevice*> childHWDevices;
    PropertyFile *propFile;
    DeviceClass *devClass;
};

class OSSIEPARSER_API ChildHWDevice
{
public:
    ChildHWDevice(TiXmlElement *elem);
    ~ChildHWDevice();

    HWDeviceRegistration* getHWDeviceRegistration();
    DevicePkgRef* getDevicePkgRef();

private:
    ChildHWDevice(); // no default constructor
    ChildHWDevice(const ChildHWDevice& aChildHWDevice); // no copying

    HWDeviceRegistration *hwDeviceReg;
    DevicePkgRef *devPkgRef;
};

#endif


