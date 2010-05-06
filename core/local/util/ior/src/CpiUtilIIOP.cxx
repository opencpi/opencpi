#include <CpiOsAssert.h>
#include <string>
#include <vector>
#include "CpiUtilCDR.h"
#include "CpiUtilIOP.h"
#include "CpiUtilIIOP.h"

/*
 * ----------------------------------------------------------------------
 * IIOP::ProfileBody
 * ----------------------------------------------------------------------
 */

CPI::Util::IIOP::ProfileBody::
ProfileBody ()
  throw ()
{
}

CPI::Util::IIOP::ProfileBody::
ProfileBody (const std::string & data)
  throw (std::string)
{
  decode (data);
}

CPI::Util::IIOP::ProfileBody::
ProfileBody (const ProfileBody & other)
  throw ()
  : iiop_version (other.iiop_version),
    host (other.host),
    port (other.port),
    object_key (other.object_key),
    components (other.components)
{
}

CPI::Util::IIOP::ProfileBody &
CPI::Util::IIOP::ProfileBody::
operator= (const ProfileBody & other)
  throw ()
{
  iiop_version = other.iiop_version;
  host = other.host;
  port = other.port;
  object_key = other.object_key;
  components = other.components;
  return *this;
}

void
CPI::Util::IIOP::ProfileBody::
decode (const std::string & data)
  throw (std::string)
{
  try {
    CPI::Util::CDR::Decoder cd (data);

    bool bo;
    cd.getBoolean (bo);
    cd.byteorder (bo);

    cd.getOctet (iiop_version.major);
    cd.getOctet (iiop_version.minor);

    if (iiop_version.major != 1 ||
        iiop_version.minor > 3) {
      throw std::string ("unknown IIOP version");
    }

    cd.getString (host);
    cd.getUShort (port);
    cd.getOctetSeq (object_key);

    if (iiop_version.minor >= 1) {
      CPI::OS::uint32_t numComponents;
      cd.getULong (numComponents);

      if (numComponents*8 > cd.remainingData()) {
        throw CPI::Util::CDR::Decoder::InvalidData();
      }

      components.resize (numComponents);
      
      for (CPI::OS::uint32_t pi=0; pi<numComponents; pi++) {
        cd.getULong (components[pi].tag);
        cd.getOctetSeq (components[pi].component_data);
      }
    }
    else {
      components.clear ();
    }
  }
  catch (const CPI::Util::CDR::Decoder::InvalidData &) {
    iiop_version.major = iiop_version.minor = 0;
    host = object_key = "";
    port = 0;
    components.clear ();
    throw std::string ("invalid data");
  }
}

std::string
CPI::Util::IIOP::ProfileBody::
encode () const
  throw (std::string)
{
  if (iiop_version.major != 1 ||
      iiop_version.minor > 3) {
    throw std::string ("unknown IIOP version");
  }

  CPI::Util::CDR::Encoder ce;

  bool bo = CPI::Util::CDR::nativeByteorder ();
  ce.putBoolean (bo);

  ce.putOctet (iiop_version.major);
  ce.putOctet (iiop_version.minor);

  ce.putString (host);
  ce.putUShort (port);
  ce.putOctetSeq (object_key);

  if (iiop_version.minor >= 1) {
    unsigned long numComponents = components.size ();
    ce.putULong (numComponents);

    for (unsigned long pi=0; pi<numComponents; pi++) {
      ce.putULong (components[pi].tag);
      ce.putOctetSeq (components[pi].component_data);
    }
  }

  return ce.data ();
}

void
CPI::Util::IIOP::ProfileBody::
addComponent (CPI::Util::IOP::ComponentId tag, const void * data, unsigned long len)
  throw ()
{
  unsigned long numComponents = components.size ();
  components.resize (numComponents+1);
  components[numComponents].tag = tag;
  components[numComponents].component_data.assign (reinterpret_cast<const char *> (data), len);
}

void
CPI::Util::IIOP::ProfileBody::
addComponent (CPI::Util::IOP::ComponentId tag, const std::string & data)
  throw ()
{
  addComponent (tag, data.data(), data.length());
}

bool
CPI::Util::IIOP::ProfileBody::
hasComponent (CPI::Util::IOP::ComponentId tag)
  throw ()
{
  unsigned long numComponents = components.size ();

  for (unsigned long pi=0; pi<numComponents; pi++) {
    if (components[pi].tag == tag) {
      return true;
    }
  }

  return false;
}

std::string &
CPI::Util::IIOP::ProfileBody::
componentData (CPI::Util::IOP::ComponentId tag)
  throw (std::string)
{
  unsigned long numComponents = components.size ();
  unsigned long pi;

  for (pi=0; pi<numComponents; pi++) {
    if (components[pi].tag == tag) {
      break;
    }
  }

  if (pi >= numComponents) {
    throw std::string ("tag not found");
  }

  return components[pi].component_data;
}

const std::string &
CPI::Util::IIOP::ProfileBody::
componentData (CPI::Util::IOP::ComponentId tag) const
  throw (std::string)
{
  unsigned long numComponents = components.size ();
  unsigned long pi;

  for (pi=0; pi<numComponents; pi++) {
    if (components[pi].tag == tag) {
      break;
    }
  }

  if (pi >= numComponents) {
    throw std::string ("tag not found");
  }

  return components[pi].component_data;
}

unsigned long
CPI::Util::IIOP::ProfileBody::
numComponents () const
  throw ()
{
  return components.size ();
}

CPI::Util::IOP::TaggedComponent &
CPI::Util::IIOP::ProfileBody::
getComponent (unsigned long idx)
  throw (std::string)
{
  if (idx >= components.size()) {
    throw std::string ("invalid index");
  }

  return components[idx];
}

const CPI::Util::IOP::TaggedComponent &
CPI::Util::IIOP::ProfileBody::
getComponent (unsigned long idx) const
  throw (std::string)
{
  if (idx >= components.size()) {
    throw std::string ("invalid index");
  }

  return components[idx];
}
