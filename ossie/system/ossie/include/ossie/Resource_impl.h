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

#ifndef RESOURCE_IMPL_H
#define RESOURCE_IMPL_H

#include <string>

#include "ossiecf.h"
#include "LifeCycle_impl.h"
#include "PortSupplier_impl.h"
#include "PropertySet_impl.h"
#include "TestableObject_impl.h"

class OSSIECF_API Resource_impl:public virtual POA_CF::Resource, public PropertySet_impl, public PortSupplier_impl, public LifeCycle_impl, public TestableObject_impl

{
protected:
    std::string _identifier;

public:
    Resource_impl (const char *_uuid);
    void start () throw (CF::Resource::StartError, CORBA::SystemException);
    void stop () throw (CF::Resource::StopError, CORBA::SystemException);
    char *identifier () throw (CORBA::SystemException);

private:
    Resource_impl(); // No default constructor
    Resource_impl(Resource_impl &); // No copying
};
#endif
