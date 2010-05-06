#include <string>
#include <fstream>
#include <iostream>
#include <CpiLoggerLogger.h>
#include <CpiUtilIIOP.h>
#include <CpiUtilLwLoggerOutput.h>

#if !defined (NDEBUG)
#include <CpiOsDebug.h>
#endif

void
usage (const char * argv0)
{
  std::cout << "usage: " << argv0 << " <IOR|file|-> <message>" << std::endl;
  std::cout << "    <IOR>   Uses this IOR." << std::endl;
  std::cout << "    <file>  Reads IOR from file." << std::endl;
  std::cout << "    -       Reads IOR from standard input." << std::endl;
}

int
main (int argc, char *argv[])
{
  int iorpos=0, msgpos=0;

#if !defined (NDEBUG)
  for (int i=1; i<argc; i++) {
    if (std::strcmp (argv[i], "--break") == 0) {
      CPI::OS::debugBreak ();
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
    else if (!iorpos) {
      iorpos = cmdidx;
    }
    else if (!msgpos) {
      msgpos = cmdidx;
    }
    else {
      usage (argv[0]);
      return 1;
    }
  }

  if (!iorpos || !msgpos) {
    usage (argv[0]);
    return 0;
  }

  std::string stringifiedIOR;

  if (std::strncmp (argv[iorpos], "IOR:", 4) == 0 ||
      std::strncmp (argv[iorpos], "ior:", 4) == 0) {
    stringifiedIOR = argv[iorpos];
  }
  else if (std::strcmp (argv[iorpos], "-") == 0) {
    std::getline (std::cin, stringifiedIOR);
  }
  else {
    std::ifstream ifs (argv[iorpos]);

    if (!ifs.good()) {
      std::cout << "oops: can not open \"" << argv[iorpos] << " for reading."
                << std::endl;
      return 1;
    }

    std::getline (ifs, stringifiedIOR);
  }

  CPI::Util::IOP::IOR ior;

  try {
    ior = CPI::Util::IOP::string_to_ior (stringifiedIOR);
  }
  catch (const std::string & oops) {
    std::cout << "oops: " << oops << "." << std::endl;
    return 1;
  }

  CPI::Util::LwLoggerOutput logger (ior);

  logger << CPI::Logger::Level::ADMINISTRATIVE_EVENT
         << CPI::Logger::ProducerName ("main")
         << argv[msgpos]
         << std::flush;

  return 0;
}
