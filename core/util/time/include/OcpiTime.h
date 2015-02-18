// These calls are not OS specific, but may be CPU specific - hopefully captured
// in the "cycle.h" file from fftw.org

// Get best real time
// Could be from our FPGA, could be from fast time, could be from OS.
// Get fastest tick time
// Get fastest tick and our time together.

// So we have our time source:
// - initialize from system to FPGA, then CPU.
// - initialize time from FPGA + GPS



#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#ifdef __tile__
static inline uint64_t getticks()
{
  return 0;
}
#endif


#ifdef __arm__ 
static inline uint64_t getticks()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  
  return
    ((uint64_t)tv.tv_usec << 32) |
    ((tv.tv_usec * ((uint64_t)0x100000000ull + 500))/1000);
}
#else
#include "cycle.h" // This is an externally supplied header file from fftw.org
#endif

class Time {
public:
  static inline uint64_t getTicks() { return getticks(); }

  typedef uint64_t GpsTime, UtcTime, OcpiTime;
  static GpsTime utc2gps(time_t utc);
  static UtcTime gps2utc(GpsTime gps);
};
