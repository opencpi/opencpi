/**
  @brief
  This file contains the implementation for the RPL container for the OC FPGA
  Reference Platformm
  FIXME: we need to abstract the RPL aspects from the OCRP FPGA aspects.
  It implements the CPI::RPL::Container class, which implements the
  CPI::Container::Interface class.
  There is no separate header file for this class since its only purpose
  is to implement the CPI::Container::Interface and no one else will use it.

  Revision History:

    5/6/2009 - Jim Kulp
    Initial version.

************************************************************************** */
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include "ezxml.h"
#include <CpiOsMisc.h>
#include "PciScanner.h"
#include "CpiContainerInterface.h"
#include "CpiWorker.h"
#include "CpiApplication.h"
#include "CpiArtifact.h"
#include "CpiProperty.h"
#include "CpiContainerPort.h"
#include "CpiPValue.h"
#include "CpiDriver.h"
#include "CpiContainerMisc.h"
#include "OCCP.h"
#include "OCDP.h"

#define wmb()        asm volatile("sfence" ::: "memory"); usleep(0)
#define clflush(p) asm volatile("clflush %0" : "+m" (*(char *)(p))) //(*(volatile char __force *)p))

namespace CPI {
  namespace Container{}
  namespace RPL {
    namespace CC = CPI::Container;
    namespace CO = CPI::OS;
    namespace CM = CPI::Metadata;
    namespace CU = CPI::Util;

    static inline unsigned max(unsigned a,unsigned b) { return a > b ? a : b;}
    // This is the alignment constraint of DMA buffers in the processor's memory.
    // It could be a cache line or a malloc granule...
    // It should come from somewhere else.  FIXME
    static const unsigned LOCAL_BUFFER_ALIGN = 32;
    // This is the constraint based both on the local issues and the OCDP's DMA engine.
    static const unsigned LOCAL_DMA_ALIGN = max(LOCAL_BUFFER_ALIGN, OCDP_FAR_BUFFER_ALIGN);
    class ExternalPort;
    class Driver : public CU::Driver {

      friend class ExternalPort; // for pcimemfd
      // The fd for mapped memory, until we have a driver to restrict it.
      static int pciMemFd;

      // Create a container
      CC::Interface *create(const char *name, PCI::Bar *bars, unsigned nbars,
                            const char *&err);

      // Create a dummy container using shm_open for the device
      CC::Interface *createDummy(const char *name, const char *df, const CU::PValue *);

      // Callback function for PciScanner::search which creates a container
      static bool found(const char *name, PCI::Bar *bars, unsigned nbars);
    public:

      // This constructor simply registers itself. This class has no state.
      Driver () :
        CU::Driver("OCFRP", "Global", true) {
      }

      // This driver method is called when container-discovery happens, to see if there
      // are any container devices supported by this driver
      // It uses a generic PCI scanner to find candidates, and when found, calls the
      // "found" method.
      virtual unsigned search(const CPI::Util::PValue*, const char **exclude)
        throw (CPI::Util::EmbeddedException)
      {
        const char *df = getenv("CPI_OCFRP_DUMMY");
        if (df) {
          createDummy("0000:99:00.0", df, 0);
          return 1;
        } else {
          unsigned n = 0;
          const char *err = PCI::search(exclude, OCFRP0_VENDOR, OCFRP0_DEVICE,
                                        OCFRP0_CLASS, OCFRP0_SUBCLASS, found, n);
          if (err)
            fprintf(stderr, "PCI Scanner Error: %s\n", err);
          return n;
        }
      }


      virtual CU::Device *probe(const CU::PValue *props, const char *which )
        throw (CPI::Util::EmbeddedException)
      {
        const char *df = getenv("CPI_OCFRP_DUMMY");
        if (df)
          return createDummy(which, df, props);
        // Real probe
        PCI::Bar bars[2];
        unsigned nbars = 2;
        const char *err = 0;
        if (!PCI::probe(which, OCFRP0_VENDOR, OCFRP0_DEVICE,
                        OCFRP0_CLASS, OCFRP0_SUBCLASS, bars, nbars, err) || err)
          CC::ApiError("Error probing \"", which, "\": ", err, NULL);
        CC::Interface *c = create(which, bars, nbars, err);
        if (!c)
          CC::ApiError("Error creating \"", which, "\" (which probed ok): ", err, NULL);
        return c;
      }
    } driver;


    bool Driver::found(const char *name, PCI::Bar *bars, unsigned nbars) {
      const char *err = 0;
      CC::Interface *c = driver.create(name, bars, nbars, err);
      if (c)
        return true;
      fprintf(stderr, "Error during probe for OCFRP: %s\n", err);
      return false;
    }
    class Container : public CC::Interface {
      volatile OccpAdminRegisters *adminRegisters;
      volatile OccpSpace* occp;
      uint8_t *bar1Vaddr;
      uint64_t basePaddr, bar1Offset;
      uint8_t *baseVaddr;     // base virtual address of whole region containing both BARs
      uint64_t endPointSize; // size in bytes of whole region containing both BARs.
      friend class WciControl;
      friend class Driver;
      friend class Port;
    protected:
      Container(CU::Driver &driver, const char *name, uint64_t bar0Paddr,
                volatile OccpSpace *occp, uint64_t bar1Paddr, uint8_t *bar1Vaddr,
                uint32_t bar1Size)  :
        CC::Interface(driver, name), occp(occp), bar1Vaddr(bar1Vaddr)
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
      }
      ~Container() throw() {
        // ref count driver static resources, like closing pciMemFd
      }
      // friends
      void getWorkerAccess(unsigned index, volatile OccpWorkerRegisters *&r,
                           volatile uint8_t *&c)
      {
        if (index >= OCCP_MAX_WORKERS)
          throw CC::ApiError("Invalid occpIndex property", 0);
        // check this against something in the admin registers
        c = occp->config[index];
        r = &occp->worker[index].control;
      }
      void releaseWorkerAccess(unsigned index)
      {
        // potential unmapping/ref counting
      }
      // support worker ids for those who want it
      CC::Worker &findWorker(CC::WorkerId)
      {
        static CC::Worker *w; return *w;
      }
      CC::Artifact &
      createArtifact(const char *url, CU::PValue *artifactParams);
#if 0
    public:
      CC::Application *
      createApplication() throw();
      // impl below depends on Application


