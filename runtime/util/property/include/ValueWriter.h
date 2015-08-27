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
  void newItem(Member &m);
public:
  void beginSequence(Member &m, size_t nElements);
  void beginStruct(Member &m);
  void endStruct(Member &);
  void beginType(Member &m);
  void endType(Member &);
  void writeString(Member &m, WriteDataPtr p, size_t strLen, bool start, bool /*top*/);
  void writeData(Member &m, WriteDataPtr p, size_t nBytes, size_t );
};
  }
}
#endif
