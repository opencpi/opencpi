/*
 * This file is the only header that includes opencl.
 */
#ifndef __OCL_DEVICE_H__
#define __OCL_DEVICE_H__
#pragma weak clGetPlatformIDs
#include <opencl.h>
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

#define OCLDEV_STRING(name, var)		\
    do {					\
      OCLDEV(name, info, sizeof(info));		\
      var = info;				\
    } while(0)

#define OCLDEV_VAR(name, var) OCLDEV(name, &var, sizeof(var))

#define OCL_RC(r,call)		  \
    do {			  \
      cl_int rc;         	  \
      (r) = call;		  \
      if (!(r) || rc)		  \
	throwOclError(rc, "from %s, file %s, line %u", #call, __FILE__, __LINE__); \
    } while(0)

    struct OclFamily;
    struct OclVendor;
    
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
      cl_command_queue m_cmdq;
      size_t m_bufferAlignment;
      bool m_isCPU;
      cl_platform_id m_pid;
      OclVendor *m_vendor;
      OclFamily *m_family;
    protected:

      Device(const std::string &dname, cl_platform_id pid, cl_device_id did, bool verbose,
	     bool print);
      ~Device();
      cl_platform_id id() const { return m_pid; }
      cl_command_queue &cmdq() { return m_cmdq; }
      const std::string &name() const { return m_name; }
      size_t bufferAlignment() const { return m_bufferAlignment; }
      const OclVendor *vendor() const { return m_vendor; }
      const OclFamily *family() const { return m_family; }
      const std::string &type() const { return m_type; }
      
      
    public:
      bool isCPU() const { return m_isCPU; }
      cl_context &context() { return m_context; }
      cl_device_id &id() { return m_id; }
    };
    void throwOclError(cl_int errnum, const char *fmt, ...);
    const char* ocl_strerror(cl_int errnum);
  }
}
#endif
