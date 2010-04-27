/**
  @brief
  This file contains the implementation for the RCC container.
  There is no separate header file for this class since its only purpose
  is to implement the CPI::Container::Interface and no one else will use it.

  Revision History:

    7/1/2009 - Jim Kulp
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
#include <CpiContainerInterface.h>
#include "CpiWorker.h"
#include "CpiApplication.h"
#include "CpiArtifact.h"
#include "CpiProperty.h"
#include "CpiContainerPort.h"
#include "CpiPValue.h"
#include "CpiDriver.h"

namespace CPI {
  namespace Container{}
  namespace RCC {
    namespace CC = CPI::Container;
    namespace CO = CPI::OS;
    namespace CM = CPI::Metadata;
    namespace CU = CPI::Util;

#if 0
    const unsigned
      DEFAULT_NBUFFERS = 2,
      DEFAULT_BUFFER_SIZE = 2*1024;
#endif
      
    class Driver : public CU::Driver {
    public:
      // This constructor simply registers itself. This class has no state.
      Driver () 
	throw() 
	: CU::Driver( "Container1", "RCC", true ) {
      }
      // This driver method is called to see if a particular container device exists,
      // and if so, to instantiate a container
      CU::Device *probe(const CU::PValue* props, const char *which )
	throw (CPI::Util::EmbeddedException);

      // Per driver discovery routine to create devices
      unsigned search(const CU::PValue* props, const char **exclude)
	throw (CPI::Util::EmbeddedException) {return 1;}

      virtual ~Driver()
	throw (){}

    private:

      // This should be generated in search
      std::vector<std::string> m_endpoints;

    } driver;


    class Container : public CC::Interface {
      friend class Driver;
    protected:
      Container(CU::Driver &driver, const char *name=NULL, const char *endpoint=NULL, const CU::PValue *props=NULL) :
	CC::Interface(driver, name)
      {
      }
      ~Container() throw() {
	// ref count driver static resources, like closing pciMemFd
      }
      // support worker ids for those who want it
      CC::Worker &findWorker(CC::WorkerId)
      {
	static CC::Worker *w; return *w;
      }
    public:

      CC::Application *
      createApplication() 
	throw (CPI::Util::EmbeddedException);

      CC::Artifact &
      createArtifact(const char *url, CU::PValue *artifactParams);
    };

    CU::Device* Driver::probe(const CU::PValue*, const char *which ) 
      throw (CPI::Util::EmbeddedException)
    {
      return new Container(*this);
    }


    class Application :
      public CC::Application
    {
      friend class Container;
      Application(Container &container, const char *name=NULL, CU::PValue *p=NULL) :
	CC::Application(container, name, p)
      {
      }      
    };


    // Container methods that depend on Application
    CC::Application *
    Container::createApplication()
      throw ( CPI::Util::EmbeddedException )
    {
      return new Application( *this );
    }      

    class Artifact :
      public CC::Artifact
    {
      ~Artifact() {
      }
      friend class Container;
    protected:
      Artifact(Container &c, const char *url, CU::PValue *artifactParams) :
	CC::Artifact(c, url) {
	/* load dll here */
      }
      virtual CC::Worker &
      createWorkerX(CC::Application &, ezxml_t impl, ezxml_t inst, CU::PValue *execProps);
    };
    
    CC::Artifact & Container::
    createArtifact(const char *url, CU::PValue *artifactParams)
    {
      return *new Artifact(*this, url, artifactParams);
    }
    class Worker : public CC::Worker {
      friend class Artifact;
      friend class Port;
      CM::Worker::ControlState myState;
      uint32_t controlMask;
      uint8_t *myProperties;
      Worker(CC::Application &application, ezxml_t implXml, CU::PValue* execProps) :
	CC::Worker(application, implXml, 0),
	myState(CM::Worker::EXISTS) 
      {
	const char *ops = ezxml_attr(implXml, "controlOperations");
#define CONTROL_OP(x, c, t, s1, s2, s3)  if (ops && strstr(ops, #x)) controlMask |= 1 << CM::Worker::Op##c;
      CPI_CONTROL_OPS
#undef CONTROL_OP
	controlMask |= 1 << CM::Worker::OpStart;
	// Maybe this should be delayed until construction of derived class is done? FIXME?
	initialize();
      }
      ~Worker()
      {
      }


      // IMPLEMENT ME !!
      WCI_error control(WCI_control, WCI_options){return 0;};
      WCI_error status(WCI_status*){return 0;};
      WCI_error read(WCI_u32, WCI_u32, WCI_data_type, WCI_options, void*){return 0;};
      WCI_error write(WCI_u32, WCI_u32, WCI_data_type, WCI_options, const void*){return 0;};
      WCI_error close(WCI_options){return 0;};
      std::string getLastControlError()
	throw (CPI::Util::EmbeddedException){std::string error("Not Implemented"); return error;}




      // Add the hardware considerations to the property object that supports
      // fast memory-mapped property access directly to users
      // the key members are "readVaddr" and "writeVaddr"
      virtual void prepareProperty(CM::Property &md, CC::Property &cp) {
	if (myProperties)
	  if (!md.is_struct && !md.is_sequence && !md.types->type != CM::Property::CPI_String &&
	      CM::Property::tsize[md.types->type] <= 64) {
	    if (!md.write_error)
	      cp.writeVaddr = myProperties + md.offset;
	    if (!md.read_error)
	      cp.readVaddr = myProperties + md.offset;
	  }
      }
      // Should check for illegal sequences
#define CONTROL_OP(x, c, t, s1, s2, s3)					\
      virtual void x() {						\
	if (myState == CM::Worker::s1 ||				\
	    (CM::Worker::s2 != CM::Worker::NONE && myState == CM::Worker::s2) || \
	    (CM::Worker::s3 != CM::Worker::NONE && myState == CM::Worker::s3)) { \
	  if (controlMask & (1 << CM::Worker::Op##c))			\
	    x();							\
	  myState = CM::Worker::t;					\
	} else								\
	  throw CC::ApiError("Illegal control state for operation",0); \
      }
      CPI_CONTROL_OPS
#undef CONTROL_OP
      // These property access methods are called when the fast path
      // is not enabled, either due to no MMIO or that the property can
      // return errors.  OCCP has MMIO, so it must be the latter
#undef CPI_DATA_TYPE_S
      // Set a scalar property value
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
      void set##pretty##Property(unsigned ord, const run val) {		\
	CM::Property &p = property(ord);				\
	if (!p.is_writable)						\
	  throw CC::ApiError("attempt to set property that is not writable", NULL); \
	store *pp = (store *)(myProperties + p.offset);			\
	if (bits > 32) {						\
	  assert(bits == 64);						\
	  uint32_t *p32 = (uint32_t *)pp;				\
	  p32[1] = ((uint32_t *)&val)[1];				\
	  p32[0] = ((uint32_t *)&val)[0];				\
	} else								\
	  *pp = *(store *)&val;						\
      }									\
      void set##pretty##SequenceProperty(unsigned ord,const run *vals, unsigned length) { \
	CM::Property &p = property(ord);				\
	assert(p.types->type == CM::Property::CPI_##pretty);		\
	if (!p.is_writable)						\
	  throw; /*"attempt to set property that is not writable" */	\
	if (length > p.sequence_size)					\
	  throw;							\
	memcpy((void *)(myProperties + p.offset + p.maxAlign), vals, length * sizeof(run)); \
	*(uint32_t *)(myProperties + p.offset) = length;		\
      }
      // Set a string property value
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)		\
      virtual void set##pretty##Property(unsigned ord, const run val) {	\
	CM::Property &p = property(ord);				\
	assert(p.types->type == CM::Property::CPI_##pretty);		\
	if (!p.is_writable)						\
	  throw; /*"attempt to set property that is not writable" */	\
	unsigned cpi_length;						\
	if (!val || (cpi_length = strlen(val)) > p.types->size)		\
	  throw; /*"string property too long"*/;			\
	uint32_t *p32 = (uint32_t *)(myProperties + p.offset);		\
	/* if length to be written is more than 32 bits */		\
	if (++cpi_length > 32/CHAR_BIT)					\
	  memcpy(p32 + 1, val + 32/CHAR_BIT, cpi_length - 32/CHAR_BIT); \
	uint32_t i;							\
	memcpy(&i, val, 32/CHAR_BIT);					\
	p32[0] = i;							\
      }									\
      void set##pretty##SequenceProperty(unsigned ord,const run *vals, unsigned length) { \
	CM::Property &p = property(ord);				\
	assert(p.types->type == CM::Property::CPI_##pretty);		\
	if (!p.is_writable)						\
	  throw; /*"attempt to set property that is not writable" */	\
	if (length > p.sequence_size)					\
	  throw;							\
	char *cp = (char *)(myProperties + p.offset + 32/CHAR_BIT);	\
	for (unsigned i = 0; i < length; i++) {				\
	  unsigned len = strlen(vals[i]);				\
	  if (len > p.types->size)					\
	    throw; /* "string in sequence too long" */			\
	  memcpy(cp, vals[i], len+1);					\
	}								\
	*(uint32_t *)(myProperties + p.offset) = length;		\
      }
      CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE_S
#undef CPI_DATA_TYPE
      // Get Scalar Property
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
      virtual run get##pretty##Property(unsigned ord) {			\
	CM::Property &p = property(ord);				\
	if (!p.is_readable)						\
	  throw; /*"attempt to set property that is not writable" */	\
	  throw; /*"worker has errors before read "*/			\
	uint32_t *pp = (uint32_t *)(myProperties + p.offset);		\
	union {								\
		run r;							\
		uint32_t u32[bits/32];					\
	} u;								\
	if (bits > 32)							\
	  u.u32[1] = pp[1];						\
	u.u32[0] = pp[0];						\
	return u.r;							\
      }									\
      unsigned get##pretty##SequenceProperty(unsigned ord, run *vals, unsigned length) { \
	CM::Property &p = property(ord);				\
	if (!p.is_readable)						\
	  throw; /*"attempt to set property that is not writable" */	\
	uint32_t n = *(uint32_t *)(myProperties + p.offset);		\
	if (n > length)							\
	  throw; /* sequence longer than provided buffer */		\
	memcpy(vals, (void*)(myProperties + p.offset + p.maxAlign),	\
	       n * sizeof(run));					\
	return n;							\
      }

      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)		\
      virtual void get##pretty##Property(unsigned ord, char *cp, unsigned length) { \
	CM::Property &p = property(ord);				\
	assert(p.types->type == CM::Property::CPI_##pretty);		\
	if (!p.is_readable)						\
	  throw; /*"attempt to set property that is not writable" */	\
	if (length < p.types->size+1)					\
	  throw; /*"string buffer smaller than property"*/;		\
	uint32_t i32, *p32 = (uint32_t *)(myProperties + p.offset);	\
	memcpy(cp + 32/CHAR_BIT, p32 + 1, p.types->size + 1 - 32/CHAR_BIT); \
	i32 = *p32;							\
	memcpy(cp, &i32, 32/CHAR_BIT);					\
      }									\
      unsigned get##pretty##SequenceProperty			        \
      (unsigned ord, run *vals, unsigned length, char *buf, unsigned space) { \
	CM::Property &p = property(ord);				\
	assert(p.types->type == CM::Property::CPI_##pretty);		\
	if (!p.is_readable)						\
	  throw; /*"attempt to get property that is not readable" */	\
	if (length > p.sequence_size)					\
	  throw;							\
	uint32_t							\
	  n = *(uint32_t *)(myProperties + p.offset),			\
	  wlen = p.types->size + 1;					\
	if (n > length)							\
	  throw; /* sequence longer than provided buffer */		\
	char *cp = (char *)(myProperties + p.offset + 32/CHAR_BIT);	\
	for (unsigned i = 0; i < n; i++) {				\
	  if (space < wlen)						\
	    throw;							\
	  memcpy(buf, cp, wlen);					\
	  cp += wlen;							\
	  vals[i] = buf;						\
	  unsigned slen = strlen(buf) + 1;				\
	  buf += slen;							\
	  space -= slen;						\
	}								\
	return n;							\
      }
      CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE_S
#undef CPI_DATA_TYPE
#define CPI_DATA_TYPE_S CPI_DATA_TYPE
      // Forward references for methods that can't be in the class def.
      CC::Port & createPort(CM::Port &metaport);
      // Legacy?
      CC::Port &
      createOutputPort(CC::PortId portId,
		       CPI::OS::uint32_t bufferCount,
		       CPI::OS::uint32_t bufferSize, 
		       CPI::Util::PValue* props) throw();
      CC::Port &
      createInputPort(CC::PortId portId,
		      CPI::OS::uint32_t bufferCount,
		      CPI::OS::uint32_t bufferSize, 
		      CPI::Util::PValue* props) throw();
    };
    // Internal constructor after artifact's XML metadata processing
    CC::Worker & Artifact::
    createWorkerX(CC::Application &a, ezxml_t impl, ezxml_t inst,
		  CU::PValue *execProps) {
      return *new Worker(a, impl, execProps);
    }
    // This port class really has two cases: externally connected ports and 
    // internally connected ports.
    // Also ports are either user or provider.
    // So this class takes care of all 4 cases, since the differences are so 
    // minor as to not be worth (re)factoring currenty.
    class Port : public CC::Port {
      friend class Worker;
      // These are for external ports
      std::string myInitialPortInfo;
      PortData myOtherPortData;
      // Assume that the userDataBaseBufferAddress and dataBufferBaseAddr's are set.
      void setFinalAddresses() {
#if 0
	if (bufferCount * (CC::roundup(bufferSize, 16) + OCDP_METADATA_SIZE) > myOcdpSize)
	  throw CC::ApiError("Requested buffer count and size won't fit in the OCDP module", 0);
	// Compute final info FIXME: symmetry
	CPI::RDT::Descriptors &d = connectionData.data;
	d.desc.nBuffers = bufferCount;
	d.desc.dataBufferSize = bufferSize;
	d.desc.dataBufferPitch = CC::roundup(bufferSize, 16);
	d.desc.metaDataBaseAddr =
	  d.desc.dataBufferBaseAddr +
	  myOcdpSize - bufferCount * OCDP_METADATA_SIZE;
	userMetadataBaseAddr = (OcdpMetadata *)(userDataBaseAddr +
						myOcdpSize - bufferCount * OCDP_METADATA_SIZE);
#endif
      }
      Port(Worker &w,
	   CM::Port &mPort) : // the port metadata from when the worker was created
	CC::Port(w, mPort)
      {
	CPI::RDT::Descriptors &d = connectionData.data;
	// These will be determined at connection time
	d.desc.nBuffers           = 0;
	d.desc.dataBufferSize     = 0;
	d.desc.dataBufferPitch    = 0;
	d.desc.metaDataBaseAddr   = 0;
	// These are fixed
	d.desc.metaDataPitch      = 16;
	d.desc.dataBufferBaseAddr = 0;

	//	snprintf(d.desc.oob.oep, sizeof(d.desc.oob.oep),
	//		 "cpi-pci-pio://%s.%llu.%llu:0.1.10", busId, busAddress, busSize);
	if ( ! isProvider()) {
	  d.type = CPI::RDT::ConsumerDescT;
	  // The doorbells are in the OCDP's register space.
	  // "full" is the doorbell telling me a buffer has become full
	  // This register is for WRITING
	  //	  d.desc.fullFlagBaseAddr = (uint8_t*)&myOcdpRegisters->nRemoteDone - (uint8_t *)myContainer.occp;
	  d.desc.fullFlagSize       = 4;
	  d.desc.fullFlagPitch      = 0;
	  d.desc.fullFlagValue      = 1; // Can actaully be a count!
	  // The nReady register is the empty flag, which tells the master how many
	  // empty buffers there are to fill
	  // This register is for READING
	  //	  d.desc.emptyFlagBaseAddr = (uint8_t*)&myOcdpRegisters->nReady - (uint8_t *)myContainer.occp;
	  d.desc.emptyFlagSize       = 4;
	  d.desc.emptyFlagPitch      = 0;
	  d.desc.emptyFlagValue      = 0;
	} else {
	  d.type = CPI::RDT::ProducerDescT;
	  // The doorbells are in the OCDP's register space.
	  // The doorbell tells us that a buffer has BECOME EMPTY
	  // This register is for writing.
	  //	  d.desc.emptyFlagBaseAddr = (uint8_t*)&myOcdpRegisters->nRemoteDone - (uint8_t *)myContainer.occp;
	  d.desc.emptyFlagSize     = 4;
	  d.desc.emptyFlagPitch    = 0;
	  d.desc.emptyFlagValue    = 1; // Can actually be a count of buffers consumed
	  // The nReady register is the full flag, which tells the master how many
	  // full buffers there are to read/take
	  // This register is for READING
	  //	  d.desc.fullFlagBaseAddr = (uint8_t*)&myOcdpRegisters->nReady - (uint8_t *)myContainer.occp;
	  d.desc.fullFlagSize       = 4;
	  d.desc.fullFlagPitch      = 0;
	  d.desc.fullFlagValue      = 0; // register is for reading
	}
	//	userDataBaseAddr = myContainer.bar1 + myOcdpOffset;
	// Set this provisionally, for debug mostly.
	setFinalAddresses();
      }

      void disconnect( )
	throw ( CPI::Util::EmbeddedException )
      {
	throw CPI::Util::EmbeddedException("disconnect not yet implemented !!");
      }

      // Do the work on this port when connection properties are specified.
      // This is still prior to receiving info from the other side, this is the final info.
      int bufferCount;
      int bufferSize;
      void applyConnectParams(CU::PValue *props) {
	if (!props)
	  return;
	for (CU::PValue *p = props; p->name; p++) {
	  if (strcmp(p->name, "bufferCount") == 0) {
	    if (p->type != CM::Property::CPI_ULong)
	      throw CC::ApiError("bufferCount property has wrong type, should be ULong", NULL);
	    if (p->vULong < minBufferCount)
	      throw CC::ApiError("bufferCount is below worker's minimum", NULL);
	    bufferCount = p->vULong;
	  }
	  if (strcmp(p->name, "bufferSize") == 0) {
	    if (p->type != CM::Property::CPI_ULong)
	      throw CC::ApiError("bufferSize property has wrong type, should be ULong", 0);
	    if (p->vULong < minBufferSize)
	      throw CC::ApiError("bufferSize is below worker's minimum", NULL);
	    if (maxBufferSize && p->vULong > maxBufferSize)
	      throw CC::ApiError("bufferSize exceeds worker's maximum", NULL);
	    bufferSize = p->vULong;
	  }
	}
	setFinalAddresses();
      }
      void finishConnection(CPI::RDT::Descriptors &other) {
	// Here is where we can setup the OCDP producer/user
#if 0
	cpiAssert(myOcdpRegisters->foodFace == 0xf00dface);
	myOcdpRegisters->nLocalBuffers = bufferCount;
	myOcdpRegisters->bufferSize = bufferSize;
	myOcdpRegisters->localBufferBase = 0;
	myOcdpRegisters->localMetadataBase = myOcdpSize - bufferCount * OCDP_METADATA_SIZE;
	myOcdpRegisters->localRole = myOcdpRole;
	switch (myOcdpRole) {
	case OCDP_PASSIVE:
	  // Nothing more to do.
	  break;
	case OCDP_ACTIVE_DOORBELL:
	  // Remote Doorbell address and pitch.
	  cpiAssert(0);
	  break;
	case OCDP_ACTIVE_MESSAGE:
	  // Remote Buffer Address and pitch
	  cpiAssert(0);
	  break;
	}
#endif
      }
      // Connection between two ports inside this container
      // We know they must be in the same artifact, and have a metadata-defined connection
      void connectInside(CC::Port &provider, CU::PValue *myProps, CU::PValue *otherProps) {
	cpiAssert(myParent->myParent == provider.myParent->myParent);
	// We're both in the same runtime artifact object, so we know the port class
	Port &pport = static_cast<Port&>(provider);
	// FIXME: we should error check against bitstream-fixed parameters
	pport.applyConnectParams(otherProps);
	applyConnectParams(myProps);
	// For now we assume there is nothing to actually adjust in the bitstream.
      }
      // Directly connect to this port
#if 1
      CC::ExternalPort &connectExternal(const char *name, CU::PValue *props, CU::PValue *uprops) {
	static CC::ExternalPort *p;
	return *p;
      }
#else
      void connectUser(CU::PValue *props) {
	if (!canBeExternal)
	  throw CC::ApiError("Port is locally connected in the bitstream.", 0);
	applyConnectParams(props);
	finishConnection();
      }
#endif
      void checkConnectParams() {}
    };




    // CPI API
    CC::Port &Worker::
    createPort(CM::Port &metaPort) {
      return *new Port(*this, metaPort);
    }
    // Here because these depend on Port
    CC::Port &Worker::
    createOutputPort(CC::PortId portId,
		     CPI::OS::uint32_t bufferCount,
		     CPI::OS::uint32_t bufferSize, 
		     CPI::Util::PValue* props) throw() {
      return *(Port *)0;//return *new Port(*this);
    }
    CC::Port &Worker::
    createInputPort(CC::PortId portId,
		    CPI::OS::uint32_t bufferCount,
		    CPI::OS::uint32_t bufferSize, 
		    CPI::Util::PValue* props) throw() {
      return *(Port *)0;//      return *new Port(*this);
    }
  }
}
