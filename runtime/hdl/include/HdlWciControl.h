#ifndef HDL_WCI_CONTROL_H
#define HDL_WCI_CONTROL_H

#include "ContainerWorker.h"
#include "HdlOCCP.h"
#include "HdlAccess.h"

namespace OCPI {
  namespace HDL {

    // The class that knows about WCI interfaces and the OCCP.
    class Device;
    class WciControl : public Access, virtual public OCPI::Container::Controllable,
      virtual public OCPI::API::PropertyAccess, virtual OCPI::Container::WorkerControl {
      
      friend class Port;
      friend class Device;
      friend class Container;
      const char *m_implName, *m_instName;
      mutable size_t m_window; // perfect use-case for mutable..
      bool m_hasControl;
      size_t m_timeout;
      //      std::string m_wName;
    protected:
      Access m_properties;              // The accessor to the remote property space
      Device &m_device;
      size_t m_occpIndex;
      OCPI::Util::Property *m_propInfo; // the array of property descriptors
      WciControl(Device &device, const char *impl, const char *inst, unsigned index, bool hasControl);
    public:
      WciControl(Device &device, ezxml_t implXml, ezxml_t instXml, OCPI::Util::Property *props, bool doInit = true);
      virtual ~WciControl();
    protected:
      // This is shadowed by real application workers, but is used when this is 
      // standalone.
      //      const std::string &name() const { return m_wName; }
      void init(bool redo, bool doInit);
      bool isReset() const;
      inline size_t index() const { return m_occpIndex; }
      void propertyWritten(unsigned ordinal) const;
      void propertyRead(unsigned ordinal) const;
      // Add the hardware considerations to the property object that supports
      // fast memory-mapped property access directly to users
      // the key members are "readVaddr" and "writeVaddr"
      virtual void prepareProperty(OCPI::Util::Property &md, 
				   volatile void *&writeVaddr,
				   const volatile void *&readVaddr);
      // Map the control op numbers to structure members
      static const unsigned controlOffsets[];
      void checkControlState();
      void controlOperation(OCPI::Util::Worker::ControlOperation op);
      bool controlOperation(OCPI::Util::Worker::ControlOperation op, std::string &err);
      inline uint32_t checkWindow(size_t offset, size_t nBytes) const {
	ocpiAssert(m_hasControl);
	size_t window = offset & ~(OCCP_WORKER_CONFIG_SIZE-1);
        ocpiAssert(window == ((offset+nBytes)&~(OCCP_WORKER_CONFIG_SIZE-1)));
	if (window != m_window) {
	  set32Register(window, OccpWorkerRegisters,
			(uint32_t)(window >> OCCP_WORKER_CONFIG_WINDOW_BITS));
	  m_window = window;
	}
	return offset & (OCCP_WORKER_CONFIG_SIZE - 1);
      }
      void throwPropertyReadError(uint32_t status, uint32_t offset, size_t n, uint64_t val) const;
      void throwPropertyWriteError(uint32_t status) const;
      void throwPropertySequenceError() const;

#define PUT_GET_PROPERTY(n,wb)                                                    \
      void                                                                        \
	setProperty##n(const OCPI::API::PropertyInfo &info, uint##n##_t val,      \
                       unsigned idx) const;					  \
      inline uint##n##_t						          \
      getProperty##n(const OCPI::API::PropertyInfo &info, unsigned idx) const {	  \
        uint32_t offset = checkWindow(info.m_offset + idx * (n/8), n/8); \
	uint32_t status = 0;                                                      \
        uint##wb##_t val##wb;							  \
	uint##n##_t val;						          \
	if (m_properties.m_registers) {					          \
	  if (!info.m_readError ||					          \
	      !(status =						          \
		get32Register(status, OccpWorkerRegisters) &		          \
		OCCP_STATUS_READ_ERRORS)) {                                       \
	    val##wb = m_properties.get##n##RegisterOffset(offset);      	  \
	    switch ((uint32_t)val##wb) {                                          \
	    case OCCP_TIMEOUT_RESULT:                                             \
            case OCCP_RESET_RESULT:                                               \
            case OCCP_ERROR_RESULT:                                               \
            case OCCP_FATAL_RESULT:                                               \
            case OCCP_BUSY_RESULT:                                                \
	      /* The returned data value matches our error codes so we read */    \
	      /* the status register to be sure */			          \
	      status = get32Register(status, OccpWorkerRegisters) &	          \
		       OCCP_STATUS_READ_ERRORS;				          \
	    default:							          \
	      val = (uint##n##_t)val##wb;                                         \
            }								          \
	  } else                                                                  \
            val = 0;                                                              \
	  if (!status && info.m_readError)				          \
	    status =							          \
	      get32Register(status, OccpWorkerRegisters) &		          \
	      OCCP_STATUS_READ_ERRORS;					          \
	} else								          \
	  val = (uint##n##_t)						\
	    (n == 64 ?							\
	     m_properties.m_accessor->get64(m_properties.m_base + offset, &status) : \
	     m_properties.m_accessor->get(m_properties.m_base + offset,	sizeof(uint##n##_t), \
					  &status));	\
	if (status)							          \
	  throwPropertyReadError(status, offset, n/8, val);			\
	return val;							          \
      }
      PUT_GET_PROPERTY(8,32)
      PUT_GET_PROPERTY(16,32)
      PUT_GET_PROPERTY(32,32)
      PUT_GET_PROPERTY(64,64)
#undef PUT_GET_PROPERTY
      void setPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
			    const uint8_t *data, size_t nBytes, unsigned idx) const;

      void getPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset, uint8_t *buf,
			    size_t nBytes, unsigned idx, bool string) const;
      void setPropertySequence(const OCPI::API::PropertyInfo &p,
			       const uint8_t *val,
			       size_t nItems, size_t nBytes) const;
      unsigned getPropertySequence(const OCPI::API::PropertyInfo &p, uint8_t *buf, size_t n) const;
      
#undef OCPI_DATA_TYPE_S
      // Set a scalar property value

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)     	         \
      inline void							 \
	set##pretty##Property(unsigned ordinal, const run val, unsigned idx) const { \
	setProperty##bits(m_propInfo[ordinal], *(uint##bits##_t *)&val, idx); \
      }									 \
      inline void        						\
      set##pretty##SequenceProperty(const OCPI::API::Property &p,       \
				    const run *vals,			\
				    size_t length) const {		\
	setPropertySequence(p.m_info, (const uint8_t *)vals,			\
			    length, length * (bits/8));			\
      }									\
      inline run							\
	get##pretty##Property(unsigned ordinal, unsigned idx) const {	\
	return (run)getProperty##bits(m_propInfo[ordinal], idx);        \
      }									\
      inline unsigned							\
      get##pretty##SequenceProperty(const OCPI::API::Property &p, run *vals, \
				    size_t length) const {		     \
	return								     \
	  getPropertySequence(p.m_info, (uint8_t *)vals, length * (bits/8));	     \
      }
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE
      void setStringProperty(unsigned ordinal, const char* val, unsigned idx) const;
      void setStringSequenceProperty(const OCPI::API::Property &, const char * const *,
				     size_t ) const;
      void getStringProperty(unsigned ordinal, char *val, size_t length, unsigned idx) const;
      unsigned getStringSequenceProperty(const OCPI::API::Property &, char * *,
					 size_t ,char*, size_t) const;
    };
  }
}
#endif
