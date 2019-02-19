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

#include <string>
#include <cstring>
#include <vector>
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilCDR.h"
#include "OcpiUtilIOP.h"
#include "OcpiUtilIIOP.h"
#include "OcpiCorbaApi.h"
/*
 * ----------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------
 */

namespace OU = OCPI::Util;
namespace OCPI {
  namespace Util {
    namespace IOP {
      const ProfileId TAG_INTERNET_IOP = 0;
      const ProfileId TAG_MULTIPLE_COMPONENTS = 1;

      const ComponentId TAG_ORB_TYPE = 0;
      const ComponentId TAG_CODE_SETS = 1;
      const ComponentId TAG_POLICIES = 2;
      const ComponentId TAG_ALTERNATE_IIOP_ADDRESS = 3;
      const ComponentId TAG_OMNIORB_UNIX_TRANS = 0x41545402;
      const ComponentId TAG_OMNIORB_PERSISTENT_ID = 0x41545403;
      const ComponentId TAG_OMNIORB_RESTRICTED_CONNECTION = 0x41545404;
      const ComponentId TAG_OMNIORB_OCPI_TRANS = 0x41545405;

/*
 * ----------------------------------------------------------------------
 * Convert from and to hexadecimal
 * ----------------------------------------------------------------------
 */

namespace {

  /*
   * -1: invalid hexadecimal character.
   * -2: ignore (tab, lf, cr, space).
   * otherwise: the hexadecimal value.
   */

  const int hc2i[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -1, -1, -2, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
  };

  bool
  hex2blob (std::string & res, const char * data, unsigned long length)
    throw ()
  {
    const unsigned char * src = reinterpret_cast <const unsigned char *> (data);
    unsigned char * buf = new unsigned char [length/2];
    unsigned char * ptr = buf;
    unsigned long rl = 0;

    while (length) {
      while (length && hc2i[*src] == -2) {
        src++;
        length--;
      }

      if (!length) {
        break;
      }
      else if (hc2i[*src] == -1) {
        delete [] buf;
        return false;
      }

      unsigned char h1 = *src++;
      length--;

      while (length && hc2i[*src] == -2) {
        src++;
        length--;
      }

      if (!length || hc2i[*src] == -1) {
        delete [] buf;
        return false;
      }

      unsigned char h2 = *src++;
      length--;

      *ptr++ = (unsigned char)((hc2i[h1] << 4) | hc2i[h2]);
      rl++;
    }

    res.assign (reinterpret_cast<char *> (buf), rl);
    delete [] buf;
    return true;
  }

  const char i2hc[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
  };

