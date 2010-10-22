
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

#include <OcpiOsAssert.h>
#include <OcpiOsDataTypes.h>
#include <string>
#include <vector>
#include "OcpiUtilCDR.h"
#include "OcpiUtilIOP.h"

/*
 * ----------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------
 */

namespace OCPI {
  namespace Util {
    namespace IOP {

      const ProfileId TAG_INTERNET_IOP = 0;
      const ProfileId TAG_MULTIPLE_COMPONENTS = 1;

      const ComponentId TAG_ORB_TYPE = 0;
      const ComponentId TAG_CODE_SETS = 1;
      const ComponentId TAG_POLICIES = 2;
      const ComponentId TAG_ALTERNATE_IIOP_ADDRESS = 3;

    }
  }
}

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

      *ptr++ = (hc2i[h1] << 4) | hc2i[h2];
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

OCPI::Util::IOP::IOR::
IOR ()
  throw ()
{
}

OCPI::Util::IOP::IOR::
IOR (const std::string & data)
  throw (std::string)
{
  decode (data);
}

OCPI::Util::IOP::IOR::
IOR (const IOR & other)
  throw ()
  : m_type_id (other.m_type_id),
    m_profiles (other.m_profiles)
{
}

OCPI::Util::IOP::IOR &
OCPI::Util::IOP::IOR::
operator= (const IOR & other)
  throw ()
{
  m_type_id = other.m_type_id;
  m_profiles = other.m_profiles;
  return *this;
}

