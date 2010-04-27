#include <CpiLoggerLogger.h>
#include <CpiLoggerDebugLogger.h>
#include <CpiLoggerOStreamOutput.h>
#include <CpiUtilHttpServer.h>
#include <CpiUtilTcpServer.h>
#include <CpiUtilFileFs.h>
#include <CpiOsFileSystem.h>
#include <iostream>
#include <string>
#include <cstdlib>

int
main (int argc, char *argv[])
{
  if (argc != 1 && argc != 2) {
    std::cout << "usage: " << argv[0] << std::endl;
    return 1;
  }

  unsigned int portNo = 0;

  if (argc == 2) {
    portNo = std::strtoul (argv[1], 0, 0);
  }

  try {
    std::string currentDir = CPI::OS::FileSystem::cwd ();
    CPI::Logger::OStreamOutput logger (std::cout);
    CPI::Util::FileFs::FileFs localFs (currentDir);
    CPI::Util::Http::Server server (&localFs, &logger);
    CPI::Util::Tcp::Server serverPort (portNo, true);

    CPI::Logger::debug ("All", 42);

    std::cout << "Running on port "
	      << serverPort.getPortNo()
	      << "."
	      << std::endl;

    CPI::Util::Tcp::Stream * stream;

    while ((stream = serverPort.accept())) {
      server.resetConn (stream, stream);
      server.run ();
      delete stream;
    }
  }
  catch (const std::string & oops) {
    std::cout << "Oops: " << oops << std::endl;
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
  }

  return 0;
}
