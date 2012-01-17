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

#ifndef TESTABLEOBJECT_IMPL_H
#define TESTABLEOBJECT_IMPL_H

#include "ossiecf.h"

#include "cf.h"

/**

The testable object interface provides a means to perform stand alone testing
of an SCA component. This function is useful for built in test (BIT)
operations.
*/

class OSSIECF_API TestableObject_impl:public virtual
        POA_CF::TestableObject
{
public:
    TestableObject_impl () { }

/// Run the test specified by TestID with the values supplied in testValues.
    void runTest (CORBA::ULong TestID, CF::Properties & testValues)
    throw (CF::UnknownProperties, CF::TestableObject::UnknownTest,
           CORBA::SystemException);
};
#endif                                            /*  */