void
OCPI::Util::IOP::IOR::
decode (const std::string & data)
  throw (std::string)
{
  try {
    OCPI::Util::CDR::Decoder cd (data);

    bool bo;
    cd.getBoolean (bo);
    cd.byteorder (bo);

    cd.getString (m_type_id);

    OCPI::OS::uint32_t numProfiles;
    cd.getULong (numProfiles);

    if (numProfiles*8 > cd.remainingData()) {
      throw OCPI::Util::CDR::Decoder::InvalidData();
    }

    m_profiles.resize (numProfiles);

    for (OCPI::OS::uint32_t pi=0; pi<numProfiles; pi++) {
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
OCPI::Util::IOP::IOR::
encode () const
  throw ()
{
  OCPI::Util::CDR::Encoder ce;

  bool bo = OCPI::Util::CDR::nativeByteorder ();
  ce.putBoolean (bo);

  ce.putString (m_type_id);

  unsigned long numProfiles = m_profiles.size ();
  ce.putULong (numProfiles);

  for (unsigned long pi=0; pi<numProfiles; pi++) {
    ce.putULong (m_profiles[pi].tag);
    ce.putOctetSeq (m_profiles[pi].profile_data);
  }

  return ce.data ();
}

const std::string &
OCPI::Util::IOP::IOR::
type_id () const
  throw ()
{
  return m_type_id;
}

void
OCPI::Util::IOP::IOR::
type_id (const std::string & id)
  throw ()
{
  m_type_id = id;
}

void
OCPI::Util::IOP::IOR::
addProfile (ProfileId tag, const void * data, unsigned long len)
  throw ()
{
  unsigned long numProfiles = m_profiles.size ();
  m_profiles.resize (numProfiles+1);
  m_profiles[numProfiles].tag = tag;
  m_profiles[numProfiles].profile_data.assign (reinterpret_cast<const char *> (data), len);
}

void
OCPI::Util::IOP::IOR::
addProfile (ProfileId tag, const std::string & data)
  throw ()
{
  addProfile (tag, data.data(), data.length());
}

bool
OCPI::Util::IOP::IOR::
hasProfile (ProfileId tag)
  throw ()
{
  unsigned long numProfiles = m_profiles.size ();

  for (unsigned long pi=0; pi<numProfiles; pi++) {
    if (m_profiles[pi].tag == tag) {
      return true;
    }
  }

  return false;
}

std::string &
OCPI::Util::IOP::IOR::
profileData (ProfileId tag)
  throw (std::string)
{
  unsigned long numProfiles = m_profiles.size ();
  unsigned long pi;

  for (pi=0; pi<numProfiles; pi++) {
    if (m_profiles[pi].tag == tag) {
      break;
    }
  }

  if (pi >= numProfiles) {
    throw std::string ("tag not found");
  }

  return m_profiles[pi].profile_data;
}

const std::string &
OCPI::Util::IOP::IOR::
profileData (ProfileId tag) const
  throw (std::string)
{
  unsigned long numProfiles = m_profiles.size ();
  unsigned long pi;

  for (pi=0; pi<numProfiles; pi++) {
    if (m_profiles[pi].tag == tag) {
      break;
    }
  }

  if (pi >= numProfiles) {
    throw std::string ("tag not found");
  }

  return m_profiles[pi].profile_data;
}

unsigned long
OCPI::Util::IOP::IOR::
numProfiles () const
  throw ()
{
  return m_profiles.size ();
}

OCPI::Util::IOP::TaggedProfile &
OCPI::Util::IOP::IOR::
getProfile (unsigned long idx)
  throw (std::string)
{
  if (idx >= m_profiles.size()) {
    throw std::string ("invalid index");
  }

  return m_profiles[idx];
}

const OCPI::Util::IOP::TaggedProfile &
OCPI::Util::IOP::IOR::
getProfile (unsigned long idx) const
  throw (std::string)
{
  if (idx >= m_profiles.size()) {
    throw std::string ("invalid index");
  }

  return m_profiles[idx];
}

/*
 * ----------------------------------------------------------------------
 * Convert between an IOR and "IOR:..." hexadecimal strings.
 * ----------------------------------------------------------------------
 */

OCPI::Util::IOP::IOR
OCPI::Util::IOP::
string_to_ior (const std::string & ior)
  throw (std::string)
{
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

  std::string blob;

  if (!hex2blob (blob, ptr+4, len-4)) {
    throw std::string ("can not read ior string");
  }

  return IOR (blob);
}

std::string
OCPI::Util::IOP::
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

OCPI::Util::IOP::MultipleComponentProfile::
MultipleComponentProfile ()
  throw ()
{
}

OCPI::Util::IOP::MultipleComponentProfile::
MultipleComponentProfile (const std::string & data)
  throw (std::string)
{
  decode (data);
}

OCPI::Util::IOP::MultipleComponentProfile::
MultipleComponentProfile (const MultipleComponentProfile & other)
  throw ()
  : m_components (other.m_components)
{
}

OCPI::Util::IOP::MultipleComponentProfile &
OCPI::Util::IOP::MultipleComponentProfile::
operator= (const MultipleComponentProfile & other)
  throw ()
{
  m_components = other.m_components;
  return *this;
}

void
OCPI::Util::IOP::MultipleComponentProfile::
decode (const std::string & data)
  throw (std::string)
{
  try {
    OCPI::Util::CDR::Decoder cd (data);

    bool bo;
    cd.getBoolean (bo);
    cd.byteorder (bo);

    OCPI::OS::uint32_t numComponents;
    cd.getULong (numComponents);

    if (numComponents*8 > cd.remainingData()) {
      throw OCPI::Util::CDR::Decoder::InvalidData();
    }

    m_components.resize (numComponents);

    for (OCPI::OS::uint32_t pi=0; pi<numComponents; pi++) {
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
OCPI::Util::IOP::MultipleComponentProfile::
encode () const
  throw ()
{
  OCPI::Util::CDR::Encoder ce;

  bool bo = OCPI::Util::CDR::nativeByteorder ();
  ce.putBoolean (bo);

  unsigned long numComponents = m_components.size ();
  ce.putULong (numComponents);

  for (unsigned long pi=0; pi<numComponents; pi++) {
    ce.putULong (m_components[pi].tag);
    ce.putOctetSeq (m_components[pi].component_data);
  }

  return ce.data ();
}

void
OCPI::Util::IOP::MultipleComponentProfile::
addComponent (ComponentId tag, const void * data, unsigned long len)
  throw ()
{
  unsigned long numComponents = m_components.size ();
  m_components.resize (numComponents+1);
  m_components[numComponents].tag = tag;
  m_components[numComponents].component_data.assign (reinterpret_cast<const char *> (data), len);
}

void
OCPI::Util::IOP::MultipleComponentProfile::
addComponent (ComponentId tag, const std::string & data)
  throw ()
{
  addComponent (tag, data.data(), data.length());
}

bool
OCPI::Util::IOP::MultipleComponentProfile::
hasComponent (ComponentId tag)
  throw ()
{
  unsigned long numComponents = m_components.size ();

  for (unsigned long pi=0; pi<numComponents; pi++) {
    if (m_components[pi].tag == tag) {
      return true;
    }
  }

  return false;
}

std::string &
OCPI::Util::IOP::MultipleComponentProfile::
componentData (ComponentId tag)
  throw (std::string)
{
  unsigned long numComponents = m_components.size ();
  unsigned long pi;

  for (pi=0; pi<numComponents; pi++) {
    if (m_components[pi].tag == tag) {
      break;
    }
  }

  if (pi >= numComponents) {
    throw std::string ("tag not found");
  }

  return m_components[pi].component_data;
}

const std::string &
OCPI::Util::IOP::MultipleComponentProfile::
componentData (ComponentId tag) const
  throw (std::string)
{
  unsigned long numComponents = m_components.size ();
  unsigned long pi;

  for (pi=0; pi<numComponents; pi++) {
    if (m_components[pi].tag == tag) {
      break;
    }
  }

  if (pi >= numComponents) {
    throw std::string ("tag not found");
  }

  return m_components[pi].component_data;
}

unsigned long
OCPI::Util::IOP::MultipleComponentProfile::
numComponents () const
  throw ()
{
  return m_components.size ();
}

OCPI::Util::IOP::TaggedComponent &
OCPI::Util::IOP::MultipleComponentProfile::
getComponent (unsigned long idx)
  throw (std::string)
{
  if (idx >= m_components.size()) {
    throw std::string ("invalid index");
  }

  return m_components[idx];
}

const OCPI::Util::IOP::TaggedComponent &
OCPI::Util::IOP::MultipleComponentProfile::
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

OCPI::Util::IOP::ORBTypeComponent::
ORBTypeComponent ()
  throw ()
{
}

OCPI::Util::IOP::ORBTypeComponent::
ORBTypeComponent (OCPI::OS::uint32_t type)
  throw ()
  : orb_type (type)
{
}

OCPI::Util::IOP::ORBTypeComponent::
ORBTypeComponent (const ORBTypeComponent & other)
  throw ()
  : orb_type (other.orb_type)
{
}

OCPI::Util::IOP::ORBTypeComponent::
ORBTypeComponent (const std::string & data)
  throw (std::string)
{
  decode (data);
}

OCPI::Util::IOP::ORBTypeComponent &
OCPI::Util::IOP::ORBTypeComponent::
operator= (OCPI::OS::uint32_t type)
  throw ()
{
  orb_type = type;
  return *this;
}

OCPI::Util::IOP::ORBTypeComponent &
OCPI::Util::IOP::ORBTypeComponent::
operator= (const ORBTypeComponent & other)
  throw ()
{
  orb_type = other.orb_type;
  return *this;
}

void
OCPI::Util::IOP::ORBTypeComponent::
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
OCPI::Util::IOP::ORBTypeComponent::
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

OCPI::Util::IOP::AlternateIIOPAddressComponent::
AlternateIIOPAddressComponent ()
  throw ()
{
}

OCPI::Util::IOP::AlternateIIOPAddressComponent::
AlternateIIOPAddressComponent (const AlternateIIOPAddressComponent & other)
  throw ()
  : HostID (other.HostID),
    port (other.port)
{
}

OCPI::Util::IOP::AlternateIIOPAddressComponent::
AlternateIIOPAddressComponent (const std::string & data)
  throw (std::string)
{
  decode (data);
}

OCPI::Util::IOP::AlternateIIOPAddressComponent &
OCPI::Util::IOP::AlternateIIOPAddressComponent::
operator= (const AlternateIIOPAddressComponent & other)
  throw ()
{
  HostID = other.HostID;
  port = other.port;
  return *this;
}

void
OCPI::Util::IOP::AlternateIIOPAddressComponent::
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
OCPI::Util::IOP::AlternateIIOPAddressComponent::
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

