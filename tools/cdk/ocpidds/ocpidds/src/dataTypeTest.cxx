#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "OcpiUtilProtocol.h"
#include "OcpiUtilValue.h"
#include "ocpidds.h"

namespace OU = OCPI::Util;
namespace OA = OCPI::API;

// Our reader object that supplies data from our Value data structure
class Reader : public OU::Reader {
  OU::Value **m_values;
  OU::Value *m_v;
  OU::Value *m_parent; // parent of current node
  bool m_first;
  unsigned m_n;        // index in top level vector (m_values);

public:
  Reader(OU::Value **v)
    : m_values(v), m_v(0), m_parent(NULL), m_first(true), m_n(0) {
  }

  // Return the value we should use.
  void nextItem(OU::Member &m, bool seq = false) {
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
	OU::StructValue sv = m_parent->m_vt->m_arrayRank || m_parent->m_vt->m_isSequence ?
	  m_parent->m_pStruct[m_parent->m_next] : m_parent->m_Struct;
	m_v = sv[m.m_ordinal];
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
  
public:
  // Either a top level sequence, or a sequence under type or struct
  // This makes it obvious when we are recursing.
  size_t beginSequence(OU::Member &m) {
    nextItem(m, true);
#if 0
    if (m_v) {
      return m_v->m_nElements;
    } else
      return 0;
#endif
        return m_v ? m_v->m_nElements : 0;
  }
  void beginStruct(OU::Member &m) {
    nextItem(m, false);
    if (!m_v) {
      // missing member value for member that is struct
      m_parent = new OU::Value(m, m_parent);
    } else
      m_parent = m_v;
  }
  void endStruct(OU::Member &) {
    OU::Value *v = m_parent;
    m_parent = m_parent->m_parent;
    if (v->m_struct == NULL)
      delete v;
  }
  void beginType(OU::Member &m) {
    nextItem(m, false);
    if (!m_v) {
      // missing member value for member that is struct
      m_parent = new OU::Value(m, m_parent);
    } else
      m_parent = m_v;
  }
  void endType(OU::Member &) {
    OU::Value *v = m_parent;
    m_parent = m_parent->m_parent;
    if (v->m_types == NULL)
      delete v;
  }

  // A leaf request
  size_t beginString(OU::Member &m, const char *&chars, bool start) {
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
  void readData(OU::Member &m, OU::ReadDataPtr p, size_t nBytes, size_t nElements) {
    nextItem(m);
    (void)nElements;
    if (!m_v)
      memset((void *)p.data, 0, nBytes);
    else
      memcpy((void *)p.data,
	     (void *)(m.m_isSequence || m.m_arrayRank ? m_v->m_pULong : &m_v->m_ULong),
	     nBytes);
  }
  // void end() {}
};

// Class to "write" demarshalled data into "Value" data structure.

class Writer : public OU::Writer {
  OU::Value **m_values;
  OU::Value *m_v;
  OU::Value *m_parent; // parent of current node
  //  bool m_first;
  size_t m_nElements;
  size_t m_n, m_nArgs;        // index in top level vector (m_values);

  void newItem(OU::Member &m) {
    if (!m_parent) {
      assert(m_n < m_nArgs);
      m_values[m_n++] = m_v = new OU::Value(m, NULL);
    } else if (m_parent->m_vt->m_baseType == OA::OCPI_Type) {
      assert((size_t)(m_parent->m_typeNext - m_parent->m_types) < m_parent->m_nTotal);
      m_v = m_parent->m_typeNext++;
    } else if (m_parent->m_vt->m_baseType == OA::OCPI_Struct)
      m_v = new OU::Value(m, m_parent);
    else
      assert("recursive type not struct/type"==0);
    m_v->m_nTotal = m_v->m_vt->m_nItems;
    if (m_v->m_vt->m_isSequence) {
      m_v->m_nElements = m_nElements;
      m_v->m_nTotal *= m_nElements;
    }
    m_v->allocate();
    if (m_parent && m_parent->m_vt->m_baseType == OA::OCPI_Struct) {
      OU::StructValue sv = m_parent->m_vt->m_arrayRank || m_parent->m_vt->m_isSequence ?
	m_parent->m_pStruct[m_parent->m_next] : m_parent->m_Struct;
      sv[m.m_ordinal] = m_v;
      if (m.m_ordinal == m_parent->m_vt->m_nMembers - 1)
	m_parent->m_next++;
    }
  }
public:
  Writer(OU::Value **v, size_t nArgs)
    : m_values(v), m_v(NULL), m_parent(NULL), m_nElements(0), m_n(0), m_nArgs(nArgs) // m_first(true),
  {
  }
  void beginSequence(OU::Member &m, size_t nElements) {
    m_nElements = nElements;
    if (!nElements)
      newItem(m);
  }
  void beginStruct(OU::Member &m) {
    newItem(m);
    if (m.m_isSequence || m.m_arrayRank)
      for (unsigned n = 0; n < m_v->m_nTotal; n++)
	m_v->m_pStruct[n] = &m_v->m_struct[n * m.m_nMembers];
    else
      m_v->m_Struct = m_v->m_struct;
    m_parent = m_v;
  }
  void endStruct(OU::Member &) {
    m_parent = m_parent->m_parent;
  }
  void beginType(OU::Member &m) {
    newItem(m);
    if (m.m_isSequence || m.m_arrayRank)
      for (unsigned n = 0; n < m_v->m_nTotal; n++)
	    m_v->m_pType[n] = &m_v->m_types[n];
    else
      m_v->m_Type = &m_v->m_types[0];
    m_parent = m_v;
  }
  void endType(OU::Member &) {
    m_parent = m_parent->m_parent;
  }
  void writeString(OU::Member &m, OU::WriteDataPtr p, size_t strLen, bool start, bool /*top*/) {

    if (start)
      newItem(m);
    assert(m_v->m_stringNext + strLen + 1 <= m_v->m_stringSpace + m_v->m_stringSpaceLength);
    (m.m_arrayRank || m.m_isSequence ? m_v->m_pString[m_v->m_next++] : m_v->m_String) = 
      m_v->m_stringNext;
    memcpy(m_v->m_stringNext, p.data, strLen);
    m_v->m_stringNext += strLen;
    *m_v->m_stringNext++ = 0;
    // autoexpand.
  }
  void writeData(OU::Member &m, OU::WriteDataPtr p, size_t nBytes, size_t ) {
    newItem(m);
    assert(nBytes <= m_v->m_length);
    memcpy((void *)(m.m_isSequence || m.m_arrayRank ? m_v->m_pULong : &m_v->m_ULong),
	   p.data, nBytes);
  }
};
void dataTypeTest(const char *arg) {
  unsigned count;
  OU::Protocol pp, *ppp;
  if (isdigit(*arg)) {
    count = atoi(arg);
    ppp = NULL;
  } else {
    char buf[10000];
    int fd = open(arg, O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "Can't open %s\n", arg);
      return;
    }
    ssize_t nread = read(fd, buf, sizeof(buf));
    if (nread <= 0 || nread >= (ssize_t)sizeof(buf)) {
      fprintf(stderr, "Can't read file: %s\n", arg);
      return;
    }
    buf[nread] = 0;
    pp.parse(buf);
    ppp = &pp;
    count = 1;
  }
  for (unsigned n = 0; n < count; n++) {
    OU::Protocol genp;
    OU::Protocol &p = ppp ? *ppp : genp;
    if (!ppp)
      p.generate("test");
    p.printXML(stdout);
    OU::Value **v;
    uint8_t opcode = 0;
    p.generateOperation(opcode, v);
    p.printOperation(stdout, opcode, v);
    p.testOperation(stdout, opcode, v);
    printf("Min Buffer Size: %zu %zu %zu\n", p.m_minBufferSize, p.m_dataValueWidth, p.m_minMessageValues);
    fflush(stdout);
    Reader r(v);
    unsigned len = 1000000;
    uint8_t *buf = new uint8_t[len];
    memset(buf, 0, len);
    size_t rlen = p.read(r, buf, len, opcode);
    printf("Length was %zu\n", rlen);
    size_t nArgs = p.m_operations[opcode].m_nArgs;
    OU::Value **v1 = new OU::Value *[nArgs];
    Writer w(v1, nArgs);
    p.write(w, buf, len, opcode);
    uint8_t *buf1 = new uint8_t[rlen];
    memset(buf1, 0, rlen);
    Reader r1(v1);
    size_t rlen1 = p.read(r1, buf1, rlen, opcode);
    assert(rlen == rlen1);
    int dif = memcmp(buf, buf1, rlen);
    if (dif)
      for (unsigned n = 0; n < rlen; n++)
	if (buf[n] != buf1[n]) {
	  printf("Buffer differs at byte %u [%llx %llx]\n",
		 n, (long long unsigned)&buf[n], (long long unsigned)&buf1[n]);
	  assert("buffers different"==0);
	}
    for (unsigned n = 0; n < nArgs; n++) {
      delete v[n];
      delete v1[n];
    }
    delete []v;
    delete []v1;
    delete []buf;
    delete []buf1;
  }
  fprintf(stderr, "Data type test succeeded with %u randomly generated types and values.\n", count);
}
