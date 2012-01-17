
/*******************************************************************************

Copyright 2006, 2007 Virginia Polytechnic Institute and State University

This file is part of the OSSIE  waveform loader.

OSSIE Waveform Loader is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

OSSIE Sample Waveform is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OSSIE Waveform loader; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


*******************************************************************************/

#include <string>

#include "ossie/cf.h"

#include "tinyxml.h"

#ifndef DASPARSER_H
#define DASPARSER_H

class DASParser
{

public:
    DASParser(CF::File_ptr file);
//    ~DASParser();

    CF::DeviceAssignmentSequence *das() {
        return das_var._retn();
    };

protected:
    TiXmlNode *root;
    TiXmlDocument XMLDoc;

private:
    DASParser();

    void add_element() {
        num_elements++;
        das_var->length(num_elements);
    };

    unsigned int num_elements;
    CF::DeviceAssignmentSequence_var das_var;

};

#endif
