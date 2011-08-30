
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */




/**
  @brief
  This file contains the implementation for the RPL container for the OC FPGA
  Reference Platformm
  FIXME: we need to abstract the RPL aspects from the OCRP FPGA aspects.
  It implements the OCPI::RPL::Container class, which implements the
  OCPI::Container::Interface class.
  There is no separate header file for this class since its only purpose
  is to implement the OCPI::Container::Interface and no one else will use it.

  Revision History:

    5/6/2009 - Jim Kulp
    Initial version.

************************************************************************** */
#include <climits>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <uuid/uuid.h>
// FIXME: integrate this into our UUID utility properly
#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char uuid_string_t[50]; // darwin has 37 - lousy unsafe interface
#endif
#include "ezxml.h"
#include <OcpiOsMisc.h>
#include "PciScanner.h"
#include "OcpiContainerManager.h"
#include "OcpiWorker.h"
#include "OcpiPValue.h"
#include "OcpiContainerMisc.h"
#include "HdlOCCP.h"
#include "HdlOCDP.h"

#define wmb()        asm volatile("sfence" ::: "memory"); usleep(0)
#define clflush(p) asm volatile("clflush %0" : "+m" (*(char *)(p))) //(*(volatile char __force *)p))

namespace OCPI {
  namespace Container {}
  namespace HDL {
    namespace OA = OCPI::API;
    namespace OC = OCPI::Container;
    namespace OS = OCPI::OS;
    namespace OM = OCPI::Metadata;
    namespace OU = OCPI::Util;
    namespace OP = OCPI::Util::Prop;

    static inline unsigned max(unsigned a,unsigned b) { return a > b ? a : b;}
    // This is the alignment constraint of DMA buffers in the processor's memory.
    // It could be a cache line or a malloc granule...
    // It should come from somewhere else.  FIXME
    static const unsigned LOCAL_BUFFER_ALIGN = 32;
    // This is the constraint based both on the local issues and the OCDP's DMA engine.
    static const unsigned LOCAL_DMA_ALIGN = max(LOCAL_BUFFER_ALIGN, OCDP_FAR_BUFFER_ALIGN);
    class ExternalPort;
    class Container;
    const char *hdl = "hdl";
    class Driver : public OC::DriverBase<Driver, Container, hdl>, OCPI::PCI::Driver {

      friend class ExternalPort; // for pcimemfd
      // The fd for mapped memory, until we have a driver to restrict it.
      static int pciMemFd;

      // Create a container
      Container *create(const char *name, PCI::Bar *bars, unsigned nbars,
			const char *&err, const OU::PValue *props = NULL);

      // Create a dummy container using shm_open for the device
      Container *createDummy(const char *name, const char *df, const OA::PValue *);

      // Callback function for PciScanner::search which creates a container
      bool found(const char *name, PCI::Bar *bars, unsigned nbars);
    public:
      // This driver method is called when container-discovery happens, to see if there
      // are any container devices supported by this driver
      // It uses a generic PCI scanner to find candidates, and when found, calls the
      // "found" method.
      unsigned search(const OA::PValue*, const char **exclude);
      OC::Container *probeContainer(const char *which, const OA::PValue *props);

    };
    OC::RegisterContainerDriver<Driver> driver;

    bool Driver::found(const char *name, PCI::Bar *bars, unsigned nbars) {
      const char *err = 0;
      if (create(name, bars, nbars, err))
        return true;
      fprintf(stderr, "Error during probe for OCFRP: %s\n", err);
      return false;
    }
    class Artifact;
    class Port;
    class Application;
    class Container : public OC::ContainerBase<Driver, Container, Application, Artifact> {
      volatile OccpAdminRegisters *adminRegisters;
      volatile OccpSpace* occp;
      uint8_t *bar1Vaddr;
      uint64_t basePaddr, bar1Offset;
      uint8_t *baseVaddr;     // base virtual address of whole region containing both BARs
      uint64_t endPointSize; // size in bytes of whole region containing both BARs.
      std::string m_device, m_loadParams;
      uuid_t m_loadedUUID;
      friend class WciControl;
      friend class Driver;
      friend class Port;
      friend class Artifact;
    protected:
      Container(const char *name, uint64_t bar0Paddr,
                volatile OccpSpace *aOccp, uint64_t bar1Paddr,
		uint8_t *aBar1Vaddr, uint32_t bar1Size, const ezxml_t config = NULL,
		const OU::PValue *props = NULL) 
        : OC::ContainerBase<Driver,Container,Application,Artifact>(name, config, props),
	  occp(aOccp), bar1Vaddr(aBar1Vaddr)
      {
        if (bar0Paddr < bar1Paddr) {
          basePaddr = bar0Paddr;
          endPointSize = bar1Paddr + bar1Size - basePaddr;
          baseVaddr = (uint8_t *)occp;
        } else {
          basePaddr = bar1Paddr;
          endPointSize = bar0Paddr + sizeof(OccpSpace) - basePaddr;
          baseVaddr = bar1Vaddr;
        }
        bar1Offset = bar1Paddr - basePaddr;
	// Capture the UUID info that tells us about the platform
	HdlUUID myUUID;
	for (unsigned n = 0; n < sizeof(HdlUUID); n++)
	  ((uint8_t*)&myUUID)[sizeof(HdlUUID) - 1 - n] = ((volatile uint8_t *)&occp->admin.uuid)[n];
	if (myUUID.platform[0] && myUUID.platform[1])
	  m_platform.assign(myUUID.platform, sizeof(myUUID.platform));
	if (myUUID.device[0] && myUUID.device[1])
	  m_device.assign(myUUID.device, sizeof(myUUID.device));
	if (myUUID.load[0])
	  m_loadParams.assign(myUUID.load, sizeof(myUUID.load));
	memcpy(m_loadedUUID, myUUID.uuid, sizeof(m_loadedUUID));
	
	if (config) {
	  // what do I not know about this?
	  // usb port for jtag loading
	  // part type to look for artifacts
	  // esn for checking/asserting that
	  const char *cp = ezxml_cattr(config, "platform");
	  if (cp)
	    m_platform = cp;
	  cp = ezxml_cattr(config, "loadParams");
	  if (cp)
	    m_loadParams = cp;
	}
      }
      bool isLoadedUUID(const std::string &uuid) {
	uuid_string_t parsed;
	uuid_unparse(m_loadedUUID, parsed);
	return uuid == parsed;
      }
    public:
      ~Container() {
	this->lock();
        // FIXME: ref count driver static resources, like closing pciMemFd
      }
      void start() {}
      void stop() {}
      bool dispatch() { return false; }
      // friends
      void getWorkerAccess(unsigned index, volatile OccpWorkerRegisters *&r,
                           volatile uint8_t *&c)
      {
        if (index >= OCCP_MAX_WORKERS)
          throw OC::ApiError("Invalid occpIndex property", 0);
        // check this against something in the admin registers
        c = occp->config[index];
        r = &occp->worker[index].control;
      }
      void releaseWorkerAccess(unsigned index)
      {
	(void)index;
        // potential unmapping/ref counting
      }
#if 0
      // support worker ids for those who want it
      OC::Worker &findWorker(OC::WorkerId)
      {
        static OC::Worker *w; return *w;
      }
#endif
      OC::Artifact &
      createArtifact(OCPI::Library::Artifact &lart, const OA::PValue *artifactParams);
      OA::ContainerApplication *
      createApplication(const char *name, const OCPI::Util::PValue *props)
	throw ( OCPI::Util::EmbeddedException );
      bool needThread() { return false; }
    };
    
