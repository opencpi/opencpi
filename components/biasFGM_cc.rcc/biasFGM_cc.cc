/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Jun  5 21:24:44 2014 EDT
 * BASED ON THE FILE: bias_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the bias_cc worker in C++
 */

#include "OcpiApi.hh"
#include "biasFGM_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants

#define min(x,y) (x < y) ? x : y;


static bool spawned=false;
class MyTaskC : public RCCUserTask {
private:
  int m_rank;
  uint32_t m_count;
  uint32_t m_len;
  uint32_t m_bv;
  const uint32_t * m_indata;
  uint32_t * m_outdata;

public:
  void spawn( const uint32_t * in, uint32_t * out, int rank, uint32_t count, uint32_t len, uint32_t bv ) {
    m_rank = rank;
    m_count = count;
    m_len   = len;
    m_indata = in;
    m_outdata = out;
    m_bv = bv;
    RCCUserTask::spawn();
  }
  virtual void run() {
    printf( " In MyTask run. my rank=%d, count=%d, len=%d, bv=%d !!\n", m_rank, m_count,m_len,m_bv );
    int idx = m_rank;
    for (unsigned n = 0; n<m_len; n++) {// n is length in sequence elements of input
      m_outdata[idx+n] = m_indata[idx+n] + m_bv;
    }
  }
  void done() {
    printf("In task done\n");
  }
};


class BiasFGM_ccWorker : public BiasFGM_ccWorkerBase {
  static const int TC = 5;
  MyTaskC tasks[TC];
  RCCResult release() {
    this->join(true);
    return RCC_OK;
  }
  RCCResult stop() {
    printf("bias worker stopping\n");
    return RCC_OK;
  }


  RCCResult run(bool /*timedout*/) {


    const uint32_t *inData  = in.data().data().data();   // data arg of data message at "in" port
    uint32_t *outData = out.data().data().data();  // same at "out" port
    out.checkLength(in.length());               // make sure input will fit in output buffer
    if ( in.length() <= 1 ) {
      for (unsigned n = in.length(); n; n--) 
	*outData++ = *inData++ + properties().biasValue;
    }
    else {
      // We only need to spawn tasks if there arent any already working
      if ( ! spawned ) {
	printf("Spawing AGAIN !!\n");
	spawned = true;
	int count = min(TC,in.length()/4);
	unsigned part_len = in.length() / count / 4;
	for ( int n = 0; n < count; n++ ) {
	  tasks[n].spawn(inData, outData, n, count, part_len, properties().biasValue);
	}
      }
    }
    out.setInfo(in.opCode(), in.length());      // Set the metadata for the output message
    
    if ( join(false) ) {
      return in.length() ? RCC_ADVANCE : RCC_ADVANCE_DONE;  
      spawned = false;
    }
    else {
      return RCC_OK;
    }
  }

};

BIASFGM_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
BIASFGM_CC_END_INFO
