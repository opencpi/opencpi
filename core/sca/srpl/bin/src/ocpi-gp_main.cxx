
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

/*
 * ----------------------------------------------------------------------
 * "main" for the SCA Generic Proxy.
 *
 * Note that there is no entrypoint for the SCA.  This is because the
 * SCA Generic Proxy is normally started via the Executable Device,
 * which instantiates SCA Generic Proxies locally, in the same address
 * space.  Standalone SCA Generic Proxies are only started for test and
 * debug purposes.
 * ----------------------------------------------------------------------
 */

#include <cstring>
#include <string>
#include <fstream>
#include <CosNaming.h>
#include <OcpiOsMisc.h>
#include <OcpiOsDebug.h>
#include <OcpiOsAssert.h>
#include <OcpiStringifyCorbaException.h>
#include <OcpiCORBAUtilNameServiceBind.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include "Cp289GenericProxy.h"
/*
 * ----------------------------------------------------------------------
 * "Main"
 * ----------------------------------------------------------------------
 */

class StandaloneGenericProxyConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  StandaloneGenericProxyConfigurator ();

public:
  bool help;
#if !defined (NDEBUG)
  bool debugBreak;
#endif
  bool writeIORFile;
  bool registerWithNamingService;
  std::string identifier;
  std::string iorFileName;
  std::string namingServiceName;

private:
  static CommandLineConfiguration::Option g_options[];
};

StandaloneGenericProxyConfigurator::
StandaloneGenericProxyConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
#if !defined (NDEBUG)
    debugBreak (false),
#endif
    writeIORFile (false),
    registerWithNamingService (false)
{
}

OCPI::Util::CommandLineConfiguration::Option
StandaloneGenericProxyConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "identifier", "Resource identifier",
    OCPI_CLC_OPT(&StandaloneGenericProxyConfigurator::identifier), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "writeIORFile", "Write IOR to file",
    OCPI_CLC_OPT(&StandaloneGenericProxyConfigurator::iorFileName),
    OCPI_CLC_SENT(&StandaloneGenericProxyConfigurator::writeIORFile) },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "registerWithNamingService", "Register with Naming Service",
    OCPI_CLC_OPT(&StandaloneGenericProxyConfigurator::namingServiceName),
    OCPI_CLC_SENT(&StandaloneGenericProxyConfigurator::registerWithNamingService) },
#if !defined (NDEBUG)
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "break", "Whether to break on startup",
    OCPI_CLC_OPT(&StandaloneGenericProxyConfigurator::debugBreak), 0 },
#endif
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&StandaloneGenericProxyConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (StandaloneGenericProxyConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options] <container-type> <container-name> <endpoint> <codefile> <entry[/inst[/port]]>+" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

/*
 * Start the Generic Proxy when started from the command line.
 */

