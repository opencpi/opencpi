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

#include <string.h>

#include "tinyxml.h"

#include "ossie/cf.h"
#include "ossie/Connection.h"
#include "ossie/debug.h"

Connection::Connection(TiXmlElement *elem) :
        findBy(NULL), usesPort(NULL),
        providesPort(NULL), componentSupportedInterface(NULL),
        ifUsesPort(false), ifProvidesPort(false),
        ifComponentSupportedInterface(false), ifFindBy(false)
{
    DEBUG(4, Connection, "In constructor.");

    parseElement(elem);
}

Connection::Connection (const Connection & _conn):
        findBy(NULL), usesPort(NULL),
        providesPort(NULL), componentSupportedInterface(NULL),
        ifUsesPort(false), ifProvidesPort(_conn.ifProvidesPort),
        ifComponentSupportedInterface(_conn.ifComponentSupportedInterface),
        ifFindBy(_conn.ifFindBy)
{
    DEBUG(4, Connection, "In constructor.");

    if (_conn.usesPort != NULL) {
        this->usesPort = new UsesPort (*(_conn.usesPort));
        this->ifUsesPort=true;
    }
    if (_conn.providesPort != NULL)
        this->providesPort = new ProvidesPort (*(_conn.providesPort));

    if (_conn.findBy != NULL)
        this->findBy = new FindBy (*(_conn.findBy));

    connectionId = _conn.connectionId;

}

Connection::~Connection()
{
}


void Connection::parseElement(TiXmlElement *elem)
{
    DEBUG(4, Connection, "In parseElement.");

    if (elem->Attribute("id"))
        connectionId = elem->Attribute("id");
    DEBUG(9, Connection, "Processing connectionId " << connectionId);

    TiXmlElement *port = elem->FirstChild()->ToElement();

    if (!port)
        throw CF::ApplicationFactory::CreateApplicationError(CF::CFNOTSET , "No ports for connectioninterface.");

    for (; port; port = port->NextSiblingElement()) {
        DEBUG(9, Connection, "Found port " << port->ValueStr());

        std::string nodeName = port->ValueStr();

        if (nodeName == "usesport") {
            DEBUG(9, Connection, "Found usesport.");
            usesPort = new UsesPort (port);
            ifUsesPort = true;

        } else if (nodeName == "providesport") {
            DEBUG(9, Connection, "Found providesport.");
            providesPort = new ProvidesPort (port);
            ifProvidesPort = true;

        } else if (nodeName == "findby") {
            DEBUG(9, Connection, "Found findby.");
            findBy = new FindBy(port);
            ifFindBy = true;

        } else if (nodeName == "componentsupportedinterface") {
            DEBUG(9, Connection, "Found componentsupportedinterface.");
            componentSupportedInterface = new ComponentSupportedInterface (port);
            DEBUG(9, Connection, "Done creating an instance of ComponentSupportedInterface");
            ifComponentSupportedInterface = true;

        } else {
            DEBUG(1, Connection, "Unknown port type " << nodeName);
        }
    }

    DEBUG(9, Connection, "Finished processing connection.");
}

const char* Connection::getID()
{
    return connectionId.c_str();
}


UsesPort* Connection::getUsesPort() const
{
    return usesPort;
}


ProvidesPort* Connection::getProvidesPort() const
{
    return providesPort;
}


ComponentSupportedInterface* Connection::getComponentSupportedInterface() const
{
    return componentSupportedInterface;
}


FindBy* Connection::getFindBy() const
{
    return findBy;
}


bool Connection::isProvidesPort()
{
    return ifProvidesPort;
}


bool Connection::isComponentSupportedInterface()
{
    return ifComponentSupportedInterface;
}


bool Connection::isFindBy()
{
    return ifFindBy;
}

