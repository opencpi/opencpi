/****************************************************************************

Copyright 2006, Virginia Polytechnic Institute and State University

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

#ifndef __RADIOMETADATA_IMPL_H__
#define __RADIOMETADATA_IMPL_H__

#include <string>
#include <vector>

#include "standardinterfaces-metadata/RadioMetaData.h"

namespace standardInterfacesMD
{

/// \brief Initialize a MetaData CORBA object
void InitializeMetaData( MetaData &m );

/// \brief Print MetaData information to screen
void PrintMetaData( MetaData &m );

}

#endif