    unsigned Driver::search(const OA::PValue*, const char **exclude)
    {
      const char *df = getenv("OCPI_OCFRP_DUMMY");
      if (df) {
	createDummy("0000:99:00.0", df, 0);
	return 1;
      } else {
	unsigned n = 0;
	const char *err = PCI::search(exclude, OCFRP0_VENDOR, OCFRP0_DEVICE,
				      OCFRP0_CLASS, OCFRP0_SUBCLASS, *this, n);
	if (err)
	  fprintf(stderr, "PCI Scanner Error: %s\n", err);
	return n;
      }
    }
    OC::Container *Driver::probeContainer(const char *which, const OA::PValue *props)
    {
      const char *df = getenv("OCPI_OCFRP_DUMMY");
      if (df)
	return createDummy(which, df, props);
      // Real probe
      PCI::Bar bars[2];
      unsigned nbars = 2;
      const char *err = 0;
      if (!PCI::probe(which, OCFRP0_VENDOR, OCFRP0_DEVICE,
		      OCFRP0_CLASS, OCFRP0_SUBCLASS, bars, nbars, err) || err)
	OC::ApiError("Error probing \"", which, "\": ", err, NULL);
      Container *c = create(which, bars, nbars, err, props);
      if (!c)
	OC::ApiError("Error creating \"", which, "\" (which probed ok): ", err, NULL);
      return c;
    }

    // Create a dummy device emulated by a shared memory buffer
    Container *Driver::
    createDummy(const char *name, const char *df, const OA::PValue *) {
      int fd;
      uint8_t *bar0, *bar1;
      fprintf(stderr, "DF: %s, Page %d, Occp %" PRIsize_t ", SC pagesize %lu off_t %" PRIsize_t " bd %" PRIsize_t "\n",
              df, getpagesize(), sizeof(OccpSpace), sysconf(_SC_PAGE_SIZE),
              sizeof(off_t), sizeof(OC::PortData));
      umask(0);
      ocpiCheck((fd = shm_open(df, O_CREAT | O_RDWR, 0666)) >= 0);
      ocpiCheck(ftruncate(fd, sizeof(OccpSpace) + 64*1024) >= 0);
      ocpiCheck((bar0 = (uint8_t*)mmap(NULL, sizeof(OccpSpace) + 64*1024,
                                       PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0))
                != (uint8_t*)-1);
      bar1 = bar0 + sizeof(OccpSpace);
      return new Container(name, 0, (volatile OccpSpace*)bar0,
                           sizeof(OccpSpace), bar1, 64*1024);
    }

