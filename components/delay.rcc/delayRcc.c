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
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Jul  2 16:01:20 2010 EDT
 * BASED ON THE FILE: delayRcc.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: delayRcc
 */
#include "delayRcc_Worker.h"

DELAYRCC_METHOD_DECLARATIONS;
RCCDispatch delayRcc = {
  /* insert any custom initializations here */
  DELAYRCC_DISPATCH
};

/*
 * Methods to implement for worker delayRcc, based on metadata.
*/

static RCCResult initialize(RCCWorker *self) {
  return RCC_OK;
}

static RCCResult start(RCCWorker *self) {
  return RCC_OK;
}

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  // Just put some hand-authored text here to make this file not replaced all the time
  return RCC_ADVANCE;
}
