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

#ifndef HDL_WCI_CONTROL_H
#define HDL_WCI_CONTROL_H

#include "ContainerWorker.h"
#include "HdlOCCP.h"
#include "XferAccess.h"

namespace OCPI {
  namespace HDL {

    // The class that knows about WCI interfaces and the OCCP.
    class Device;
    typedef DataTransfer::Access Access;
    typedef DataTransfer::Accessor Accessor;
    typedef DataTransfer::RegisterOffset RegisterOffset;
    class WciControl : public Access, virtual public OCPI::Container::Controllable,
      virtual public OCPI::API::PropertyAccess, virtual OCPI::Container::WorkerControl {
      
      friend class Port;
      friend class Device;
      friend class Container;
      friend class Artifact;
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
      inline size_t index() const { return m_occpIndex; }
    protected:
      // This is shadowed by real application workers, but is used when this is 
      // standalone.
      //      const std::string &name() const { return m_wName; }
      void init(bool redo, bool doInit);
      bool isReset() const;
      void propertyWritten(unsigned ordinal) const;
      void propertyRead(unsigned ordinal) const;
      // Add the hardware considerations to the property object that supports
      // fast memory-mapped property access directly to users
      // the key members are "readVaddr" and "writeVaddr"
      virtual void prepareProperty(OCPI::Util::Property &md, 
				   volatile uint8_t *&writeVaddr,
				   const volatile uint8_t *&readVaddr);
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
      setProperty##n(const OCPI::API::PropertyInfo &info, size_t offset, uint##n##_t val, \
		     unsigned idx) const;				          \
      inline uint##n##_t						          \
      getProperty##n(const OCPI::API::PropertyInfo &info, size_t off, unsigned idx) const { \
        uint32_t offset = checkWindow(info.m_offset + off + idx * (n/8), n/8); \
	uint32_t status = 0;                                                      \
        uint##wb##_t val##wb;							  \
	uint##n##_t val;						          \
	if (m_properties.registers()) {					          \
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
	     m_properties.accessor()->get64(m_properties.base() + offset, &status) : \
	     m_properties.accessor()->get(m_properties.base() + offset,	sizeof(uint##n##_t), \
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

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
      void								\
      set##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member *, \
			    size_t off, const run val, unsigned idx) const { \
	setProperty##bits(info, off, *(uint##bits##_t *)&val, idx);	\
      }									\
      void								\
      set##pretty##SequenceProperty(const OCPI::API::Property &p,       \
				    const run *vals,			\
				    size_t length) const {		\
	setPropertySequence(p.m_info, (const uint8_t *)vals,			\
			    length, length * (bits/8));			\
      }									\
      run								\
      get##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member *, \
			    size_t offset, unsigned idx) const {	\
	return (run)getProperty##bits(info, offset, idx);		\
      }									\
      unsigned								\
      get##pretty##SequenceProperty(const OCPI::API::Property &p, run *vals, \
				    size_t length) const {		\
	return								     \
	  getPropertySequence(p.m_info, (uint8_t *)vals, length * (bits/8));	     \
      }
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE
      void setStringProperty(const OCPI::API::PropertyInfo &info, const Util::Member *,
			     size_t offset, const char* val, unsigned idx) const;
      void setStringSequenceProperty(const OCPI::API::Property &, const char * const *,
				     size_t ) const;
      void getStringProperty(const OCPI::API::PropertyInfo &info, const Util::Member *,
			     size_t offset, char *val, size_t length, unsigned idx) const;
      unsigned getStringSequenceProperty(const OCPI::API::Property &, char * *,
					 size_t ,char*, size_t) const;
    };
    // This is a dummy worker for accesssing workers outside the perview of executing
    // applications - used by ocpihdl etc.
    struct DirectWorker : public OCPI::Container::Worker, public WciControl {
      std::string m_name, m_wName;
      Access &m_wAccess;
      unsigned m_timeout;
      DirectWorker(Device &dev, const Access &cAccess, Access &wAccess, ezxml_t impl,
		   ezxml_t inst, const char *idx, unsigned timeout);
      virtual void control(const char *op); // virtual due to driver access
      virtual void status(); // virtual due to driver access
      OCPI::Container::Port *findPort(const char *);
      const std::string &name() const;
      void
      prepareProperty(OCPI::Util::Property &, volatile uint8_t *&, const volatile uint8_t *&);
      OCPI::Container::Port &
      createPort(const OCPI::Util::Port &, const OCPI::Util::PValue *);
      OCPI::Container::Port &
      createOutputPort(OCPI::Util::PortOrdinal, size_t, size_t, const OCPI::Util::PValue*)
        throw (OCPI::Util::EmbeddedException);
      OCPI::Container::Port &
      createInputPort(OCPI::Util::PortOrdinal, size_t, size_t, const OCPI::Util::PValue*)
        throw (OCPI::Util::EmbeddedException);
      OCPI::Container::Application *application();
      OCPI::Container::Worker *nextWorker();
      void read(size_t, size_t, void *);
      void write(size_t, size_t, const void *);
    };
  }
}
#endif