    // Container methods that depend on Application
    CC::Application *
    Container::createApplication()
      throw ( CPI::Util::EmbeddedException )
    {

    }
#endif


    };

    // Create a dummy device emulated by a shared memory buffer
    CC::Interface *Driver::
    createDummy(const char *name, const char *df, const CU::PValue *) {
      int fd;
      uint8_t *bar0, *bar1;
      fprintf(stderr, "DF: %s, Page %d, Occp %ld, SC pagesize %ld off_t %ld bd %ld\n",
              df, getpagesize(), sizeof(OccpSpace), sysconf(_SC_PAGE_SIZE),
              sizeof(off_t), sizeof(CC::PortData));
      umask(0);
      cpiAssert((fd = shm_open(df, O_CREAT | O_RDWR, 0666)) >= 0);
      cpiAssert(ftruncate(fd, sizeof(OccpSpace) + 64*1024) >= 0);
      cpiAssert((bar0 = (uint8_t*)mmap(NULL, sizeof(OccpSpace) + 64*1024,
                                       PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0))
                != (uint8_t*)-1);
      bar1 = bar0 + sizeof(OccpSpace);
      return new Container(*this, name, 0, (volatile OccpSpace*)bar0,
                           sizeof(OccpSpace), bar1, 64*1024);
    }

    // Internal driver method
    // Return container pointer OR null, and if NULL, set "err" output arg.
    CC::Interface *Driver::
    create(const char *name, PCI::Bar *bars, unsigned nbars, const char *&err) {
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
          static union { char string[5]; uint32_t value; }
//          magic1 = {{'n', 'e', 'p', 'O'}}, magic2 = {{0, 'I', 'P', 'C'}};
          magic1 = {{'O', 'p', 'e', 'n'}}, magic2 = {{'C', 'P', 'I', '\0'}};
          //          magic1 = {OCCP_MAGIC1}, magic2 = {OCCP_MAGIC2};
          if (occp->admin.magic1 != magic1.value || occp->admin.magic2 != magic2.value) {
            err = "Magic numbers do not match in region/bar 0";
            fprintf(stderr, "PCI Device matches OCFRP vendor/device, but not OCCP signature: "
                    "magic1: 0x%x (sb 0x%x), magic2: 0x%x (sb 0x%x)",
                    occp->admin.magic1, magic1.value, occp->admin.magic2,
                    magic2.value);
          } else {
            char tbuf[30];
            time_t bd = occp->admin.birthday;
            fprintf(stderr, "OCFRP: %s, with bitstream birthday: %s", name, ctime_r(&bd, tbuf));
            return new Container(driver, name, bars[0].address, occp, bars[1].address,
                                         (uint8_t*)bar1, bars[1].size);
          }
        }
        if (bar0)
          munmap(bar0, sizeof(OccpSpace));
        if (bar1)
          munmap(bar1, bars[1].size);
      }
      return 0;
    }
#if 0
    class Application :
      public CC::Application
    {
      friend class Container;
      Application(Container &container) : CC::Application(container)
      {
      }
    };
    // Container methods that depend on Application
    CC::Application *
    Container::createApplication() throw()
    {
      return new Application(*this);
    }
