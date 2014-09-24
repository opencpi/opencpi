/*
 Lime Proxy Worker Source Code

 File: lime_proxy.cc
 Company: Geon Technologies, LLC
 Engineer: Brandon Luquette

 Description: This worker translates top level transceiver properties into low level 
 register reads and writes for the Lime Microsytems LM6002D SPI Device Worker 

 Revision Log:

 09/22/14: BEL
 File Created

 */

#include "lime_proxy-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants

class Lime_proxyWorker : public Lime_proxyWorkerBase {

  RCCResult start() {
    uint8_t oldValue;
    //Set device worker properties according to initial proxy properties
    oldValue = slave.get_top_ctl0();
    //TX enable
    if(m_properties.tx_enable){
      slave.set_top_ctl0(oldValue | (1 << 3));
    }
    else{
      slave.set_top_ctl0(oldValue & (~(1 << 3)));
    }
    
    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    //Monitor proxy properties and update device worker properties when proxy properties change
    return RCC_ADVANCE;
  }
};

LIME_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
LIME_PROXY_END_INFO
