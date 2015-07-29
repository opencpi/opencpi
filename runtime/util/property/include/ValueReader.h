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
  void readData(const Member &m, ReadDataPtr p, size_t nBytes, size_t nElements);
};
  }
}
#endif
