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

#if defined (CPI_USES_OMNI)
#include <omniORB4/CORBA.h>
#include <omniORB4/streamOperators.h>
#endif

#if defined (CPI_USES_TAO)
#include <tao/corba.h>
#include <tao/PortableServer/PortableServer.h>
#include <tao/AnyTypeCode/BooleanSeqA.h>
#include <tao/AnyTypeCode/CharSeqA.h>
#include <tao/AnyTypeCode/DoubleSeqA.h>
#include <tao/AnyTypeCode/FloatSeqA.h>
#include <tao/AnyTypeCode/ShortSeqA.h>
#include <tao/AnyTypeCode/LongSeqA.h>
#include <tao/AnyTypeCode/OctetSeqA.h>
#include <tao/AnyTypeCode/ULongSeqA.h>
#include <tao/AnyTypeCode/UShortSeqA.h>
#include <tao/AnyTypeCode/StringSeqA.h>
#endif
