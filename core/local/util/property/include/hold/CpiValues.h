#ifndef CPIVALUES_H
#define CPIVALUES_H
#include "CpiOsStdint.h"
#include "CpiMetadataProperty.h"
#include "CpiPValue.h"

namespace CPI {

  namespace Util {

extern bool
  parseBool(const char *a, unsigned length, bool *b),
  getUNum64(const char *s, uint64_t *valp);

extern const char
  *parseValue(const char *value, CPI::Metadata::Property::Type type, unsigned length,
	      Value *v);

}}

#endif
