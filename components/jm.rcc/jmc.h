
extern "C++" {
#include "jm-worker.hh"
#include <stdio.h>
}

#ifndef __cilkplusplus
#define __cilkplusplus
#endif
#include "cilk.h"


class JmWorkerCilk {

 public:

  // method executed in cilk context
  OCPI::RCC::RCCResult __cilk _cilk_run( JmWorkerBase & );


};



