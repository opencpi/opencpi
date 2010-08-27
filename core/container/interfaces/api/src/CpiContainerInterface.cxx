


#include <CpiContainerInterface.h>
#include <CpiContainerPort.h>
#include <CpiRDTInterface.h>
#include <CpiOsAssert.h>
#include <CpiUtilCDR.h>
#include <CpiPortMetaData.h>
#include <CpiApplication.h>
#include <CpiArtifact.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include "CpiDriver.h"


using namespace CPI::Util;
using namespace CPI::OS;
using namespace CPI::RDT;


namespace CPI {
  namespace Container {




    Application * 
    Interface::
    createApplication()
      throw ( CPI::Util::EmbeddedException )
    {
      return new Application(*this,NULL,NULL);
    }



    std::string
    packDescriptor (CPI::Util::CDR::Encoder& packer, const Descriptors & desc)
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
    unpackDescriptor ( CPI::Util::CDR::Decoder& unpacker,
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
      catch (const CPI::Util::CDR::Decoder::InvalidData &) {
        return false;
      }

      return true;
    }

    Interface::Interface(CPI::Util::Driver &driver, const char *name)
      throw ( CPI::Util::EmbeddedException )
      : CPI::Util::Device(driver,name), m_name(name)
    {}
    Interface::Interface(CPI::Util::Driver &driver, const char *name, const CPI::Util::PValue* props )
      throw ( CPI::Util::EmbeddedException )
      : CPI::Util::Device(driver,name), 
        m_name(name)
    {
      (void)props;
    }


    Artifact & Interface::loadArtifact(const char *url, CPI::Util::PValue *artifactParams) {
      // check whether the url is already loaded - canonicalize? FIXME
      Artifact *a = Parent<Artifact>::findChild(&Artifact::hasUrl, url);
      return a ? *a : createArtifact(url, artifactParams);
    }

    // Ultimately there would be a set of "base class" generic properties
    // and the derived class would merge them.
    // FIXME: define base class properties for all apps
    CPI::Util::PValue *Interface::getProperties() {
      return 0;
    }
    CPI::Util::PValue *Interface::getProperty(const char *) {
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
  CPI::RDT::Descriptors d;
  std::string data;
  bool good;

  std::memset (&d, 0, sizeof (CPI::RDT::Descriptors));
  d.mode = CPI::RDT::ConsumerDescType;
  d.desc.c.fullFlagValue = 42;
  std::strcpy (d.desc.c.oob.oep, "Hello World");
  data = packDescriptor (d);
  std::memset (&d, 0, sizeof (CPI::RDT::Descriptors));
  good = unpackDescriptor (data, d);
  cpiAssert (good);
  cpiAssert (d.mode == CPI::RDT::ConsumerDescType);
  cpiAssert (d.desc.c.fullFlagValue == 42);
  cpiAssert (std::strcmp (d.desc.c.oob.oep, "Hello World") == 0);

  std::memset (&d, 0, sizeof (CPI::RDT::Descriptors));
  d.mode = CPI::RDT::ProducerDescType;
  d.desc.p.emptyFlagValue = 42;
  std::strcpy (d.desc.p.oob.oep, "Hello World");
  data = packDescriptor (d);
  std::memset (&d, 0, sizeof (CPI::RDT::Descriptors));
  good = unpackDescriptor (data, d);
  cpiAssert (good);
  cpiAssert (d.mode == CPI::RDT::ProducerDescType);
  cpiAssert (d.desc.p.emptyFlagValue == 42);
  cpiAssert (std::strcmp (d.desc.p.oob.oep, "Hello World") == 0);

  data[0] = ((data[0] == '\0') ? '\1' : '\0'); // Hack: flip byteorder
  good = unpackDescriptor (data, d);
  cpiAssert (!good);

  return 0;
  }
*/


std::string CPI::Container::Interface::packPortDesc(  PortData & port  )
  throw()
{

  std::string s;
  CPI::Util::CDR::Encoder packer;
  packer.putBoolean (CPI::Util::CDR::nativeByteorder());
  packer.putULong (port.connectionData.container_id);
  packer.putULongLong( port.connectionData.port );
  
  packDescriptor( packer, port.connectionData.data );
  return packer.data();
}


int CPI::Container::Interface::portDescSize(){return sizeof(PortData);}

CPI::Container::PortData * CPI::Container::Interface::unpackPortDesc( const std::string& data, PortData* port )
  throw ()
{
  CPI::Util::CDR::Decoder unpacker (data);
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
  catch (const CPI::Util::CDR::Decoder::InvalidData &) {
    return false;
  }

  return port;
}



void CPI::Container::Interface::start(DataTransfer::EventManager* event_manager)
  throw()
{
  (void)event_manager;
  m_start = true;
}

void CPI::Container::Interface::stop(DataTransfer::EventManager* event_manager)
  throw()
{
  (void)event_manager;
  m_start = false;
}


std::vector<std::string> 
CPI::Container::Interface::
getSupportedEndpoints()
  throw ()
{
  std::vector<std::string> l;
  return l;
}
