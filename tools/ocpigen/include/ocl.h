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

#ifndef OCL_H
#define OCL_H
#include "wip.h"
#include "data.h"

#if 0
#define RCC_ASSEMBLY_ATTRS "platform", "config", "configuration"
#define RCC_ASSEMBLY_ELEMS "connection"
// These are for all implementaitons whether assembly or written
#define RCC_TOP_ATTRS "Pattern", "PortPattern", "DataWidth", "Language", "library"
// These are for implementaitons that you write (e.g. not generated assemblies)
#define RCC_IMPL_ATTRS GENERIC_IMPL_CONTROL_ATTRS, "RawProperties", "FirstRawProperty", "outer"
#define RCC_IMPL_ELEMS "timeinterface", "memoryinterface", "streaminterface", "messageinterface", "signal", "cpmaster", "time_service", "control", "metadata"
#endif

class OclPort : public DataPort {
public:
  OclPort(Worker &w, ezxml_t x, DataPort *sp, int ordinal, const char *&err);
};
#if 0
class RccAssembly : public Worker {
public:  
  static RccAssembly *
  create(ezxml_t xml, const char *xfile, const char *&err);
  RccAssembly(ezxml_t xml, const char *xfile, const char *&err);
  virtual ~RccAssembly();
};
#endif
#endif
