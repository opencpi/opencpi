
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <corba.h>
#include "OcpiCloneSystemException.h"

CORBA::SystemException *
OCPI::CORBAUtil::Misc::
cloneSystemException (const CORBA::SystemException & ex)
  throw ()
{
  CORBA::SystemException & ncex = const_cast<CORBA::SystemException &> (ex);

#define CSE_EXCHECK(type) \
  if ((CORBA:: type ::_downcast (&ncex))) { \
    return new CORBA:: type (*CORBA:: type ::_downcast(&ncex)); \
  }

  CSE_EXCHECK(UNKNOWN);
  CSE_EXCHECK(BAD_PARAM);
  CSE_EXCHECK(NO_MEMORY);
  CSE_EXCHECK(IMP_LIMIT);
  CSE_EXCHECK(COMM_FAILURE);
  CSE_EXCHECK(INV_OBJREF);
  CSE_EXCHECK(NO_PERMISSION);
  CSE_EXCHECK(INTERNAL);
  CSE_EXCHECK(MARSHAL);
  CSE_EXCHECK(INITIALIZE);
  CSE_EXCHECK(NO_IMPLEMENT);
  CSE_EXCHECK(BAD_TYPECODE);
  CSE_EXCHECK(BAD_OPERATION);
  CSE_EXCHECK(NO_RESOURCES);
  CSE_EXCHECK(NO_RESPONSE);
  CSE_EXCHECK(PERSIST_STORE);
  CSE_EXCHECK(BAD_INV_ORDER);
  CSE_EXCHECK(TRANSIENT);
  CSE_EXCHECK(FREE_MEM);
  CSE_EXCHECK(INV_IDENT);
  CSE_EXCHECK(INV_FLAG);
  CSE_EXCHECK(INTF_REPOS);
  CSE_EXCHECK(BAD_CONTEXT);
  CSE_EXCHECK(OBJ_ADAPTER);
  CSE_EXCHECK(DATA_CONVERSION);
  CSE_EXCHECK(OBJECT_NOT_EXIST);
  CSE_EXCHECK(TRANSACTION_REQUIRED);
  CSE_EXCHECK(TRANSACTION_ROLLEDBACK);
  CSE_EXCHECK(INVALID_TRANSACTION);
  CSE_EXCHECK(INV_POLICY);
  CSE_EXCHECK(CODESET_INCOMPATIBLE);
  CSE_EXCHECK(REBIND);
  CSE_EXCHECK(TIMEOUT);
  CSE_EXCHECK(TRANSACTION_UNAVAILABLE);
  CSE_EXCHECK(TRANSACTION_MODE);
  CSE_EXCHECK(BAD_QOS);
  //  CSE_EXCHECK(INVALID_ACTIVITY);
  //  CSE_EXCHECK(ACTIVITY_COMPLETED);
  //  CSE_EXCHECK(ACTIVITY_REQUIRED);

#undef CSE_EXCHECK

  /*
   * If we made it here, we don't recognize the system exception.
   */

  return new CORBA::UNKNOWN;
}
