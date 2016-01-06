/*
 * This file is the only header that includes opencl.
 */
#ifndef __OCL_DEVICE_H__
#define __OCL_DEVICE_H__
#pragma weak clGetPlatformIDs
#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <opencl.h>
#endif
namespace OCPI {
  namespace OCL {
#define OCL(call) \
  do { \
    cl_int rc = call; \
    if (rc) \
      throwOclError(rc, "from %s, file %s, line %u", #call, __FILE__, __LINE__); \
  } while(0)

#define OCLDEV(name, varp, size)				            \
    do {								    \
      if ((rc = clGetDeviceInfo(m_id, CL_DEVICE_##name, size, varp, 0)))    \
	throwOclError(rc, "clGetDeviceInfo() failed for CL_DEVICE_" #name); \
    } while (0)

#define OCLDEVR(name, varp, size, rsize)                                    \
    do {								    \
      if ((rc = clGetDeviceInfo(m_id, CL_DEVICE_##name, size, varp, &rsize))) \
	throwOclError(rc, "clGetDeviceInfo() failed for CL_DEVICE_" #name); \
    } while (0)

#define OCLDEV_STRING(name, var)		\
    do {					\
      OCLDEV(name, info, sizeof(info));		\
      var = info;				\
    } while(0)

#define OCLDEV_VAR(name, var) OCLDEV(name, &var, sizeof(var))

#define OCLDEVR_VAR(name, var, rsize) OCLDEVR(name, &var, sizeof(var), rsize)

#define OCL_RC(r,call)		  \
    do {			  \
      cl_int rc;         	  \
      (r) = call;		  \
      if (!(r) || rc)		  \
	throwOclError(rc, "from %s, file %s, line %u", #call, __FILE__, __LINE__); \
    } while(0)

    struct OclFamily;
    struct OclVendor;
    struct OclDevice;
    
    // The Device before it is a container
    // This info is for runtime.  We don't save everything from discovery time
    class Container;
    class Device {
      friend class Container;
      friend class Driver;
      friend class Artifact;
      friend class Worker;
      friend class Port;
      std::string m_name, m_vendorName, m_type;
      cl_device_id m_id;
      cl_context m_context;
      static const uint16_t MAX_CMDQ_LEN = 32;
      cl_command_queue m_cmdq[MAX_CMDQ_LEN];
      size_t m_bufferAlignment;
      bool m_isCPU;
      cl_platform_id m_pid;
      OclVendor *m_vendor;   // Our canonicalized vendor
      OclFamily *m_family;   // Our per-vendor device "family"
      OclDevice *m_platform; // OpenCPI platform name from OpenCL device name etc.
      cl_uint m_nUnits;
      uint32_t m_nextQOrd;
      static const uint32_t MAX_SUB_DEVICES = 256;
      cl_device_id  m_outDevices[MAX_SUB_DEVICES];
      cl_uint  m_numSubDevices;
      size_t m_maxGroupSize;
      cl_uint m_maxComputeUnits;
    protected:

      Device(const std::string &dname, cl_platform_id pid, cl_device_id did,
	     const std::string &vendor, bool verbose, bool print);
      ~Device();
      cl_platform_id id() const { return m_pid; }
      cl_command_queue &cmdq(int idx ) { return m_cmdq[idx]; }
      size_t bufferAlignment() const { return m_bufferAlignment; }
      const OclVendor *vendor() const { return m_vendor; }
      const OclFamily *family() const { return m_family; }
      const OclDevice *platform() const { return m_platform; }
      const std::string &type() const { return m_type; }
      size_t maxGroupSize() const { return m_maxGroupSize; }
      cl_uint maxComputeUnits() const { return m_maxComputeUnits; }
      cl_uint numSubDevices() const { return m_numSubDevices; }
    public:
      const std::string &name() const { return m_name; }
      bool isCPU() const { return m_isCPU; }
      cl_context &context() { return m_context; }
      cl_device_id &id() { return m_id; }
      uint32_t nextQOrd() { return (++m_nextQOrd)%m_nUnits; }
    };
    void throwOclError(cl_int errnum, const char *fmt, ...);
    const char* ocl_strerror(cl_int errnum);
  }
}
#endif
