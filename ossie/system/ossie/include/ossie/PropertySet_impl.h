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

#ifndef PROPERTYSET_IMPL_H
#define PROPERTYSET_IMPL_H

#include "ossiecf.h"

#include "cf.h"

/**
Figure out how to describe this interface.
*/

///\todo Why can't I use CF::PropertySet???
class OSSIECF_API PropertySet_impl:public virtual POA_CF::PropertySet
{
public:

    PropertySet_impl () {};

/// The core framework provides an implementation for this method.
    void
    configure (const CF::Properties & configProperties)
    throw (CF::PropertySet::PartialConfiguration,
           CF::PropertySet::InvalidConfiguration, CORBA::SystemException);

/// The core framework provides an implementation for this method.
    void
    query (CF::Properties & configProperties)
    throw (CF::UnknownProperties, CORBA::SystemException);

protected:

    CF::Properties
    propertySet;
    CF::DataType
    getProperty (CORBA::String_var id);
    void
    validate (CF::Properties property, CF::Properties & validProps,
              CF::Properties & invalidProps);

};
#endif                                            /*  */
