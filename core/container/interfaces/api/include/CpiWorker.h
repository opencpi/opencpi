// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

#ifndef CPI_WORKER_H
#define CPI_WORKER_H

#include "ezxml.h"
#include "CpiParentChild.h"
#include "CpiContainerDataTypes.h"
#include "CpiMetadataWorker.h"
#include <CpiWciWorker.h>

namespace CPI {

  namespace Util {
    class EmbeddedException;
  }

  namespace Container {
    class Application;
    class Port;
    class Property;
    class Worker : public CPI::Util::Parent<Port>, public CPI::Util::Child<Application, Worker>,
      public CPI::Metadata::Worker,  public WCI__worker, public CPI::WCI::WorkerOO {

      friend class Property;
      friend class Artifact;
      friend class Application;
    protected:
      ezxml_t myXml;
      const char *myImplTag, *myInstTag;
      Worker();
      Worker( Application & );
      // Provide hardware-related property information
      Worker(Application &, ezxml_t impl, ezxml_t inst);
      virtual void prepareProperty(Metadata::Property &p, Property &) = 0;
      virtual Port &createPort(CPI::Metadata::Port &metaport) = 0;

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
	throw ( CPI::Util::EmbeddedException )=0;


      bool hasImplTag(const char *tag);
      bool hasInstTag(const char *tag);
      typedef unsigned Ordinal;
      // generate the simple-type-specific setting methods
      // this also works fine for strings
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
      virtual void set##pretty##Property(Ordinal,const run) = 0;	\
      inline void set##pretty##Property(const char *name, const run val) { \
	set##pretty##Property(whichProperty(name), val);		\
      }									\
      virtual void set##pretty##SequenceProperty(Ordinal,const run *, unsigned length) = 0; \
      inline void set##pretty##SequenceProperty(const char *name, const run* vals, unsigned length) { \
	set##pretty##SequenceProperty(whichProperty(name), vals, length);	\
      }
    CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
#undef CPI_DATA_TYPE_S
      // generate the simple-type-specific getting methods
      // need a special item for strings
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
      virtual run get##pretty##Property(Ordinal ord) = 0;		\
      inline run get##pretty##Property(const char *name) {		\
	return get##pretty##Property(whichProperty(name));		\
      }									\
      virtual unsigned get##pretty##SequenceProperty(Ordinal, run *, unsigned length) = 0; \
      inline unsigned get##pretty##SequenceProperty(const char *name,  run* vals, unsigned length) { \
	return get##pretty##SequenceProperty(whichProperty(name), vals, length); \
      }
#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)		\
      virtual void get##pretty##Property(Ordinal ord, run, unsigned length) = 0;		\
      inline void get##pretty##Property(const char *name, run val, unsigned length) {	\
	get##pretty##Property(whichProperty(name), val, length);	\
      }									\
      virtual unsigned get##pretty##SequenceProperty \
	(Ordinal ord, run *, unsigned length, char *buf, unsigned space) = 0; \
      inline unsigned get##pretty##SequenceProperty			\
	(const char *name, run *vals, unsigned length, char *buf, unsigned space) {	\
	return get##pretty##SequenceProperty(whichProperty(name), vals, length, buf, space); \
      }

    CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
#undef CPI_DATA_TYPE_S
#define CPI_DATA_TYPE_S CPI_DATA_TYPE

      Worker(ezxml_t metadata);
      virtual ~Worker();
      virtual Port &getPort(const char *name);



      virtual Port & createOutputPort(PortId portId,
				     CPI::OS::uint32_t bufferCount,
				     CPI::OS::uint32_t bufferSize, 
				      CPI::Util::PValue* props=NULL) 
	throw ( CPI::Util::EmbeddedException ) = 0;

      virtual Port & createInputPort(PortId portId,
				     CPI::OS::uint32_t bufferCount,
				     CPI::OS::uint32_t bufferSize, 
				     CPI::Util::PValue* props=NULL) 
	throw ( CPI::Util::EmbeddedException ) = 0;



#define CONTROL_OP(x,c,t,s1,s2,s3) virtual void x() = 0;
CPI_CONTROL_OPS
#undef CONTROL_OP      
    };
  }
}
#endif



