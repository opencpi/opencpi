/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Dec 16 19:54:46 2013 EST
 * BASED ON THE FILE: gen/hello_world.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello_world
 */
#include "hello_world_Worker.h"

HELLO_WORLD_METHOD_DECLARATIONS;
RCCDispatch hello_world = {
 /* insert any custom initializations here */
 HELLO_WORLD_DISPATCH
};

/*
 * Methods to implement for worker hello_world, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)self;(void)timedOut;(void)newRunCondition;
 printf("Hello, world\n");
 sleep(1);
 return RCC_ADVANCE;
}
