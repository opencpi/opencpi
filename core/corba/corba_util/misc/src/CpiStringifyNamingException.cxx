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
#include <CpiUtilMisc.h>
#include "CpiStringifyCorbaException.h"
#include "CpiStringifyNamingException.h"

#if defined (CPI_USES_TAO)
#include <sstream>
#endif

std::string
CPI::CORBAUtil::Misc::
stringifyNamingException (const CORBA::Exception & ex,
                          CosNaming::NamingContext_ptr ns)
  throw ()
{
  std::string reason;

  const CosNaming::NamingContext::NotFound * nf =
    CosNaming::NamingContext::NotFound::_downcast (&ex);
  const CosNaming::NamingContext::CannotProceed * cp =
    CosNaming::NamingContext::CannotProceed::_downcast (&ex);

  if (nf) {
    reason = "Not found, ";

    if (nf->why == CosNaming::NamingContext::missing_node) {
      reason += "missing node";
    }
    else if (nf->why == CosNaming::NamingContext::not_context) {
      reason += "object where context was expected";
    }
    else if (nf->why == CosNaming::NamingContext::not_object) {
      reason += "context where object was expected";
    }

    CosNaming::NamingContextExt_var nc =
      CosNaming::NamingContextExt::_narrow (ns);

    if (!CORBA::is_nil (nc) && nf->rest_of_name.length()) {
      CORBA::String_var rest;

      try {
        rest = nc->to_string (nf->rest_of_name);
      }
      catch (...) {
        rest = const_cast<const char *> ("(unknown)");
      }

      reason += ", remaining name \"";
      reason += rest.in();
      reason += "\"";
    }
  }
  else if (cp) {
    reason = "Cannot proceed";

    CosNaming::NamingContextExt_var nc =
      CosNaming::NamingContextExt::_narrow (ns);

    if (!CORBA::is_nil (nc) && cp->rest_of_name.length()) {
      CORBA::String_var rest;

      try {
        rest = nc->to_string (cp->rest_of_name);
      }
      catch (...) {
        rest = const_cast<const char *> ("(unknown)");
      }

      reason += ", remaining name \"";
      reason += rest.in();
      reason += "\"";
    }
  }
  else if (CosNaming::NamingContext::InvalidName::_downcast (&ex)) {
    reason = "Invalid name";
  }
  else {
    reason = stringifyCorbaException (ex);
  }

  return reason;
}

