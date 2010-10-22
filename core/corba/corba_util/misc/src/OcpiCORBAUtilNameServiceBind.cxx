
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


#include <string>
#include <corba.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilMisc.h>
#include "OcpiStringifyCorbaException.h"
#include "OcpiStringifyNamingException.h"
#include "OcpiCORBAUtilNameServiceBind.h"

void
OCPI::CORBAUtil::Misc::
nameServiceBind (CosNaming::NamingContextExt_ptr ns,
                 CORBA::Object_ptr objToBind,
                 const std::string & nameStr,
                 bool createPath,
                 bool rebind)
  throw (std::string)
{
  if (CORBA::is_nil (ns)) {
    throw std::string ("Naming Service reference is nil");
  }

  try { 
    CosNaming::NamingContext_var context =
      CosNaming::NamingContext::_duplicate (ns);

    CosNaming::Name_var name = ns->to_name (nameStr.c_str());
    CORBA::ULong nameDirs = name->length () - 1;

    for (CORBA::ULong nameDirIdx=0; nameDirIdx<nameDirs; nameDirIdx++) {
      CosNaming::Name dirName;
      dirName.length (1);
      dirName[0] = name[nameDirIdx];

      CORBA::Object_var dirNc;

      try {
        dirNc = context->resolve (dirName);
      }
      catch (const CosNaming::NamingContext::NotFound &) {
        if (createPath) {
          dirNc = context->bind_new_context (dirName);
        }
        else {
          throw;
        }
      }

      context = CosNaming::NamingContext::_narrow (dirNc);
    }

    CosNaming::Name elementName;
    elementName.length (1);
    elementName[0] = name[nameDirs];

    if (rebind) {
      context->rebind (elementName, objToBind);
    }
    else {
      context->bind (elementName, objToBind);
    }
  }
  catch (const CORBA::Exception & ex) {
    throw stringifyNamingException (ex);
  }
  catch (...) {
    ocpiAssert (0);
  }

}

