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

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <climits>
#include <sys/mman.h>
#include <regex.h>
#include "OcpiContainerRunConditionApi.h"
#include "OCL_Worker.h"
#include "OclContainer.h"
#include "OclDevice.h"
#include "OclPlatformManager.h"

#ifdef USE_EXT
static clCreateSubDevicesEXT_fn pfn_clCreateSubDevicesEXT = NULL;
static clReleaseDeviceEXT_fn pfn_clReleaseDeviceEXT = NULL;
#define clCreateSubDevices pfn_clCreateSubDevicesEXT
#define clReleaseDevice pfn_clReleaseDeviceEXT
#define cl_device_partition_property cl_device_partition_property_ext
#define CL_DEVICE_PARTITION_EQUALLY CL_DEVICE_PARTITION_EQUALLY_EXT
#endif
// Init extension function pointer for device.
// The function really comes from the platform, but is enabled by the
// extension on the device.  It will be null if it is not supported.
#define INIT_CL_EXT_FCN_PTR(name) \
  do {									                \
    bool  dev = strstr(#name, extensions.c_str()) != NULL;		                \
    m_##name = (name##_fn)clGetExtensionFunctionAddressForPlatform(m_pid, #name);    \
    ocpiDebug("For extension function %s: dev %u ptr %p", #name, dev, m_##name);     \
    if (dev && !m_##name)						                \
      throw OU::Error("Cannot get pointer to OpenCL extension function for %s", #name); \
  } while(0)

namespace OCPI {
  namespace OCL {
    namespace OS = OCPI::OS;
    namespace OA = OCPI::API;
    namespace OU = OCPI::Util;
    namespace OC = OCPI::Container;
    namespace OR = OCPI::RDT;
    namespace OO = OCPI::OCL;
    namespace OX = OCPI::Util::EzXml;


    // Data base of vendor, family, device
    struct OclVendor {
      const char *name;
      const char *regexpr; // To match the "vendorname" device attribute
    } vendors[] = {{ "intel",  "intel|GenuineIntel" },
                   { "amd",    "amd|Advanced Micro Devices" },
		   { "apple",  "apple" },
		   { "nvidia", "nvidia"},
		   { NULL, NULL}};

    // A target (family+vendor) has binary compatibility
    struct OclFamily {
      const char *name;
      OclVendor *vendor;
    } families[] = {{ "cpu", &vendors[0]},    
		    { "cpu", &vendors[1]}, // We assume amd cpu is incompatible with intel cpu
		    { "cpu", &vendors[2]}, // We assume amd cpu is incompatible with intel cpu
		    { "iris",     &vendors[0]},
		    { "firepro", &vendors[1]},
		    { "geforce",  &vendors[3]},
		    { NULL, NULL}};
		    
    // The OpenCL concrete device type mapps to the OpenCPI "platform".
    // The OpenCL concept of "platform" maps to the (weak) OpenCPI concept of "runtime".
    // This table uses regex mapping from the 
    struct OclDevice {
      const char *platform; // The OpenCPI canonical platform name
      const char *regexpr;
      OclFamily *family;
    } devices[] = {{ "intel_i7",       "i7-",             &families[0] }, // intel cpu
		   { "intel_xeon",     "Xeon\\(R\\)",     &families[0] }, // intel cpu
		   { "iris_pro",       "Iris Pro",        &families[3] }, // intel gpu
		   { "firepro",        "Bonaire",         &families[4] }, // amd gpu
		   { "geforce_gt750m", "GeForce GT 750M", &families[5] }, // nvidia gpu
		   { NULL, NULL, NULL}};

    // Since they are retrieved as a vector, we'll leave them that way
    typedef std::vector<cl_kernel> ClKernels;
    // Ancillary information we retrieve per kernel from the artifact
    struct Kernel {
      std::string m_name;
      size_t m_compile_work_group_size[3];
      cl_uint m_nDims;
      size_t m_work_group_size;
    };
    typedef std::vector<Kernel> Kernels;

    void printExtensions(const char *space, const std::string &extensions) {
      int indent;
      printf("%sExtensions: %n", space, &indent);
      for (const char *next = 0, *cp = extensions.c_str(); *cp; cp = ++next) {
	if (next)
	  printf("%*s", indent, "");
	if ((next = strchr(cp, ' ')))
	  printf("%.*s\n", (int)(next - cp), cp);
	else {
	  printf("%s\n", cp);
	  break;
	}
      }
    }
    const char* ocl_strerror(cl_int errnum) {
      switch (errnum) {
      case CL_SUCCESS:
	return "CL_SUCCESS";
      case CL_DEVICE_NOT_FOUND:
	return "CL_DEVICE_NOT_FOUND";
      case CL_DEVICE_NOT_AVAILABLE:
	return "CL_DEVICE_NOT_AVAILABLE";
      case CL_COMPILER_NOT_AVAILABLE:
	return "CL_COMPILER_NOT_AVAILABLE";
      case CL_MEM_OBJECT_ALLOCATION_FAILURE:
	return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
      case CL_OUT_OF_RESOURCES:
	return "CL_OUT_OF_RESOURCES";
      case CL_OUT_OF_HOST_MEMORY:
	return "CL_OUT_OF_HOST_MEMORY";
      case CL_PROFILING_INFO_NOT_AVAILABLE:
	return "CL_PROFILING_INFO_NOT_AVAILABLE";
      case CL_MEM_COPY_OVERLAP:
	return "CL_MEM_COPY_OVERLAP";
      case CL_IMAGE_FORMAT_MISMATCH:
	return "CL_IMAGE_FORMAT_MISMATCH";
      case CL_IMAGE_FORMAT_NOT_SUPPORTED:
	return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
      case CL_BUILD_PROGRAM_FAILURE:
	return "CL_BUILD_PROGRAM_FAILURE";
      case CL_MAP_FAILURE:
	return "CL_MAP_FAILURE";
      case CL_MISALIGNED_SUB_BUFFER_OFFSET:
	return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
      case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
	return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
      case CL_COMPILE_PROGRAM_FAILURE:
	return "CL_COMPILE_PROGRAM_FAILURE";
      case CL_LINKER_NOT_AVAILABLE:
	return "LINKER_NOT_AVAILABLE";
      case CL_LINK_PROGRAM_FAILURE:
	return "CL_LINK_PROGRAM_FAILURE";
      case CL_DEVICE_PARTITION_FAILED:
	return "CL_DEVICE_PARTITION_FAILED";
      case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
	return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
      case -20:
	return "reserved error 8";
      case -21:
	return "reserved error 9";
      case -22:
	return "reserved error 10";
      case -23:
	return "reserved error 11";
      case -24:
	return "reserved error 12";
      case -25:
	return "reserved error 13";
      case -26:
	return "reserved error 14";
      case -27:
	return "reserved error 15";
      case -28:
	return "reserved error 16";
      case -29:
	return "reserved error 17";
      case CL_INVALID_VALUE:
	return "CL_INVALID_VALUE";
      case CL_INVALID_DEVICE_TYPE:
	return "CL_INVALID_DEVICE_TYPE";
      case CL_INVALID_PLATFORM:
	return "CL_INVALID_PLATFORM";
      case CL_INVALID_DEVICE:
	return "CL_INVALID_DEVICE";
      case CL_INVALID_CONTEXT:
	return "CL_INVALID_CONTEXT";
      case CL_INVALID_QUEUE_PROPERTIES:
	return "CL_INVALID_QUEUE_PROPERTIES";
      case CL_INVALID_COMMAND_QUEUE:
	return "CL_INVALID_COMMAND_QUEUE";
      case CL_INVALID_HOST_PTR:
	return "CL_INVALID_HOST_PTR";
      case CL_INVALID_MEM_OBJECT:
	return "CL_INVALID_MEM_OBJECT";
      case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
	return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
      case CL_INVALID_IMAGE_SIZE:
	return "CL_INVALID_IMAGE_SIZE";
      case CL_INVALID_SAMPLER:
	return "CL_INVALID_SAMPLER";
      case CL_INVALID_BINARY:
	return "CL_INVALID_BINARY";
      case CL_INVALID_BUILD_OPTIONS:
	return "CL_INVALID_BUILD_OPTIONS";
      case CL_INVALID_PROGRAM:
	return "CL_INVALID_PROGRAM";
      case CL_INVALID_PROGRAM_EXECUTABLE:
	return "CL_INVALID_PROGRAM_EXECUTABLE";
      case CL_INVALID_KERNEL_NAME:
	return "CL_INVALID_KERNEL_NAME";
      case CL_INVALID_KERNEL_DEFINITION:
	return "CL_INVALID_KERNEL_DEFINITION";
      case CL_INVALID_KERNEL:
	return "CL_INVALID_KERNEL";
      case CL_INVALID_ARG_INDEX:
	return "CL_INVALID_ARG_INDEX";
      case CL_INVALID_ARG_VALUE:
	return "CL_INVALID_ARG_VALUE";
      case CL_INVALID_ARG_SIZE:
	return "CL_INVALID_ARG_SIZE";
      case CL_INVALID_KERNEL_ARGS:
	return "CL_INVALID_KERNEL_ARGS";
      case CL_INVALID_WORK_DIMENSION:
	return "CL_INVALID_WORK_DIMENSION";
      case CL_INVALID_WORK_GROUP_SIZE:
	return "CL_INVALID_WORK_GROUP_SIZE";
      case CL_INVALID_WORK_ITEM_SIZE:
	return "CL_INVALID_WORK_ITEM_SIZE";
      case CL_INVALID_GLOBAL_OFFSET:
	return "CL_INVALID_GLOBAL_OFFSET";
      case CL_INVALID_EVENT_WAIT_LIST:
	return "CL_INVALID_EVENT_WAIT_LIST";
      case CL_INVALID_EVENT:
	return "CL_INVALID_EVENT";
      case CL_INVALID_OPERATION:
	return "CL_INVALID_OPERATION";
      case CL_INVALID_GL_OBJECT:
	return "CL_INVALID_GL_OBJECT";
      case CL_INVALID_BUFFER_SIZE:
	return "CL_INVALID_BUFFER_SIZE";
      case CL_INVALID_MIP_LEVEL:
	return "CL_INVALID_MIP_LEVEL";
      case CL_INVALID_GLOBAL_WORK_SIZE:
	return "CL_INVALID_GLOBAL_WORK_SIZE";
#ifdef CL_PLATFORM_NOT_FOUND_KHR
      case  CL_PLATFORM_NOT_FOUND_KHR:
	return "CL_PLATFORM_NOT_FOUND_KHR";
#endif
      default:
	return "Unknown error";
      }
    }
    void throwOclError(cl_int errnum, const char *fmt, ...) {
      va_list ap;
      va_start(ap, fmt);
      std::string s;
      OU::formatAddV(s, fmt, ap);
      va_end(ap);
      OU::formatAdd(s, ": OpenCL error: %s [%d/0x%x]", ocl_strerror(errnum), errnum, errnum);
      throw OU::Error("Ocl Error: %s", s.c_str());
    }


    // The Device before it is a container
    // This info is for runtime.  We don't save everything from discovery time
    Device::
    Device(const std::string &dname, cl_platform_id pid, cl_device_id did,
	   const std::string &a_vendor, bool verbose, bool print)
      : m_name(dname), m_id(did), m_context(NULL), m_bufferAlignment(0), m_isCPU(false),
	m_pid(pid), m_vendor(NULL), m_family(NULL), m_platform(NULL), m_nUnits(0),
	m_nextQOrd(0), m_numSubDevices(0), m_maxGroupSize(0), m_maxComputeUnits(0) {
      memset(m_cmdq, 0, sizeof(m_cmdq));
      memset(m_outDevices, 0, sizeof(m_outDevices));
      //      cl_int rc;
      cl_device_type l_type;
      cl_uint vendorId, nDimensions;
      size_t *sizes, argSize;
      cl_bool available, compiler;
      cl_device_exec_capabilities capabilities;
      cl_command_queue_properties properties;
      OCLDEV_VAR(TYPE, l_type);
      m_isCPU = l_type == CL_DEVICE_TYPE_CPU;

      OCLDEV_VAR(VENDOR_ID, vendorId);
      OCLDEV_VAR(MAX_COMPUTE_UNITS, m_maxComputeUnits);
      cl_uint maxSubDevices;
      OCLDEV_VAR(PARTITION_MAX_SUB_DEVICES, maxSubDevices);
      cl_device_partition_property p[10];
      OCLDEVR_VAR(PARTITION_PROPERTIES, p, argSize);
      ocpiDebug("OCL partitioning: max %u, sub %u, propsize %zu, first %llx",
		m_maxComputeUnits, maxSubDevices, argSize, (long long)p[0]);
      for (unsigned n = 0; p[n] != CL_DEVICE_PARTITION_BY_COUNTS_LIST_END; n++)
	ocpiDebug("partition property: %llx", (unsigned long long)(p[n]));

      if (maxSubDevices && argSize) {
	cl_device_partition_property pp[] = {CL_DEVICE_PARTITION_EQUALLY, 2, 0 };
	OCL(clCreateSubDevices(m_id, pp, m_maxComputeUnits, NULL, &m_numSubDevices));
	ocpiDebug("So NUMSUBDEV: %u", m_numSubDevices);
	OCL(clCreateSubDevices(m_id, pp, m_maxComputeUnits, m_outDevices, &m_numSubDevices));
	ocpiDebug("OCL got %d partitioned subunits", m_numSubDevices );	
	ocpiInfo("OCL Device %s has %d compute units, partitioned into %d subdevices",
		 m_name.c_str(), m_nUnits, m_numSubDevices);
	assert(m_numSubDevices <= MAX_CMDQ_LEN);
	m_nUnits = m_numSubDevices;
      } else {
	ocpiInfo("OCL Device %s cannot be partitioned, even though it has %u compute units",
		 m_name.c_str(), m_maxComputeUnits);
	m_outDevices[0] = m_id;
	m_numSubDevices = 1;
	m_nUnits = 1;
      }

      cl_context_properties ctx_props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)pid, 0};
      OCL_RC(m_context, clCreateContext(ctx_props, 1, &m_id, 0, 0, &rc));
      for (unsigned n = 0; n < m_nUnits; n++)
	OCL_RC(m_cmdq[n], clCreateCommandQueue(m_context, m_outDevices[n], 0, &rc));


      OCLDEV_VAR(MAX_WORK_ITEM_DIMENSIONS, nDimensions);

      sizes = new size_t[nDimensions];

      OCLDEV(MAX_WORK_ITEM_SIZES, sizes, nDimensions * sizeof(size_t));
      OCLDEV_VAR(MAX_WORK_GROUP_SIZE, m_maxGroupSize);
// BUG ALERT: It appears that Apple lies about this.
// But it does not complain at compile time.
#ifdef __APPLE__
      if (m_isCPU)
	m_maxGroupSize = 1;
#endif
      ocpiDebug("OCL %s: max dims %u, max groupsize %zu, max sizes %zu,%zu,%zu",
		dname.c_str(), nDimensions, m_maxGroupSize, sizes[0], sizes[1], sizes[2]);
      delete [] sizes;

      OCLDEV_VAR(MAX_PARAMETER_SIZE, argSize);
      OCLDEV_VAR(AVAILABLE, available);
      OCLDEV_VAR(COMPILER_AVAILABLE, compiler);
      OCLDEV_VAR(EXECUTION_CAPABILITIES, capabilities);
      OCLDEV_VAR(QUEUE_PROPERTIES, properties);
      cl_uint addrAlign;
      OCLDEV_VAR(MEM_BASE_ADDR_ALIGN, addrAlign);
      m_bufferAlignment = addrAlign / 8;
      char info[1024];
      std::string driverVersion, profile, version, cVersion, extensions;
      OCLDEV_STRING(NAME, m_type);
      OCLDEV_STRING(VENDOR, m_vendorName);
      OCLDEV_STRING(PROFILE, profile);
      OCLDEV_STRING(VERSION, version);
      OCLDEV_STRING(OPENCL_C_VERSION, cVersion); // not on Apple?
      OCLDEV_STRING(EXTENSIONS, extensions);
#ifdef CL_MEM_BUS_ADDRESSABLE_AMD
      INIT_CL_EXT_FCN_PTR(clEnqueueWaitSignalAMD);
      INIT_CL_EXT_FCN_PTR(clEnqueueWriteSignalAMD);
      INIT_CL_EXT_FCN_PTR(clEnqueueMakeBuffersResidentAMD);
#endif

      for (OclVendor *v = vendors; v->name; v++) {
	regex_t rx;
	if (regcomp(&rx, v->regexpr, REG_NOSUB|REG_ICASE|REG_EXTENDED))
	  throw OU::Error("Invalid regular expression for OpenCL vendor: %s", v->regexpr);
	int r = regexec(&rx, m_vendorName.c_str(), 0, NULL, 0);
	regfree(&rx);
	if (!r) {
	  m_vendor = v;
	  break;
	}
      }
      if (!m_vendor)
	ocpiInfo("OpenCL device \"%s\": unknown vendor \"%s\"",
		 m_type.c_str(), m_vendorName.c_str());
      for (OclDevice *d = devices; d->regexpr; d++) {
	regex_t rx;
	if (regcomp(&rx, d->regexpr, REG_NOSUB|REG_ICASE|REG_EXTENDED))
	  throw OU::Error("Invalid regular expression for OpenCL device: %s", d->regexpr);
	int r = regexec(&rx, m_type.c_str(), 0, NULL, 0);
	regfree(&rx);
	if (!r) {
	  if (!strcasecmp(d->family->name, "cpu")) {
	    for (OclFamily *f = families; f->name; f++)
	      if (!strcasecmp(f->name, "cpu")) {
		if (regcomp(&rx, f->vendor->regexpr, REG_NOSUB|REG_ICASE|REG_EXTENDED))
		  throw OU::Error("Invalid regular expression for OpenCL vendor: %s",
				  f->vendor->regexpr);
		r = regexec(&rx, a_vendor.c_str(), 0, NULL, 0);
		regfree(&rx);
		if (!r) {
		  m_vendor = f->vendor;
		  m_family = f;
		  break;
		}
	      }
	  } else
	    m_family = d->family;
	  assert(m_family->vendor == m_vendor);
	  m_platform = d;
	}
      }
      if (print || verbose)
	printf("OpenCPI OCL Device Found: '%s': target %s-%s, platform %s, device \"%s\"\n",
	       m_name.c_str(), m_vendor ? m_vendor->name : "unknown",
	       m_family ? m_family->name : "unknown",
	       m_platform ? m_platform->platform : "unknown", m_type.c_str());
      if (verbose) {
	printf("    Vendor:     \"%s\" profile \"%s\" version \"%s\" C version \"%s\"\n",
	       m_vendorName.c_str(), profile.c_str(), version.c_str(), cVersion.c_str());
	printf("    Alignment:  %zu\n", m_bufferAlignment);
	printf("    MaxArgSize: %zu\n", argSize);
	if (m_platform)
	  printf("    OpenCPI:    Platform: %s, Family: %s, Vendor: %s, Target: %s-%s\n",
		 m_platform->platform, m_family->name, m_family->vendor->name,
		 m_family->vendor->name, m_family->name);
	printExtensions("    ", extensions);
      }

    }

    Device::
    ~Device() {
      for (unsigned n = 0; n < m_nUnits; n++)
	OCL(clReleaseCommandQueue(m_cmdq[n]));
      OCL(clReleaseContext(m_context));
    }


    class ExternalPort;

    const char *ocl = "ocl";

    OC::RegisterContainerDriver<Driver> driver;

    class Port;
    class Application;

    class Artifact : public OC::ArtifactBase<Container,Artifact> {
      friend class Container;
      friend class Worker;
      friend class Application;
      cl_program m_program;
      ClKernels  m_clKernels;
      Kernels    m_kernels;

      Artifact(Container &c, OCPI::Library::Artifact &lart, const OA::PValue *artifactParams) :
	OC::ArtifactBase<Container,Artifact>(c, *this, lart, artifactParams) {
	ocpiInfo("Loading artifact %s (UUID \"%s\") on OCL container %s\n",
		 name().c_str(), lart.uuid().c_str(), c.name().c_str());
	cl_device_id &id = c.device().id();
	cl_context &ctx = c.device().context();

	int fd = ::open(name().c_str(), O_RDONLY);
	try {
	  off_t length;
	  void *mapped;
	  if (fd < 0 ||
	      (length = lseek(fd, 0, SEEK_END)) == -1 ||
	      (mapped = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED)
	    throw OU::Error("Can't load artifact: %s (%s, %d)\n",
			    name().c_str(), strerror(errno), errno);
	  cl_int rc, rc1;
	  size_t bytes = length - lart.metadataLength();

	  const unsigned char *binary = (const unsigned char *)mapped;
	  ocpiDebug("clCreateProgramWithBinary ctx %p id %p bytes %zu", ctx, id, bytes);
	  m_program = clCreateProgramWithBinary(ctx, 1, &id, &bytes, &binary, &rc, &rc1);

	  switch (rc) {
	  case CL_INVALID_BINARY:
	    throw OU::Error("Artifact \"%s\" not valid for OpenCL device", name().c_str());
	  default:
	    throw OU::Error("clCreateProgramWithBinary error from \%s\": %s (%d)",
			    name().c_str(), ocl_strerror(rc), rc);
	  case CL_SUCCESS:;
	  }
	  if (rc1 != CL_SUCCESS)
	    throw OU::Error("clCreateProgramWithBinary error from \%s\": %s (%d)",
			    name().c_str(), ocl_strerror(rc1), rc1);

	  OCL(clBuildProgram(m_program, 1, &id, "-g -cl-opt-disable", 0, 0));

	  size_t n;
	  OCL(clGetProgramBuildInfo(m_program, id, CL_PROGRAM_BUILD_LOG, 0, 0, &n));
	  std::vector<char> log(n + 1, 0);
	  OCL(clGetProgramBuildInfo(m_program, id, CL_PROGRAM_BUILD_LOG, n, &log[0], NULL));
	  ocpiDebug("OpenCL Binary Loading start log:\n%s\n====End Log", &log[0]);
	  cl_uint cln;
	  OCL(clCreateKernelsInProgram(m_program, 0, 0, &cln));
	  m_clKernels.resize(cln);
	  m_kernels.resize(cln);
	  OCL(clCreateKernelsInProgram(m_program, cln, &m_clKernels[0], 0));
	  for (size_t k = 0; k < m_kernels.size(); k++) {
	    char str[1024];
	    OCL(clGetKernelInfo(m_clKernels[k], CL_KERNEL_FUNCTION_NAME, sizeof(str), str,
				0));
	    ocpiDebug("In artifact %s, found kernel %s", name().c_str(), str);
	    m_kernels[k].m_name = str;
	    OCL(clGetKernelWorkGroupInfo(m_clKernels[k], id,
					 CL_KERNEL_COMPILE_WORK_GROUP_SIZE,
					 sizeof(m_kernels[k].m_compile_work_group_size),
					 (void*)m_kernels[k].m_compile_work_group_size, 0));
	    OCL(clGetKernelWorkGroupInfo(m_clKernels[k], id,
					 CL_KERNEL_WORK_GROUP_SIZE,
					 sizeof(m_kernels[k].m_work_group_size),
					 (void*)&m_kernels[k].m_work_group_size, 0));
#if 0 // This call is only valid on builtin or custom devices
	    OCL(clGetKernelWorkGroupInfo(m_clKernels[k], id,
					 CL_KERNEL_GLOBAL_WORK_SIZE,
					 sizeof(m_kernels[k].m_global_work_size),
					 (void*)m_kernels[k].m_global_work_size, 0));
#endif
	    size_t compile_wgs = 1;
	    for (unsigned nn = 0; nn < 3; nn++)
	      if (m_kernels[k].m_compile_work_group_size[nn]) {
		compile_wgs *= m_kernels[k].m_compile_work_group_size[nn];
		m_kernels[k].m_nDims++;
	      }
	    ocpiDebug("for kernel %s, wgs: %zu, dims %u, cwgs: %zu,%zu,%zu",
		      str, m_kernels[k].m_work_group_size, m_kernels[k].m_nDims,
		      m_kernels[k].m_compile_work_group_size[0],
		      m_kernels[k].m_compile_work_group_size[1],
		      m_kernels[k].m_compile_work_group_size[2]);
	    if (compile_wgs > m_kernels[k].m_work_group_size)
	      throw OU::Error("OCL kernel %s in %s: required work group size (%zu) "
			      "in kernel code exceeds device limits (%zu)",
			      str, name().c_str(), compile_wgs, m_kernels[k].m_work_group_size);
	    if (!m_kernels[k].m_nDims)
	      // This kernel has no required work group size
	      m_kernels[k].m_nDims = 1;
	  }
	} catch (...) {
	  if (fd >= 0)
	    ::close(fd);
	  throw;
	}
      }
      ~Artifact() {
	for (unsigned n = 0; n < m_clKernels.size(); n++)
	  OCL(clReleaseKernel(m_clKernels[n]));
	OCL(clReleaseProgram(m_program));
      }
    protected:
      OC::Worker &createWorker(Application &, const char* appInstName, ezxml_t impl, ezxml_t inst,
			       const OC::Workers &slaves, bool hasMaster, size_t member,
			       size_t crewSize, const OU::PValue* wParams, int que);

      cl_program &program() { return m_program; }
    };

    Container::
    Container(OCPI::OCL::Device &a_device, const ezxml_t config, const OU::PValue* params)
      : OC::ContainerBase<Driver,Container,Application,Artifact>
	(*this, a_device.name().c_str(), config, params),
	m_device(a_device) {
      m_model = "ocl";
      m_os = "opencl";
      if (a_device.family() && a_device.family()->vendor) {
	m_osVersion = a_device.family()->vendor->name;
	m_arch = a_device.family()->name;
	m_platform = a_device.platform()->platform;
      } else
	m_platform = m_osVersion = m_arch = "unknown";
    }

    Container::~Container() {
      OC::Container::shutdown();
      this->lock();
      OU::Parent<Application>::deleteChildren();
    }
    bool
    Container::supportsImplementation(OU::Worker &i) {
      size_t rwgs;
      const char *err;
      if ((err = OX::getNumber(i.m_xml, "requiredworkgroupsize", &rwgs)))
	ocpiInfo("Error parsing worker XML for worker %s: %s", i.cname(), err);
      if (rwgs > m_device.maxGroupSize()) {
	ocpiInfo("OCL Worker %s needs group size %zu but device only supports %zu",
		 i.cname(), rwgs, m_device.maxGroupSize());
	return false;
      }
      return OC::Container::supportsImplementation(i);
    }

    uint8_t Driver::
    s_logLevel = 0;

    Driver::
    Driver() {
      const char *ll = getenv("OCPI_OPENCL_LOG_LEVEL");
      if (ll)
	s_logLevel = OCPI_UTRUNCATE(uint8_t, atoi(ll));
    }
    unsigned Driver::
    search(const OA::PValue*params, const char**exclude, bool discoveryOnly) {
      return search(params, exclude, discoveryOnly, NULL, NULL, NULL);
    }

    Device &Driver::
    find(const char *target) {
      Device *d = NULL;

      search(NULL, NULL, false, target, &d, NULL);
      
      if (d)
	return *d;
      throw OU::Error("No OpenCL device found for target: %s", target);
    }

    unsigned Driver::
    search(const OA::PValue*params, const char**exclude, bool /*discoveryOnly*/,
	   const char *type, Device **found, std::set<std::string> *targets) {
      cl_uint np, nd;
      unsigned ndevs = 0;
      OclFamily *family = NULL;
      if (type) {
	std::string vendorName, familyName;
	const char *dash = strchr(type, '-');
	if (dash) {
	  vendorName.assign(type, dash - type);
	  familyName = dash + 1;
	} else {
	  familyName = type;
	}

	OclFamily *f;
	for (f = families; f->name; f++) {
	  ocpiDebug("Vendor: %s Family: %s, looking at %s (%s)",
		    vendorName.c_str(), familyName.c_str(), f->name, f->vendor->name);
	  //	  printf("****  Vendor: %s Family: %s, looking at %s (%s)\n",  vendorName.c_str(), familyName.c_str(), f->name, f->vendor->name);	  
	  if (!strcasecmp(f->name, familyName.c_str())) {
	    if (vendorName.size()) {
	      if (!strcasecmp(f->vendor->name, vendorName.c_str())) {
		family = f;
		break;
	      } else
		continue;
	    }
	    if (family)
	      throw OU::Error("Ambiguous OCL target \"%s\".  Need <vendor>-<family>", type);
	    family = f;
	  }
	}
	if (!family)
	  throw OU::Error("No OCL target named \"%s\"", type);
      }
      bool printOnly = false, verbose = false;
      OU::findBool(params, "printOnly", printOnly);
      OU::findBool(params, "verbose", verbose);
      cl_int rc = clGetPlatformIDs(0, 0,&np);
      if (rc)
	ocpiInfo("OpenCL clGetPlatformIDs query returned error: %s [%d]",
		 ocl_strerror(rc), rc);
      if (rc || !np)
	return 0;
      std::vector<cl_platform_id> pids(np);
      OCL(clGetPlatformIDs(np, &pids[0], 0));


      for (size_t p = 0; p < np; p++) {
	std::string l_name, vendor, profile, version, extensions;
        char info[1024];
        OCL(clGetPlatformInfo(pids[p], CL_PLATFORM_PROFILE, sizeof(info), info, 0));
	profile = info;
        OCL(clGetPlatformInfo(pids[p], CL_PLATFORM_VERSION, sizeof(info), info, 0));
	version = info;
        OCL(clGetPlatformInfo(pids[p], CL_PLATFORM_NAME, sizeof(info), info, 0));
	l_name = info;
        OCL(clGetPlatformInfo(pids[p], CL_PLATFORM_VENDOR, sizeof(info), info, 0));
	vendor = info;
        OCL(clGetPlatformInfo(pids[p], CL_PLATFORM_EXTENSIONS, sizeof(info), info, 0));
	extensions = info;
        OCL(clGetDeviceIDs(pids[p], CL_DEVICE_TYPE_ALL, 0, 0, &nd));
	std::vector<cl_device_id> dids(nd);

	OCL(clGetDeviceIDs(pids[p], CL_DEVICE_TYPE_ALL, nd, &dids[0], 0));

	if (verbose) {
	  printf("OpenCL Platform: %zu/%p \"%s\" vendor \"%s\" profile \"%s\" version \"%s\"\n",
		 p, pids[p], l_name.c_str(), vendor.c_str(), profile.c_str(), version.c_str());
	  printExtensions("    ", extensions);
	}
	for (size_t d = 0; d < nd; d++) {
	  std::string dname;
	  OU::format(dname, "ocl%zu.%zu", p, d);

	  for (const char **ep = exclude; ep && *ep; ep++)
	    if (!strcasecmp(*ep, dname.c_str()))
	      goto cont2;
	  {
	    Device *dev = new Device(dname, pids[p], dids[d], vendor, verbose, printOnly);
	    if (type)
	      if (family == dev->family()) {
		*found = dev;
		return 0;
	      } else
		delete dev;
	    else if (printOnly)
	      delete dev;
	    else if (targets) {
	      std::string target;
	      OU::format(target, "%s=%s-%s", 
			 dev->platform()->platform, dev->vendor()->name, dev->family()->name);
	      targets->insert(target);
	      delete dev;
	    } else
	      new Container(*dev);
	  }
	  ndevs++;
	cont2:;
	}
      }
      return ndevs;
    }
    Device *Driver::
    open(const char *a_name, bool verbose, bool print, std::string &error) {
      unsigned p, d;
      if (sscanf(a_name, "ocl%u.%u", &p, &d) != 2)
	OU::format(error, "Bad format for OCL device: \"%s\".  Must be \"ocl<pnum>.<dnum>\"",
		   a_name);
      else {
	cl_uint np, nd;
	OCL(clGetPlatformIDs(0, 0,&np));
	if (!np)
	  error = "There are no OpenCL platforms";
	else if (p >= np)
	  OU::format(error, "Bad platform number in OCL device name: \"%s\"", a_name);
	else {
	  std::vector<cl_platform_id> pids(np);
	  OCL(clGetPlatformIDs(np, &pids[0], 0));
	  OCL(clGetDeviceIDs(pids[p], CL_DEVICE_TYPE_ALL, 0, 0, &nd));
	  if (d >= nd)
	    OU::format(error, "Bad device number in OCL device name: \"%s\"", a_name);
	  else {
	    std::string vendor;
	    char info[1024];
	    OCL(clGetPlatformInfo(pids[p], CL_PLATFORM_VENDOR, sizeof(info), info, 0));
	    vendor = info;
	    std::vector<cl_device_id> dids(nd);
	    OCL(clGetDeviceIDs(pids[p], CL_DEVICE_TYPE_ALL, nd, &dids[0], 0));
	    std::string dname;
	    OU::format(dname, "ocl%u.%u", p, d);
	    return new Device(dname, pids[p], dids[d], vendor, verbose, print);
	  }
	}
      }
      return NULL;
    }

    OC::Container* Driver::
    probeContainer(const char* which, std::string &error, const OA::PValue *params) {
      Device *dev = open(which, false, false, error);
      if (!dev)
	return NULL;
      return new Container(*dev, NULL, params);
    }

    OC::Artifact& Container::
    createArtifact(OCPI::Library::Artifact &lart, const OA::PValue *artifactParams) {
      return *new Artifact(*this, lart, artifactParams);
    }

    class Worker;

    class Application : public OC::ApplicationBase<Container,Application,Worker> {
      friend class Container;

      Application(Container &con, const char *a_name, const OA::PValue *params)
	: OC::ApplicationBase<Container, Application, Worker>(con, *this, a_name, params) {
      }

      OC::Worker &
      createWorker(OC::Artifact* artifact, const char* appInstName, ezxml_t impl, ezxml_t inst,
		   const OC::Workers &slaves, bool hasMaster, size_t member, size_t crewSize,
		   const OU::PValue* params) {
	assert(slaves.empty());
	assert(artifact);
	Artifact &art = static_cast<Artifact&>(*artifact);

	int nq = static_cast<Container&>(container()).device().nextQOrd();

	return art.createWorker(*this, appInstName, impl, inst, slaves, hasMaster, member,
				crewSize, params, nq);
      }

      void
      run(DataTransfer::EventManager* event_manager, bool& more_to_do);
    }; // End: class Application

    OA::ContainerApplication* Container::
    createApplication (const char *a_name, const OCPI::Util::PValue *params)
      throw (OCPI::Util::EmbeddedException)
    {
      return new Application(*this, a_name, params);
    }

    OC::Container::DispatchRetCode Container::
    dispatch(DataTransfer::EventManager* event_manager) throw (OU::EmbeddedException) {
      bool more_to_do = false;
      if (!m_enabled)
	return Stopped;

      OU::SelfAutoMutex guard(this);

      // Process the workers
      for (Application* a = OU::Parent<Application>::firstChild(); a; a = a->nextChild())
	a->run(event_manager, more_to_do);
      return more_to_do ? MoreWorkNeeded : Spin;
    }


    class Worker : public OC::WorkerBase<Application,Worker,Port>, public OCPI::Time::Emit {
      friend class Port;
      friend class Artifact;
      friend class Application;
      friend class ExternalPort;
      friend class ExternalBuffer;

      uint8_t        *m_persistent;
      size_t          m_persistBytes;
      cl_mem          m_clPersistent; // handle on OpenCL buffer object
      uint8_t        *m_memory;
      OCLReturned    *m_returned;
      OCLWorker      *m_self;
      uint8_t        *m_properties;
      OCLWorker      *m_oclWorker;
      uint8_t        *m_oclWorkerBytes;
      size_t          m_oclWorkerSize;
      OCLPort        *m_oclPorts;
      OA::RunCondition  m_defaultRunCondition;
      OA::RunCondition  m_myRunCondition;
      OA::RunCondition *m_runCondition;
      Kernel                  &m_kernel;
      Container               &m_container;
      bool                     m_isEnabled;
      cl_kernel                m_clKernel;
      OCPI::OS::Timer          m_runTimer;
      std::vector<Port*>       m_myPorts;
      uint32_t                 m_que;


      bool                     m_running;
      size_t                   m_minReady;
      OCLPortMask              m_relevantMask;
      cl_event                 m_taskEvent;


      Worker(Application &app, Artifact &art, Kernel &k, const char *a_name, ezxml_t implXml,
	     ezxml_t instXml, const OC::Workers &a_slaves, bool a_hasMaster, size_t a_member,
	     size_t a_crewSize, const OA::PValue* params, uint32_t que )
	  : OC::WorkerBase<Application, Worker, Port>(app, *this, &art, a_name, implXml, instXml,
						      a_slaves, a_hasMaster, a_member, a_crewSize,
						      params),
	    OCPI::Time::Emit(&parent().parent(), "Worker", a_name), m_kernel(k),
	    m_container(app.parent()), m_isEnabled(false), m_clKernel(NULL), m_que(que),
            m_running(false) {

	assert(!(sizeof(OCLWorker) & 7));
	assert(!(sizeof(OCLPort) & 7));
	assert(!(sizeof(OCLReturned) & 7));
	m_oclWorkerSize = sizeof(OCLWorker) + m_nPorts * sizeof(OCLPort);
	m_persistBytes =
	  sizeof(OCLReturned) + m_oclWorkerSize + OU::roundUp(totalPropertySize(), 8);
	size_t nMemories;
	OU::Memory *m = memories(nMemories);
	for (size_t n = 0; n < nMemories; n++, m++)
	  m_persistBytes += OU::roundUp(m->m_nBytes, 8);

	OCL_RC(m_clPersistent,
	       clCreateBuffer(m_container.device().context(),
			      CL_MEM_READ_WRITE|CL_MEM_ALLOC_HOST_PTR, m_persistBytes,
			      NULL, &rc));
	
	void *vp;
	ocpiDebug("enqueuemap persistent(%u)", m_que);
	OCL_RC(vp, clEnqueueMapBuffer(m_container.device().cmdq(m_que), m_clPersistent,
				      false, CL_MAP_READ|CL_MAP_WRITE, 0, m_persistBytes,
				      0, 0, 0, &rc));

	ocpiDebug("finish(0)");
	OCL(clFinish(m_container.device().cmdq(0)));

	m_persistent = (uint8_t *)vp;
	memset(m_persistent, 0, m_persistBytes);
	m_returned = (OCLReturned *)m_persistent;

	m_self = (OCLWorker *)(m_returned + 1);
	m_properties = (uint8_t *)(m_self) + sizeof(OCLReturned) + m_nPorts * sizeof(OCLPort);
	m_properties = (uint8_t *)(m_self + 1) + m_nPorts * sizeof(OCLPort);

	m_memory = m_properties + OU::roundUp(totalPropertySize(), 8);
	m_oclWorkerBytes = new uint8_t[m_oclWorkerSize];
	m_oclWorker = (OCLWorker*)m_oclWorkerBytes;
	m_oclPorts = (OCLPort *)(m_oclWorker + 1);
	memset(m_oclWorker, 0, sizeof(*m_oclWorker));
	m_oclWorker->crew_size = OCPI_UTRUNCATE(uint16_t, a_crewSize);
	m_oclWorker->member = OCPI_UTRUNCATE(uint16_t, a_member);
	m_oclWorker->firstRun = true;
	m_oclWorker->timedOut = false;
	m_oclWorker->nPorts = OCPI_UTRUNCATE(uint8_t, m_nPorts);
	m_oclWorker->logLevel = OCPI_UTRUNCATE(uint8_t, OS::logGetLevel());
	m_oclWorker->kernelLogLevel = Driver::s_logLevel;
	ocpiDebug("Worker/kernel %s in %s has persistent size of %zu bytes, self %zu bytes",
		  a_name, art.name().c_str(), m_persistBytes, m_oclWorkerSize);
	// FIXME: how can the worker declare a run condition?
	// Perhaps with an init? or XML? (might be nice).
	m_runCondition = &m_defaultRunCondition;
	OCL_RC(m_clKernel, clCreateKernel(art.program(), m_kernel.m_name.c_str(), &rc));
	// This argument will change each time because the contents of the structure change
	//	  OCL(clSetKernelArg(m_clKernel, 0, m_oclWorkerSize, m_oclWorkerBytes));
	// This argument is persistent and will never change.
	OCL(clSetKernelArg(m_clKernel, 0, sizeof(m_clPersistent), &m_clPersistent));
	// The rest of the arguments are created by the ports
	setControlOperations(ezxml_cattr(implXml, "controlOperations"));
	m_myPorts.resize(m_nPorts, NULL);
      }
      ~Worker() {
	try {
	  if (m_isEnabled) {
	    m_isEnabled = false;
	    controlOperation ( OU::Worker::OpStop );
	  }
	  controlOperation ( OU::Worker::OpRelease );
	} catch (...) {
	}
	deleteChildren();
	if (m_clPersistent)
	  clReleaseMemObject(m_clPersistent);
	// destruct rest
      }

      void kernelProlog(OCLOpCode opcode) {
	m_oclWorker->controlOp = opcode;
	memcpy(m_self, m_oclWorkerBytes, m_oclWorkerSize);
	ocpiDebug("enqueueunmap persistent(%u)", m_que);
	OCL(clEnqueueUnmapMemObject(m_container.device().cmdq(m_que), m_clPersistent,
				    m_persistent, 0, 0, 0));
      }

      void kernelEpilog() {
	memcpy(m_oclWorkerBytes, m_self, m_oclWorkerSize);
      }

      // Defined below the port class since it needs it to be defined.
      void controlOperation(OU::Worker::ControlOperation op);
      void run(bool &anyone_run);
      bool chkForCompletion();

      public:
      void read ( size_t, size_t, void* ) {
	ocpiAssert ( 0 );
      }

      void write (size_t, size_t, const void*) {
	ocpiAssert ( 0 );
      }

      OC::Port &createPort(const OU::Port &metaport, const OA::PValue *props);

      bool enabled() const {
	return m_isEnabled;
      }

      void prepareProperty(OU::Property& md,
			   volatile uint8_t *&writeVaddr,
			   const volatile uint8_t *&readVaddr) {
	if (md.m_baseType != OA::OCPI_Struct && !md.m_isSequence &&
	    md.m_baseType != OA::OCPI_String && OU::baseTypeSizes[md.m_baseType] <= 32 &&
	    !md.m_writeError) {
	  if ((md.m_offset + md.m_nBytes) > totalPropertySize())
	    throw OU::Error( "OCL property is out of bounds." );
	  readVaddr = m_properties + md.m_offset;
	  writeVaddr = m_properties + md.m_offset;
	}

      }
      
      void
      propertyWritten(unsigned /*ordinal*/) const {
      }
      void
      propertyRead(unsigned /*ordinal*/) const {
      }
#undef OCPI_DATA_TYPE_S
      // Set a scalar property value
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      void set##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member *mp, \
				 size_t offset, const run val, unsigned idx) const { \
        const OU::Member &m = mp ? *mp : info; \
        if (info.m_writeError ) \
          throw; /*"worker has errors before write */ \
        volatile store *pp = (volatile store *)(m_properties + info.m_offset + \
						offset + m.m_elementBytes * idx);    \
        if (bits > 32) { \
          assert(bits == 64); \
          volatile uint32_t *p32 = (volatile uint32_t *)pp; \
          p32[1] = ((const uint32_t *)&val)[1]; \
          p32[0] = ((const uint32_t *)&val)[0]; \
        } else \
          *pp = *(const store *)&val; \
        if (info.m_writeError) \
          throw; /*"worker has errors after write */ \
      } \
      void set##pretty##SequenceProperty(const OA::Property &p,const run *vals, size_t length) const { \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors before write */ \
        memcpy((void *)(m_properties + p.m_info.m_offset + p.m_info.m_align), vals, length * sizeof(run)); \
        *(volatile uint32_t *)(m_properties + p.m_info.m_offset) = (uint32_t)length; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors after write */ \
      }
      // Set a string property value FIXME redundant length check???
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                         \
      virtual void							                 \
      set##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member *mp, \
			    size_t offset, const run val, unsigned idx) const {          \
        const OU::Member &m = mp ? *mp : info;				                 \
        size_t ocpi_length;						                 \
        if (!val || (ocpi_length = strlen(val)) > m.m_stringLength)                      \
          throw; /*"string property too long"*/;			                 \
        if (info.m_writeError)						                 \
          throw; /*"worker has errors before write */			                 \
        uint32_t *p32 = (uint32_t *)(m_properties + info.m_offset + offset +             \
				     m.m_elementBytes * idx);	                         \
        /* if length to be written is more than 32 bits */                               \
        if (++ocpi_length > 32/CHAR_BIT)                                                 \
          memcpy(p32 + 1, val + 32/CHAR_BIT, ocpi_length - 32/CHAR_BIT);                 \
        uint32_t i;							                 \
        memcpy(&i, val, 32/CHAR_BIT);					                 \
        p32[0] = i;							                 \
        if (info.m_writeError)						                 \
          throw; /*"worker has errors after write */                                     \
      } \
      void set##pretty##SequenceProperty(const OA::Property &p, const run *vals, \
                                         size_t length) const {			 \
        if (length > p.m_info.m_sequenceLength) \
          throw; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors before write */ \
        char *cp = (char *)(m_properties + p.m_info.m_offset + 32/CHAR_BIT); \
        for (size_t i = 0; i < length; i++) { \
          size_t len = strlen(vals[i]); \
          if (len > p.m_info.m_sequenceLength) \
            throw; /* "string in sequence too long" */ \
          memcpy(cp, vals[i], len+1); \
        } \
        *(uint32_t *)(m_properties + p.m_info.m_offset) = (uint32_t)length; \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors after write */ \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
      virtual run \
      get##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member *mp, \
			    size_t offset, unsigned idx) const { \
        const OU::Member &m = mp ? *mp : info;			 \
        if (info.m_readError) \
          throw; /*"worker has errors before read "*/ \
        uint32_t *pp = (uint32_t *)(m_properties + info.m_offset + offset + \
				    m.m_elementBytes * idx);		\
        union { \
                run r; \
                uint32_t u32[bits/32]; \
        } u; \
        if (bits > 32) \
          u.u32[1] = pp[1]; \
        u.u32[0] = pp[0]; \
        if (info.m_readError) \
          throw; /*"worker has errors after read */ \
        return u.r; \
      } \
      unsigned get##pretty##SequenceProperty(const OA::Property &p, run *vals, size_t length) const { \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors before read "*/ \
        uint32_t n = *(uint32_t *)(m_properties + p.m_info.m_offset); \
        if (n > length) \
          throw; /* sequence longer than provided buffer */ \
        memcpy(vals, (void*)(m_properties + p.m_info.m_offset + p.m_info.m_align), \
               n * sizeof(run)); \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors after read */ \
        return n; \
      }

      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this. FIXME redundant length check
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) \
      virtual void \
      get##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member *mp, \
			    size_t off, char *cp, size_t length, unsigned idx) const { \
        const OU::Member &m = mp ? *mp : info;				   \
        size_t stringLength = m.m_stringLength;                           \
        if (length < stringLength + 1) \
          throw; /*"string buffer smaller than property"*/; \
        if (info.m_readError) \
          throw; /*"worker has errors before write */ \
        uint32_t i32, *p32 = (uint32_t *)(m_properties + info.m_offset + off + \
                                          m.m_elementBytes * idx);	   \
        memcpy(cp + 32/CHAR_BIT, p32 + 1, stringLength + 1 - 32/CHAR_BIT); \
        i32 = *p32; \
        memcpy(cp, &i32, 32/CHAR_BIT); \
        if (info.m_readError) \
          throw; /*"worker has errors after write */ \
      } \
      unsigned get##pretty##SequenceProperty \
      (const OA::Property &p, char **vals, size_t length, char *buf, size_t space) const { \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors before read */ \
        uint32_t n = *(uint32_t *)(m_properties + p.m_info.m_offset);	       \
        size_t wlen = p.m_info.m_stringLength + 1;		       \
        if (n > length) \
          throw; /* sequence longer than provided buffer */ \
        char *cp = (char *)(m_properties + p.m_info.m_offset + 32/CHAR_BIT); \
        for (unsigned i = 0; i < n; i++) { \
          if (space < wlen) \
            throw; \
          memcpy(buf, cp, wlen); \
          cp += wlen; \
          vals[i] = buf; \
          size_t slen = strlen(buf) + 1; \
          buf += slen; \
          space -= slen; \
        } \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors after read */ \
        return n; \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#if 0
      // older code
      void setProperty##n(const OA::PropertyInfo &info, uint##n##_t val, unsigned idx) const { \
        ((uint##n##_t*)(m_properties+info.m_offset))[idx] = val;		\
      }									         \
      uint##n##_t getProperty##n(const OA::PropertyInfo &info, unsigned idx) const {	\
        return ((uint##n##_t*)(m_properties+info.m_offset))[idx];		\
      }
#endif      
#define PUT_GET_PROPERTY(n)						         \
      void setProperty##n(const OA::PropertyInfo &, size_t, uint##n##_t, unsigned) const {} \
      uint##n##_t getProperty##n(const OA::PropertyInfo &, size_t, unsigned) const { return 0; }
      PUT_GET_PROPERTY(8)
      PUT_GET_PROPERTY(16)
      PUT_GET_PROPERTY(32)
      PUT_GET_PROPERTY(64)
      void setPropertyBytes(const OA::PropertyInfo &, size_t, const uint8_t *, size_t, unsigned)
        const {}
      void getPropertyBytes(const OA::PropertyInfo &, size_t, uint8_t *, size_t, unsigned, bool)
	const {}

    }; // End: class Worker


    OC::Worker& Artifact::
    createWorker(Application &app, const char* appInstName, ezxml_t impl, ezxml_t inst,
		 const OC::Workers &slaves, bool hasMaster, size_t member, size_t crewSize,
		 const OU::PValue* wParams, int que) {
      const char *kname = ezxml_cattr(impl, "name");
      assert(kname);
      std::string kstr(kname);

      kstr += "_kernel";
      for (unsigned n = 0; n < m_kernels.size(); n++) {
	if (m_kernels[n].m_name == kstr) {
	  Worker * w = 
	    new Worker(app, *this, m_kernels[n], appInstName, impl, inst, slaves, hasMaster,
		       member, crewSize, wParams, que);
	  return *w;
	}
      }
      throw OU::Error("Could not find OCL worker(kernel) named \"%s\" in \"%s\"",
		      kname, name().c_str());
    }

    void Application::
    run(DataTransfer::EventManager* /*event_manager*/, bool &more_to_do) {
      for (Worker* w = OU::Parent<Worker>::firstChild (); w; w = w->nextChild())
        w->run(more_to_do);
    }

    class Port : public OC::PortBase<Worker,Port,ExternalPort> {
      friend class Worker;
      cl_mem m_clBuffers;

    protected:
      Port(Worker& w, const OA::PValue* params, const OU::Port& mPort)
	: OC::PortBase<OO::Worker,OO::Port,OO::ExternalPort> (w, *this, mPort, params),
	  m_clBuffers(NULL) {
      }

      ~Port() {
	if (m_clBuffers) {
	  freeBuffers(allocation());
	  allocation() = NULL;
	}
      }
    private:
      OC::Container *hasAllocator() {
	return &parent().m_container;
      }
      uint8_t *allocateBuffers(size_t nBytes) {
	// Allow host reading and writing for now
	OCL_RC(m_clBuffers,
	       clCreateBuffer(parent().m_container.device().context(),
			      (isProvider() ? CL_MEM_READ_ONLY : CL_MEM_WRITE_ONLY) |
			      CL_MEM_ALLOC_HOST_PTR,
			      nBytes, NULL, &rc));
	void *vp;
	uint32_t que = static_cast<Worker&>(parent()).m_que;
	ocpiDebug("Allocating buffer set for OCL port \"%s\". nbuf %zu, total %zu enqueue(%u)",
		  name().c_str(), m_nBuffers, nBytes, que);
	OCL_RC(vp, clEnqueueMapBuffer(parent().m_container.device().cmdq(que), m_clBuffers,
				      false,
				      CL_MAP_READ|CL_MAP_WRITE,
				      0, nBytes,
				      0, 0, 0, &rc));
      ocpiDebug("finish port(%u)", que);
	OCL(clFinish(parent().m_container.device().cmdq(que)));
	return (uint8_t *)vp;
      }
      virtual void freeBuffers(uint8_t *) {
	assert(m_clBuffers);
	OCL(clReleaseMemObject(m_clBuffers));
      }
      virtual void mapBuffers(size_t offset, size_t size) {
	uint32_t que = static_cast<Worker&>(parent()).m_que;
	ocpiDebug("OCL map buffers %s %s %p %zu %zu %p %p %p enqueuemap(%u)",
		  parent().name().c_str(), name().c_str(),
		  this, offset, size, m_clBuffers, allocation(), m_forward, que);
	if (m_clBuffers) {
	  void *vp;
	  OCL_RC(vp, clEnqueueMapBuffer(parent().m_container.device().cmdq(que), m_clBuffers,
					false,
					CL_MAP_READ|CL_MAP_WRITE,
					offset, size,
					0, 0, 0, &rc));
	  assert(vp == (m_forward ? m_forward : this)->allocation() + offset);
	} else {
	  assert(m_forward);
	  m_forward->mapBuffers(offset, size);
	}

      };
      virtual void unmapBuffers(size_t offset, size_t size) {
	uint32_t que = static_cast<Worker&>(parent()).m_que;
	ocpiDebug("OCL unmap buffers %s %s %p %zu %zu %p %p %p enqueueunmap(%u)",
		  parent().name().c_str(), name().c_str(),
		  this, offset, size, m_clBuffers, allocation(), m_forward, que);
	if (m_clBuffers)
	  OCL(clEnqueueUnmapMemObject(parent().m_container.device().cmdq(que), m_clBuffers,
				      allocation() + offset, 0, 0, 0));
	else {
	  assert(m_forward);
	  m_forward->unmapBuffers(offset, size);
	}
      };
      intptr_t clBuffers() {
	return (intptr_t)m_clBuffers;
      }
      cl_mem myClBuffers() {
	intptr_t clb = clBuffers() ? clBuffers() : (m_forward ? m_forward->clBuffers() : NULL);
	assert(clb);
	return (cl_mem)(clb);
      }
      void disconnect ()
        throw ( OCPI::Util::EmbeddedException )
      {
	throw OU::Error( "OCL disconnect not yet implemented." );
      }

    protected:
      // This is a hook that happens when considering to run a worker.
      // Return the number of buffers available
      unsigned checkReady() {
	return isProvider() ? fullCount() : emptyCount();
      }
      // This port is in-process if it is bound to other in-process port that is NOT
      // an OCL port in a different card if both this container and the other one
      // support DMA....
      // Note that if the other port is not in process we don't have to figure that out.
      bool isInProcess(OC::LocalPort *other) const {
	if (other && &container().driver() == &other->container().driver()) {
	  Container &otherContainer = *static_cast<Container*>(&other->container());
	  if (parent().m_container.m_device.isAmdDma() && otherContainer.m_device.isAmdDma())
	    return false;
	}
	return true;
      }
      size_t bufferAlignment() const { return parent().m_container.device().bufferAlignment(); }

      // Start a connection that uses DMA
      // It is only at this point that we know that this port will use DMA and not local buffers.
      const OCPI::RDT::Descriptors *
      startConnect(const OCPI::RDT::Descriptors */*other*/, OCPI::RDT::Descriptors &/*feedback*/,
		   bool &/*done*/) {
	if (!parent().m_container.m_device.isAmdDma())
	  throw OU::Error("Connection to port %s of worker %s without OpenCL DMA not supported",
			  name().c_str(), parent().name().c_str());
#ifdef CL_MEM_BUS_ADDRESSABLE_AMD
	cl_bus_address_amd otherAddrs;
	if (isProvider()) {
	  OCL_RC(m_clBuffers,
		 clCreateBuffer(parent().m_container.device().context(),
				CL_MEM_BUS_ADDRESSABLE_AMD, 4096, &otherAddrs, &rc));
	  OCL(parent().m_container.device().m_clEnqueueMakeBuffersResidentAMD(parent().m_container.device().cmdq(parent().m_que),
					      1, &m_clBuffers, CL_TRUE, &otherAddrs, 0, NULL, NULL));
	} else {
	  otherAddrs.surface_bus_address = 0;
	  otherAddrs.marker_bus_address = 0; // Must be 4K page aligned...???
	  // Create a buffer that is actually memory on the other device
	  OCL_RC(m_clBuffers,
		 clCreateBuffer(parent().m_container.device().context(),
				CL_MEM_EXTERNAL_PHYSICAL_AMD, 4096, &otherAddrs, &rc));
	  OCL(clEnqueueMigrateMemObjects(parent().m_container.device().cmdq(parent().m_que),
					 1, &m_clBuffers, 0, 0, NULL, NULL));
	}
#endif
	return NULL;
      }
    }; // End: class Port

    void Worker::
    controlOperation(OU::Worker::ControlOperation op) {
      if (op == OU::Worker::OpStart) {
	m_oclWorker->connectedPorts = connectedPorts();
	for (Port *p = firstChild(); p; p = p->nextChild()) {
	  assert(p->ordinal() < m_nPorts);
	  m_myPorts[p->ordinal()] = p;
	}
	// Initialize the port structures in the kernel args.
	OCLPort *oclp = m_oclPorts;
	for (unsigned n = 0; n < m_nPorts; n++, oclp++) {
	  Port &p = *m_myPorts[n];
	  memset(oclp, 0, sizeof(*oclp));
	  oclp->maxLength =
	    OCPI_UTRUNCATE(uint32_t, p.bufferSize());
	  oclp->isConnected = (connectedPorts() & (1 << n)) != 0;
	  oclp->isOutput = !p.isProvider();
	  oclp->connectedCrewSize = OCPI_UTRUNCATE(uint32_t, p.nOthers());
	  oclp->dataValueWidthInBytes = OCPI_UTRUNCATE(uint32_t, p.metaPort().m_dataValueWidth);
	  oclp->bufferStride = OCPI_UTRUNCATE(uint32_t, p.bufferStride());
	  oclp->endOffset = OCPI_UTRUNCATE(uint32_t, p.bufferStride() * p.nBuffers());
	  cl_mem clb = p.myClBuffers();
	  OCL(clSetKernelArg(m_clKernel, 1+n, sizeof(p.m_clBuffers), &clb));
	}
      }
      if ((getControlMask () & (1 << op))) {
	kernelProlog((OCLOpCode)op);
	cl_event event;
	ocpiDebug("enqueue task(%u)", m_que);
	OCL(clEnqueueTask(m_container.device().cmdq(m_que), m_clKernel, 0, 0, &event));
	kernelEpilog();
      } else
	m_returned->result = OCL_OK;
      switch (op) {
      case OU::Worker::OpStart:
	m_isEnabled = true;
	m_runTimer.reset();
	m_runTimer.start();
	break;
      case  OU::Worker::OpStop:
      case OU::Worker::OpRelease:
	if (m_isEnabled) {
	  m_runTimer.stop();
	  m_runTimer.reset();
	}
	m_isEnabled = false;
	break;
      default:
	break;
      }
      switch (m_returned->result) {
      case OCL_OK:
	break;
      case OCL_ERROR:
	throw OU::Error("Control operation \"%s\" on OCL worker \"%s\" returned ERROR",
			OU::Worker::s_controlOpNames[op], m_name.c_str());
	break;
      case OCL_FATAL:
	m_isEnabled = false;
	setControlState(OU::Worker::UNUSABLE);
	throw OU::Error("Control operation \"%s\" on OCL worker \"%s\" returned FATAL ",
			OU::Worker::s_controlOpNames[op], m_name.c_str());
	break;
      default:
	m_isEnabled = false;
	throw
	  OU::Error("Control operation \"%s\" on OCL worker \"%s\" returned invalid "
		    "RCCResult value: 0x%x", OU::Worker::s_controlOpNames[op],
		    m_name.c_str(), m_returned->result);
      }    
    }



    bool
    Worker::
    chkForCompletion() {
      if ( ! m_running ) {
	return true;
      }


      cl_int info;
      OCL(clGetEventInfo(m_taskEvent, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), (void *)&info, NULL));
      bool done = false;
      ocpiDebug("Checking for event %p: now info: %u", m_taskEvent, info);

      /* Checking the return values corresponding to CL_EVENT_COMMAND_EXECUTION_STATUS flag */
      if ( info == CL_SUBMITTED ){
	// printf("the command has been submitted successfully.\n");
      } else if ( info == CL_QUEUED ){
	// printf("the command has been queued successfully.\n");
      } else if ( info == CL_RUNNING ){
	// printf("the command is running.\n");
      } else if ( info < 0 ){
	//	printf("the command was terminated abnormally.\n");
      } else if ( info == CL_COMPLETE ){
	//       	printf("the command has been completed successfully.\n");
	done = true;
      }

      if ( !done )
	return false;


      kernelEpilog();


      OCLPort *op = m_oclPorts;
      // FIXME: We must retrieve opcode, length, direct, eof for output.
      for (unsigned n = 0; n < m_nPorts; n++, op++)
	if (m_relevantMask & (1 << n)) {
	  Port *p = m_myPorts[n];
	  OC::ExternalBuffer *b =
	    p->isProvider() ? p->nextToRelease() : p->nextToPut();
	  for (unsigned r = 0; r < m_minReady; r++) {
	    assert(b);
	    // Buffer was being read or written by the GPU, and now should be switch to the CPU
	    p->mapBuffers(b->offset(), p->bufferStride());
	    assert(b->next());
	    b = b->next();
	  }
	}


      m_running = false;


      //      ocpiDebug("Execution of OCL worker %s completes, m_minReady %zu", name().c_str(), m_minReady);
      m_oclWorker->firstRun = false;
      switch (m_returned->result) {
      case OCL_ADVANCE:
      case OCL_ADVANCE_DONE:
	for (unsigned n = 0; n < m_nPorts; n++, op++)
	  if (m_relevantMask & (1 << n)) {
	    Port *p = m_myPorts[n];
	    for (unsigned r = 0; r < m_minReady; r++) {
	      OC::ExternalBuffer *b =
		p->isProvider() ? p->nextToRelease() : p->nextToPut();
	      assert(b);
	      if (p->isProvider())
		b->release();
	      else
		b->put();
	      assert(b);
	    }
	  }
	if (m_returned->result == OCL_ADVANCE)
	  break;
      case OCL_DONE:
	if (m_isEnabled)
	  m_runTimer.stop();
	m_isEnabled = false;
	break;
      case OCL_OK:
	/* Nothing to do */
	break;
      case OCL_FATAL:
      default:
	if (m_isEnabled)
	    m_runTimer.stop();
        m_isEnabled = false;
	setControlState(OU::Worker::UNUSABLE);
      }

      return true;

    }


    void Worker::
    run(bool& /*anyone_run*/) {

      if (!m_isEnabled)
	return;

      // If we are running in a Q, we need to wait for it to complete
      if ( ! chkForCompletion() ) {
	return;
      }

      bool timedOut = false, dont = false;
      m_minReady = 1;
      m_relevantMask = 0;
      do {
	// First do the checks that don't depend on port readiness.
	if (m_runCondition->shouldRun(m_runTimer, timedOut, dont))
	  break;
	else if (dont)
	  return;
	// Start out assuming optional unconnected ports are "ready"
	OCLPortMask readyMask = optionalPorts() & ~connectedPorts();
	m_relevantMask = connectedPorts() & m_runCondition->m_allMasks;
	OCLPort *oclPort = m_oclPorts;
	OCLPortMask portBit = 1;
	size_t nReady;
	m_minReady = SIZE_MAX;
	for (unsigned n = 0; n < m_nPorts; n++, oclPort++, portBit <<= 1)
	  if ((portBit & m_relevantMask) && (nReady = m_myPorts[n]->checkReady())) {
	    readyMask |= portBit;
	    if (nReady < m_minReady)
	      m_minReady = nReady;
	  }
	if (!m_minReady || m_minReady == SIZE_MAX)
	  return;
	// See if any of our masks are satisfied
	OCLPortMask *pmp, pm = 0;
	for (pmp = m_runCondition->m_portMasks; (pm = *pmp); pmp++)
	  if ((pm & readyMask) == (pm & ~(OCL_ALL_PORTS << m_nPorts)))
	    break;
	if (!pm)
	  return;
	m_oclWorker->runCount = OCPI_UTRUNCATE(uint8_t, m_minReady);
	OCLPort *op = m_oclPorts;
	for (unsigned n = 0; n < m_nPorts; n++, op++)
	  if (m_relevantMask & (1 << n)) {
	    Port *p = m_myPorts[n];


	    int count = 10;
	    while(( p->checkReady() < m_minReady ) && count > 0 ) {
	      usleep( 1000);
	      count--;
	    }
	    ocpiAssert(p->checkReady() >= m_minReady);
	    for (unsigned r = 0; r < m_minReady; r++) {
	      OC::ExternalBuffer *b = p->isProvider() ? p->getFullBuffer() : p->getEmptyBuffer();
	      //if (!b)
	      //p->debug(n);
	      assert(b);
	      // Buffer was being read or written by the CPU, and now should be switch to the GPU
	      p->unmapBuffers(b->offset(), p->bufferStride());
	      if (r == 0)
		op->readyOffset = OCPI_UTRUNCATE(uint32_t, b->offset());
	    }
	  }
      } while (0);
      /* Set the arguments to the worker */
      kernelProlog(OCPI_OCL_RUN);
      ocpiDebug("Enqueueing (%p:%u) OCL worker kernel for %s, m_minReady %zu, nDims %u:%zu,%zu,%zu",
		m_clKernel, m_que, name().c_str(), m_minReady, m_kernel.m_nDims,
		m_kernel.m_compile_work_group_size[0],
		m_kernel.m_compile_work_group_size[1],
		m_kernel.m_compile_work_group_size[2]);
      size_t global_work_sizes[3] = { m_container.device().maxComputeUnits() /
				     m_container.device().numSubDevices(), 1, 1};
      ocpiDebug("Enqueueing (%p:%u) OCL kernel for %s, m_minReady %zu, nDims %u:%zu,%zu,%zu gws: %zu",
		m_clKernel, m_que, name().c_str(), m_minReady, m_kernel.m_nDims,
		m_kernel.m_compile_work_group_size[0],
		m_kernel.m_compile_work_group_size[1],
		m_kernel.m_compile_work_group_size[2],
		global_work_sizes[0]);
      OCL(clEnqueueNDRangeKernel(m_container.device().cmdq(m_que),
				 m_clKernel,
				 m_kernel.m_nDims,
				 NULL,                     // global work offset unsupported.
				 global_work_sizes,        // global work size
				 m_kernel.m_compile_work_group_size[0] ?
				 m_kernel.m_compile_work_group_size : NULL,
				 0,
				 0,
				 0));
      ocpiDebug("Enqueueing MapBuffer");
      void *vp;
      OCL_RC(vp, clEnqueueMapBuffer(m_container.device().cmdq(m_que), m_clPersistent,
				    false, CL_MAP_READ|CL_MAP_WRITE, 0, m_persistBytes,
				    0, NULL, &m_taskEvent, &rc));
      ocpiDebug("Enqueued MapBuffer: %p", m_taskEvent);
      OCL(clFlush(m_container.device().cmdq(m_que)));
      assert(vp = m_persistent);
      m_running = true;

    }

    OC::Port& Worker::
    createPort(const OU::Port &mPort, const OA::PValue* props) {
      return *new Port(*this, props, mPort);
    }
  } // End: namespace OCL

} // End: namespace OCPI

