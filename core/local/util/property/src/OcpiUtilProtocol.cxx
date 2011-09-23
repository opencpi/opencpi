
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
#include "OcpiUtilProtocol.h"

namespace OCPI {
  namespace OP = OCPI::Util::Prop;
  namespace OE = OCPI::Util::EzXml;
  namespace Metadata {
    Operation::Operation()
      : m_isTwoWay(false), m_nArgs(0), m_args(NULL), m_maxAlign(1), m_myOffset(0), m_sub32(false) {
    }
    Operation::~Operation() {
      if (m_args)
	delete [] m_args;
    }
    const char *Operation::parse(ezxml_t op) {
      const char *err;
      if ((err = OE::checkAttrs(op, "Name", "Twoway", (void*)0)))
	return err;
      const char *name = ezxml_cattr(op, "Name");
      if (!name)
	return "Missing \"Name\" attribute for operation";
      m_name = name;
      if (!(err = OE::getBoolean(op, "TwoWay", &m_isTwoWay)))
	err = OP::Member::parseMembers(op, m_nArgs, m_args, m_maxAlign,
				       m_myOffset, m_sub32, "argument");
      return err;
    }
    Protocol::Protocol()
      : m_nOperations(0), m_operations(NULL), m_op(NULL), m_dataValueWidth(8),
	m_maxMessageValues(0), m_dataValueGranularity(1), m_variableMessageLength(false),
	m_zeroLengthMessages(false), m_diverseDataSizes(false), m_isTwoWay(false)
    {
    }
    Protocol::
    Protocol(const Protocol & p )
    {
      *this = p;
    }
    Protocol::~Protocol() {
      if (m_operations)
	delete [] m_operations;
    }
    Protocol & 
    Protocol::
    operator=( const Protocol & p ) 
    {
      return operator=(&p);
    }
    Protocol & 
    Protocol::
    operator=( const Protocol * p )
    {
      m_nOperations = p->m_nOperations;
      m_name = p->m_name;
      m_dataValueWidth = p->m_dataValueWidth;
      m_maxMessageValues = p->m_maxMessageValues;
      m_dataValueGranularity = p->m_dataValueGranularity;
      m_variableMessageLength = p->m_variableMessageLength;
      m_zeroLengthMessages = p->m_zeroLengthMessages;
      m_diverseDataSizes = p->m_diverseDataSizes;
      m_isTwoWay = p->m_isTwoWay;
      m_operations = new Operation[m_nOperations];
      for ( unsigned int n=0; n<m_nOperations; n++ ) {
	m_operations[n] = p->m_operations[n];
      }
      return *this;
    }


    Operation & 
    Operation::
    operator=(const Operation & p )
    {
      return operator=(&p);
    }
    
    Operation & 
    Operation::
    operator=(const Operation * p )
    {
      m_name = p->m_name;
      m_isTwoWay = p->m_isTwoWay;
      m_nArgs = p->m_nArgs;
      m_maxAlign = p->m_maxAlign;
      m_myOffset = p->m_myOffset;
      m_sub32 = p->m_sub32;
      m_args = new OCPI::Util::Prop::Member[ m_nArgs ];
      for (unsigned int n=0; n<m_nArgs; n++ ) {
	m_args[n] = p->m_args[n];
      }
      return *this;
    }



    const char *Protocol::
    parseOperation(ezxml_t op) {
      const char *name = ezxml_name(op);
      if (!name || strcasecmp(name, "Operation"))
	return "Element under Protocol is neither Operation, Protocol or or xi:include";
      // If this is NULL we're just counting properties.
      if (!m_operations) {
	m_nOperations++;
	return NULL;
      }
      Operation *o = m_op++;
      const char *err = o->parse(op);
      if (!err) {
	// Infer protocol attributes from the operation
	if (o->m_isTwoWay)
	  m_isTwoWay = true;
	if (o->m_nArgs) {
	  OP::Member *arg = o->m_args;
	  for (unsigned i = 0; i < o->m_nArgs; i++, arg++) {
	    if (m_dataValueWidth &&
		arg->bits != m_dataValueWidth)
	      m_diverseDataSizes = true;
	    if (!m_dataValueWidth ||
		arg->bits < m_dataValueWidth)
	      m_dataValueWidth = arg->bits;
	  }
	  if (m_maxMessageValues &&
	      m_maxMessageValues != o->m_myOffset)
	    m_variableMessageLength = true;
	  if (o->m_myOffset > m_maxMessageValues)
	    m_maxMessageValues = o->m_myOffset; // still in bytes until later
	} else
	  m_zeroLengthMessages = true;
      }
      return err;
    }
    // Interface for C iterator in ezxml
    const char *doOperation(ezxml_t op, void *arg) {
      return ((Protocol *)arg)->parseOperation(op);
    }
    const char *Protocol::parse(ezxml_t prot) {
      const char *err;
      if ((err = OE::checkAttrs(prot, "Name", (void*)0)))
	return err;
      const char* n = ezxml_cattr(prot, "name");
      if (n) m_name = n;

      // First time we call this it will just be for counting.
      if (!(err = OE::ezxml_children(prot, doOperation, this))) {
	m_operations = m_op = new Operation[m_nOperations];
	// Now we call a second time t make them.
	if (!(err = OE::ezxml_children(prot, doOperation, this)) &&
	    m_dataValueWidth) {
	  // Convert max size from bytes back to values
	  unsigned bytes = (m_dataValueWidth + CHAR_BIT - 1) / CHAR_BIT;
	  m_maxMessageValues += bytes - 1;
	  m_maxMessageValues /= bytes;
	  m_dataValueGranularity = 1;  // FIXME - compute this for real
	}
      }
      return err;
    }
  }
}
