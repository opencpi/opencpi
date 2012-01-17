/****************************************************************************

Copyright 2007, Virginia Polytechnic Institute and State University

This file is part of the OSSIE Core Framework.

OSSIE Core Framework is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Core Framework is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Core Framework; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/

#include <string>
#include <iostream>

#include "ossie/FileSystem_impl.h"

#include <ossie/ossieSupport.h>
#include <ossie/debug.h>

void ossieSupport::createProfileFromFileName(std::string fileName, std::string &profile)
{
    profile = "<profile filename=\"" + fileName + "\" />";

    return;
}

bool ossieSupport::isValidFileName(const char *fileName)
{
  return FileSystem_impl::isValidPath(fileName);
}

const char * ossieSupport::spd_rel_file(const char *spdFile, const char *name, std::string &fileName)
{
    DEBUG(9, spd_rel_file, "Called with spdFile: " << spdFile << ", name: " << name);

    FileSystem_impl::joinPath(spdFile, name, fileName);

    DEBUG(9, spd_rel_file, "Result: " << fileName);

    return fileName.c_str();
}
