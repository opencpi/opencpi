
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
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

/*

 Definitions for worker metadata encoding,decoding.
 Offline, in tools, this information is encoded into a string format 
 suitable for program level arguments (argv).  All properties are encoded into
 a single string.  This relieves runtime of any parsing overhead (time or space)
 or dependencies on external parsing libraries.

 The "scope" of this property support is configuration properties for CP289
 components.  Thus it is not (yet) intended to support SCA GPP components.

 This file defines the binary (non-string) format of SCA component properties,
 as well as the functions to encode (binary to string) and decode 
 (string to binary).

 
*/

//  This file implements workerk metadata

#ifndef OCPI_UTIL_WORKER_H
#define OCPI_UTIL_WORKER_H

#include "ezxml.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilException.h"
#include "OcpiUtilProperty.h"
#include "OcpiUtilProtocol.h"
#include "OcpiUtilPort.h"
#include "OcpiUtilMemory.h"
#include "OcpiExprEvaluator.h"

//!!!!!!! There is also this list in the OcpiContainerApi.h
#define CONTROL_OP_I CONTROL_OP
#define OCPI_CONTROL_OPS                                                        \
  CONTROL_OP_I(initialize,   Initialize,     INITIALIZED, EXISTS,      NONE,        NONE) \
  CONTROL_OP(start,          Start,          OPERATING,   SUSPENDED,   INITIALIZED, NONE) \
  CONTROL_OP(stop,           Stop,           SUSPENDED,   OPERATING,   NONE,        NONE) \
  CONTROL_OP(release,        Release,        EXISTS,      INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(beforeQuery,    BeforeQuery,    NONE,        INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(afterConfigure, AfterConfigure, NONE,        INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(test,           Test,           NONE,        INITIALIZED, NONE,        NONE) \
  /**/

namespace OCPI {
  namespace Util {

    // Attributes of an artifact and/or implementation
    // Generally shared by all the implementations in an artifact
    class Attributes {
    public:
	std::string
	  m_uuid,
	  m_os, m_osVersion,
	  m_platform,
	  m_tool, m_toolVersion,
	  m_runtime, m_runtimeVersion;
      inline const std::string &uuid() { return m_uuid; }
    protected:
      // Parse from target string
      void parse(const char *pString);
      // Parse from xml
      void parse(ezxml_t x);
      void validate();
    };

    class Test {
      friend class Implementation;
      unsigned int m_testId;
      unsigned int m_nInputs, m_nResults;
      unsigned int *m_inputValues;  // reference to property[n]
      unsigned int *m_resultValues;
    };

    // This class represents what we know, generically, about a component implementation
    // Currently there is no separate "spec" metadata - it is redundant in each implementation
    class Implementation  : public IdentResolver {
      std::string m_specName, m_name, m_model;
      Attributes *m_attributes; // not a reference due to these being in arrays
      Port *m_ports;
      Memory *m_memories;
      Test *m_tests;
      unsigned m_nPorts, m_nTests, m_nMemories;//, size;
      uint32_t m_totalPropertySize;
      Test &findTest(unsigned int testId) const;
    public:
      unsigned m_nProperties;
      Property *m_properties;
      ezxml_t m_xml;
      Implementation();
      ~Implementation();
      inline const std::string &model() const { return m_model; }
      inline const std::string &specName() const { return m_specName; }
      inline const Attributes &attributes() const { return *m_attributes; }
      const char *parse(ezxml_t xml, Attributes &attr);
      Property &findProperty(const char *id) const;
      unsigned whichProperty(const char *id) const;
      const char *getValue(const std::string &sym, ExprValue &val);
      inline Property *properties(unsigned &np) const {
        np = m_nProperties;
        return m_properties;
      }
      inline Property &property(unsigned long which) const
      {
        ocpiAssert(which < m_nProperties);
        return m_properties[which];
      }
      Port *findPort(const std::string &id) const;
      inline Port &port(unsigned long which) const
      {
        ocpiAssert(which < m_nPorts);
        return m_ports[which];
      }
      inline Port* ports( unsigned int& n_ports ) const
      {
        n_ports = m_nPorts;
        return m_ports;
      }
      inline unsigned int nPorts( ) const
      {
        return m_nPorts;
      }
      inline Memory* memories( unsigned int& n_memories ) const
      {
        n_memories = m_nMemories;
        return m_memories;
      }
      inline uint32_t totalPropertySize( ) const
      {
        return m_totalPropertySize;
      }
      enum ControlState {
        EXISTS,
        INITIALIZED,
        OPERATING,
        SUSPENDED,
        UNUSABLE,
        NONE
      };
      enum ControlOperation {
#define CONTROL_OP(x, c, t, s1, s2, s3)  Op##c,
      OCPI_CONTROL_OPS
#undef CONTROL_OP
      OpsLimit
      };
    };
  }
}
#endif
