// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

/*
 * Abstact:
 *   This file defines the PValue class for types property value lists
 *
 * Revision History: 
 * 
 *    Author: Jim Kulp
 *    Date: 7/2009
 *    Revision Detail: Created
 *
 */

#ifndef CPI_PVALUE_H
#define CPI_PVALUE_H

#include <CpiMetadataProperty.h>
#include <cstring>


namespace CPI {

  namespace Util {

    // Convenience class for type safe property values for "simplest" api.
    // Only non-type-safe aspect is PVEnd()
    // Typical syntax would be like:
    // PValue props[] = { PVString("label", "foolabel"), PVULong("nbuffers", 10), PVEnd()};
    // A less type-safe would be to use varargs, which only saves a single character per property...
    // PVList props("label", PVString, "foolabel", "nbuffers", PVUlong, 10, 0);

    class PValue {
    public:
      const char *name;
      CPI::Metadata::Property::Type type;
      int width;
      union {
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) run v##pretty;
CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
      };
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) friend class PV##pretty;
CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE  
     static inline const PValue* find( const PValue* p, const char* name ) {
     if (!p ) return NULL;
     for ( int n=0; p[n].name; n++) {
         if (strcmp(p[n].name, name ) == 0) {
           return &p[n];
         }
       }
       return NULL;
     }
    };
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)        \
    class PV##pretty : public PValue {                                \
    public:                                                        \
          inline PV##pretty(const char *aname, const run value) {        \
            name = aname;                                        \
            type = CPI::Metadata::Property::CPI_##pretty;        \
            v##pretty = (run)value;                                \
            width = sizeof(v##pretty);                          \
          }                                                        \
    };
    CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
    extern PVULong PVEnd;
  } // Container
} // CPI
#endif