    // Internal driver method
    // Return container pointer OR null, and if NULL, set "err" output arg.
    Container *Driver::
    create(const char *name, PCI::Bar *bars, unsigned nbars, const char *&err,
	   const OU::PValue *props) {
      if (nbars != 2 || bars[0].io || bars[0].prefetch || bars[1].io || bars[1].prefetch ||
          bars[0].addressSize != 32 || bars[0].size != sizeof(OccpSpace))
        err = "OCFRP found but bars are misconfigured\n";
      else {
        void *bar0 = 0, *bar1 = 0;
        // PCI config info looks good.  Now check the OCCP signature.
        if (pciMemFd < 0 && (pciMemFd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
          err = "Can't open /dev/mem";
        else if ((bar0 = mmap(NULL, sizeof(OccpSpace), PROT_READ|PROT_WRITE, MAP_SHARED,
                              pciMemFd, bars[0].address)) == (void*)-1)
          err = "can't mmap /dev/mem for bar0";
        else if ((bar1 = mmap(NULL, bars[1].size, PROT_READ|PROT_WRITE, MAP_SHARED,
                              pciMemFd, bars[1].address)) == (void*)-1)
          err = "can't mmap /dev/mem for bar1";
        else {
          volatile OccpSpace *occp = (OccpSpace *)bar0;
	  //          static union { char string[5]; uint32_t value; }
	  //          magic1 = {{'n', 'e', 'p', 'O'}}, magic2 = {{0, 'I', 'P', 'C'}};
          //magic1 = {{'O', 'p', 'e', 'n'}}, magic2 = {{'C', 'P', 'I', '\0'}};
          //          magic1 = {OCCP_MAGIC1}, magic2 = {OCCP_MAGIC2};
          //if (occp->admin.magic1 != magic1.value || occp->admin.magic2 != magic2.value) {
	  if (occp->admin.magic1 != OCCP_MAGIC1 || occp->admin.magic2 != OCCP_MAGIC2) {
            err = "Magic numbers do not match in region/bar 0";
	    fprintf(stderr, "PCI Device matches OCFRP vendor/device, but not OCCP signature: "
		    "magic1: 0x%x (sb 0x%x), magic2: 0x%x (sb 0x%x)",
		    occp->admin.magic1, OCCP_MAGIC1, occp->admin.magic2, OCCP_MAGIC2);
          } else {
            char tbuf[30];
            time_t bd = occp->admin.birthday;
            fprintf(stderr, "OCFRP: %s, with bitstream birthday: %s", name, ctime_r(&bd, tbuf));
	    return new Container(name, bars[0].address, occp, bars[1].address,
				 (uint8_t*)bar1, bars[1].size, getDeviceConfig(name),
				 props);
          }
        }
        if (bar0)
          munmap(bar0, sizeof(OccpSpace));
        if (bar1)
          munmap(bar1, bars[1].size);
      }
      return 0;
    }

    class Artifact : public OC::ArtifactBase<Container,Artifact> {
      friend class Container;
      Artifact(Container &c, OCPI::Library::Artifact &lart, const OA::PValue *artifactParams) :
        OC::ArtifactBase<Container,Artifact>(c, lart, artifactParams) {
	if (!lart.uuid().empty() && c.isLoadedUUID(lart.uuid()))
	  printf("For HDL container %s, when loading bitstream %s, uuid matches what is already loaded\n",
		 c.name().c_str(), name().c_str());
	else {
	  printf("Loading bitstream %s on HDL container %s\n",
		 name().c_str(), c.name().c_str());
	  // FIXME: there should be a utility to run a script in this way
	  char *command, *base = getenv("OCPI_CDK_DIR");
	  if (!base)
	    throw "OCPI_CDK_DIR environment variable not set";
	  asprintf(&command, "%s/scripts/loadBitStreamOnPlatform \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
		   base, name().c_str(), c.name().c_str(), c.m_platform.c_str(), c.m_device.c_str(), c.m_loadParams.c_str());
	  printf("Executing command to load bit stream for container %s: \"%s\"\n",
		 c.name().c_str(), command);
	  int rc = system(command);
	  const char *err = 0;
	  switch (rc) {
	  case 127:
	    err = "Couldn't execute bitstream loading command.  Bad OCPI_CDK_DIR environment variable?";
	    break;
	  case -1:
	    err = esprintf("Unknown system error (errno %d) while executing bitstream loading command",
			   errno);
	    break;
	  case 0:
	    printf("Successfully loaded bitstream file: \"%s\" on HDL container \"%s\"\n",
		   lart.name().c_str(), c.name().c_str());
	    break;
	  default:
	    err = esprintf("Bitstream loading error (%d) loading \"%s\" on HDL container \"%s\"",
			   rc, lart.name().c_str(), c.name().c_str());
	  }
	  if (err)
	    throw OC::ApiError(err, NULL);
	}
      }
    public:
      ~Artifact() {}
    };

    // We know we have not already loaded it, but it still might be loaded on the device.
    OC::Artifact & Container::
    createArtifact(OCPI::Library::Artifact &lart, const OA::PValue *artifactParams)
    {
      return *new Artifact(*this, lart, artifactParams);
    }
    // The class that knows about WCI interfaces and the OCCP.
    class WciControl : virtual public OC::Controllable {
      const char *implName, *instName;
    protected:
      Container &myWciContainer;
      // myRegisters is zero when this WCI does not really exist.
      // (since we inherit this in some cases were it is not needed).
      volatile OccpWorkerRegisters *myRegisters;
      unsigned myOccpIndex;
      volatile uint8_t *myProperties;
      WciControl(Container &container, ezxml_t implXml, ezxml_t instXml)
        : implName(0), instName(0), myWciContainer(container), myRegisters(0) {
	setControlOperations(ezxml_cattr(implXml, "controlOperations"));
        if (!implXml)
          return;
        implName = ezxml_attr(implXml, "name");
        instName = ezxml_attr(instXml, "name");
	setControlMask(getControlMask() | 1 << OM::Worker::OpStart);
        myOccpIndex = OC::getAttrNum(instXml, "occpIndex");
        uint32_t timeout = OC::getAttrNum(implXml, "timeout", true);
        if (!timeout)
          timeout = 16;
        unsigned logTimeout = 31;
        for (uint32_t u = 1 << logTimeout; !(u & timeout);
             u >>= 1, logTimeout--)
          ;
        printf("Timeout for $%s is %d\n", implName, logTimeout);
        myWciContainer.getWorkerAccess(myOccpIndex, myRegisters, myProperties);
        // Assert Reset
        myRegisters->control =  logTimeout;
#ifndef SHEP_FIXME_THE_RESET
        struct timespec spec;
        spec.tv_sec = 0;
        spec.tv_nsec = 10000;
        int bad = nanosleep(&spec, 0);
        ocpiCheck(bad == 0);
#endif
        // Take out of reset
        myRegisters->control = OCCP_CONTROL_ENABLE | logTimeout ;
        if (getenv("OCPI_OCFRP_DUMMY")) {
          *(uint32_t *)&myRegisters->initialize = OCCP_SUCCESS_RESULT; //fakeout
          *(uint32_t *)&myRegisters->start = OCCP_SUCCESS_RESULT; //fakeout
        }
      }
      virtual ~WciControl() {
        if (myRegisters)
          myWciContainer.releaseWorkerAccess(myOccpIndex);
      }
      // Add the hardware considerations to the property object that supports
      // fast memory-mapped property access directly to users
      // the key members are "readVaddr" and "writeVaddr"
      virtual void prepareProperty(OP::Property &md, OA::Property &cp) {
        if (myRegisters)
          if (!md.isStruct && !md.members->type.isSequence &&
	      !md.members->type.scalar != OP::Scalar::OCPI_String &&
              OP::Scalar::sizes[md.members->type.scalar] <= 32 &&
	      md.m_offset < OCCP_WORKER_CONFIG_SIZE &&
              !md.m_writeError)
            cp.m_writeVaddr = myProperties + md.m_offset;
      }
      // Map the control op numbers to structure members
      static const unsigned controlOffsets[];
#if 0
 = {
#define CONTROL_OP(x, c, t, s1, s2, s3) \
      offsetof(OccpWorkerRegisters,x) / sizeof (uint32_t),
      OCPI_CONTROL_OPS
#undef CONTROL_OP
	0};
#endif
    public:
      void controlOperation(OCPI::Metadata::Worker::ControlOperation op) {
	if (getControlMask() & (1 << op)) {
	  uint32_t result = *((volatile uint32_t *)myRegisters + controlOffsets[op]);
	  if (result != OCCP_SUCCESS_RESULT) {
	    const char *oops;
	    switch (result) {
	    case OCCP_TIMEOUT_RESULT:
	      oops = "timed out performing control operation";
	      break;
	    case OCCP_ERROR_RESULT:
	      oops = "indicated an error from control operation";
	      break;
	    case OCCP_RESET_RESULT:
	      oops = "was in a reset state when control operation was requested";
	      break;
	    case OCCP_FATAL_RESULT:
	      oops = "indicated a fatal error from control operation";
	      break;
	    default:
	      oops = "returned unknown result value from control operation";
	    }
	    throw OC::ApiError("Worker \"", implName, ":", instName, "\" ", oops, 0);
	  }
	}
      }
    };
    // Idiotic c++ doesn't allow static initializations in class definitions.
    const unsigned WciControl::controlOffsets[] = {
#define CONTROL_OP(x, c, t, s1, s2, s3) \
      offsetof(OccpWorkerRegisters,x) / sizeof (uint32_t),
      OCPI_CONTROL_OPS
#undef CONTROL_OP
	0};

    class Worker;
    class Application : public OC::ApplicationBase<Container, Application, Worker> {
      friend class Container;
      Application(Container &con, const char *name, const OA::PValue *props) 
	: OC::ApplicationBase<Container, Application, Worker>(con, name, props)
      {}
      // Note that this cannot be a "default" implementation in the OC::Application
      // base class since we must pass the correct/derived type of application
      // to the artifact (to at least avoid casting...)
      OC::Worker & createWorker(OC::Artifact *art, const char *appInstName,
				ezxml_t impl, ezxml_t inst,
				const OCPI::Util::PValue *wParams);
    };
    OA::ContainerApplication *Container::
    createApplication(const char *name, const OCPI::Util::PValue *props)
      throw ( OCPI::Util::EmbeddedException ) {
      return new Application(*this, name, props);
    };
    class Worker : public OC::WorkerBase<Application, Worker, Port>,  public WciControl {
      friend class Application;
      friend class Port;
      Container &m_container;
      Worker(Application &app, OC::Artifact *art, const char *name,
             ezxml_t implXml, ezxml_t instXml, const OA::PValue* execProps) :
        OC::WorkerBase<Application, Worker, Port>(app, art, name, implXml, instXml, execProps),
        WciControl(app.parent(), implXml, instXml),
        m_container(app.parent())
      {
	(void)execProps;
      }
    public:
      ~Worker()
      {
      }
      inline void controlOperation(OM::Worker::ControlOperation op) {
	WciControl::controlOperation(op);
      }

      // FIXME: These (and sequence/string stuff above) need to be sensitive to
      // addresing windows in OCCP.
      void read(uint32_t, uint32_t, void*) {
      }
      void write(uint32_t, uint32_t, const void*) {
      }

      OC::Port & createPort(const OM::Port &metaport, const OA::PValue *props);

      virtual void prepareProperty(OP::Property &mp, OA::Property &cp) {
        return WciControl::prepareProperty(mp, cp);
      }

      OC::Port &
      createOutputPort(OM::PortOrdinal portId,
                       OS::uint32_t bufferCount,
                       OS::uint32_t bufferSize,
                       const OA::PValue* props) throw();
      OC::Port &
      createInputPort(OM::PortOrdinal portId,
                      OS::uint32_t bufferCount,
                      OS::uint32_t bufferSize,
                      const OA::PValue* props) throw();

      // These property access methods are called when the fast path
      // is not enabled, either due to no MMIO or that the property can
      // return errors.  OCCP has MMIO, so it must be the latter
#undef OCPI_DATA_TYPE_S
      // Set a scalar property value
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
      void set##pretty##Property(OA::Property &p, const run val) {                \
        if (p.m_info.m_writeError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before write */                        \
        volatile store *pp = (volatile store *)(myProperties + p.m_info.m_offset);                        \
        if (bits > 32) {                                                \
          assert(bits == 64);                                                \
          volatile uint32_t *p32 = (volatile uint32_t *)pp;                                \
          p32[1] = ((const uint32_t *)&val)[1];                                \
          p32[0] = ((const uint32_t *)&val)[0];                                \
        } else                                                                \
          *pp = *(const store *)&val;                                                \
        if (p.m_info.m_writeError && myRegisters->status & OCCP_STATUS_ALL_ERRORS) \
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      void set##pretty##SequenceProperty(OA::Property &p,const run *vals, unsigned length) { \
        if (p.m_info.m_writeError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before write */                        \
        memcpy((void *)(myProperties + p.m_info.m_offset + p.m_info.m_maxAlign), vals, length * sizeof(run)); \
        *(volatile uint32_t *)(myProperties + p.m_info.m_offset) = length;                \
        if (p.m_info.m_writeError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after write */                        \
      }
      // Set a string property value FIXME redundant length check??? 
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
      virtual void set##pretty##Property(OA::Property &p, const run val) {        \
        unsigned ocpi_length;                                                \
        if (!val || (ocpi_length = strlen(val)) > p.m_type.stringLength)                \
          throw; /*"string property too long"*/;                        \
        if (p.m_info.m_writeError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before write */                        \
        uint32_t *p32 = (uint32_t *)(myProperties + p.m_info.m_offset);                \
        /* if length to be written is more than 32 bits */                \
        if (++ocpi_length > 32/CHAR_BIT)                                        \
          memcpy(p32 + 1, val + 32/CHAR_BIT, ocpi_length - 32/CHAR_BIT); \
        uint32_t i;                                                        \
        memcpy(&i, val, 32/CHAR_BIT);                                        \
        p32[0] = i;                                                        \
        if (p.m_info.m_writeError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      void set##pretty##SequenceProperty(OA::Property &p,const run *vals, unsigned length) { \
        if (length > p.m_type.length)                                        \
          throw;                                                        \
        if (p.m_info.m_writeError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before write */                        \
        char *cp = (char *)(myProperties + p.m_info.m_offset + 32/CHAR_BIT);        \
        for (unsigned i = 0; i < length; i++) {                                \
          unsigned len = strlen(vals[i]);                                \
          if (len > p.m_type.length)                                        \
            throw; /* "string in sequence too long" */                        \
          memcpy(cp, vals[i], len+1);                                        \
        }                                                                \
        *(uint32_t *)(myProperties + p.m_info.m_offset) = length;                \
        if (p.m_info.m_writeError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after write */                        \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
      virtual run get##pretty##Property(OA::Property &p) {                        \
        if (p.m_info.m_readError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before read "*/                        \
        uint32_t *pp = (uint32_t *)(myProperties + p.m_info.m_offset);                \
        union {                                                                \
                run r;                                                        \
                uint32_t u32[bits/32];                                        \
        } u;                                                                \
        if (bits > 32)                                                        \
          u.u32[1] = pp[1];                                                \
        u.u32[0] = pp[0];                                                \
        if (p.m_info.m_readError && myRegisters->status & OCCP_STATUS_ALL_ERRORS) \
          throw; /*"worker has errors after read */                        \
        return u.r;                                                        \
      }                                                                        \
      unsigned get##pretty##SequenceProperty(OA::Property &p, run *vals, unsigned length) { \
        if (p.m_info.m_readError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before read "*/                        \
        uint32_t n = *(uint32_t *)(myProperties + p.m_info.m_offset);                \
        if (n > length)                                                        \
          throw; /* sequence longer than provided buffer */                \
        memcpy(vals, (void*)(myProperties + p.m_info.m_offset + p.m_info.m_maxAlign),        \
               n * sizeof(run));                                        \
        if (p.m_info.m_readError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after read */                        \
        return n;                                                        \
      }

      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this. FIXME redundant length check
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
      virtual void get##pretty##Property(OA::Property &p, char *cp, unsigned length) { \
        unsigned stringLength = p.m_type.stringLength;		\
        if (length < stringLength + 1)                                        \
          throw; /*"string buffer smaller than property"*/;                \
        if (p.m_info.m_readError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before write */                        \
        uint32_t i32, *p32 = (uint32_t *)(myProperties + p.m_info.m_offset);        \
        memcpy(cp + 32/CHAR_BIT, p32 + 1, stringLength + 1 - 32/CHAR_BIT); \
        i32 = *p32;                                                        \
        memcpy(cp, &i32, 32/CHAR_BIT);                                        \
        if (p.m_info.m_readError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      unsigned get##pretty##SequenceProperty                                \
      (OA::Property &p, run *vals, unsigned length, char *buf, unsigned space) { \
        if (p.m_info.m_readError &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before read */                        \
        uint32_t                                                        \
          n = *(uint32_t *)(myProperties + p.m_info.m_offset),                        \
          wlen = p.m_type.stringLength + 1;                            \
        if (n > length)                                                        \
          throw; /* sequence longer than provided buffer */                \
        char *cp = (char *)(myProperties + p.m_info.m_offset + 32/CHAR_BIT);        \
        for (unsigned i = 0; i < n; i++) {                                \
          if (space < wlen)                                                \
            throw;                                                        \
          memcpy(buf, cp, wlen);                                        \
          cp += wlen;                                                        \
          vals[i] = buf;                                                \
          unsigned slen = strlen(buf) + 1;                                \
          buf += slen;                                                        \
          space -= slen;                                                \
        }                                                                \
        if (p.m_info.m_readError &&						\
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after read */                        \
        return n;                                                        \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
    };
    OC::Worker & Application::createWorker(OC::Artifact *art, const char *appInstName,
					   ezxml_t impl, ezxml_t inst,
					   const OCPI::Util::PValue *wParams) {
	return *new Worker(*this, art, appInstName, impl, inst, wParams);
      }
    // This port class really has two cases: externally connected ports and
    // internally connected ports.
    // Also ports are either user or provider.
    // So this class takes care of all 4 cases, since the differences are so
    // minor as to not be worth (re)factoring (currently).
    class Port : public OC::PortBase<Worker,Port,ExternalPort>, WciControl {
      friend class Worker;
      friend class ExternalPort;
      ezxml_t m_connection;
      // These are for external ports
      // Which would be in a different class if we separate them
      unsigned myOcdpSize;
      volatile OcdpProperties *myOcdpRegisters;
      // For direct user access to ports
      uint8_t *userDataBaseAddr;
      volatile OcdpMetadata *userMetadataBaseAddr;
      bool userConnected;
      static int dumpFd;

      void disconnect()
        throw ( OCPI::Util::EmbeddedException )
      {
        throw OCPI::Util::EmbeddedException("disconnect not yet implemented !!");
      }

      // Called after connection PValues have been set, which is after our constructor
      // userDataBaseAddr, dataBufferBaseAddr are assumed set
      // Also error-check for bad combinations or values of parameters
      // FIXME:  we are relying on dataBufferBaseAddr being set before we know
      // buffer sizes etc.  If we are sharing a memory pool, this will not be the case,
      // and we would probably allocate the whole thing here.
      void checkConnectParams() {
        if (!m_canBeExternal)
          return;
        if (myDesc.nBuffers *
            (OC::roundup(myDesc.dataBufferSize, OCDP_LOCAL_BUFFER_ALIGN) + OCDP_METADATA_SIZE) > myOcdpSize)
          throw OC::ApiError("Requested buffer count and size won't fit in the OCDP's memory", 0);
        myDesc.dataBufferPitch = OC::roundup(myDesc.dataBufferSize, OCDP_LOCAL_BUFFER_ALIGN);
        myDesc.metaDataBaseAddr =
          myDesc.dataBufferBaseAddr +
          myOcdpSize - myDesc.nBuffers * OCDP_METADATA_SIZE;
        userMetadataBaseAddr = (OcdpMetadata *)(userDataBaseAddr +
                                                (myDesc.metaDataBaseAddr -
                                                 myDesc.dataBufferBaseAddr));
      }
      // I am an input port, the other guy is an output port.
      // My job is to emulate a bitstream that consumes from me, and produces at otherPort
      // This is for testing
      void loopback(const OA::PValue *pProps, OA::Port &apiUserPort, const OA::PValue *uProps) {
	OC::Port &uPort = *static_cast<OC::Port*>(&apiUserPort);
        ocpiAssert(isProvider());
        ocpiAssert(! uPort.isProvider());
        // default to something useful
        connectionData.data.role = OCPI::RDT::Passive;
        uPort.connectionData.data.role = OCPI::RDT::Passive;
        applyConnectParams(pProps);
        uPort.applyConnectParams(uProps);
        // We must initialize the emulated register file for use by other software
        Port &other = *static_cast<Port *>(&uPort);
        myOcdpRegisters->nRemoteDone = 0;
        myOcdpRegisters->nReady = myDesc.nBuffers;
        other.myOcdpRegisters->nRemoteDone = 0;
        other.myOcdpRegisters->nReady = 0;
        unsigned doneCount = 0, copyCount = 0, myUserNext = 0, otherUserNext = 0;
        for (;;) {
          if (myOcdpRegisters->nRemoteDone != 0) {
            doneCount++;
            ocpiAssert(myOcdpRegisters->nReady != 0);
            myOcdpRegisters->nReady--;
            myOcdpRegisters->nRemoteDone = 0;
            copyCount++;
          }
          if (other.myOcdpRegisters->nRemoteDone != 0) {
            ocpiAssert(other.myOcdpRegisters->nReady != 0);
            other.myOcdpRegisters->nReady--;
            other.myOcdpRegisters->nRemoteDone = 0;
          }
          while (copyCount && other.myOcdpRegisters->nReady != other.myDesc.nBuffers) {
            uint8_t *remoteData = userDataBaseAddr + myUserNext * myDesc.dataBufferSize;
            volatile OcdpMetadata *remoteMetadata = userMetadataBaseAddr + myUserNext;
            if (++myUserNext >= myDesc.nBuffers)
              myUserNext = 0;
            uint8_t *otherData = other.userDataBaseAddr + otherUserNext * other.myDesc.dataBufferSize;
            volatile OcdpMetadata *otherMetadata = other.userMetadataBaseAddr + otherUserNext;
            memcpy((void *)otherMetadata, (void *)remoteMetadata, sizeof(OcdpMetadata));
            memcpy(otherData, remoteData, remoteMetadata->length);
            myOcdpRegisters->nReady++;
            other.myOcdpRegisters->nReady++;
            if (++otherUserNext >= other.myDesc.nBuffers)
              otherUserNext = 0;
            copyCount--;
          }
        }
      }
      Port(Worker &w,
	   const OA::PValue *props,
           const OM::Port &mPort, // the parsed port metadata
           ezxml_t connXml, // the xml connection for this port
           ezxml_t icwXml,  // the xml interconnect/infrastructure worker attached to this port if any
           ezxml_t icXml, // the xml interconnect instance attached to this port if any
	   bool argIsProvider) :
        OC::PortBase<Worker,Port,ExternalPort>(w, props, mPort, argIsProvider),
        WciControl(w.m_container, icwXml, icXml),
        m_connection(connXml),
        myOcdpRegisters((volatile OcdpProperties *)myProperties),
        userConnected(false)
      {
        if (!icXml) {
          m_canBeExternal = false;
          return;
        }
        m_canBeExternal = true;
        // This will eventually be in the IP:  FIXME
        uint32_t myOcdpOffset = OC::getAttrNum(icXml, "ocdpOffset");
        myOcdpSize = myOcdpRegisters->memoryBytes;
        // myOcdpSize = OC::getAttrNum(icXml, "ocdpSize");
        //FIXME:  extra level of port metadata from worker, to impl, to instance?
        // get buffer sizes from port from somewhere.
        // from port xml? myMinBuffersize = getAttrNum(icXml, "minBufferSize", true);
        // from port xml? myMinNumBuffers = getAttrNum(icXml, "minNumBuffers", true);
        // Fill in the transport information with defaults.
        // It will be updated at connect time.
        // FIXME: do we need to assert a preference here?
        connectionData.data.role = OCPI::RDT::NoRole;
        connectionData.data.options =
          (1 << OCPI::RDT::Passive) |
          (1 << OCPI::RDT::ActiveFlowControl) |
          (1 << OCPI::RDT::ActiveMessage);
        const char *busId = "0";
        // These will be determined at connection time
        myDesc.dataBufferPitch   = 0;
        myDesc.metaDataBaseAddr  = 0;
        // Fixed values not set later in checkConnectParams
        myDesc.fullFlagSize      = 4;
        myDesc.fullFlagPitch     = 0;
        myDesc.fullFlagValue     = 1; // will be a count of buffers made full someday?
        myDesc.emptyFlagSize     = 4;
        myDesc.emptyFlagPitch    = 0;
        myDesc.emptyFlagValue    = 1; // Will be a count of buffers made empty
        myDesc.metaDataPitch     = sizeof(OcdpMetadata);
        myDesc.dataBufferBaseAddr = w.m_container.bar1Offset + myOcdpOffset;

#ifdef PORT_COMPLETE
        myDesc.oob.pid = (uint64_t)(OC::Port *)this;
        myDesc.oob.cid = 0;
#endif

        snprintf(myDesc.oob.oep, sizeof(myDesc.oob.oep),
                 "ocpi-pci-pio://%s.%lld:%lld.3.10", busId,
                 (long long unsigned)w.m_container.basePaddr,
                 (long long unsigned)w.m_container.endPointSize);
        if ( isProvider()) {
          // CONSUMER
          // BasicPort does this: connectionData.data.type = OCPI::RDT::ConsumerDescT;
          // The flag is in the OCDP's register space.
          // "full" is the flag telling me (the consumer) a buffer has become full
          // Mode dependent usage:
          // *Passive/ActiveFlowCtl: producer hits this after writing/filling local buffer
          //  (thus it is a "local buffer is full")
          // *Active Message: producer hits this when remote buffer is ready to pull
          //  (thus it is a "remote buffer is full")
          // This register is for WRITING.
          myDesc.fullFlagBaseAddr =
            (uint8_t*)&myOcdpRegisters->nRemoteDone - (uint8_t *)myWciContainer.baseVaddr;
          // The nReady register is the empty flag, which tells the producer how many
          // empty buffers there are to fill when consumer is in PASSIVE MODE
          // Other modes it is not used.
          // This register is for READING (in passive mode)
          myDesc.emptyFlagBaseAddr =
            (uint8_t*)&myOcdpRegisters->nReady - (uint8_t *)myWciContainer.baseVaddr;
        } else {
          // BasicPort does this: connectionData.data.type = OCPI::RDT::ProducerDescT;
          // The flag is in the OCDP's register space.
          // "empty" is the flag telling me (the producer) a buffer has become empty
          // Mode dependent usage:
          // *Passive/ActiveFlowCtl: consumer hits this after making a local buffer empty
          //  (thus it is a "local buffer is empty")
          // *ActiveMessage: consumer hits this after making a remote buffer empty
          // (thus it is a "remote buffer is empty")
          // This register is for writing.
          myDesc.emptyFlagBaseAddr =
            (uint8_t*)&myOcdpRegisters->nRemoteDone - (uint8_t *)myWciContainer.baseVaddr;
          // The nReady register is the full flag, which tells the consumer how many
          // full buffers there are to read/take when producer is PASSIVE
          // This register is for READING (in passive mode)
          myDesc.fullFlagBaseAddr =
            (uint8_t*)&myOcdpRegisters->nReady - (uint8_t *)myWciContainer.baseVaddr;
        }
        userDataBaseAddr = myWciContainer.bar1Vaddr + myOcdpOffset;
        const char *df = getenv("OCPI_DUMP_PORTS");
        if (df) {
          if (dumpFd < 0)
            ocpiCheck((dumpFd = creat(df, 0666)) >= 0);
          OC::PortData *pd = this;
          ocpiCheck(::write(dumpFd, (void *)pd, sizeof(*pd)) == sizeof(*pd));
        }
        if (getenv("OCPI_OCFRP_DUMMY"))
          *(uint32_t*)&myOcdpRegisters->foodFace = 0xf00dface;
	// Allow default connect params on port construction prior to connect
	applyConnectParams(props);
      }
      // All the info is in.  Do final work to (locally) establish the connection
      void finishConnection(OCPI::RDT::Descriptors &other) {
        // Here is where we can setup the OCDP producer/user
        ocpiAssert(myOcdpRegisters->foodFace == 0xf00dface);
        myOcdpRegisters->nLocalBuffers = myDesc.nBuffers;
        myOcdpRegisters->localBufferSize = myDesc.dataBufferPitch;
        myOcdpRegisters->localBufferBase = 0;
        myOcdpRegisters->localMetadataBase = myOcdpSize - myDesc.nBuffers * OCDP_METADATA_SIZE;
        OcdpRole myOcdpRole;
        OCPI::RDT::PortRole myRole = (OCPI::RDT::PortRole)connectionData.data.role;
        // FIXME - can't we avoid string processing here?
        unsigned busId;
        uint64_t busAddress, busSize;
        if (sscanf(other.desc.oob.oep, "ocpi-pci-pio://%x.%lld:%lld.3.10", &busId,
                   (long long unsigned *)&busAddress,
                   (long long unsigned *)&busSize) != 3)
          throw OC::ApiError("other port's endpoint description wrong: \"",
                             other.desc.oob.oep, "\"", NULL);



        printf("\n\n\n base = %lld, offset = %lld, RFB = %lld  \n\n\n",  (long long)busAddress,
	       (long long)(isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr ),
          (long long)(busAddress +
                     (isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr )) );

               printf("Other ep = %s\n", other.desc.oob.oep );



        switch (myRole) {
        case OCPI::RDT::ActiveFlowControl:
          myOcdpRole = OCDP_ACTIVE_FLOWCONTROL;
          myOcdpRegisters->remoteFlagBase = busAddress +
            (isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr);
          myOcdpRegisters->remoteFlagPitch =
            (isProvider() ?
             other.desc.emptyFlagPitch : other.desc.fullFlagPitch);
          break;
        case OCPI::RDT::ActiveMessage:
          myOcdpRole = OCDP_ACTIVE_MESSAGE;
          myOcdpRegisters->remoteBufferBase = busAddress + other.desc.dataBufferBaseAddr;
          myOcdpRegisters->remoteMetadataBase = busAddress + other.desc.metaDataBaseAddr;
          if ( isProvider()) {
            if (other.desc.dataBufferSize > myDesc.dataBufferSize)
              throw OC::ApiError("At consumer, remote buffer size is larger than mine", NULL);
          } else if (other.desc.dataBufferSize < myDesc.dataBufferSize) {
            throw OC::ApiError("At producer, remote buffer size smaller than mine", NULL);
          }
          myOcdpRegisters->nRemoteBuffers = other.desc.nBuffers;
          myOcdpRegisters->remoteBufferSize = other.desc.dataBufferPitch;
#ifdef WAS
          myOcdpRegisters->remoteMetadataSize = OCDP_METADATA_SIZE;
#else
          myOcdpRegisters->remoteMetadataSize = other.desc.metaDataPitch;
#endif
          myOcdpRegisters->remoteFlagBase = busAddress +
            ( isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr);
          myOcdpRegisters->remoteFlagPitch =
            ( isProvider() ?
             other.desc.emptyFlagPitch : other.desc.fullFlagPitch);
          break;
        case OCPI::RDT::Passive:
          myOcdpRole = OCDP_PASSIVE;
          break;
        default:
          myOcdpRole = OCDP_PASSIVE; // quiet compiler warning
          ocpiAssert(0);
        }
        myOcdpRegisters->control =
          OCDP_CONTROL(isProvider() ? OCDP_CONTROL_CONSUMER : OCDP_CONTROL_PRODUCER,
                       myOcdpRole);
	// We aren't a worker so someone needs to start us.
	controlOperation(OM::Worker::OpInitialize);
	controlOperation(OM::Worker::OpStart);
      }
      // Connection between two ports inside this container
      // We know they must be in the same artifact, and have a metadata-defined connection
      void connectInside(OC::Port &provider, const OA::PValue *myProps, const OA::PValue *otherProps) {
        // We're both in the same runtime artifact object, so we know the port class
        Port &pport = static_cast<Port&>(provider);
        if (m_connection != pport.m_connection)
          throw OC::ApiError("Ports are both local in bitstream/artifact, but are not connected", 0);
        pport.applyConnectParams(otherProps);
        applyConnectParams(myProps);
        // For now we assume there is nothing to actually adjust in the bitstream.
      }
      // Connect to a port in a like container (same driver)
      bool connectLike(const OA::PValue *uProps, OC::Port &provider, const OA::PValue *pProps) {
        // We're both in the same runtime artifact object, so we know the port class
        Port &pport = static_cast<Port&>(provider);
        ocpiAssert(m_canBeExternal && pport.m_canBeExternal);
        pport.applyConnectParams(pProps);
        applyConnectParams(uProps);
        establishRoles(provider.connectionData.data);
        finishConnection(provider.connectionData.data);
        pport.finishConnection(connectionData.data);
        return true;
      }
      // Directly connect to this port
      // which creates a dummy user port
      OC::ExternalPort &connectExternal(const char *name, const OA::PValue *userProps,
					const OA::PValue *props);
    };
    int Port::dumpFd = -1;

    // The port may be bidirectional.  If so we need to defer its direction.
    OC::Port &Worker::
    createPort(const OM::Port &metaPort, const OA::PValue *props) {
      const char *myName = metaPort.name;
      bool isProvider = metaPort.provider;
#if 0
      // Process direction overrides for the instance
      if (myInstXml()) {
	if (OU::EzXml::inList(name, ezxml_cattr(myInstXml(), "inputs")))
	  isProvider = true;
	if (OU::EzXml::inList(name, ezxml_cattr(myInstXml(), "outputs")))
	  isProvider = false;
      }
#endif
      // Find connections attached to this port
      ezxml_t conn, ic = 0, icw = 0;
      for (conn = ezxml_child(myXml()->parent, "connection"); conn; conn = ezxml_next(conn)) {
        const char
          *from = ezxml_attr(conn,"from"), // instance with user port
          *to = ezxml_attr(conn,"to"),     // instance with provider port
          *out = ezxml_attr(conn, "out"),  // user port name
          *in = ezxml_attr(conn, "in");    // provider port name
        if (from && to && out && in) {
	  bool iAmTo;
	  if (!strcmp(instTag().c_str(), to) && !strcmp(in, myName))
	    iAmTo = true;
	  else if (!strcmp(instTag().c_str(), from) && !strcmp(out, myName))
	    iAmTo = false;
	  else
	    continue;
          // We have a connection.  See if it is to an external interconnect.  FIXME i/o later
          for (ic = ezxml_child(myXml()->parent, "interconnect"); ic; ic = ezxml_next(ic)) {
            const char *icName = ezxml_attr(ic, "name");
            if (icName &&
                (iAmTo && !strcmp(icName, from) ||
                 !iAmTo && !strcmp(icName, to))) {
              // We have a connection on this port to an interconnect worker!
              // Find its details
              const char *icwName = ezxml_attr(ic, "worker");
              if (icwName)
                for (icw = ezxml_child(myXml()->parent, "worker"); icw; icw = ezxml_next(icw)) {
                  const char *nameAttr = ezxml_attr(icw, "name");
                  if (nameAttr && !strcmp(nameAttr, icwName))
                    break;
                }
              if (!icw)
                throw OC::ApiError("For port \"", myName,
                                   "\": interconnect worker missing for connection", NULL);
	      // If we are bidirectional, this external connection sets our direction
	      if (metaPort.bidirectional)
		isProvider = iAmTo;
              break; // we found an external connection
            }
          } // loop over interconnects
	  break; // we found a connection
	}
      } // loop over all connections
      return *new Port(*this, props, metaPort, conn, icw, ic, isProvider);
    }
    // Here because these depend on Port
    OC::Port &Worker::
    createOutputPort(OM::PortOrdinal portId,
                     OS::uint32_t bufferCount,
                     OS::uint32_t bufferSize,
                     const OA::PValue* props) throw() {
      (void)portId; (void)bufferCount; (void)bufferSize;(void)props;
      return *(Port *)0;//return *new Port(*this);
    }
    OC::Port &Worker::
    createInputPort(OM::PortOrdinal portId,
                    OS::uint32_t bufferCount,
                    OS::uint32_t bufferSize,
                    const OA::PValue* props) throw() {
      (void)portId; (void)bufferCount; (void)bufferSize;(void)props;
      return *(Port *)0;//      return *new Port(*this);
    }

    // Buffers directly used by the "user" (non-container/component) API
    class ExternalBuffer : OC::ExternalBuffer {
      friend class ExternalPort;
      ExternalPort *myExternalPort;
      OcdpMetadata *metadata;   // where is the metadata buffer
      uint8_t *data;            // where is the data buffer
      uint32_t length;          // length of the buffer (not message)
      volatile uint32_t *readyForLocal;  // where is the flag set by remote on data movement
      volatile uint32_t *readyForRemote; // where is ready flag for remote data movement
      bool busy;                // in use by local processing (for error checking)
      bool last;                // last buffer in the set
      void release();
      void put(uint32_t dataLength, uint8_t opCode, bool endOfData) {
	(void)endOfData;
        ocpiAssert(dataLength <= length);
        metadata->opCode = opCode;
        metadata->length = dataLength;
        release();
      }
    };

    // Producer or consumer
    class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort> {
      friend class ExternalBuffer;
      // What we know about a far buffer
      struct FarBuffer {
        // When we are active, we use these far data pointers
        volatile OcdpMetadata *metadata;
        volatile uint8_t *data;
        // We use this all the time.
        volatile uint32_t *ready;
        bool last;
      };
      //      uint32_t nBuffers, *ready, next;
      OcdpMetadata *metadata;
      uint32_t *flags;
      ExternalBuffer *localBuffers, *nextLocal, *nextRemote;
      FarBuffer *farBuffers, *nextFar;
      uint8_t *localData;
      friend class Port;

      ExternalPort(Port &port, const char *name, bool isProvider, const OA::PValue *props) :
        OC::ExternalPortBase<Port,ExternalPort>(port, name, props, port.metaPort(), isProvider)
      {
        // Default is active only (host is master, never slave)
        connectionData.data.options =
          (1 << OCPI::RDT::ActiveFlowControl) |
          (1 << OCPI::RDT::ActiveMessage) |
          (1 << OCPI::RDT::ActiveOnly);
        applyConnectParams(props);
        port.establishRoles(connectionData.data);
        unsigned nFar = parent().connectionData.data.desc.nBuffers;
        unsigned nLocal = myDesc.nBuffers;
        myDesc.dataBufferPitch = parent().connectionData.data.desc.dataBufferPitch;
        myDesc.metaDataPitch = parent().connectionData.data.desc.metaDataPitch;
        myDesc.fullFlagPitch = sizeof(uint32_t);
        myDesc.emptyFlagPitch = sizeof(uint32_t);
        myDesc.emptyFlagValue = 1;
        myDesc.fullFlagValue = 1;
        // Allocate my local memory, making everything on a nice boundary.
        // (assume empty flag pitch same as full flag pitch)
        unsigned nAlloc =
          OC::roundup(myDesc.dataBufferPitch * nLocal, LOCAL_DMA_ALIGN) +
          OC::roundup(myDesc.metaDataPitch * nLocal, LOCAL_DMA_ALIGN) +
          OC::roundup(sizeof(uint32_t) * nLocal, LOCAL_DMA_ALIGN) + // local flags
          // These might actually be remote
          OC::roundup(sizeof(uint32_t) * nLocal, LOCAL_DMA_ALIGN) + // remote flags
          // These might not be needed if we are ActiveFlowControl
          OC::roundup(sizeof(uint32_t) * nFar, LOCAL_DMA_ALIGN);
        // Now we allocate all the (local) endpoint memory
        uint8_t *allocation = 0;
        static const char *dma = getenv("OCPI_DMA_MEMORY");
        static bool done = false;  // FIXME not thread safe, and generates incorrect compiler error
        static uint64_t base, size;
        if (!done) {
          if (dma) {
            unsigned sizeM;
            ocpiCheck(sscanf(dma, "%uM$0x%llx", &sizeM,
                             (unsigned long long *) &base) == 2);
            size = (unsigned long long)sizeM * 1024 * 1024;
            fprintf(stderr, "DMA Memory:  %uM at 0x%llx\n", sizeM,
                    (unsigned long long)base);
          }
          done = true;
        }
        snprintf(myDesc.oob.oep, sizeof(myDesc.oob.oep),
                 "ocpi-pci-pio://%s.%lld:%lld.3.10", "0", (unsigned long long)base,
                 (unsigned long long)nAlloc);
        // If we are ActiveOnly we need no DMAable memory at all, so get it from the heap.
        if (connectionData.data.role == OCPI::RDT::ActiveOnly)
          allocation = new uint8_t[nAlloc];
        else {
          if (!dma)
            throw OC::ApiError("Asking for DMA without OCPI_DMA_MEMORY environment var", NULL);
          allocation = (uint8_t*)mmap(NULL, nAlloc, PROT_READ|PROT_WRITE, MAP_SHARED,
                                      Driver::pciMemFd, base);
          ocpiAssert(allocation != (uint8_t*)-1);
          base += nAlloc;
          base += getpagesize();
          base &= ~(getpagesize() - 1);
          // Get the local endpoint corresponding to the known remote endpoint
#if 0
          myEndpoint = OCPI::RDT::GetEndpoint("ocpi-pci//bus-id");
          if (!myEndpoint)
            OC::ApiError("No local (CPU) endpoint support for pci bus %s", NULL);
          allocation = myEndpoint->alloc(nAlloc);
#endif
        }

#ifdef NEEDED
        memset((void *)allocation, 0, nAlloc);
#endif

        localData = allocation;
        myDesc.dataBufferBaseAddr  = 0;
        allocation += OC::roundup(myDesc.dataBufferPitch * nLocal, LOCAL_BUFFER_ALIGN);
        metadata = (OcdpMetadata *)allocation;
        myDesc.metaDataBaseAddr = allocation - localData;
        allocation += OC::roundup(myDesc.metaDataPitch * nLocal, LOCAL_BUFFER_ALIGN);
        uint32_t *localFlags = (uint32_t*)allocation;
        allocation += OC::roundup(sizeof(uint32_t) * nLocal, LOCAL_BUFFER_ALIGN);
        uint32_t *remoteFlags = (uint32_t*)allocation;
        allocation += OC::roundup(sizeof(uint32_t) * nLocal, LOCAL_BUFFER_ALIGN);
        uint32_t *farFlags = (uint32_t*)allocation;
        switch (connectionData.data.role) {
        case OCPI::RDT::ActiveMessage:
          // my exposed addresses are the flags in my memory that indicate far buffer state
          myDesc.emptyFlagBaseAddr =
            myDesc.fullFlagBaseAddr = allocation - localData;
          // FALL THROUGH
        case OCPI::RDT::ActiveOnly:
          {
            FarBuffer *fb = nextFar = farBuffers = new FarBuffer[nFar];
            for (unsigned i = 0; i < nFar; i++, fb++) {
              fb->metadata = parent().userMetadataBaseAddr + i;
              fb->data = parent().userDataBaseAddr + i * parent().myDesc.dataBufferPitch;
              fb->ready = farFlags + i; // not used for ActiveOnly
              *fb->ready = parent().isProvider();
              fb->last = false;
            }
            (fb-1)->last = true;
          }
          break;
        case OCPI::RDT::ActiveFlowControl:
          // here the far side needs to know about the remote flags;
          myDesc.emptyFlagBaseAddr =
            myDesc.fullFlagBaseAddr = (uint8_t*)localFlags - localData;
        }

        // Initialize our structures that keep track of LOCAL buffer status
        ExternalBuffer *lb = nextLocal = nextRemote = localBuffers = new ExternalBuffer[nLocal];
        for (unsigned i = 0; i < nLocal; i++, lb++) {
          lb->myExternalPort = this;
          lb->metadata = metadata + i;
          lb->data = localData + i * myDesc.dataBufferPitch;
          lb->length = myDesc.dataBufferPitch;
          lb->last = false;
          lb->busy = false;
          lb->readyForLocal = localFlags + i;
          *lb->readyForLocal = parent().isProvider();
          lb->readyForRemote = remoteFlags + i; //&parent().myOcdpRegisters->nRemoteDone;
          *lb->readyForRemote = !parent().isProvider();
        }
        (lb-1)->last = true;
      }
    public:
      ~ExternalPort() {
	delete [] localBuffers;
      }
void memcpy64(uint64_t *to, uint64_t *from, unsigned nbytes)
{
  while (nbytes > 128) {
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    nbytes -= 128;
  }
  while (nbytes > 8) {
    *to++ = *from++;
    nbytes -= 8;
  }
  if (nbytes)
    memcpy(to, from, nbytes);
}
      // We know a move can be done.  Do it.
      // We are either ActiveOnly or ActiveMessage
      void moveData() {
        if (parent().isProvider()) {
          // Here to far
          memcpy64((uint64_t *)nextFar->metadata, (uint64_t *)nextRemote->metadata, sizeof(OcdpMetadata));
          memcpy64((uint64_t *)nextFar->data, (uint64_t *)nextRemote->data, nextRemote->metadata->length);
        } else {
          // Far to here
          memcpy64((uint64_t *)nextRemote->metadata, (uint64_t *)nextFar->metadata, sizeof(OcdpMetadata));
          memcpy64((uint64_t *)nextRemote->data, (uint64_t *)nextFar->data, nextRemote->metadata->length);
        }
        // Set the local indication of readiness of the far buffer to false.
        // Far side will make it true (or us when we are ActiveOnly).
        *nextFar->ready = false;
        //        __asm__ __volatile__  ("lock; addl $0,0(%%esp)": : :"memory");
        // FIXME:  memory barrier to be sure?
        // Tell the far side that a its buffer has been used (filled or emptied)
        //wmb();
        if (parent().myOcdpRegisters->foodFace != 0xf00dface)
          abort();
        parent().myOcdpRegisters->nRemoteDone = 1;
        // Advance our far side status
        if (nextFar->last)
          nextFar = farBuffers;
        else
          nextFar++;
        // Advance remote state (we are moving data)
        // The local buffer state has become ready for local access
        *nextRemote->readyForLocal = true;
        *nextRemote->readyForRemote = false;
        if (nextRemote->last)
          nextRemote = localBuffers;
        else
          nextRemote++;
      }
      // Try to move some data, return if there is data that can't be moved
      void tryMove() {
        // Try to advance my remote side
        switch (connectionData.data.role) {
        case OCPI::RDT::ActiveOnly:
          // Use far side "ready" register to determine whether far buffers are ready
          // Thus we need to do a remote PCIe read to know far size status
          if (*nextRemote->readyForRemote) { // avoid remote read if local is not ready
            for (uint32_t nReady = parent().myOcdpRegisters->nReady;
                 nReady && *nextRemote->readyForRemote; nReady--)
	      moveData();
          }
          break;
        case OCPI::RDT::ActiveMessage:
          // Use local version of far-is-ready flag for the far buffer,
          // which will be written by the ActiveFlowControl far side.
          while (*nextRemote->readyForRemote && *nextFar->ready)
            moveData();
          break;
        case OCPI::RDT::ActiveFlowControl:
          // Nothing to do here.  We don't move data.
          // When the other side moves data it will set our far-is-ready flag
          break;
        case OCPI::RDT::Passive:
        case OCPI::RDT::NoRole:
          ocpiAssert(0);
        }
      }
      bool getLocal() {
        tryMove();
        if (!*nextLocal->readyForLocal)
          return false;
        ocpiAssert(connectionData.data.role == OCPI::RDT::ActiveFlowControl ||
                  connectionData.data.role == OCPI::RDT::Passive ||
                  !*nextLocal->readyForRemote);
        ocpiAssert(!nextLocal->busy);
        nextLocal->busy = true; // to ensure callers use the API correctly
        *nextLocal->readyForLocal = 0;
        return true;
      }
      // The input method = get a buffer that has data in it.
      OA::ExternalBuffer *
      getBuffer(uint8_t *&bdata, uint32_t &length, uint8_t &opCode, bool &end) {
        ocpiAssert(!parent().isProvider());
        if (!getLocal())
          return 0;
        bdata = nextLocal->data;
        length = nextLocal->metadata->length;
        opCode = nextLocal->metadata->opCode;
        end = false; // someday bit in metadata
        //FIXME cast unnecessary
        return static_cast<OC::ExternalBuffer *>(nextLocal);
      }
      OC::ExternalBuffer *getBuffer(uint8_t *&bdata, uint32_t &length) {
        ocpiAssert(parent().isProvider());
        if (!getLocal())
          return 0;
        bdata = nextLocal->data;
        length = nextLocal->length;
        return static_cast<OC::ExternalBuffer *>(nextLocal);
      }
      void endOfData() {
        ocpiAssert(parent().isProvider());
      }
      bool tryFlush() {
        ocpiAssert(parent().isProvider());
        tryMove();
        switch (connectionData.data.role) {
        case OCPI::RDT::ActiveOnly:
        case OCPI::RDT::ActiveMessage:
	  return *nextRemote->readyForRemote != 0;
        case OCPI::RDT::ActiveFlowControl:
	  {
	    ExternalBuffer *local = nextLocal; 
	    do {
	      if (local == localBuffers)
		local = localBuffers + myDesc.nBuffers - 1;
	      else
		local--;
	      if (!*local->readyForLocal)
		return true;
	    } while (local != nextLocal);
	  }
	  break;
        case OCPI::RDT::Passive:
        case OCPI::RDT::NoRole:
          ocpiAssert(0);
	}
	return false;
      }
      void advanceLocal() {
        if (connectionData.data.role == OCPI::RDT::ActiveFlowControl) {
          //          if (parent().myOcdpRegisters->foodFace != 0xf00dface)
          //            abort();
          //          wmb();
          parent().myOcdpRegisters->nRemoteDone = 1;
          //usleep(0);
        }
        if (nextLocal->last)
          nextLocal = localBuffers;
        else
          nextLocal++;
        tryMove();
      }
    };

    // FIXME make readyForRemote zero when active flow control
    void ExternalBuffer::release() {
      ocpiAssert(myExternalPort->connectionData.data.role == OCPI::RDT::ActiveFlowControl ||
                myExternalPort->connectionData.data.role == OCPI::RDT::Passive ||
                !*readyForRemote);
      // The buffer is not ready for local processing
      //      *readyForLocal = false;
      //      clflush(readyForLocal);
      // The remote process can use this local buffer now
      *readyForRemote = true;
      // The local buffer is not being used locally
      busy = false;
      // Tell the other side that the buffer has become available
      myExternalPort->advanceLocal();
    }
    OC::ExternalPort &Port::connectExternal(const char *extName, const OA::PValue *userProps,
					    const OA::PValue *props) {
      if (!m_canBeExternal)
        throw OC::ApiError ("For external port \"", extName, "\", port \"",
			    name().c_str(), "\" of worker \"",
			    parent().implTag().c_str(), "/", parent().instTag().c_str(), "/",
			    parent().name().c_str(),
			    "\" is locally connected in the HDL bitstream. ", NULL);
      applyConnectParams(props);
      // UserPort constructor must know the roles.
      ExternalPort *myExternalPort = new ExternalPort(*this, extName, !isProvider(), userProps);
      finishConnection(myExternalPort->connectionData.data);
      return *myExternalPort;
    }
    int Driver::pciMemFd = -1;
  }
}

