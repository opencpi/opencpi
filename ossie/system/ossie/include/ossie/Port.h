/*******************************************************************************

Copyright 2004, 2007, Virginia Polytechnic Institute and State University

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

#ifndef PORT_H
#define PORT_H

#include <string>
#include <iostream>

#include "ossie/FindBy.h"

#include "tinyxml.h"

class OSSIEPARSER_API Port
{
public:
    Port(TiXmlElement*);
    Port();
    virtual ~Port();

    const char* getComponentInstantiationRefID() {
        return componentInstantiationRefId.c_str();
    }

    const char* getDeviceThatLoadedThisComponentRef() {
        return deviceThatLoadedThisComponentRefId.c_str();
    }

    const char* getDeviceUsedByThisComponentRefID() {
        return deviceUsedByThisComponentRefId.c_str();
    }

    const char* getDeviceUsedByThisComponentRefUsesRefID() {
        return deviceUsedByThisComponentRefUsesRefId.c_str();
    }

    FindBy* getFindBy() const {
        return findBy;
    }

    bool isComponentInstantiationRef() const {
        return ifComponentInstantiationRef;
    }

    bool isDeviceThatLoadedThisComponentRef() const {
        return ifDeviceThatLoadedThisComponentRef;
    }

    bool isDeviceUsedByThisComponentRef() const {
        return ifDeviceUsedByThisComponentRef;
    }

    bool isFindBy() const {
        return ifFindBy;
    }

protected:
    FindBy* findBy;

    bool ifComponentInstantiationRef;
    bool ifDeviceThatLoadedThisComponentRef;
    bool ifDeviceUsedByThisComponentRef;
    bool ifFindBy;
    std::string componentInstantiationRefId;
    std::string deviceThatLoadedThisComponentRefId;
    std::string deviceUsedByThisComponentRefId;
    std::string deviceUsedByThisComponentRefUsesRefId;

    void parsePort(TiXmlElement *elem);
    void parseFindBy(TiXmlElement *elem);
    void parseComponentInstantiationRef(TiXmlElement *elem);
    void parseDeviceUsedByThisComponentRef(TiXmlElement *elem);
    void parseDeviceThatLoadedThisComponentRef(TiXmlElement *elem);

private:
    Port(const Port&);
};
#endif