#endif


    class Artifact :
      public CC::Artifact
    {
      friend class Container;
      Artifact(CC::Interface &c, const char *url, CU::PValue *artifactParams) :
        CC::Artifact(c, url) {
        /* load bitstream here or on first worker creation or
           on first worker init/start */
      }
      ~Artifact() {
      }
      virtual CC::Worker &
      createWorkerX(CC::Application &a, ezxml_t impl, ezxml_t inst, CU::PValue *execProps);
    };

    CC::Artifact & Container::
    createArtifact(const char *url, CU::PValue *artifactParams)
    {
      return *new Artifact(*this, url, artifactParams);
    }
    // The class that knows about WCI interfaces and the OCCP.
    class WciControl {
      const char *implName, *instName;
    protected:
      Container &myWciContainer;
      // myRegisters is zero when this WCI does not really exist.
      // (since we inherit this in some cases were it is not needed).
      volatile OccpWorkerRegisters *myRegisters;
      uint32_t controlMask;
      unsigned myOccpIndex;
      volatile uint8_t *myProperties;
      CM::Worker::ControlState myState;
      WciControl(Container &container, ezxml_t implXml, ezxml_t instXml) :
        implName(0), instName(0), myWciContainer(container), myRegisters(0),
        myState(CM::Worker::EXISTS)  {
        if (!implXml)
          return;
        implName = ezxml_attr(implXml, "name");
        instName = ezxml_attr(instXml, "name");
        const char *ops = ezxml_attr(implXml, "controlOperations");
#define CONTROL_OP(x, c, t, s1, s2, s3) \
        if (ops && strstr(ops, #x)) controlMask |= 1 << CM::Worker::Op##c;
        CPI_CONTROL_OPS
#undef CONTROL_OP
        controlMask |= 1 << CM::Worker::OpStart;
        myOccpIndex = CC::getAttrNum(instXml, "occpIndex");
        uint32_t timeout = CC::getAttrNum(implXml, "timeout", true);
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
        cpiAssert(bad == 0);
#endif
        // Take out of reset
        myRegisters->control = OCCP_CONTROL_ENABLE | logTimeout ;
        if (getenv("CPI_OCFRP_DUMMY")) {
          *(uint32_t *)&myRegisters->initialize = OCCP_SUCCESS_RESULT; //fakeout
          *(uint32_t *)&myRegisters->start = OCCP_SUCCESS_RESULT; //fakeout
        }
        // Maybe this should be delayed until construction of derived class is done? FIXME?
        initialize();
      }
      virtual ~WciControl() {
        if (myRegisters)
          myWciContainer.releaseWorkerAccess(myOccpIndex);
      }
      // Add the hardware considerations to the property object that supports
      // fast memory-mapped property access directly to users
      // the key members are "readVaddr" and "writeVaddr"
      virtual void prepareProperty(CM::Property &md, CC::Property &cp) {
        if (myRegisters)
          if (!md.is_struct && !md.is_sequence && !md.types->type != CM::Property::CPI_String &&
              CM::Property::tsize[md.types->type] <= 32 &&
              !md.write_error)
            cp.writeVaddr = myProperties + md.offset;
      }
      void checkResult(uint32_t result) {
        const char *oops;
        switch (result) {
        case OCCP_TIMEOUT_RESULT:
          oops = "worker timed out performing control operation";
          break;
        case OCCP_ERROR_RESULT:
          oops = "worker indicated an error from control operation";
          break;
        case OCCP_RESET_RESULT:
          oops = "worker was in a reset state when control operation was requested";
          break;
        case OCCP_FATAL_RESULT:
          oops = "worker indicated a fatal error from control operation";
          break;
        default:
          oops = "unknown result value from control operation";
        }
        throw CC::ApiError("Worker \"", implName, ":", instName, "\"", oops, 0);
      }
      // Should check for illegal sequences
#define CONTROL_OP(x, c, t, s1, s2, s3)                                \
      virtual void x() {                                \
        if (myState == CM::Worker::s1 ||                        \
            (CM::Worker::s2 != CM::Worker::NONE && myState == CM::Worker::s2) || \
            (CM::Worker::s3 != CM::Worker::NONE && myState == CM::Worker::s3)) { \
          if (controlMask & (1 << CM::Worker::Op##c)) {                        \
            uint32_t result = myRegisters->x;                        \
            if (result != OCCP_SUCCESS_RESULT)                        \
              checkResult(result);                                \
          }                                                        \
          myState = CM::Worker::t;                                \
        } else                                                        \
          throw CC::ApiError("Illegal control state for operation",0); \
      }
      CPI_CONTROL_OPS
#undef CONTROL_OP
    };
    class Worker : public WciControl, public CC::Worker {
      friend class Artifact;
      friend class Port;
      Container &myRplContainer;
      Worker(CC::Application &app, Container &container,
             ezxml_t implXml, ezxml_t instXml,
             CU::PValue* execProps) :
        WciControl(container, implXml, instXml),
        CC::Worker(app, implXml, instXml),
        myRplContainer(container)
      {
      }
      ~Worker()
      {
      }


      WCI_error control(WCI_control, WCI_options){return WCI_SUCCESS;}
      WCI_error status(WCI_status*){return WCI_SUCCESS;}
      WCI_error read(WCI_u32, WCI_u32, WCI_data_type, WCI_options, void*){return WCI_SUCCESS;}
      WCI_error write(WCI_u32, WCI_u32, WCI_data_type, WCI_options, const void*){return WCI_SUCCESS;}
      WCI_error close(WCI_options){return WCI_SUCCESS;}
      std::string getLastControlError()
        throw (CPI::Util::EmbeddedException){std::string s("No Error");return s;}



      CC::Port & createPort(CM::Port &metaport);
      // FIXME: why is this neessary?  Why isn't the inheritance of WciControl enough to satisfy this?
#define CONTROL_OP(x, c, t, s1, s2, s3)                                \
      virtual void x() { WciControl::x(); }
      CPI_CONTROL_OPS
#undef CONTROL_OP
      // FIXME: why is this neessary?  Why isn't the inheritance of WciControl enough to satisfy this?
      virtual void prepareProperty(CM::Property &mp, CC::Property &cp) {
        return WciControl::prepareProperty(mp, cp);
      }

      CC::Port &
      createOutputPort(CC::PortId portId,
                       CPI::OS::uint32_t bufferCount,
                       CPI::OS::uint32_t bufferSize,
                       CU::PValue* props) throw();
      CC::Port &
      createInputPort(CC::PortId portId,
                      CPI::OS::uint32_t bufferCount,
                      CPI::OS::uint32_t bufferSize,
                      CU::PValue* props) throw();

      // These property access methods are called when the fast path
      // is not enabled, either due to no MMIO or that the property can
      // return errors.  OCCP has MMIO, so it must be the latter
#undef CPI_DATA_TYPE_S
      // Set a scalar property value
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
      void set##pretty##Property(unsigned ord, const run val) {                \
        CM::Property &p = property(ord);                                \
        if (!p.is_writable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (p.write_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before write */                        \
        store *pp = (store *)(myProperties + p.offset);                        \
        if (bits > 32) {                                                \
          assert(bits == 64);                                                \
          uint32_t *p32 = (uint32_t *)pp;                                \
          p32[1] = ((uint32_t *)&val)[1];                                \
          p32[0] = ((uint32_t *)&val)[0];                                \
        } else                                                                \
          *pp = *(store *)&val;                                                \
        if (p.write_error && myRegisters->status & OCCP_STATUS_ALL_ERRORS) \
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      void set##pretty##SequenceProperty(unsigned ord,const run *vals, unsigned length) { \
        CM::Property &p = property(ord);                                \
        assert(p.types->type == CM::Property::CPI_##pretty);                \
        if (!p.is_writable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (length > p.sequence_size)                                        \
          throw;                                                        \
        if (p.write_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before write */                        \
        memcpy((void *)(myProperties + p.offset + p.maxAlign), vals, length * sizeof(run)); \
        *(uint32_t *)(myProperties + p.offset) = length;                \
        if (p.write_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after write */                        \
      }
      // Set a string property value
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
      virtual void set##pretty##Property(unsigned ord, const run val) {        \
        CM::Property &p = property(ord);                                \
        assert(p.types->type == CM::Property::CPI_##pretty);                \
        if (!p.is_writable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        unsigned cpi_length;                                                \
        if (!val || (cpi_length = strlen(val)) > p.types->size)                \
          throw; /*"string property too long"*/;                        \
        if (p.write_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before write */                        \
        uint32_t *p32 = (uint32_t *)(myProperties + p.offset);                \
        /* if length to be written is more than 32 bits */                \
        if (++cpi_length > 32/CHAR_BIT)                                        \
          memcpy(p32 + 1, val + 32/CHAR_BIT, cpi_length - 32/CHAR_BIT); \
        uint32_t i;                                                        \
        memcpy(&i, val, 32/CHAR_BIT);                                        \
        p32[0] = i;                                                        \
        if (p.write_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      void set##pretty##SequenceProperty(unsigned ord,const run *vals, unsigned length) { \
        CM::Property &p = property(ord);                                \
        assert(p.types->type == CM::Property::CPI_##pretty);                \
        if (!p.is_writable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (length > p.sequence_size)                                        \
          throw;                                                        \
        if (p.write_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before write */                        \
        char *cp = (char *)(myProperties + p.offset + 32/CHAR_BIT);        \
        for (unsigned i = 0; i < length; i++) {                                \
          unsigned len = strlen(vals[i]);                                \
          if (len > p.types->size)                                        \
            throw; /* "string in sequence too long" */                        \
          memcpy(cp, vals[i], len+1);                                        \
        }                                                                \
        *(uint32_t *)(myProperties + p.offset) = length;                \
        if (p.write_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after write */                        \
      }
      CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE_S
#undef CPI_DATA_TYPE
      // Get Scalar Property
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
      virtual run get##pretty##Property(unsigned ord) {                        \
        CM::Property &p = property(ord);                                \
        if (!p.is_readable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (p.read_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before read "*/                        \
        uint32_t *pp = (uint32_t *)(myProperties + p.offset);                \
        union {                                                                \
                run r;                                                        \
                uint32_t u32[bits/32];                                        \
        } u;                                                                \
        if (bits > 32)                                                        \
          u.u32[1] = pp[1];                                                \
        u.u32[0] = pp[0];                                                \
        if (p.read_error && myRegisters->status & OCCP_STATUS_ALL_ERRORS) \
          throw; /*"worker has errors after read */                        \
        return u.r;                                                        \
      }                                                                        \
      unsigned get##pretty##SequenceProperty(unsigned ord, run *vals, unsigned length) { \
        CM::Property &p = property(ord);                                \
        if (!p.is_readable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (p.read_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before read "*/                        \
        uint32_t n = *(uint32_t *)(myProperties + p.offset);                \
        if (n > length)                                                        \
          throw; /* sequence longer than provided buffer */                \
        memcpy(vals, (void*)(myProperties + p.offset + p.maxAlign),        \
               n * sizeof(run));                                        \
        if (p.read_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after read */                        \
        return n;                                                        \
      }

      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
      virtual void get##pretty##Property(unsigned ord, char *cp, unsigned length) { \
        CM::Property &p = property(ord);                                \
        assert(p.types->type == CM::Property::CPI_##pretty);                \
        if (!p.is_readable)                                                \
          throw; /*"attempt to set property that is not writable" */        \
        if (length < p.types->size+1)                                        \
          throw; /*"string buffer smaller than property"*/;                \
        if (p.read_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before write */                        \
        uint32_t i32, *p32 = (uint32_t *)(myProperties + p.offset);        \
        memcpy(cp + 32/CHAR_BIT, p32 + 1, p.types->size + 1 - 32/CHAR_BIT); \
        i32 = *p32;                                                        \
        memcpy(cp, &i32, 32/CHAR_BIT);                                        \
        if (p.read_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      unsigned get##pretty##SequenceProperty                                \
      (unsigned ord, run *vals, unsigned length, char *buf, unsigned space) { \
        CM::Property &p = property(ord);                                \
        assert(p.types->type == CM::Property::CPI_##pretty);                \
        if (!p.is_readable)                                                \
          throw; /*"attempt to get property that is not readable" */        \
        if (length > p.sequence_size)                                        \
          throw;                                                        \
        if (p.read_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors before read */                        \
        uint32_t                                                        \
          n = *(uint32_t *)(myProperties + p.offset),                        \
          wlen = p.types->size + 1;                                        \
        if (n > length)                                                        \
          throw; /* sequence longer than provided buffer */                \
        char *cp = (char *)(myProperties + p.offset + 32/CHAR_BIT);        \
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
        if (p.read_error &&                                                \
            myRegisters->status & OCCP_STATUS_ALL_ERRORS)                \
          throw; /*"worker has errors after read */                        \
        return n;                                                        \
      }
      CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE_S
#undef CPI_DATA_TYPE
#define CPI_DATA_TYPE_S CPI_DATA_TYPE
    };
    // Internal constructor after artifact's XML metadata processing
    CC::Worker & Artifact::
    createWorkerX(CC::Application &app, ezxml_t impl, ezxml_t inst,
                  CU::PValue *execProps) {
      return *new Worker(app, static_cast<Container&>(*app.myParent),
                         impl, inst, execProps);
    }
    // This port class really has two cases: externally connected ports and
    // internally connected ports.
    // Also ports are either user or provider.
    // So this class takes care of all 4 cases, since the differences are so
    // minor as to not be worth (re)factoring (currently).
    class Port : public CC::Port, WciControl {
      friend class Worker;
      friend class ExternalPort;
      ezxml_t myConnection;
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
        throw ( CPI::Util::EmbeddedException )
      {
        throw CPI::Util::EmbeddedException("disconnect not yet implemented !!");
      }

      // Called after connection PValues have been set, which is after our constructor
      // userDataBaseAddr, dataBufferBaseAddr are assumed set
      // Also error-check for bad combinations or values of parameters
      // FIXME:  we are relying on dataBufferBaseAddr being set before we know
      // buffer sizes etc.  If we are sharing a memory pool, this will not be the case,
      // and we would probably allocate the whole thing here.
      void checkConnectParams() {
        if (!canBeExternal)
          return;
        if (myDesc.nBuffers *
            (CC::roundup(myDesc.dataBufferSize, OCDP_LOCAL_BUFFER_ALIGN) + OCDP_METADATA_SIZE) > myOcdpSize)
          throw CC::ApiError("Requested buffer count and size won't fit in the OCDP's memory", 0);
        myDesc.dataBufferPitch = CC::roundup(myDesc.dataBufferSize, OCDP_LOCAL_BUFFER_ALIGN);
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
      void loopback(CU::PValue *pProps, CC::Port &uPort, CU::PValue *uProps) {
        cpiAssert(isProvider());
        cpiAssert(! uPort.isProvider());
        // default to something useful
        connectionData.data.role = CPI::RDT::Passive;
        uPort.connectionData.data.role = CPI::RDT::Passive;
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
            cpiAssert(myOcdpRegisters->nReady != 0);
            myOcdpRegisters->nReady--;
            myOcdpRegisters->nRemoteDone = 0;
            copyCount++;
          }
          if (other.myOcdpRegisters->nRemoteDone != 0) {
            cpiAssert(other.myOcdpRegisters->nReady != 0);
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
           CM::Port &mPort, // the port metadata from when the worker was created
           ezxml_t connXml, // the xml connection for this port
           ezxml_t icwXml,  // the xml interconnect/infrastructure worker attached to this port if any
           ezxml_t icXml) : // the xml interconnect instance attached to this port if any
        CC::Port(w, mPort),
        WciControl(w.myRplContainer, icwXml, icXml),
        myConnection(connXml),
        myOcdpRegisters((volatile OcdpProperties *)myProperties),
        userConnected(false)
      {
        if (!icXml) {
          canBeExternal = false;
          return;
        }
        canBeExternal = true;
        // This will eventually be in the IP:  FIXME
        uint32_t myOcdpOffset = CC::getAttrNum(icXml, "ocdpOffset");
        myOcdpSize = myOcdpRegisters->memoryBytes;
        // myOcdpSize = CC::getAttrNum(icXml, "ocdpSize");
        //FIXME:  extra level of port metadata from worker, to impl, to instance?
        // get buffer sizes from port from somewhere.
        // from port xml? myMinBuffersize = getAttrNum(icXml, "minBufferSize", true);
        // from port xml? myMinNumBuffers = getAttrNum(icXml, "minNumBuffers", true);
        // Fill in the transport information with defaults.
        // It will be updated at connect time.
        // FIXME: do we need to assert a preference here?
        connectionData.data.role = CPI::RDT::NoRole;
        connectionData.data.options =
          (1 << CPI::RDT::Passive) |
          (1 << CPI::RDT::ActiveFlowControl) |
          (1 << CPI::RDT::ActiveMessage);
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
        myDesc.dataBufferBaseAddr = w.myRplContainer.bar1Offset + myOcdpOffset;

#ifdef PORT_COMPLETE
        myDesc.oob.pid = (uint64_t)(CC::Port *)this;
        myDesc.oob.cid = 0;
#endif

        snprintf(myDesc.oob.oep, sizeof(myDesc.oob.oep),
                 "cpi-pci-pio://%s.%lld:%lld.3.10", busId,
                 (long long unsigned)w.myRplContainer.basePaddr,
                 (long long unsigned)w.myRplContainer.endPointSize);
        if ( isProvider()) {
          // CONSUMER
          connectionData.data.type = CPI::RDT::ConsumerDescT;
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
          connectionData.data.type = CPI::RDT::ProducerDescT;
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
        // Set this provisionally, for debug mostly.
        checkConnectParams();
        const char *df = getenv("CPI_DUMP_PORTS");
        if (df) {
          if (dumpFd < 0)
            cpiAssert((dumpFd = creat(df, 0666)) >= 0);
          CC::PortData *pd = this;
          cpiAssert(::write(dumpFd, (void *)pd, sizeof(*pd)) == sizeof(*pd));
        }
        if (getenv("CPI_OCFRP_DUMMY"))
          *(uint32_t*)&myOcdpRegisters->foodFace = 0xf00dface;
      }
      // All the info is in.  Do final work to (locally) establish the connection
      void finishConnection(CPI::RDT::Descriptors &other) {
        // Here is where we can setup the OCDP producer/user
        cpiAssert(myOcdpRegisters->foodFace == 0xf00dface);
        myOcdpRegisters->nLocalBuffers = myDesc.nBuffers;
        myOcdpRegisters->localBufferSize = myDesc.dataBufferPitch;
        myOcdpRegisters->localBufferBase = 0;
        myOcdpRegisters->localMetadataBase = myOcdpSize - myDesc.nBuffers * OCDP_METADATA_SIZE;
        OcdpRole myOcdpRole;
        CPI::RDT::PortRole myRole = (CPI::RDT::PortRole)connectionData.data.role;
        // FIXME - can't we avoid string processing here?
        unsigned busId;
        uint64_t busAddress, busSize;
        if (sscanf(other.desc.oob.oep, "cpi-pci-pio://%x.%lld:%lld.3.10", &busId,
                   (long long unsigned *)&busAddress,
                   (long long unsigned *)&busSize) != 3)
          throw CC::ApiError("other port's endpoint description wrong: \"",
                             other.desc.oob.oep, "\"", NULL);



	printf("\n\n\n base = %lld, offset = %lld, RFB = %lld  \n\n\n",  (uint64_t)busAddress, 
         (isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr ),
	  (uint64_t)(busAddress +
		     (isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr )) );

	       printf("Other ep = %s\n", other.desc.oob.oep );



        switch (myRole) {
        case CPI::RDT::ActiveFlowControl:
          myOcdpRole = OCDP_ACTIVE_FLOWCONTROL;
          myOcdpRegisters->remoteFlagBase = busAddress +
            (isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr);
          myOcdpRegisters->remoteFlagPitch =
            (isProvider() ?
             other.desc.emptyFlagPitch : other.desc.fullFlagPitch);
          break;
        case CPI::RDT::ActiveMessage:
          myOcdpRole = OCDP_ACTIVE_MESSAGE;
          myOcdpRegisters->remoteBufferBase = busAddress + other.desc.dataBufferBaseAddr;
          myOcdpRegisters->remoteMetadataBase = busAddress + other.desc.metaDataBaseAddr;
          if ( isProvider()) {
            if (other.desc.dataBufferSize > myDesc.dataBufferSize)
              throw CC::ApiError("At consumer, remote buffer size is larger than mine", NULL);
          } else if (other.desc.dataBufferSize < myDesc.dataBufferSize) {
            throw CC::ApiError("At producer, remote buffer size smaller than mine", NULL);
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
        case CPI::RDT::Passive:
          myOcdpRole = OCDP_PASSIVE;
          break;
        default:
          cpiAssert(0);
        }
        myOcdpRegisters->control =
          OCDP_CONTROL(isProvider() ? OCDP_CONTROL_CONSUMER : OCDP_CONTROL_PRODUCER,
                       myOcdpRole);
        // We're all set, enable the adapter.
        //        initialize(); // temp FIXME
        start();
      }
      // Connection between two ports inside this container
      // We know they must be in the same artifact, and have a metadata-defined connection
      void connectInside(CC::Port &provider, CU::PValue *myProps, CU::PValue *otherProps) {
        cpiAssert(myParent->myParent == provider.myParent->myParent);
        // We're both in the same runtime artifact object, so we know the port class
        Port &pport = static_cast<Port&>(provider);
        if (myConnection != pport.myConnection)
          throw CC::ApiError("Ports are both local in bitstream/artifact, but are not connected", 0);
        pport.applyConnectParams(otherProps);
        applyConnectParams(myProps);
        // For now we assume there is nothing to actually adjust in the bitstream.
      }
      // Connect to a port in a like container (same driver)
      bool connectLike(CU::PValue *uProps, CC::Port &provider, CU::PValue *pProps) {
        cpiAssert(myParent->myParent->myParent->myParent == provider.myParent->myParent->myParent->myParent);
        // We're both in the same runtime artifact object, so we know the port class
        Port &pport = static_cast<Port&>(provider);
        cpiAssert(canBeExternal && pport.canBeExternal);
        pport.applyConnectParams(pProps);
        applyConnectParams(uProps);
        establishRoles(provider.connectionData.data);
        finishConnection(provider.connectionData.data);
        pport.finishConnection(connectionData.data);
        return true;
      }
      // Directly connect to this port
      // which creates a dummy user port
      CC::ExternalPort &connectExternal(const char *name, CU::PValue *userProps, CU::PValue *props);
    };
    int Port::dumpFd = -1;
    // CPI API
    CC::Port &Worker::
    createPort(CM::Port &metaPort) {
      bool isProvider = metaPort.provider;
      const char *name = metaPort.name;
      // Find connections attached to this port
      ezxml_t conn, ic = 0, icw = 0;
      for (conn = ezxml_child(myXml->parent, "connection"); conn; conn = ezxml_next(conn)) {
        const char
          *from = ezxml_attr(conn,"from"), // instance with user port
          *to = ezxml_attr(conn,"to"),     // instance with provider port
          *out = ezxml_attr(conn, "out"),  // user port name
          *in = ezxml_attr(conn, "in");    // provider port name
        if (from && to && out && in &&
            (isProvider && !strcmp(myInstTag, to) && !strcmp(in, name) ||
             !isProvider && !strcmp(myInstTag, from) && !strcmp(out, name))) {
          // We have a connection.  See if it is to an external interconnect.  FIXME i/o later
          for (ic = ezxml_child(myXml->parent, "interconnect"); ic; ic = ezxml_next(ic)) {
            const char *icName = ezxml_attr(ic, "name");
            if (icName &&
                (isProvider && !strcmp(icName, from) ||
                 !isProvider && !strcmp(icName, to))) {
              // We have a connection on this port to an interconnect worker!
              // Find its details
              const char *icwName = ezxml_attr(ic, "worker");
              if (icwName)
                for (icw = ezxml_child(myXml->parent, "worker"); icw; icw = ezxml_next(icw)) {
                  const char *nameAttr = ezxml_attr(icw, "name");
                  if (nameAttr && !strcmp(nameAttr, icwName))
                    break;
                }
              if (!icw)
                throw CC::ApiError("For port \"", name,
                                   "\": interconnect worker missing for connection", NULL);
              break; // we found an external connection
            }
          }
          break; // we found a connection
        }
      }
      return *new Port(*this, metaPort, conn, icw, ic);
    }
    // Here because these depend on Port
    CC::Port &Worker::
    createOutputPort(CC::PortId portId,
                     CPI::OS::uint32_t bufferCount,
                     CPI::OS::uint32_t bufferSize,
                     CU::PValue* props) throw() {
      return *(Port *)0;//return *new Port(*this);
    }
    CC::Port &Worker::
    createInputPort(CC::PortId portId,
                    CPI::OS::uint32_t bufferCount,
                    CPI::OS::uint32_t bufferSize,
                    CU::PValue* props) throw() {
      return *(Port *)0;//      return *new Port(*this);
    }



    // Buffers directly used by the "user" (non-container/component) API
    class ExternalBuffer : CC::ExternalBuffer {

      ExternalBuffer(CC::ExternalPort*p)
        : CC::ExternalBuffer(*p){}

      ExternalBuffer(){}

      friend class ExternalPort;
      ExternalPort *myExternalPort;     // which user port do we belong to
      OcdpMetadata *metadata;   // where is the metadata buffer
      uint8_t *data;                // where is the data buffer
      uint32_t length;          // length of the buffer (not message)
      volatile uint32_t *readyForLocal;  // where is the flag set by remote on data movement
      volatile uint32_t *readyForRemote; // where is ready flag for remote data movement
      bool busy;                // in use by local processing (for error checking)
      bool last;                // last buffer in the set
      void release();
      void put(uint8_t opCode, uint32_t dataLength, bool endOfData) {
        cpiAssert(dataLength <= length);
        metadata->opCode = opCode;
        metadata->length = dataLength;
        release();
      }
    };




    // Producer or consumer
    class ExternalPort : public CC::ExternalPort, public CC::BasePort {

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


      Port &myPort;
      //      uint32_t nBuffers, *ready, next;
      OcdpMetadata *metadata;
      uint32_t *flags;
      ExternalBuffer *localBuffers, *nextLocal, *nextRemote;
      FarBuffer *farBuffers, *nextFar;
      uint8_t *localData;
      friend class Port;

      ExternalPort(const char *name, Port &port, CU::PValue *props) :
        CC::ExternalPort(name),
        BasePort( port.myMetaPort ),
        myPort(port)
      {
        // Default is active only (host is master, never slave)
        connectionData.data.options =
          (1 << CPI::RDT::ActiveFlowControl) |
          (1 << CPI::RDT::ActiveMessage) |
          (1 << CPI::RDT::ActiveOnly);
        applyConnectParams(props);
        port.establishRoles(connectionData.data);
        unsigned nFar = myPort.connectionData.data.desc.nBuffers;
        unsigned nLocal = myDesc.nBuffers;
        myDesc.dataBufferPitch = myPort.connectionData.data.desc.dataBufferPitch;
        myDesc.metaDataPitch = myPort.connectionData.data.desc.metaDataPitch;
        myDesc.fullFlagPitch = sizeof(uint32_t);
        myDesc.emptyFlagPitch = sizeof(uint32_t);
        myDesc.emptyFlagValue = 1;
        myDesc.fullFlagValue = 1;
        // Allocate my local memory, making everything on a nice boundary.
        // (assume empty flag pitch same as full flag pitch)
        unsigned nAlloc =
          CC::roundup(myDesc.dataBufferPitch * nLocal, LOCAL_DMA_ALIGN) +
          CC::roundup(myDesc.metaDataPitch * nLocal, LOCAL_DMA_ALIGN) +
          CC::roundup(sizeof(uint32_t) * nLocal, LOCAL_DMA_ALIGN) + // local flags
          // These might actually be remote
          CC::roundup(sizeof(uint32_t) * nLocal, LOCAL_DMA_ALIGN) + // remote flags
          // These might not be needed if we are ActiveFlowControl
          CC::roundup(sizeof(uint32_t) * nFar, LOCAL_DMA_ALIGN);
        // Now we allocate all the (local) endpoint memory
        uint8_t *allocation = 0;
        static const char *dma = getenv("CPI_DMA_MEMORY");
        static bool done = false;
        static uint64_t base, size;
        if (!done) {
          if (dma) {
            unsigned sizeM;
            cpiAssert(sscanf(dma, "%uM$0x%llx", &sizeM,
                             (unsigned long long *) &base) == 2);
            size = (unsigned long long)sizeM * 1024 * 1024;
            fprintf(stderr, "DMA Memory:  %uM at 0x%llx\n", sizeM,
                    (unsigned long long)base);
          }
          done = true;
        }
        snprintf(myDesc.oob.oep, sizeof(myDesc.oob.oep),
                 "cpi-pci-pio://%s.%lld:%lld.3.10", "0", (unsigned long long)base,
                 (unsigned long long)nAlloc);
        // If we are ActiveOnly we need no DMAable memory at all, so get it from the heap.
        if (connectionData.data.role == CPI::RDT::ActiveOnly)
          allocation = new uint8_t[nAlloc];
        else {
          if (!dma)
            throw CC::ApiError("Asking for DMA without CPI_DMA_MEMORY environment var", NULL);
          allocation = (uint8_t*)mmap(NULL, nAlloc, PROT_READ|PROT_WRITE, MAP_SHARED,
                                      Driver::pciMemFd, base);
          cpiAssert(allocation != (uint8_t*)-1);
          base += nAlloc;
          base += getpagesize();
          base &= ~(getpagesize() - 1);
          // Get the local endpoint corresponding to the known remote endpoint
#if 0
          myEndpoint = CPI::RDT::GetEndpoint("cpi-pci//bus-id");
          if (!myEndpoint)
            CC::ApiError("No local (CPU) endpoint support for pci bus %s", NULL);
          allocation = myEndpoint->alloc(nAlloc);
#endif
        }

#ifdef NEEDED
        memset((void *)allocation, 0, nAlloc);
#endif

        localData = allocation;
        myDesc.dataBufferBaseAddr  = 0;
        allocation += CC::roundup(myDesc.dataBufferPitch * nLocal, LOCAL_BUFFER_ALIGN);
        metadata = (OcdpMetadata *)allocation;
        myDesc.metaDataBaseAddr = allocation - localData;
        allocation += CC::roundup(myDesc.metaDataPitch * nLocal, LOCAL_BUFFER_ALIGN);
        uint32_t *localFlags = (uint32_t*)allocation;
        allocation += CC::roundup(sizeof(uint32_t) * nLocal, LOCAL_BUFFER_ALIGN);
        uint32_t *remoteFlags = (uint32_t*)allocation;
        allocation += CC::roundup(sizeof(uint32_t) * nLocal, LOCAL_BUFFER_ALIGN);
        uint32_t *farFlags = (uint32_t*)allocation;
        switch (connectionData.data.role) {
        case CPI::RDT::ActiveMessage:
          // my exposed addresses are the flags in my memory that indicate far buffer state
          myDesc.emptyFlagBaseAddr =
            myDesc.fullFlagBaseAddr = allocation - localData;
          // FALL THROUGH
        case CPI::RDT::ActiveOnly:
          {
            FarBuffer *fb = nextFar = farBuffers = new FarBuffer[nFar];
            for (unsigned i = 0; i < nFar; i++, fb++) {
              fb->metadata = myPort.userMetadataBaseAddr + i;
              fb->data = myPort.userDataBaseAddr + i * myPort.myDesc.dataBufferPitch;
              fb->ready = farFlags + i; // not used for ActiveOnly
              *fb->ready = myPort.isProvider();
              fb->last = false;
            }
            (fb-1)->last = true;
          }
          break;
        case CPI::RDT::ActiveFlowControl:
          // here the far side needs to know about the remote flags;
          myDesc.emptyFlagBaseAddr =
            myDesc.fullFlagBaseAddr = (uint8_t*)localFlags - localData;
        }

        // Initialize our structures that keep track of LOCAL buffer status
        ExternalBuffer *lb = nextLocal = nextRemote = localBuffers = new ExternalBuffer[nLocal];
        for (unsigned i = 0; i < nLocal; i++, lb++) {
          lb->myParent = this;
          this->addChild( *lb );
          lb->myExternalPort = this;
          lb->metadata = metadata + i;
          lb->data = localData + i * myDesc.dataBufferPitch;
          lb->length = myDesc.dataBufferPitch;
          lb->last = false;
          lb->busy = false;
          lb->readyForLocal = localFlags + i;
          *lb->readyForLocal = myPort.isProvider();
          lb->readyForRemote = remoteFlags + i; //&myPort.myOcdpRegisters->nRemoteDone;
          *lb->readyForRemote = !myPort.isProvider();
        }
        (lb-1)->last = true;
      }
      // We know a move can be done.  Do it.
      // We are either ActiveOnly or ActiveMessage
      void moveData() {
        if (myPort.isProvider()) {
          // Here to far
          memcpy((void *)nextFar->metadata, nextRemote->metadata, sizeof(OcdpMetadata));
          memcpy((void *)nextFar->data, nextRemote->data, nextRemote->metadata->length);
        } else {
          // Far to here
          memcpy(nextRemote->metadata, (const void *)nextFar->metadata, sizeof(OcdpMetadata));
          memcpy(nextRemote->data, (const void *)nextFar->data, nextRemote->metadata->length);
        }
        // Set the local indication of readiness of the far buffer to false.
        // Far side will make it true (or us when we are ActiveOnly).
        *nextFar->ready = false;
        //        __asm__ __volatile__  ("lock; addl $0,0(%%esp)": : :"memory");
        // FIXME:  memory barrier to be sure?
        // Tell the far side that a its buffer has been used (filled or emptied)
        //wmb();
        if (myPort.myOcdpRegisters->foodFace != 0xf00dface)
          abort();
        myPort.myOcdpRegisters->nRemoteDone = 1;
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
      // Try to move some data
      void tryMove() {
        // Try to advance my remote side
        switch (connectionData.data.role) {
        case CPI::RDT::ActiveOnly:
          // Use far side "ready" register to determine whether far buffers are ready
          // Thus we need to do a remote PCIe read to know far size status
          if (*nextRemote->readyForRemote) { // avoid remote read if local is not ready
            for (uint32_t nReady = myPort.myOcdpRegisters->nReady;
                 nReady && *nextRemote->readyForRemote; nReady--)
            moveData();
          }
          break;
        case CPI::RDT::ActiveMessage:
          // Use local version of far-is-ready flag for the far buffer,
          // which will be written by the ActiveFlowControl far side.
          while (*nextRemote->readyForRemote && *nextFar->ready)
            moveData();
          break;
        case CPI::RDT::ActiveFlowControl:
          // Nothing to do here.  We don't move data.
          // When the other side moves data it will set our far-is-ready flag
          break;
        case CPI::RDT::Passive:
        case CPI::RDT::NoRole:
          cpiAssert(0);
        }
      }
      bool getLocal() {
        tryMove();
        if (!*nextLocal->readyForLocal)
          return false;
        cpiAssert(connectionData.data.role == CPI::RDT::ActiveFlowControl ||
                  connectionData.data.role == CPI::RDT::Passive ||
                  !*nextLocal->readyForRemote);
        cpiAssert(!nextLocal->busy);
        nextLocal->busy = true; // to ensure callers use the API correctly
        *nextLocal->readyForLocal = 0;
        return true;
      }
      // The input method = get a buffer that has data in it.
      CC::ExternalBuffer *
      getBuffer(uint8_t &opCode, uint8_t *&bdata, uint32_t &length, bool &end) {
        cpiAssert(!myPort.isProvider());
        if (!getLocal())
          return 0;
        bdata = nextLocal->data;
        length = nextLocal->metadata->length;
        opCode = nextLocal->metadata->opCode;
        end = false; // someday bit in metadata
        //FIXME cast unnecessary
        return static_cast<CC::ExternalBuffer *>(nextLocal);
      }
      CC::ExternalBuffer *getBuffer(uint8_t *&bdata, uint32_t &length) {
        cpiAssert(myPort.isProvider());
        if (!getLocal())
          return 0;
        bdata = nextLocal->data;
        length = nextLocal->length;
        return static_cast<CC::ExternalBuffer *>(nextLocal);
      }
      void endOfData() {
        cpiAssert(myPort.isProvider());
      }
      void tryFlush() {
        cpiAssert(myPort.isProvider());
        tryMove();
      }
      void advanceLocal() {
        if (connectionData.data.role == CPI::RDT::ActiveFlowControl) {
          //          if (myPort.myOcdpRegisters->foodFace != 0xf00dface)
          //            abort();
          //          wmb();
          myPort.myOcdpRegisters->nRemoteDone = 1;
          usleep(0);
        }
        if (nextLocal->last)
          nextLocal = localBuffers;
        else
          nextLocal++;
        tryMove();
      }
      void checkConnectParams() {
      }
    };

    // FIXME make readyForRemote zero when active flow control
    void ExternalBuffer::release() {
      cpiAssert(myExternalPort->connectionData.data.role == CPI::RDT::ActiveFlowControl ||
                myExternalPort->connectionData.data.role == CPI::RDT::Passive ||
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
    CC::ExternalPort &Port::connectExternal(const char *name, CU::PValue *userProps, CU::PValue *props) {
      if (!canBeExternal)
        throw CC::ApiError("Port is locally connected in the bitstream.", 0);
      applyConnectParams(props);
      // UserPort constructor must know the roles.
      ExternalPort *myExternalPort = new ExternalPort(name, *this, userProps);
      finishConnection(myExternalPort->connectionData.data);
      return *myExternalPort;
    }
    int Driver::pciMemFd = -1;
  }
}
