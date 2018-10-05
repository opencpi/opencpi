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

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Sep 10 10:28:53 2015 EDT
 * BASED ON THE FILE: ptest_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the ptest_cc worker in C++
 */

#include "ptest_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Ptest_ccWorkerTypes;

class Ptest_ccWorker : public Ptest_ccWorkerBase {
  RCCResult initialize() {
    // until language mapping is yet more c++11ish
#define SET(name,type,length,...)					\
    do {								\
      m_properties.name.resize(length);					\
      type tmp[length] = __VA_ARGS__;					\
      for (unsigned i = 0; i < length; i++)				\
        m_properties.name.data[i] = tmp[i];				\
    } while(0)
#define SETA(name,type,length,...)					\
    do {								\
      type tmp[length] = __VA_ARGS__;					\
      for (unsigned i = 0; i < length; i++)				\
        m_properties.name[i] = tmp[i];					\
    } while(0)
#define SETSTR(name,length,slength,...)					\
    do {								\
      m_properties.name.resize(length);					\
      char tmp[length][slength+1] = __VA_ARGS__;			\
      for (unsigned i = 0; i < length; i++)				\
	for (unsigned j = 0; j <= slength; j++)				\
	  m_properties.name.data[i][j] = tmp[i][j];			\
    } while(0)
      
    SET(testvolseqbool,bool,4,{false,true,false,true});
    SET(testvolsequlonglong,uint64_t,4,{0, 4, ~0ull, 1ull << 36});
    SETSTR(testvolseqstring,5,5,{"hell0","hell1","hell2",""});
    SET(testvolseqenum,Testvolseqenum,6,{TESTVOLSEQENUM_R, TESTVOLSEQENUM_G, TESTVOLSEQENUM_B});
    SET(testrdseqbool,bool,4,{false,true,false,true});
    SET(testrdsequlonglong,uint64_t,4,{0, 4, ~0ull, 1ull << 36});
    SETSTR(testrdseqstring,5,5,{"hell0","hell1","hell2",""});
    SET(testrdseqenum,Testrdseqenum,6,{TESTRDSEQENUM_R, TESTRDSEQENUM_G, TESTRDSEQENUM_B});
    SETA(testvolarybool,bool,4,{false,true,false,true});
    SETA(testrdarybool,bool,4,{false,true,false,true});

    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    return RCC_DONE;
  }
};

PTEST_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
PTEST_CC_END_INFO
