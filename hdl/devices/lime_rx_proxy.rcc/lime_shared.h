#ifndef _LIME_SHARED_H_
#define _LIME_SHARED_H_
#include <stdint.h>
// Shared functions between lime rx and tx proxies.
namespace OCPI {
  namespace Lime {
  struct Divider {
    uint8_t nint, nfrac_hi, nfrac_mid, nfrac_lo;
  };
  void calcDividers(double pll_hz, double lo_hz, Divider &div);
  // Return a string error if there was one.  Otherwise NULL
  const char
    *setVcoCap(uint8_t (*readVtune)(void *arg), void (*writeVcoCap)(void *arg, uint8_t val),
	      void *arg),
    *getLpfBwValue(float hz, uint8_t &regval),
    *getFreqValue(float hz, uint8_t &regval);
  }
}
#endif
