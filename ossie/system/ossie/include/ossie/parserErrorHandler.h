/*
(c) 2005, Virginia Polytechnic Institute and State University

OSSIE Parser is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published
by the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Parser is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Parser; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

\todo Fix docs for parser Error handler
\brief Ossie Error Handler for OSSIE Parsers.

\author Tuan Pham
\version 0.4
\date 11/01/05

*/

#ifndef OSSIEERRORHANDLER_H
#define OSSIEERRORHANDLER_H

#include "ossie/ossieparser.h"

class OSSIEPARSER_API parserErrorHandler: public ErrorHandler
{
public:
    parserErrorHandler();
    ~parserErrorHandler();

    void warning(const SAXParseException& exc);
    void error(const SAXParseException& exc);
    void fatalError(const SAXParseException& exc);

    void resetErrors(void);

private:
    void printSaxParseException(const SAXParseException& exc);

};

#endif
