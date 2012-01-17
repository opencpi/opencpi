/****************************************************************************

Copyright 2008, Virginia Polytechnic Institute and State University

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

#include "ossie/Resource_impl.h"

Resource_impl::Resource_impl (const char *_uuid)
{
    _identifier = _uuid;
}


void Resource_impl::start () throw (CORBA::SystemException, CF::Resource::StartError)
{
// set CF::Device::UsageType = ACTIVE;
//      generally, you'll overload this with some implementation that is dependent to your specific component
}


void Resource_impl::stop () throw (CORBA::SystemException, CF::Resource::StopError)
{
// set CF::Device::UsageType = IDLE;
//      generally, you'll overload this with some implementation that is dependent to your specific component
}


char *Resource_impl::identifier () throw (CORBA::SystemException)
{
    return CORBA::string_dup(_identifier.c_str());
}
