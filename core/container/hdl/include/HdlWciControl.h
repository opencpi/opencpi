#ifndef HDL_WCI_CONTROL_H
#define HDL_WCI_CONTROL_H
#include "OcpiContainerApi.h"
#include "OcpiWorker.h"
#include "HdlAccess.h"

namespace OCPI {
  namespace HDL {

    // The class that knows about WCI interfaces and the OCCP.
    class Device;
    class WciControl : public Access, virtual public OCPI::Container::Controllable {
      friend class Port;
      friend class Device;
      const char *m_implName, *m_instName;
      mutable size_t m_window; // perfect use-case for mutable..
      bool m_hasControl;
      size_t m_timeout;
    protected:
      Access m_properties;
      Device &m_device;
      // myRegisters is zero when this WCI does not really exist.
      // (since we inherit this in some cases were it is not needed).
      size_t m_occpIndex;
      WciControl(Device &device, const char *impl, const char *inst, unsigned index, bool hasControl);
      WciControl(Device &device, ezxml_t implXml, ezxml_t instXml);
      virtual ~WciControl();

      void init(bool redo);
      // Add the hardware considerations to the property object that supports
      // fast memory-mapped property access directly to users
      // the key members are "readVaddr" and "writeVaddr"
      virtual void prepareProperty(OCPI::Util::Property &md, 
				   volatile void *&writeVaddr,
				   const volatile void *&readVaddr);
      // Map the control op numbers to structure members
      static const unsigned controlOffsets[];
      void controlOperation(OCPI::Metadata::Worker::ControlOperation op);
      bool controlOperation(OCPI::Metadata::Worker::ControlOperation op, std::string &err);
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
      static void throwPropertyReadError(uint32_t status);
      static void throwPropertyWriteError(uint32_t status);
      static void throwPropertySequenceError();

#define PUT_GET_PROPERTY(n)						          \
      void                                                                        \
      setProperty##n(const OCPI::API::PropertyInfo &info, uint##n##_t val) const; \
      inline uint##n##_t						          \
	getProperty##n(const OCPI::API::PropertyInfo &info) const {	          \
        uint32_t offset = checkWindow(info.m_offset, n/8);			  \
	uint32_t status = 0;						          \
	uint##n##_t val;						          \
	if (m_properties.m_registers) {					          \
	  if (!info.m_readError ||					          \
	      !(status =						          \
		get32Register(status, OccpWorkerRegisters) &		          \
		OCCP_STATUS_READ_ERRORS))				          \
	    val = m_properties.get##n##RegisterOffset(offset);                    \
	  if (info.m_readError && !status)				          \
	    status =							          \
	      get32Register(status, OccpWorkerRegisters) &		          \
	      OCCP_STATUS_READ_ERRORS;					          \
	} else								          \
	  val = m_properties.m_accessor->get##n(m_properties.m_base + offset,     \
						&status);                         \
	if (status)							          \
	  throwPropertyReadError(status);				          \
	return val;							          \
      }
      PUT_GET_PROPERTY(8)
      PUT_GET_PROPERTY(16)
      PUT_GET_PROPERTY(32)
      PUT_GET_PROPERTY(64)
#undef PUT_GET_PROPERTY
      void setPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset,
			    const uint8_t *data, size_t nBytes) const;

      void getPropertyBytes(const OCPI::API::PropertyInfo &info, size_t offset, uint8_t *buf,
			    size_t nBytes) const;
      void setPropertySequence(const OCPI::API::Property &p,
			       const uint8_t *val,
			       size_t nItems, size_t nBytes) const;
      unsigned getPropertySequence(const OCPI::API::Property &p, uint8_t *buf, size_t n) const;
      
#undef OCPI_DATA_TYPE_S
      // Set a scalar property value

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)     	         \
      inline void							         \
      set##pretty##Property(const OCPI::API::Property &p, const run val) const { \
	setProperty##bits(p.m_info, *(uint##bits##_t *)&val);		         \
      }									         \
      inline void        						\
      set##pretty##SequenceProperty(const OCPI::API::Property &p,       \
				    const run *vals,			\
				    size_t length) const {		\
	setPropertySequence(p, (const uint8_t *)vals,			\
			    length, length * (bits/8));			\
      }									\
      inline run							\
      get##pretty##Property(const OCPI::API::Property &p) const {	\
	return (run)getProperty##bits(p.m_info);			\
      }									\
      inline unsigned							\
      get##pretty##SequenceProperty(const OCPI::API::Property &p, run *vals, \
				    size_t length) const {		     \
	return								     \
	  getPropertySequence(p, (uint8_t *)vals, length * (bits/8));	     \
      }
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE
      void setStringProperty(const OCPI::API::Property &p, const char* val) const;
      void setStringSequenceProperty(const OCPI::API::Property &, const char * const *,
				     size_t ) const;
      void getStringProperty(const OCPI::API::Property &p, char *val, size_t length) const;
      unsigned getStringSequenceProperty(const OCPI::API::Property &, char * *,
					 size_t ,char*, size_t) const;
    };
  }
}
#endif
