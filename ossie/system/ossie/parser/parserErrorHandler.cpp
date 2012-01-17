/*
(c) 2005, Virginia Polytechnic Institute and State University

This file is a part of OSSIE Parser.

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

\author Tuan Pham
\version 0.4
\date 11/01/05

*/
#include <iostream>
using namespace std;

#include "ossie/parserErrorHandler.h"

parserErrorHandler::parserErrorHandler()
{}

parserErrorHandler::~parserErrorHandler()
{}

void parserErrorHandler::warning(const SAXParseException& exc)
{

    cerr << "Warning: ";
    printSaxParseException(exc);
}

void parserErrorHandler::error(const SAXParseException&  exc)
{

    cerr << "Error: ";
    printSaxParseException(exc);
}

void parserErrorHandler::fatalError(const SAXParseException& exc)
{

    cerr << "Fatal: ";
    printSaxParseException(exc);
    throw 0;
}

void parserErrorHandler::resetErrors(void)

{

    cerr << "Rest Error Handler" << endl; // Basically, do nothing
}


void parserErrorHandler::printSaxParseException(const SAXParseException & exc)

{

    char *tmpStr = XMLString::transcode(exc.getMessage());
    if (tmpStr) {
        cerr << tmpStr << endl;
        XMLString::release(&tmpStr);
    }

    tmpStr = XMLString::transcode(exc.getPublicId());
    if (tmpStr) {
        cerr << "PublicId : " << tmpStr << endl;
        XMLString::release(&tmpStr);
    }

    tmpStr = XMLString::transcode(exc.getSystemId());
    if (tmpStr) {
        cerr << "SystemId : " << tmpStr << endl;
        XMLString::release(&tmpStr);

    }

    cerr << "Line number : " << exc.getLineNumber() << endl;

    cerr << "Column number : " << exc.getColumnNumber() << endl;

}
