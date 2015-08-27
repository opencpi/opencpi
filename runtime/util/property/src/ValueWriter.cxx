#include <assert.h>
#include <string.h>
#include "ValueWriter.h"
namespace OA = OCPI::API;
namespace OCPI {
  namespace Util {

// Class to "write" demarshalled data into "Value" data structure.

void ValueWriter::
newItem(Member &m) {
  if (!m_parent) {
    assert(m_n < m_nArgs);
    m_values[m_n++] = m_v = new Value(m, NULL);
  } else if (m_parent->m_vt->m_baseType == OA::OCPI_Type) {
    assert((size_t)(m_parent->m_typeNext - m_parent->m_types) < m_parent->m_nTotal);
    m_v = m_parent->m_typeNext++;
  } else if (m_parent->m_vt->m_baseType == OA::OCPI_Struct)
    m_v = new Value(m, m_parent);
  else
    assert("recursive type not struct/type"==0);
  m_v->m_nTotal = m_v->m_vt->m_nItems;
  if (m_v->m_vt->m_isSequence) {
    m_v->m_nElements = m_nElements;
    m_v->m_nTotal *= m_nElements;
  }
  m_v->allocate();
  if (m_parent && m_parent->m_vt->m_baseType == OA::OCPI_Struct) {
    StructValue sv = m_parent->m_vt->m_arrayRank || m_parent->m_vt->m_isSequence ?
      m_parent->m_pStruct[m_parent->m_next] : m_parent->m_Struct;
    sv[m.m_ordinal] = m_v;
    if (m.m_ordinal == m_parent->m_vt->m_nMembers - 1)
      m_parent->m_next++;
  }
}
ValueWriter::
ValueWriter(Value **v, size_t nArgs)
  : m_values(v), m_v(NULL), m_parent(NULL), m_nElements(0), m_n(0),
    m_nArgs(nArgs) {
}
void ValueWriter::
beginSequence(Member &m, size_t nElements) {
  m_nElements = nElements;
  if (!nElements)
    newItem(m);
}
void ValueWriter::
beginStruct(Member &m) {
  newItem(m);
  if (m.m_isSequence || m.m_arrayRank)
    for (unsigned n = 0; n < m_v->m_nTotal; n++)
      m_v->m_pStruct[n] = &m_v->m_struct[n * m.m_nMembers];
  else
    m_v->m_Struct = m_v->m_struct;
  m_parent = m_v;
}
void ValueWriter::
endStruct(Member &) {
  m_parent = m_parent->m_parent;
}
void ValueWriter::
beginType(Member &m) {
  newItem(m);
  if (m.m_isSequence || m.m_arrayRank)
    for (unsigned n = 0; n < m_v->m_nTotal; n++)
      m_v->m_pType[n] = &m_v->m_types[n];
  else
    m_v->m_Type = &m_v->m_types[0];
  m_parent = m_v;
}
void ValueWriter::
endType(Member &) {
  m_parent = m_parent->m_parent;
}
void ValueWriter::
writeString(Member &m, WriteDataPtr p, size_t strLen, bool start, bool /*top*/) {
  if (start)
    newItem(m);
  assert(m_v->m_stringNext + strLen + 1 <= m_v->m_stringSpace + m_v->m_stringSpaceLength);
  (m.m_arrayRank || m.m_isSequence ? m_v->m_pString[m_v->m_next++] : m_v->m_String) = 
    m_v->m_stringNext;
  if (strLen)
    memcpy(m_v->m_stringNext, p.data, strLen);
  m_v->m_stringNext += strLen;
  *m_v->m_stringNext++ = 0;
  // autoexpand.
}
void ValueWriter::
writeData(Member &m, WriteDataPtr p, size_t nBytes, size_t ) {
  newItem(m);
  assert(nBytes <= m_v->m_length);
  memcpy((void *)(m.m_isSequence || m.m_arrayRank ? m_v->m_pULong : &m_v->m_ULong),
	 p.data, nBytes);
}

  }
}