  std::string
  blob2hex (const char * data, unsigned long length)
    throw ()
  {
    const unsigned char * src = reinterpret_cast <const unsigned char *> (data);
    unsigned char * buf = new unsigned char [length*2];
    unsigned char * ptr = buf;
    unsigned long olen = length;

    while (length) {
      *ptr++ = i2hc[*src>>4];
      *ptr++ = i2hc[*src&15];
      length--;
      src++;
    }

    std::string res (reinterpret_cast<char *> (buf), olen*2);
    delete [] buf;
    return res;
  }
}

/*
 * ----------------------------------------------------------------------
 * IOR class
 * ----------------------------------------------------------------------
 */

IOR::
IOR ()
  throw ()
{
}

IOR::
IOR (const std::string & data)
  throw (std::string)
{
  decode (data);
}

// return false on error
static bool
decode_ior_string(const char *ior, std::string &out) {
  while (*ior && (*ior == ' ' || *ior == '\t' ||
                 *ior == '\r' || *ior == '\n'))
    ior++;

  if (!*ior ||
      (ior[0] != 'I' && ior[0] != 'i') ||
      (ior[1] != 'O' && ior[1] != 'o') ||
      (ior[2] != 'R' && ior[2] != 'r') ||
      ior[3] != ':')
    return true;
  ior += 4;
  return hex2blob(out, ior, strlen(ior));
}

IOR::
IOR (const char *uri)
  throw (std::string)
{
  std::string data;
  if (!decode_ior_string(uri, data))
    throw  std::string("Cannot parse stringified IOR");
  decode(data);
}

IOR::
IOR (const IOR & other)
  throw ()
  : m_type_id (other.m_type_id),
    m_profiles (other.m_profiles)
{
}

IOR &
IOR::
operator= (const IOR & other)
  throw ()
{
  m_type_id = other.m_type_id;
  m_profiles = other.m_profiles;
  return *this;
}

void
IOR::
decode (const std::string & data)
  throw (std::string)
{
  try {
    OCPI::Util::CDR::Decoder cd (data);

    bool bo;
    cd.getBoolean (bo);
    cd.byteorder (bo);

    cd.getString (m_type_id);

    OCPI::OS::uint32_t numProfs;
    cd.getULong (numProfs);

    if (numProfs*8 > cd.remainingData()) {
      throw OCPI::Util::CDR::Decoder::InvalidData();
    }

    m_profiles.resize (numProfs);

    for (OCPI::OS::uint32_t pi=0; pi<numProfs; pi++) {
      cd.getULong (m_profiles[pi].tag);
      cd.getOctetSeq (m_profiles[pi].profile_data);
    }
  }
  catch (const OCPI::Util::CDR::Decoder::InvalidData &) {
    m_type_id.clear ();
    m_profiles.clear ();
    throw std::string ("invalid data");
  }
}

std::string
IOR::
encode () const
  throw ()
{
  OCPI::Util::CDR::Encoder ce;

  bool bo = OCPI::Util::CDR::nativeByteorder ();
  ce.putBoolean (bo);

  ce.putString (m_type_id);

  size_t numProfs = m_profiles.size ();
  ce.putULong ((uint32_t)numProfs);

  for (unsigned long pi=0; pi<numProfs; pi++) {
    ce.putULong (m_profiles[pi].tag);
    ce.putOctetSeq (m_profiles[pi].profile_data);
  }

  return ce.data ();
}

const std::string &
IOR::
type_id () const
  throw ()
{
  return m_type_id;
}

void
IOR::
type_id (const std::string & id)
  throw ()
{
  m_type_id = id;
}

void
IOR::
addProfile (ProfileId tag, const void * data, unsigned long len)
  throw ()
{
  unsigned long numProfs = m_profiles.size ();
  m_profiles.resize (numProfs+1);
  m_profiles[numProfs].tag = tag;
  m_profiles[numProfs].profile_data.assign (reinterpret_cast<const char *> (data), len);
}

void
IOR::
addProfile (ProfileId tag, const std::string & data)
  throw ()
{
  addProfile (tag, data.data(), data.length());
}

bool
IOR::
hasProfile (ProfileId tag)
  throw ()
{
  unsigned long numProfs = m_profiles.size ();

  for (unsigned long pi=0; pi<numProfs; pi++) {
    if (m_profiles[pi].tag == tag) {
      return true;
    }
  }

  return false;
}

std::string &
IOR::
profileData (ProfileId tag)
  throw (std::string)
{
  unsigned long numProfs = m_profiles.size ();
  unsigned long pi;

  for (pi=0; pi<numProfs; pi++) {
    if (m_profiles[pi].tag == tag) {
      break;
    }
  }

  if (pi >= numProfs) {
    throw std::string ("tag not found");
  }

  return m_profiles[pi].profile_data;
}

const std::string &
IOR::
profileData (ProfileId tag) const
  throw (std::string)
{
  unsigned long numProfs = m_profiles.size ();
  unsigned long pi;

  for (pi=0; pi<numProfs; pi++) {
    if (m_profiles[pi].tag == tag) {
      break;
    }
  }

  if (pi >= numProfs) {
    throw std::string ("tag not found");
  }

  return m_profiles[pi].profile_data;
}

unsigned long
IOR::
numProfiles () const
  throw ()
{
  return m_profiles.size ();
}

TaggedProfile &
IOR::
getProfile (unsigned long idx)
  throw (std::string)
{
  if (idx >= m_profiles.size()) {
    throw std::string ("invalid index");
  }

  return m_profiles[idx];
}

const TaggedProfile &
IOR::
getProfile (unsigned long idx) const
  throw (std::string)
{
  if (idx >= m_profiles.size()) {
    throw std::string ("invalid index");
  }

  return m_profiles[idx];
}

void IOR::
doAddress(const std::string &addr) throw () {
  m_corbaloc += m_corbaloc.empty() ? "corbaloc:" : ",";
  m_corbaloc += addr;
}

void IOR::
doIPAddress(unsigned major, unsigned minor, const std::string &host, uint16_t port)
  throw()
{
  std::string addr;
  OU::formatString(addr, "iiop:%u.%u@%s:%u", major, minor, host.c_str(), port);
  doAddress(addr);
}


void IOR::
doComponent(TaggedComponent &tc) throw(std::string) {
  switch (tc.tag) {
  case TAG_ALTERNATE_IIOP_ADDRESS:
    {
      AlternateIIOPAddressComponent aiac(tc.component_data);
      doIPAddress(1, 2, aiac.HostID, aiac.port);
    }
    break;
  case TAG_OMNIORB_UNIX_TRANS:
    {
      OCPI::Util::CDR::Decoder cd (tc.component_data);

      bool bo;
      std::string host, fileName;
      cd.getBoolean(bo);
      cd.byteorder(bo);
      cd.getString(host);
      cd.getString(fileName);
      std::string addr("omniunix:");
      addr += fileName;
      doAddress(addr);
    }
    break;
  case TAG_OMNIORB_OCPI_TRANS:
    {
      OCPI::Util::CDR::Decoder cd (tc.component_data);

      bool bo;
      std::string endpoint;
      cd.getBoolean(bo);
      cd.byteorder(bo);
      cd.getString(endpoint);
      std::string addr("omniocpi:");
      addr += endpoint;
      doAddress(addr);
    }
    break;
  default:
    ocpiWeird("Unknown/ignored IOR tag: %d", tc.tag);
  }
}

void IOR::
doKey(const std::string &key) throw() {
  m_corbaloc += "/";
  const char *cp = key.data();
  ocpiDebug("key length is %zu", key.length());
  for (size_t len = key.length(); len; len--, cp++)
    if (isalnum(*cp))
      m_corbaloc += *cp;
    else switch (*cp) {
      case ';': case '/': case ':': case '?': case '.': case '@': case '&':
      case '=': case '+': case '$': case ',': case '-': case '_': case '!':
      case '~': case '*': case '(': case ')': // case '\'': we escape this to make things better for shells etc.
	m_corbaloc += *cp;
	break;
      default:
	m_corbaloc += '%';
	m_corbaloc += i2hc[(*cp >> 4) & 0xf];
	m_corbaloc += i2hc[*cp & 0xf];
      }
}

// We only know IIOP and our own protocols
const std::string &IOR::
corbaloc() throw (std::string) {
  if (m_corbaloc.empty()) {
    bool key = false;
    // Now do all addresses
    for (unsigned n = 0; n < m_profiles.size(); n++) {
      TaggedProfile &tp = m_profiles[n];
      switch (tp.tag) {
      case TAG_INTERNET_IOP:
	{
	  IIOP::ProfileBody iiop(tp.profile_data);
	  doIPAddress(iiop.iiop_version.major, iiop.iiop_version.minor,
		      iiop.host, iiop.port);
	  for (unsigned c = 0; c < iiop.numComponents(); c++)
	    doComponent(iiop.getComponent(c));
	  key = true;
	  doKey(iiop.object_key);
	}
	break;
      case TAG_MULTIPLE_COMPONENTS:
	{
	  MultipleComponentProfile mcp(tp.profile_data);
	  for (unsigned c = 0; c < mcp.numComponents(); c++)
	    doComponent(mcp.getComponent(c));
	}
	break;
      default:
	// ignore
	;
      }
    }
    if (!key)
      throw std::string("No object key found in IOR");
  }
  return m_corbaloc;
}

/*
 * ----------------------------------------------------------------------
 * Convert between an IOR and "IOR:..." hexadecimal strings.
 * ----------------------------------------------------------------------
 */



IOR

string_to_ior (const std::string & ior)
  throw (std::string)
{
#if 0
  const char * ptr = ior.data ();
  unsigned long len = ior.length ();

  /*
   * Ignore leading whitespace.
   */

  while (len && (*ptr == ' ' || *ptr == '\t' ||
                 *ptr == '\r' || *ptr == '\n')) {
    ptr++;
    len--;
  }

  if (len < 4 ||
      (ptr[0] != 'I' && ptr[0] != 'i') ||
      (ptr[1] != 'O' && ptr[1] != 'o') ||
      (ptr[2] != 'R' && ptr[2] != 'r') ||
      ptr[3] != ':') {
    throw std::string ("not an ior");
  }

#endif
  std::string blob;

  if (decode_ior_string(ior.c_str(), blob)) {
    throw std::string ("can not read ior string");
  }

  return IOR (blob);
}

std::string

ior_to_string (const IOR & ior)
  throw ()
{
  std::string blob = ior.encode ();
  std::string hex = blob2hex (blob.data(), blob.length());
  std::string res = "IOR:";
  res += hex;
  return res;
}

/*
 * ----------------------------------------------------------------------
 * MultipleComponentProfile
 * ----------------------------------------------------------------------
 */

MultipleComponentProfile::
MultipleComponentProfile ()
  throw ()
{
}

MultipleComponentProfile::
MultipleComponentProfile (const std::string & data)
  throw (std::string)
{
  decode (data);
}

MultipleComponentProfile::
MultipleComponentProfile (const MultipleComponentProfile & other)
  throw ()
  : m_components (other.m_components)
{
}

MultipleComponentProfile &
MultipleComponentProfile::
operator= (const MultipleComponentProfile & other)
  throw ()
{
  m_components = other.m_components;
  return *this;
}

void
MultipleComponentProfile::
decode (const std::string & data)
  throw (std::string)
{
  try {
    OCPI::Util::CDR::Decoder cd (data);

    bool bo;
    cd.getBoolean (bo);
    cd.byteorder (bo);

    OCPI::OS::uint32_t numComps;
    cd.getULong (numComps);

    if (numComps*8 > cd.remainingData()) {
      throw OCPI::Util::CDR::Decoder::InvalidData();
    }

    m_components.resize (numComps);

    for (OCPI::OS::uint32_t pi=0; pi<numComps; pi++) {
      cd.getULong (m_components[pi].tag);
      cd.getOctetSeq (m_components[pi].component_data);
    }
  }
  catch (const OCPI::Util::CDR::Decoder::InvalidData &) {
    m_components.clear ();
    throw std::string ("invalid data");
  }
}

std::string
MultipleComponentProfile::
encode () const
  throw ()
{
  OCPI::Util::CDR::Encoder ce;

  bool bo = OCPI::Util::CDR::nativeByteorder ();
  ce.putBoolean (bo);

  size_t numComps = m_components.size ();
  ce.putULong ((uint32_t)numComps);

  for (unsigned long pi=0; pi<numComps; pi++) {
    ce.putULong (m_components[pi].tag);
    ce.putOctetSeq (m_components[pi].component_data);
  }

  return ce.data ();
}

void
MultipleComponentProfile::
addComponent (ComponentId tag, const void * data, unsigned long len)
  throw ()
{
  unsigned long numComps = m_components.size ();
  m_components.resize (numComps+1);
  m_components[numComps].tag = tag;
  m_components[numComps].component_data.assign (reinterpret_cast<const char *> (data), len);
}

void
MultipleComponentProfile::
addComponent (ComponentId tag, const std::string & data)
  throw ()
{
  addComponent (tag, data.data(), data.length());
}

bool
MultipleComponentProfile::
hasComponent (ComponentId tag)
  throw ()
{
  unsigned long numComps = m_components.size ();

  for (unsigned long pi=0; pi<numComps; pi++) {
    if (m_components[pi].tag == tag) {
      return true;
    }
  }

  return false;
}

std::string &
MultipleComponentProfile::
componentData (ComponentId tag)
  throw (std::string)
{
  unsigned long numComps = m_components.size ();
  unsigned long pi;

  for (pi=0; pi<numComps; pi++) {
    if (m_components[pi].tag == tag) {
      break;
    }
  }

  if (pi >= numComps) {
    throw std::string ("tag not found");
  }

  return m_components[pi].component_data;
}

const std::string &
MultipleComponentProfile::
componentData (ComponentId tag) const
  throw (std::string)
{
  unsigned long numComps = m_components.size ();
  unsigned long pi;

  for (pi=0; pi<numComps; pi++) {
    if (m_components[pi].tag == tag) {
      break;
    }
  }

  if (pi >= numComps) {
    throw std::string ("tag not found");
  }

  return m_components[pi].component_data;
}

unsigned long
MultipleComponentProfile::
numComponents () const
  throw ()
{
  return m_components.size ();
}

TaggedComponent &
MultipleComponentProfile::
getComponent (unsigned long idx)
  throw (std::string)
{
  if (idx >= m_components.size()) {
    throw std::string ("invalid index");
  }

  return m_components[idx];
}

const TaggedComponent &
MultipleComponentProfile::
getComponent (unsigned long idx) const
  throw (std::string)
{
  if (idx >= m_components.size()) {
    throw std::string ("invalid index");
  }

  return m_components[idx];
}

/*
 * ----------------------------------------------------------------------
 * ORBTypeComponent
 * ----------------------------------------------------------------------
 */

ORBTypeComponent::
ORBTypeComponent ()
  throw ()
{
}

ORBTypeComponent::
ORBTypeComponent (OCPI::OS::uint32_t type)
  throw ()
  : orb_type (type)
{
}

ORBTypeComponent::
ORBTypeComponent (const ORBTypeComponent & other)
  throw ()
  : orb_type (other.orb_type)
{
}

ORBTypeComponent::
ORBTypeComponent (const std::string & data)
  throw (std::string)
{
  decode (data);
}

ORBTypeComponent &
ORBTypeComponent::
operator= (OCPI::OS::uint32_t type)
  throw ()
{
  orb_type = type;
  return *this;
}

ORBTypeComponent &
ORBTypeComponent::
operator= (const ORBTypeComponent & other)
  throw ()
{
  orb_type = other.orb_type;
  return *this;
}

void
ORBTypeComponent::
decode (const std::string & data)
  throw (std::string)
{
  try {
    OCPI::Util::CDR::Decoder cd (data);

    bool bo;
    cd.getBoolean (bo);
    cd.byteorder (bo);
    cd.getULong (orb_type);
  }
  catch (const OCPI::Util::CDR::Decoder::InvalidData &) {
    orb_type = 0;
    throw std::string ("invalid data");
  }
}

std::string
ORBTypeComponent::
encode () const
  throw ()
{
  OCPI::Util::CDR::Encoder ce;

  bool bo = OCPI::Util::CDR::nativeByteorder ();
  ce.putBoolean (bo);
  ce.putULong (orb_type);

  return ce.data ();
}

/*
 * ----------------------------------------------------------------------
 * AlternateIIOPAddressComponent
 * ----------------------------------------------------------------------
 */

AlternateIIOPAddressComponent::
AlternateIIOPAddressComponent ()
  throw ()
{
}

AlternateIIOPAddressComponent::
AlternateIIOPAddressComponent (const AlternateIIOPAddressComponent & other)
  throw ()
  : HostID (other.HostID),
    port (other.port)
{
}

AlternateIIOPAddressComponent::
AlternateIIOPAddressComponent (const std::string & data)
  throw (std::string)
{
  decode (data);
}

AlternateIIOPAddressComponent &
AlternateIIOPAddressComponent::
operator= (const AlternateIIOPAddressComponent & other)
  throw ()
{
  HostID = other.HostID;
  port = other.port;
  return *this;
}

void
AlternateIIOPAddressComponent::
decode (const std::string & data)
  throw (std::string)
{
  try {
    OCPI::Util::CDR::Decoder cd (data);

    bool bo;
    cd.getBoolean (bo);
    cd.byteorder (bo);
    cd.getString (HostID);
    cd.getUShort (port);
  }
  catch (const OCPI::Util::CDR::Decoder::InvalidData &) {
    HostID.clear ();
    port = 0;
    throw std::string ("invalid data");
  }
}

std::string
AlternateIIOPAddressComponent::
encode () const
  throw ()
{
  OCPI::Util::CDR::Encoder ce;

  bool bo = OCPI::Util::CDR::nativeByteorder ();
  ce.putBoolean (bo);
  ce.putString (HostID);
  ce.putUShort (port);

  return ce.data ();
}




    }
  }
  namespace API {
    void ior2corbaloc(const char *sior, std::string &corbaloc)
      throw (std::string)
    {
      if (!strncasecmp(sior, "corbaloc:", sizeof("corbaloc:")-1))
	corbaloc = sior;
      else {
	OU::IOP::IOR ior(sior);
	corbaloc = ior.corbaloc();
      }
    }

  }
}
