#include <iostream>
#include <cstring>
#include <CpiOsDebug.h>
#include "CpiUtilUUID.h"

static
int
genUUIDInt (int, char *[])
{
  CPI::Util::UUID::BinaryUUID uuid = CPI::Util::UUID::produceRandomUUID ();
  std::cout << CPI::Util::UUID::binaryToHex (uuid) << std::endl;
  return 0;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  genUUID (int argc, char * argv[])
  {
    return genUUIDInt (argc, argv);        ;
  }
}

/*
 * Entrypoint for everybody else.
 */

int
main (int argc, char * argv[])
{
#if !defined (NDEBUG)
  {
    for (int i=1; i<argc; i++) {
      if (std::strcmp (argv[i], "--break") == 0) {
	CPI::OS::debugBreak ();
	break;
      }
    }
  }
#endif

  return genUUIDInt (argc, argv);
}
