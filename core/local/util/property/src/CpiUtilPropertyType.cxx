#include <limits.h>
#include <stdio.h>
#include "CpiUtilPropertyType.h"

namespace CPI {
  namespace Util {
    namespace Prop {
      const char *Scalar::names[] = {
        "None",
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) #pretty,
        CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
        0
      };
      uint8_t Scalar::sizes[] = {
	0,// for CPI_NONE
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) bits,
	CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
      };
    }
  }
}
