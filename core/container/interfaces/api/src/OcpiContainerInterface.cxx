
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




#include <OcpiContainerInterface.h>
#include <OcpiContainerPort.h>
#include <OcpiRDTInterface.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilCDR.h>
#include <OcpiPortMetaData.h>
#include <OcpiContainerApplication.h>
#include <OcpiArtifact.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include "OcpiDriver.h"


using namespace OCPI::Util;
using namespace OCPI::OS;
using namespace OCPI::RDT;


namespace OCPI {
  namespace Container {




    Application * 
    Interface::
    createApplication()
      throw ( OCPI::Util::EmbeddedException )
    {
      return new Application(*this,NULL,NULL);
    }



    std::string
    packDescriptor (OCPI::Util::CDR::Encoder& packer, const Descriptors & desc)
      throw ()
    {
      const OutOfBandData * oob=NULL;
      packer.putULong (desc.type);
      packer.putULong (desc.role);
      packer.putULong (desc.options);

      switch (desc.type) {
      case ConsumerDescT:
      case ConsumerFlowControlDescT:
      case ProducerDescT:
      default:
        {
          const Desc_t & d = desc.desc;
          oob = &d.oob;
          packer.putULong     (d.nBuffers);
          packer.putULongLong (d.dataBufferBaseAddr);
          packer.putULong     (d.dataBufferPitch);
          packer.putULong     (d.dataBufferSize);
          packer.putULongLong (d.metaDataBaseAddr);
          packer.putULong     (d.metaDataPitch);
          packer.putULongLong (d.fullFlagBaseAddr);
          packer.putULong     (d.fullFlagSize);
          packer.putULong     (d.fullFlagPitch);
          packer.putULongLong (d.fullFlagValue);
          packer.putULongLong (d.emptyFlagBaseAddr);
          packer.putULong     (d.emptyFlagSize);
          packer.putULong     (d.emptyFlagPitch);
          packer.putULongLong (d.emptyFlagValue);
        }
        break;
      }

      /*
       * OutOfBandData is the same for consumer and producer.
       */
      if ( oob ) { 
        packer.putULongLong (oob->port_id);
        packer.putString (oob->oep);
      }
      else {
        return false;
      }
      /*
       * Return marshaled data.
       */

      return packer.data ();
    }

    bool
    unpackDescriptor ( OCPI::Util::CDR::Decoder& unpacker,
                       Descriptors & desc)
      throw ()
    {
      OutOfBandData * oob=NULL;

      try {
        unpacker.getULong (desc.type);
        unpacker.getLong (desc.role);
        unpacker.getULong (desc.options);

        switch (desc.type) {
        case ConsumerDescT:
        case ConsumerFlowControlDescT:
        case ProducerDescT:
        default:
          {
            Desc_t & d = desc.desc;
            oob = &d.oob;
            unpacker.getULong     (d.nBuffers);
            unpacker.getULongLong (d.dataBufferBaseAddr);
            unpacker.getULong     (d.dataBufferPitch);
            unpacker.getULong     (d.dataBufferSize);
            unpacker.getULongLong (d.metaDataBaseAddr);
            unpacker.getULong     (d.metaDataPitch);
            unpacker.getULongLong (d.fullFlagBaseAddr);
            unpacker.getULong     (d.fullFlagSize);
            unpacker.getULong     (d.fullFlagPitch);
            unpacker.getULongLong (d.fullFlagValue);
            unpacker.getULongLong (d.emptyFlagBaseAddr);
            unpacker.getULong     (d.emptyFlagSize);
            unpacker.getULong     (d.emptyFlagPitch);
            unpacker.getULongLong (d.emptyFlagValue);
          }
          break;
        }

        /*
         * OutOfBandData is the same for consumer and producer.
         */

        std::string oep;
        if ( oob ) {
          unpacker.getULongLong (oob->port_id);
          unpacker.getString (oep);
        }
        else {
          return false;
        }

        if (oep.length()+1 > 128) {
          return false;
        }

        std::strncpy (oob->oep, oep.c_str(), 128);
      }
      catch (const OCPI::Util::CDR::Decoder::InvalidData &) {
        return false;
      }

      return true;
    }

    Interface::Interface(OCPI::Util::Driver &driver, const char *name)
      throw ( OCPI::Util::EmbeddedException )
      : OCPI::Util::Device(driver,name), m_name(name)
    {}
    Interface::Interface(OCPI::Util::Driver &driver, const char *name, const OCPI::Util::PValue* props )
      throw ( OCPI::Util::EmbeddedException )
      : OCPI::Util::Device(driver,name), 
        m_name(name)
    {
      (void)props;
    }


