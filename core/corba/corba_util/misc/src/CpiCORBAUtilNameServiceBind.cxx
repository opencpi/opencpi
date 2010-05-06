// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

#include <string>
#include <corba.h>
#include <CpiOsAssert.h>
#include <CpiUtilMisc.h>
#include "CpiStringifyCorbaException.h"
#include "CpiStringifyNamingException.h"
#include "CpiCORBAUtilNameServiceBind.h"

void
CPI::CORBAUtil::Misc::
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
    cpiAssert (0);
  }

}

