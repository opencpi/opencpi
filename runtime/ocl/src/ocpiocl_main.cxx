/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
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

/**
  @file
    Implementation of the OpenCPI/OpenCL worker compiler.

  $ ocpiocl
    -o <output file name> required
     other - source files
    -I <Include path>
    -D <Compiler define>

************************************************************************** */

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

#include "OcpiOsLoadableModule.h"
#include "OclPlatformManager.h"


namespace
{
  bool is_output_file ( size_t argc,
                        char* argv [ ],
                        size_t& n,
                        std::string& output )
  {
    if ( std::string ( argv [ n ] ).compare ( 0, 2, "-o" ) == 0 )
    {
      n++;
      if ( n < argc )
      {
        output = std::string ( argv [ n ] );
        return true;
      }
      else
      {
        throw std::string ( "Missing value for -o output filename." );
      }
    }
    return false;
  }

  bool is_define ( size_t argc,
                   char* argv [ ],
                   size_t& n,
                   std::string& defines )
  {
    if ( std::string ( argv [ n ] ).compare ( 0, 2, "-D" ) == 0 )
    {
      defines += std::string ( argv [ n ] );

      if ( *( argv [ n ] + 3 ) == ' ' ) // -D Value - note the space
      {
        n++;
        if ( n < argc )
        {
          defines += std::string ( argv [ n ] );
        }
        else
        {
          throw std::string ( "Missing value for -D define." );
        }
      }
      defines += std::string ( " " );
      return true;
    }
    return false;
  }

  bool is_include ( size_t argc,
                    char* argv [ ],
                    size_t& n,
                    std::string& includes )
  {
    if ( std::string ( argv [ n ] ).compare ( 0, 2, "-I" ) == 0 )
    {
      includes += "-I";//std::string ( argv [ n ] );

      if (argv[n][2]) // *( argv [ n ] + 3 ) == ' ' ) // -I Path - note the space
	includes += &argv[n][2];
      else
      {
        n++;
        if ( n < argc )
        {
          includes += std::string ( argv [ n ] );
        }
        else
        {
          throw std::string ( "Missing value for -I include." );
        }
      }
      includes += std::string ( " " );
      return true;
    }
    return false;
  }

  void source_file ( char* argv [ ],
                     size_t& n,
                     std::vector<std::string>& sources )
  {
    sources.push_back ( std::string ( argv [ n ] ) );
  }

  void get_compiler_options ( size_t argc,
                              char* argv [ ],
                              std::string& output,
                              std::string& defines,
                              std::string& includes,
                              std::vector<std::string>& sources )
  {
    for ( size_t n = 1; n < argc; n++ )
    {
      if ( is_output_file ( argc, argv, n, output ) )
      {
        continue;
      }
      else if ( is_define ( argc, argv, n, defines ) )
      {
        continue;
      }
      else if ( is_include ( argc, argv, n, includes ) )
      {
        continue;
      }
      else
      {
        source_file ( argv, n, sources );
      }
    }

    if ( output.empty ( ) )
    {
      throw std::string ( "Missing output file. Use \"-o filename\" to specify output file." );
    }

    if ( !sources.size ( ) )
    {
      throw std::string ( "Missing source files." );
    }
  }

  bool compile_ocl_worker ( size_t argc, char* argv [ ] )
  {
    std::string output_file;
    std::string defines;
    std::string includes;
    std::vector<std::string> sources;

    get_compiler_options ( argc,
                           argv,
                           output_file,
                           defines,
                           includes,
                           sources );

    const OCPI::Util::PValue* props = 0; // Use to lookup specific device

    OCPI::OCL::DeviceContext device ( props );

    std::string options = defines + includes;

    return device.compile ( options, sources, output_file );
  }

} // namespace<unamed>

int main ( int argc, char* argv [ ] )
{
  if ( argc == 1 )
  {
    std::cout << "\n\nUsage: "
              << argv [ 0 ]
              << " -o <output file> [-I<include paths>] [-D<defines>] <sources files>\n"
              << std::endl;
    return 0;
  }
  bool test = argc == 2 && !strcmp(argv[1],"test");
  const char *env = getenv("OCPI_OPENCL_OBJS");
  bool success = false;
  try
  {
    OCPI::OS::LoadableModule lm(env ? env : "libOpenCL.so", true);
    if (test)
      success = true;
    else {
    
      std::cout << "\nOCL worker compiler is running." << std::endl;

      success = compile_ocl_worker ( argc, argv );

      std::cout << "\nOCL worker compile was "
		<< ( ( success ) ? "successful." : "NOT successful." )
		<< std::endl;
    }
  }
  catch ( const std::string& s )
  {
    if (!test)
      std::cerr << "\nException(s): " << s << std::endl;
  }
  catch ( ... )
  {
    if (!test)
      std::cerr << "\nException(u): unknown" << std::endl;
  }

  return success ? 0 : 1;
}
