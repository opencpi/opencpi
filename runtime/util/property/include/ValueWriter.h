/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VALUE_WRITER_H
#define VALUE_WRITER_H

#include "OcpiUtilValue.h"

namespace OCPI {
  namespace Util {


// Class to "write" demarshalled data into "Value" data structure.

class ValueWriter : public Writer {
  Value **m_values;
  Value *m_v;
  const Value *m_parent; // parent of current node
  //  bool m_first;
  size_t m_nElements;
  size_t m_n, m_nArgs;        // index in top level vector (m_values);
public:
  ValueWriter(Value **v, size_t nArgs);
private:
  void newItem(const Member &m);
public:
  void beginSequence(const Member &m, size_t nElements);
  void beginStruct(const Member &m);
  void endStruct(const Member &);
  void beginType(const Member &m);
  void endType(const Member &);
  void writeString(const Member &m, WriteDataPtr p, size_t strLen, bool start, bool /*top*/);
  void writeData(const Member &m, WriteDataPtr p, size_t nBytes, size_t );
};
  }
}
#endif
