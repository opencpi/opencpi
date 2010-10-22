
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


#ifndef OCPI_WORKER_H
#define OCPI_WORKER_H

#include "ezxml.h"
#include "OcpiParentChild.h"
#include "OcpiContainerDataTypes.h"
#include "OcpiMetadataWorker.h"
#include <OcpiWciWorker.h>

namespace OCPI {

  namespace Util {
    class EmbeddedException;
  }
  namespace Container {
    // This class is a small module of behavior used by workers, but available for other uses
    class Controllable {
    protected:
      OCPI::Metadata::Worker::ControlState myState;
      uint32_t controlMask;
      Controllable(const char *controlOperations);
    };

    class Application;
    class Port;
    class Property;
    class Worker : public OCPI::Util::Parent<Port>, public OCPI::Util::Child<Application, Worker>,
      public OCPI::Metadata::Worker,  public Controllable, public WCI__worker, public OCPI::WCI::WorkerOO {

      friend class Property;
      friend class Artifact;
      friend class Application;
    protected:
      ezxml_t myXml;
      const char *myImplTag, *myInstTag;

      // Provide hardware-related property information
      Worker(Application &, ezxml_t impl, ezxml_t inst);
      virtual void prepareProperty(Metadata::Property &p, Property &) = 0;
      virtual Port &createPort(OCPI::Metadata::Port &metaport) = 0;

    public:

        /**
           @brief
           getLastControlError

           This method is used to get the last error that occured during a control
           operation.

           @param [ in ] workerId
           Container worker id.

           @retval std::string - last control error

           ****************************************************************** */
      virtual std::string getLastControlError()
        throw ( OCPI::Util::EmbeddedException )=0;


      bool hasImplTag(const char *tag);
      bool hasInstTag(const char *tag);
      typedef unsigned Ordinal;
      // Generic setting method
      void setProperty(const char *name, const char *value);
      // generate the simple-type-specific setting methods
      // this also works fine for strings
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                 \
      virtual void set##pretty##Property(Metadata::Property &,const run) = 0; \
      inline void set##pretty##Property(const char *name, const run val) {    \
        set##pretty##Property(findProperty(name), val);                       \
      }                                                                       \
      virtual void set##pretty##SequenceProperty(Metadata::Property &,        \
						 const run *,	              \
						 unsigned length) = 0;        \
      inline void set##pretty##SequenceProperty(const char *name,             \
						const run* vals,              \
						unsigned length) {	      \
        set##pretty##SequenceProperty(findProperty(name), vals, length);      \
      }
    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
      // generate the simple-type-specific getting methods
      // need a special item for strings
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                    \
      virtual run get##pretty##Property(Metadata::Property &) = 0;	         \
      inline run get##pretty##Property(const char *name) {                       \
        return get##pretty##Property(findProperty(name));                        \
      }                                                                          \
      virtual unsigned get##pretty##SequenceProperty(Metadata::Property&, run *, \
						     unsigned length) = 0;       \
      inline unsigned get##pretty##SequenceProperty(const char *name,            \
						    run* vals,                   \
						    unsigned length) {	         \
        return get##pretty##SequenceProperty(findProperty(name), vals, length);  \
      }
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
      virtual void get##pretty##Property(Metadata::Property &, run,            \
					 unsigned length) = 0;		       \
      inline void get##pretty##Property(const char *name, run val,             \
					unsigned length) {	               \
        get##pretty##Property(findProperty(name), val, length);                \
      }                                                                        \
      virtual unsigned get##pretty##SequenceProperty                           \
        (Metadata::Property &, run *, unsigned length, char *buf,              \
	 unsigned space) = 0;						       \
      inline unsigned get##pretty##SequenceProperty                            \
        (const char *name, run *vals, unsigned length, char *buf,              \
	 unsigned space) {						       \
        return get##pretty##SequenceProperty(findProperty(name), vals, length, \
					     buf, space);		       \
      }

    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE

      Worker(ezxml_t metadata);
      virtual ~Worker();
      virtual Port &getPort(const char *name);



      virtual Port & createOutputPort(PortId portId,
                                     OCPI::OS::uint32_t bufferCount,
                                     OCPI::OS::uint32_t bufferSize, 
                                      OCPI::Util::PValue* props=NULL) 
        throw ( OCPI::Util::EmbeddedException ) = 0;

      virtual Port & createInputPort(PortId portId,
                                     OCPI::OS::uint32_t bufferCount,
                                     OCPI::OS::uint32_t bufferSize, 
                                     OCPI::Util::PValue* props=NULL) 
        throw ( OCPI::Util::EmbeddedException ) = 0;



#define CONTROL_OP(x,c,t,s1,s2,s3) virtual void x() = 0;
OCPI_CONTROL_OPS
#undef CONTROL_OP      
    };
  }
}
#endif



