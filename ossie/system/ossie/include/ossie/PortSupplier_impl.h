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

#ifndef PORTSUPPLIER_IMPL_H
#define PORTSUPPLIER_IMPL_H

#include "ossiecf.h"

#include "cf.h"


/**
The port supplier interface provides a method that supplies an onbject
reference for a port.
*/

class OSSIECF_API PortSupplier_impl:public virtual POA_CF::PortSupplier
{
public:
    PortSupplier_impl () { }

/// Return an object reference for the named port.
    CORBA::Object* getPort (const char *) throw (CF::PortSupplier::UnknownPort, CORBA::SystemException);

};
#endif                                            /*  */