    Artifact & Interface::loadArtifact(const char *url, OCPI::Util::PValue *artifactParams) {
      // check whether the url is already loaded - canonicalize? FIXME
      Artifact *a = Parent<Artifact>::findChild(&Artifact::hasUrl, url);
      return a ? *a : createArtifact(url, artifactParams);
    }

    // Ultimately there would be a set of "base class" generic properties
    // and the derived class would merge them.
    // FIXME: define base class properties for all apps
    OCPI::Util::PValue *Interface::getProperties() {
      return 0;
    }
    OCPI::Util::PValue *Interface::getProperty(const char *) {
      return 0;
    }

    Interface::~Interface() throw(){}

  }

}

bool m_start;



/*
 * ----------------------------------------------------------------------
 * A simple test.
 * ----------------------------------------------------------------------
 */
/*
  static int
  pack_unpack_test (int argc, char *argv[])
  {
  OCPI::RDT::Descriptors d;
  std::string data;
  bool good;

  std::memset (&d, 0, sizeof (OCPI::RDT::Descriptors));
  d.mode = OCPI::RDT::ConsumerDescType;
  d.desc.c.fullFlagValue = 42;
  std::strcpy (d.desc.c.oob.oep, "Hello World");
  data = packDescriptor (d);
  std::memset (&d, 0, sizeof (OCPI::RDT::Descriptors));
  good = unpackDescriptor (data, d);
  ocpiAssert (good);
  ocpiAssert (d.mode == OCPI::RDT::ConsumerDescType);
  ocpiAssert (d.desc.c.fullFlagValue == 42);
  ocpiAssert (std::strcmp (d.desc.c.oob.oep, "Hello World") == 0);

  std::memset (&d, 0, sizeof (OCPI::RDT::Descriptors));
  d.mode = OCPI::RDT::ProducerDescType;
  d.desc.p.emptyFlagValue = 42;
  std::strcpy (d.desc.p.oob.oep, "Hello World");
  data = packDescriptor (d);
  std::memset (&d, 0, sizeof (OCPI::RDT::Descriptors));
  good = unpackDescriptor (data, d);
  ocpiAssert (good);
  ocpiAssert (d.mode == OCPI::RDT::ProducerDescType);
  ocpiAssert (d.desc.p.emptyFlagValue == 42);
  ocpiAssert (std::strcmp (d.desc.p.oob.oep, "Hello World") == 0);

  data[0] = ((data[0] == '\0') ? '\1' : '\0'); // Hack: flip byteorder
  good = unpackDescriptor (data, d);
  ocpiAssert (!good);

  return 0;
  }
*/


std::string OCPI::Container::Interface::packPortDesc(  PortData & port  )
  throw()
{

  std::string s;
  OCPI::Util::CDR::Encoder packer;
  packer.putBoolean (OCPI::Util::CDR::nativeByteorder());
  packer.putULong (port.connectionData.container_id);
  packer.putULongLong( port.connectionData.port );
  
  packDescriptor( packer, port.connectionData.data );
  return packer.data();
}


int OCPI::Container::Interface::portDescSize(){return sizeof(PortData);}

OCPI::Container::PortData * OCPI::Container::Interface::unpackPortDesc( const std::string& data, PortData* port )
  throw ()
{
  OCPI::Util::CDR::Decoder unpacker (data);
  Descriptors *desc = &port->connectionData.data;
  bool bo;

  try { 
    unpacker.getBoolean (bo);
    unpacker.byteorder (bo);
    unpacker.getULong (port->connectionData.container_id);
    unpacker.getULongLong ((uint64_t&)port->connectionData.port);
    bool good = unpackDescriptor ( unpacker, *desc);
    if ( ! good ) {
      return 0;
    }

  }
  catch (const OCPI::Util::CDR::Decoder::InvalidData &) {
    return false;
  }

  return port;
}



void OCPI::Container::Interface::start(DataTransfer::EventManager* event_manager)
  throw()
{
  (void)event_manager;
  m_start = true;
}

void OCPI::Container::Interface::stop(DataTransfer::EventManager* event_manager)
  throw()
{
  (void)event_manager;
  m_start = false;
}


std::vector<std::string> 
OCPI::Container::Interface::
getSupportedEndpoints()
  throw ()
{
  std::vector<std::string> l;
  return l;
}
