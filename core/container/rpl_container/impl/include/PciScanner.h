#ifndef PCISCANNER_H
#define PCISCANNER_H
#include <stdint.h>
namespace CPI {
  namespace PCI {
    struct Bar {
      uint64_t address;
      uint64_t size;
      bool io, prefetch;
      unsigned addressSize;
    };
    typedef bool Found(const char *name, Bar *bars, unsigned nBars);
    // See if the device with the given name is a good one.
    // nbars on input is size of Bars array.
    // nbars on output is number of good bars found
    // Return an error string or null on success
    bool
      probe(const char *name, unsigned theVendor, unsigned theDevice, unsigned theClass,
            unsigned theSubClass, Bar *bars, unsigned &nbars, const char *&err);
    // Search for devices of this vendor/device/class/subclass
    // call the "found" function for each such device,
    // set the output arg "count" to the number of devices found,
    // return an error string if something went wrong.
    const char *
      search(const char **exclude,
             unsigned vendor, unsigned device, unsigned pciClass, unsigned subClass,
                       Found found, unsigned &count);
  }
}
#endif
