#include <assert.h>
#include <string.h>
#include "ValueReader.h"

namespace OA = OCPI::API;

namespace OCPI {
  namespace Util {

ValueReader::
ValueReader(const Value **v)
  : m_values(v), m_v(0), m_parent(NULL), m_first(true), m_n(0) {
}

// Return the value we should use.
void ValueReader::
nextItem(const Member &m, bool seq) {
  if (m.m_isSequence && !seq)
    return; // sequences are explicitly started
  if (m_first) {
    m_v = m_values[0];
    m_first = false;
  } else if (!m_parent)
    m_v = m_values[++m_n];
  else if (m_parent->m_vt->m_baseType == OA::OCPI_Struct) {
    // We are driven by m_parent (the struct) and the ordinal of the incoming member
    if (m_parent->m_struct) {
      StructValue sv = m_parent->m_vt->m_arrayRank || m_parent->m_vt->m_isSequence ?
	m_parent->m_pStruct[m_parent->m_next] : m_parent->m_Struct;
      // elements of an array or sequence are allowed to be "empty"
      m_v = sv ? sv[m.m_ordinal] : NULL;
    } else
      m_v = NULL;
    if (!m_v)
      m_v = m.m_default;
    if (m.m_ordinal == m_parent->m_vt->m_nMembers - 1)
      m_parent->m_next++;
  } else if (m_parent->m_vt->m_baseType == OA::OCPI_Type) {
    m_v =
      m_parent->m_types ?
      (m_parent->m_vt->m_arrayRank || m_parent->m_vt->m_isSequence ?
       m_parent->m_pType[m_parent->m_next] : m_parent->m_Type) :
      NULL;
    if (!m_v)
      m_v = m.m_default;
    m_parent->m_next++;
  } else
    assert("Recursive type not struct/type"==0);
  if (m_v)
    m_v->m_next = 0;
}
  
// Either a top level sequence, or a sequence under type or struct
// This makes it obvious when we are recursing.
size_t ValueReader::
beginSequence(const Member &m) {
  nextItem(m, true);
#if 0
  if (m_v) {
    return m_v->m_nElements;
  } else
    return 0;
#endif
  return m_v ? m_v->m_nElements : 0;
}
void ValueReader::
beginStruct(const Member &m) {
  nextItem(m, false);
  if (!m_v) {
    // missing member value for member that is struct
    m_parent = new Value(m, m_parent);
  } else
    m_parent = m_v;
  //   m_parent->m_next = 0;
}
void ValueReader::
endStruct(const Member &) {
  const Value *v = m_parent;
  m_parent = m_parent->m_parent;
  if (v->m_struct == NULL)
    delete v;
}
void ValueReader::beginType(const Member &m) {
  nextItem(m, false);
  if (!m_v) {
    // missing member value for member that is struct
    m_parent = new Value(m, m_parent);
  } else
    m_parent = m_v;
}
void ValueReader::
endType(const Member &) {
  const Value *v = m_parent;
  m_parent = m_parent->m_parent;
  if (v->m_types == NULL)
    delete v;
}

// A leaf request
size_t ValueReader::
beginString(const Member &m, const char *&chars, bool start) {
  if (start)
    nextItem(m);
  if (!m_v) {
    chars = "";
    return 0;
  }
  chars = m.m_arrayRank || m.m_isSequence ? m_v->m_pString[m_v->m_next++] : m_v->m_String;
  return strlen(chars);
}

// Called on a leaf, with contiguous non-string data
void ValueReader::
readData(const Member &m, ReadDataPtr p, size_t nBytes, size_t nElements, bool fake) {
  nextItem(m);
  (void)nElements;
  if (fake)
    return;
  if (!m_v)
    memset((void *)p.data, 0, nBytes);
  else
    memcpy((void *)p.data,
	   (void *)(m.m_isSequence || m.m_arrayRank ? m_v->m_pULong : &m_v->m_ULong),
	   nBytes);
}
}
}
