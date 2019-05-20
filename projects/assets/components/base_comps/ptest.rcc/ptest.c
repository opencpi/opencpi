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
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Aug 28 21:01:13 2012 EDT
 * BASED ON THE FILE: ptest.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: ptest
 */
#include "ptest_Worker.h"

PTEST_METHOD_DECLARATIONS;
RCCDispatch ptest = {
 /* insert any custom initializations here */
 PTEST_DISPATCH
};

/*
 * Methods to implement for worker ptest, based on metadata.
 */

static RCCResult
initialize(RCCWorker *self) {
  PtestProperties *p = self->properties;
#define bool RCCBoolean
#define false 0
#define true 1
#define SET(name,type,len,...)						\
  do {									\
    p->name.length = len;						\
    type tmp[len] = __VA_ARGS__;					\
    for (unsigned i = 0; i < len; i++)					\
      p->name.data[i] = tmp[i];						\
  } while(0)
#define SETA(name,type,length,...)					\
  do {									\
    type tmp[length] = __VA_ARGS__;					\
    for (unsigned i = 0; i < length; i++)				\
      p->name[i] = tmp[i];						\
  } while(0)
#define SETSTR(name,len,slength,...)					\
  do {									\
    p->name.length = len;						\
    char tmp[len][slength+1] = __VA_ARGS__;				\
    for (unsigned i = 0; i < len; i++)					\
      for (unsigned j = 0; j <= slength; j++)				\
	p->name.data[i][j] = tmp[i][j];					\
  } while(0)
      
    SET(testvolseqbool,bool,4,{false,true,false,true});
    SET(testvolsequlonglong,uint64_t,4,{0, 4, ~0ull, 1ull << 36});
    SETSTR(testvolseqstring,5,5,{"hell0","hell1","hell2",""});
    SET(testvolseqenum,enum PtestTestvolseqenum,6,{PTEST_TESTVOLSEQENUM_R, PTEST_TESTVOLSEQENUM_G, PTEST_TESTVOLSEQENUM_B});
    SET(testrdseqbool,bool,4,{false,true,false,true});
    SET(testrdsequlonglong,uint64_t,4,{0, 4, ~0ull, 1ull << 36});
    SETSTR(testrdseqstring,5,5,{"hell0","hell1","hell2",""});
    SET(testrdseqenum,enum PtestTestrdseqenum,6,{PTEST_TESTRDSEQENUM_R, PTEST_TESTRDSEQENUM_G, PTEST_TESTRDSEQENUM_B});
    SETA(testvolarybool,bool,4,{false,true,false,true});
    SETA(testrdarybool,bool,4,{false,true,false,true});
 return RCC_OK;
}

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)timedOut;(void)newRunCondition;
 PtestProperties *p = self->properties;
 if (p->error)
   self->container.setError("This is a test error: %d", 1234);
 return RCC_DONE;
}
