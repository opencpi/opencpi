
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
#include <stdarg.h>
#include <unistd.h>
#include <cstdio>

#include "OcpiOsAssert.h"
#include "OcpiDriverManager.h"
#include "OcpiUtilException.h"

namespace OCPI {

  namespace API {
    Error::~Error(){}
  }
  namespace Util {
    // Convenience for single line, multi-string, API exceptions (API called badly)
    // Its easy to scan all callers for the terminating null
    ApiError::ApiError(const char *err, ...)
    {
      va_list ap;
      va_start(ap, err);
      setConcatenateV(err, ap);
      m_auxInfo = *this; // backward compatibility...
      ocpiInfo("ApiError Exception: %s", this->c_str());
    }
    ApiError::~ApiError(){}
    Error::Error(){}

    Error::Error(va_list ap, const char *err) {
      setFormatV(err, ap);
      ocpiInfo("Error Exception: %s", this->c_str());
    }
    Error::Error(std::string &s) {
      append(s.c_str());
    }
    Error::Error(const char *err, ...) {
      va_list ap;
      va_start(ap, err);
      setFormatV(err, ap);
      va_end(ap);
      ocpiInfo("Error Exception: %s", this->c_str());
    }      
    Error::Error(unsigned level, const char *err, ...) {
      va_list ap;
      va_start(ap, err);
      setFormatV(err, ap);
      va_end(ap);
      OS::logPrint(level, "Error Exception: %s", this->c_str());
    }

    void Error::setConcatenateV(const char *err, va_list ap) {
      append(err);
      const char *s;
      while ((s = va_arg(ap, const char*)))
	append(s);
    }
    void Error::setFormat(const char *err, ...) {
      va_list ap;
      va_start(ap, err);
      setFormatV(err, ap);
      va_end(ap);
    }
    void Error::setFormatV(const char *err, va_list ap) {
      char *s;
      vasprintf(&s, err, ap);
      if (OCPI::Driver::ManagerManager::exiting()) {
	static const char pre[] = "\n***Exception during shutdown: ";
	static const char post[] = "***\n";
	write(2, pre, strlen(pre));
	write(2, s, strlen(s));
	write(2, post, strlen(post));
	OCPI::OS::dumpStack();
      }
      append(s);
      free(s);
    }
    Error::~Error(){}

    EmbeddedException::EmbeddedException( 
					 OCPI::OS::uint32_t errorCode, 
					 const char* auxInfo,
					 OCPI::OS::uint32_t errorLevel )
      : m_errorCode(errorCode), m_errorLevel(errorLevel)
    {
      if (auxInfo)
	m_auxInfo = auxInfo;
      setFormat("Code 0x%x, level %u, error: '%s'", errorCode, errorLevel, auxInfo);
    }
      // String error only (error code zero)
    EmbeddedException::EmbeddedException( const char* auxInfo )
      : m_errorCode(0), m_auxInfo(auxInfo), m_errorLevel(0)
    {
      append(auxInfo);
    }
    EmbeddedException::~EmbeddedException(){}
  }
}
