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

#ifndef VALUE_READER_H
#define VALUE_READER_H
#include "OcpiUtilValue.h"

namespace OCPI {
  namespace Util {

// Our reader object that supplies data from our Value data structure
class ValueReader : public Reader {
  const Value **m_values;
  const Value *m_v;
  const Value *m_parent; // parent of current node
  bool m_first;
  unsigned m_n;        // index in top level vector (m_values);

public:
  ValueReader(const Value **v);
private:
  // Return the value we should use.
  void nextItem(const Member &m, bool seq = false);
public:
  // Either a top level sequence, or a sequence under type or struct
  // This makes it obvious when we are recursing.
  size_t beginSequence(const Member &m);
  void beginStruct(const Member &m);
  void endStruct(const Member &);
  void beginType(const Member &m);
  void endType(const Member &);
  // A leaf request
  size_t beginString(const Member &m, const char *&chars, bool start);
  // Called on a leaf, with contiguous non-string data
  void readData(const Member &m, ReadDataPtr p, size_t nBytes, size_t nElements,
		bool fake = false);
};
  }
}
#endif
