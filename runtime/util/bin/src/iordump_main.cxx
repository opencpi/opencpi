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

#include <cctype>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include "OcpiUtilIOP.h"
#include "OcpiUtilIIOP.h"
#include "OcpiUtilMisc.h"

#if !defined (NDEBUG)
#include <OcpiOsDebug.h>
#endif

const char i2hc[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

void
dumpOctetSeq (std::ostream & out,
              const std::string & data,
              const std::string & prefix)
{
  const unsigned char * ptr = reinterpret_cast<const unsigned char *> (data.data ());
  unsigned long len = data.length ();
  unsigned long i;

  std::string spacer (prefix.length(), ' ');
  bool first = true;

  while (len) {
    if (first) {
      out << prefix;
      first = false;
    }
    else {
      out << spacer;
    }

    for (i=0; i<16 && i<len; i++) {
      unsigned char c = ptr[i];
      out << i2hc[c>>4] << i2hc[c&15] << ' ';
    }

    for (; i<16; i++) {
      out << "   ";
    }

    for (i=0; i<16 && i<len; i++) {
      unsigned char c = ptr[i];
      if (std::isprint (c)) {
        out << c;
      }
      else {
        out << '.';
      }
    }

    ptr += i;
    len -= i;

    out << std::endl;
  }
}

void
dumpTaggedComponent (std::ostream & out, const OCPI::Util::IOP::TaggedComponent & tc)
{
  try {
    if (tc.tag == OCPI::Util::IOP::TAG_ORB_TYPE) {
      OCPI::Util::IOP::ORBTypeComponent otc (tc.component_data);
      out << "OCPI_CORBA_ORB Type." << std::endl;
      out << "     OCPI_CORBA_ORB Type: 0x"
          << OCPI::Util::unsignedToString ((unsigned int) otc.orb_type, 16, 8);

      if ((otc.orb_type >= 0x58505300 && otc.orb_type <= 0x5850530f) ||
          (otc.orb_type >= 0x50544300 && otc.orb_type <= 0x5054430f)) {
        out << " <PrismTech>";
      }
      else if (otc.orb_type >= 0x4f495300 && otc.orb_type <= 0x4f4953ff) {
        out << " <OIS>";
      }
      else if (otc.orb_type == 0x54414f00) {
        out << " <TAO>";
      }

      out << std::endl;
    }
    else if (tc.tag == OCPI::Util::IOP::TAG_ALTERNATE_IIOP_ADDRESS) {
      OCPI::Util::IOP::AlternateIIOPAddressComponent aa (tc.component_data);
      out << "Alternate IIOP Address." << std::endl;
      out << "         Host: " << aa.HostID << std::endl
          << "         Port: " << aa.port << std::endl;
    }
    else {
      out << "Unknown Component: Tag 0x"
          << OCPI::Util::unsignedToString ((unsigned int) tc.tag, 16, 8)
          << "." << std::endl;
      dumpOctetSeq (out, tc.component_data, "         Data: ");
    }
  }
  catch (const std::string & oops) {
    out << "Undecodeable Component: Tag 0x"
        << OCPI::Util::unsignedToString ((unsigned int) tc.tag, 16, 8)
        << ": " << oops
        << "." << std::endl;
    dumpOctetSeq (out, tc.component_data, "         Data: ");
  }
}

void
dumpIIOPProfile (std::ostream & out, const OCPI::Util::IIOP::ProfileBody & pb)
{
  unsigned long numComponents = pb.numComponents ();
  out << "IIOP Profile." << std::endl
      << "      Version: "
      << OCPI::Util::integerToString ((int) pb.iiop_version.major) << "."
      << OCPI::Util::integerToString ((int) pb.iiop_version.minor) << std::endl
      << "         Host: " << pb.host << std::endl
      << "         Port: " << pb.port << std::endl;
  dumpOctetSeq (out, pb.object_key, "   Object Key: ");

  out << " # Components: " << numComponents << std::endl;

  for (unsigned long ci=0; ci<numComponents; ci++) {
    out << std::endl << "Component # " << ci << ": ";
    dumpTaggedComponent (out, pb.getComponent (ci));
  }
}

void
dumpMultipleComponentProfile (std::ostream & out, const OCPI::Util::IOP::MultipleComponentProfile & mcp)
{
  unsigned long numComponents = mcp.numComponents ();
  out << " # Components: " << numComponents << std::endl;

  for (unsigned long ci=0; ci<numComponents; ci++) {
    out << std::endl << "Component # " << ci << ": ";
    dumpTaggedComponent (out, mcp.getComponent (ci));
  }
}

void
dumpProfile (std::ostream & out, const OCPI::Util::IOP::TaggedProfile & tp)
{
  try {
    if (tp.tag == OCPI::Util::IOP::TAG_INTERNET_IOP) {
      OCPI::Util::IIOP::ProfileBody pb (tp.profile_data);
      dumpIIOPProfile (out, pb);
    }
    else if (tp.tag == OCPI::Util::IOP::TAG_MULTIPLE_COMPONENTS) {
      OCPI::Util::IOP::MultipleComponentProfile mcp (tp.profile_data);
      dumpMultipleComponentProfile (out, mcp);
    }
    else {
      out << "Unknown Profile: Tag 0x"
          << OCPI::Util::unsignedToString ((unsigned int) tp.tag, 16, 8)
          << "." << std::endl;
      dumpOctetSeq (out, tp.profile_data, "         Data: ");
    }
  }
  catch (const std::string & oops) {
    out << "Undecodeable Profile: Tag 0x"
        << OCPI::Util::unsignedToString ((unsigned int) tp.tag, 16, 8)
        << ": " << oops
        << "." << std::endl;
    dumpOctetSeq (out, tp.profile_data, "         Data: ");
  }
}

void
dumpIOR (std::ostream & out, const OCPI::Util::IOP::IOR & ior)
{
  unsigned long numProfiles = ior.numProfiles ();

  out << "Repository Id: \"" << ior.type_id () << "\"" << std::endl;
  out << "# of Profiles: " << numProfiles << std::endl;

  for (unsigned long pi=0; pi<numProfiles; pi++) {
    out << std::endl << "Profile # " << pi << ": ";
    dumpProfile (out, ior.getProfile (pi));
  }
}

void
usage (const char * argv0)
{
  std::cout << "usage: " << argv0 << " <IOR|file|->" << std::endl;
  std::cout << "    <IOR>   Dumps this IOR." << std::endl;
  std::cout << "    <file>  Reads IOR from file." << std::endl;
  std::cout << "    -       Reads IOR from standard input." << std::endl;
}

int
main (int argc, char *argv[])
{
  int argpos=0;

#if !defined (NDEBUG)
  for (int i=1; i<argc; i++) {
    if (std::strcmp (argv[i], "--break") == 0) {
      OCPI::OS::debugBreak ();
      break;
    }
  }
#endif

  for (int cmdidx=1; cmdidx<argc; cmdidx++) {
    if (std::strcmp (argv[cmdidx], "--help") == 0) {
      usage (argv[0]);
      return 0;
    }
    else if (std::strcmp (argv[cmdidx], "--break") == 0) {
      // handled above
    }
    else if (argpos == 0) {
      argpos = cmdidx;
    }
    else {
      usage (argv[0]);
      return 1;
    }
  }

  if (!argpos) {
    usage (argv[0]);
    return 0;
  }

  std::string stringifiedIOR;

  if (std::strncmp (argv[argpos], "IOR:", 4) == 0 ||
      std::strncmp (argv[argpos], "ior:", 4) == 0) {
    stringifiedIOR = argv[argpos];
  }
  else if (std::strcmp (argv[argpos], "-") == 0) {
    std::getline (std::cin, stringifiedIOR);
  }
  else {
    std::ifstream ifs (argv[argpos]);

    if (!ifs.good()) {
      std::cout << "oops: can not open \"" << argv[argpos] << " for reading."
                << std::endl;
      return 1;
    }

    std::getline (ifs, stringifiedIOR);
  }

  OCPI::Util::IOP::IOR ior;

  try {
    ior = OCPI::Util::IOP::string_to_ior (stringifiedIOR);
  }
  catch (const std::string & oops) {
    std::cout << "oops: " << oops << "." << std::endl;
    return 1;
  }

  std::cout << std::endl;
  dumpIOR (std::cout, ior);
  std::cout << std::endl;

  return 0;
}
