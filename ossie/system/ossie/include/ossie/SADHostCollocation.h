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
on the architecture of the CRC's SCA Reference Implementation (SCARI)
see: http://www.crc.ca/en/html/rmsc/home/sdr/projects/scari

*********************************************************************************/

#ifndef SADHOSTCOLLOCATION_H
#define SADHOSTCOLLOCATION_H

#include <string>

#include "SADComponentPlacement.h"

class OSSIEPARSER_API SADHostCollocation
{
protected:
    std::string id;
    std::string name;
    std::vector<SADComponentPlacement*> collocatedComponents;

public:
    SADHostCollocation(const char* _id, const char* _name, std::vector <SADComponentPlacement*> _collocatedComponents);
    ~SADHostCollocation();

    const char* getID();
    const char* getName();
    std::vector <SADComponentPlacement*> getComponents() const;

private:
    SADHostCollocation();
    SADHostCollocation(const SADHostCollocation &);

};
#endif