static
bool
startGenericProxyCmdInt (CORBA::ORB_ptr orb,
                         int & argc, char * argv[],
                         std::string & identifier,
                         bool shutdownOrbOnRelease)
{
  StandaloneGenericProxyConfigurator config;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cout << "Oops: " << oops << std::endl;
    return false;
  }

  if (config.help || argc < 6) {
    printUsage (config, argv[0]);
    return false;
  }

  try {
    CORBA::Object_var obj = orb->resolve_initial_references ("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow (obj);

    /*
     * Activate server.
     */

    const char * driverName = argv[1];
    const char * instanceName = argv[2];
    const char * endpoint = argv[3];
    const char * codeLocalFileName = argv[4];
    char **ap = &argv[5];

    OCPI::Util::PValue cprops[] = {OCPI::Util::PVString("endpoint",(char*)endpoint),
                                  OCPI::Util::PVBool("polling",1),
                                  OCPI::Util::PVEnd };
    OCPI::API::Container *container = OCPI::API::ContainerManager::find(driverName, instanceName, cprops);
    ocpiAssert(container);
    OCPI::API::ContainerApplication *application= container->createApplication("gpmain" );
    while (*ap) {
      char
        *functionName = ap[0],
        *slash = strchr(functionName, '/'),
        *instName = 0, *portName = 0;
      if (slash) {
        *slash++ = 0;
        instName = slash;
        slash = strchr(slash, '/');
        if (slash) {
          *slash++ = 0;
          portName = slash;
        }
        if (!instName[0])
          instName = 0;
      }
      identifier = config.identifier.length() ? config.identifier : functionName;
      // We are a singleton component on a container for the purposes
      // of debug.  The generic proxy must be collocated with the OCPI::Container.
      OCPI::SCA::Cp289GenericProxy * gp =
        new OCPI::SCA::Cp289GenericProxy (orb, poa,
                                         identifier,
                                         codeLocalFileName,
                                         functionName,
                                         instName,
                                         *application,
                                         0, 0, 0, false,
                                         shutdownOrbOnRelease);
      CF::Resource_var s = gp->_this ();
      gp->_remove_ref ();

      ap++;
      if (portName) {
        CORBA::Object_ptr p = gp->getPort(portName);
        CORBA::String_var ior = orb->object_to_string (p);
        std::cout << functionName << ": " << ior << std::endl;
      }
      /*
       * Write object reference.
       */
      if (config.writeIORFile) {
        CORBA::String_var ior = orb->object_to_string (s);

        if (config.iorFileName == "-") {
          std::cout << ior << std::endl;
        } else {
          std::cout << "Writing IOR to \""
                    << config.iorFileName
                    << "\" ... "
                    << std::flush;
          std::ofstream out (config.iorFileName.c_str());
          out << ior << std::endl;

          if (out.good()) {
            std::cout << "done." << std::endl;
          }
          else {
            std::cout << "failed." << std::endl;
          }
        }
      }

      /*
       * Register with Naming Service.
       */

      if (config.registerWithNamingService) {
        std::cout << "Registering with Naming Service as \""
                  << config.namingServiceName
                  << "\" ... " << std::flush;

        try {
          CORBA::Object_var nso = orb->resolve_initial_references ("NameService");
          CosNaming::NamingContextExt_var ns = CosNaming::NamingContextExt::_narrow (nso);
          OCPI::CORBAUtil::Misc::nameServiceBind (ns, s, config.namingServiceName);
          std::cout << "done." << std::endl;
        }
        catch (const CORBA::Exception &) {
          std::cout << "failed." << std::endl;
        }
        catch (const std::string & oops) {
          std::cout << "failed: " << oops << std::endl;
        }
      }
    }
    /*
     * Activate.
     */
    PortableServer::POAManager_var mgr = poa->the_POAManager ();
    mgr->activate ();
  } catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
    return false;
  } catch (...) {
    std::cout << "Oops." << std::endl;
    return false;
  }

  return true;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  startGenericProxy (int argc, char * argv[])
  {
    CORBA::ORB_var orb;

    try {
      orb = CORBA::ORB_init (argc, argv);
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "Oops: ORB_init: "
                << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
                << std::endl;
      return -1;
    }
    catch (...) {
      std::cout << "Oops: ORB_init failed." << std::endl;
      return -1;
    }

    std::string identifier;

    if (!startGenericProxyCmdInt (orb, argc, argv, identifier, false)) {
      return -1;
    }

    std::cout << "Generic proxy "
              << identifier
              << " running." << std::endl;

    return 0;
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
        OCPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  CORBA::ORB_var orb;

  try {
    orb = CORBA::ORB_init (argc, argv);
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: ORB_init: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
    return 0;
  }
  catch (...) {
    std::cout << "Oops: ORB_init failed." << std::endl;
    return 0;
  }

  std::string identifier;

  if (!startGenericProxyCmdInt (orb, argc, argv, identifier, true)) {
    return 0;
  }

  /*
   * Run.
   */

  try {
    std::cout << "Generic proxy "
              << identifier
              << " running." << std::endl;
    orb->run ();
    std::cout << "Generic proxy "
              << identifier
              << " shutting down. " << std::endl;

    try {
      orb->shutdown (1);
    }
    catch (const CORBA::BAD_INV_ORDER &) {
      // this is expected in a synchronous server
    }
    catch (const CORBA::Exception & ex) {
      std::cout << "Oops: " << ex << std::endl;
    }

    /*
     * On VxWorks, all generic proxies may try to destroy the OCPI_CORBA_ORB,
     * only one will succeed.
     */

    try {
      orb->destroy ();
    }
    catch (...) {
    }
  }
  catch (const CORBA::Exception & ex) {
    std::cout << "Oops: "
              << OCPI::CORBAUtil::Misc::stringifyCorbaException (ex)
              << std::endl;
  }
  catch (...) {
    std::cout << "Oops." << std::endl;
  }

  return 0;
}
