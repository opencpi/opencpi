
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
  @brief
  This file contains the implementation for the OpenCPI OpenCL (OCL)
  container helper functions. This file provides a wrapper around the
  OpenCL API used by the OCL container implementation.

************************************************************************** */

#include "OcpiOclPlatformManager.h"
#include "OcpiDriverManager.h"
#include "OcpiUtilException.h"

#include <opencl.h>

#include <map>
#include <vector>
#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>

#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace
{
  const char* ocl_strerror ( cl_int errnum )
  {
    switch ( errnum )
    {
      case CL_SUCCESS:
        return "CL_SUCCESS";
      case CL_DEVICE_NOT_FOUND:
        return "CL_DEVICE_NOT_FOUND";
      case CL_DEVICE_NOT_AVAILABLE:
        return "CL_DEVICE_NOT_AVAILABLE";
      case CL_COMPILER_NOT_AVAILABLE:
        return "CL_COMPILER_NOT_AVAILABLE";
      case CL_MEM_OBJECT_ALLOCATION_FAILURE:
        return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
      case CL_OUT_OF_RESOURCES:
        return "CL_OUT_OF_RESOURCES";
      case CL_OUT_OF_HOST_MEMORY:
        return "CL_OUT_OF_HOST_MEMORY";
      case CL_PROFILING_INFO_NOT_AVAILABLE:
        return "CL_PROFILING_INFO_NOT_AVAILABLE";
      case CL_MEM_COPY_OVERLAP:
        return "CL_MEM_COPY_OVERLAP";
      case CL_IMAGE_FORMAT_MISMATCH:
        return "CL_IMAGE_FORMAT_MISMATCH";
      case CL_IMAGE_FORMAT_NOT_SUPPORTED:
        return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
      case CL_BUILD_PROGRAM_FAILURE:
        return "CL_BUILD_PROGRAM_FAILURE";
      case CL_MAP_FAILURE:
        return "CL_MAP_FAILURE";
      case 13:
        return "reserved error 1";
      case 14:
        return "reserved error 2";
      case 15:
        return "reserved error 3";
      case 16:
        return "reserved error 4";
      case 17:
        return "reserved error 5";
      case 18:
        return "reserved error 6";
      case 19:
        return "reserved error 7";
      case 20:
        return "reserved error 8";
      case 21:
        return "reserved error 9";
      case 22:
        return "reserved error 10";
      case 23:
        return "reserved error 11";
      case 24:
        return "reserved error 12";
      case 25:
        return "reserved error 13";
      case 26:
        return "reserved error 14";
      case 27:
        return "reserved error 15";
      case 28:
        return "reserved error 16";
      case 29:
        return "reserved error 17";
      case CL_INVALID_VALUE:
        return "CL_INVALID_VALUE";
      case CL_INVALID_DEVICE_TYPE:
        return "CL_INVALID_DEVICE_TYPE";
      case CL_INVALID_PLATFORM:
        return "CL_INVALID_PLATFORM";
      case CL_INVALID_DEVICE:
        return "CL_INVALID_DEVICE";
      case CL_INVALID_CONTEXT:
        return "CL_INVALID_CONTEXT";
      case CL_INVALID_QUEUE_PROPERTIES:
        return "CL_INVALID_QUEUE_PROPERTIES";
      case CL_INVALID_COMMAND_QUEUE:
        return "CL_INVALID_COMMAND_QUEUE";
      case CL_INVALID_HOST_PTR:
        return "CL_INVALID_HOST_PTR";
      case CL_INVALID_MEM_OBJECT:
        return "CL_INVALID_MEM_OBJECT";
      case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
        return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
      case CL_INVALID_IMAGE_SIZE:
        return "CL_INVALID_IMAGE_SIZE";
      case CL_INVALID_SAMPLER:
        return "CL_INVALID_SAMPLER";
      case CL_INVALID_BINARY:
        return "CL_INVALID_BINARY";
      case CL_INVALID_BUILD_OPTIONS:
        return "CL_INVALID_BUILD_OPTIONS";
      case CL_INVALID_PROGRAM:
        return "CL_INVALID_PROGRAM";
      case CL_INVALID_PROGRAM_EXECUTABLE:
        return "CL_INVALID_PROGRAM_EXECUTABLE";
      case CL_INVALID_KERNEL_NAME:
        return "CL_INVALID_KERNEL_NAME";
      case CL_INVALID_KERNEL_DEFINITION:
        return "CL_INVALID_KERNEL_DEFINITION";
      case CL_INVALID_KERNEL:
        return "CL_INVALID_KERNEL";
      case CL_INVALID_ARG_INDEX:
        return "CL_INVALID_ARG_INDEX";
      case CL_INVALID_ARG_VALUE:
        return "CL_INVALID_ARG_VALUE";
      case CL_INVALID_ARG_SIZE:
        return "CL_INVALID_ARG_SIZE";
      case CL_INVALID_KERNEL_ARGS:
        return "CL_INVALID_KERNEL_ARGS";
      case CL_INVALID_WORK_DIMENSION:
        return "CL_INVALID_WORK_DIMENSION";
      case CL_INVALID_WORK_GROUP_SIZE:
        return "CL_INVALID_WORK_GROUP_SIZE";
      case CL_INVALID_WORK_ITEM_SIZE:
        return "CL_INVALID_WORK_ITEM_SIZE";
      case CL_INVALID_GLOBAL_OFFSET:
        return "CL_INVALID_GLOBAL_OFFSET";
      case CL_INVALID_EVENT_WAIT_LIST:
        return "CL_INVALID_EVENT_WAIT_LIST";
      case CL_INVALID_EVENT:
        return "CL_INVALID_EVENT";
      case CL_INVALID_OPERATION:
        return "CL_INVALID_OPERATION";
      case CL_INVALID_GL_OBJECT:
        return "CL_INVALID_GL_OBJECT";
      case CL_INVALID_BUFFER_SIZE:
        return "CL_INVALID_BUFFER_SIZE";
      case CL_INVALID_MIP_LEVEL:
        return "CL_INVALID_MIP_LEVEL";
      case CL_INVALID_GLOBAL_WORK_SIZE:
        return "CL_INVALID_GLOBAL_WORK_SIZE";
      default:
          return "Unknown error";
    }

    return "Unknown error";
  }

  const char* device_type_str ( cl_device_type type )
  {
    switch ( type )
    {
      case CL_DEVICE_TYPE_CPU:
        return "CL_DEVICE_TYPE_CPU";
      case CL_DEVICE_TYPE_GPU:
        return "CL_DEVICE_TYPE_GPU";
      case CL_DEVICE_TYPE_ACCELERATOR:
        return "CL_DEVICE_TYPE_ACCELERATOR";
      case CL_DEVICE_TYPE:
        return "CL_DEVICE_TYPE_DEFAULT";
      default:
        return "unknown";
    }
    return "unknown";
  }

} // End: namespace<unamed>

namespace OCPI
{
  namespace OCL
  {
    struct Kernel
    {
      std::string name;
      cl_kernel kernel;
      cl_context context;
      cl_program program;
      size_t work_group_size [ 3 ];
    };

    struct Binary
    {
      std::vector<Kernel> kernels;
    };

    typedef std::map<std::string,Binary> BinaryTable;

    struct Device
    {
      Device ( )
        : id ( 0 ),
          context ( 0 ),
          type ( CL_DEVICE_TYPE_DEFAULT ),
	  sizes( NULL ),
          cmdq ( 0 ),
          binaries ( )
      {
        // Empty
      }

      ~Device ( )
      {
        // Empty
      }
      cl_device_id id;
      cl_context context;
      cl_device_type type;
      cl_uint vendorId, nUnits, nDimensions;
      size_t *sizes, groupSize, argSize;
      cl_bool available, compiler;
      cl_device_exec_capabilities capabilities;
      cl_command_queue_properties properties;
      std::string name, vendor, driverVersion, profile, version, cVersion, extensions;
      cl_command_queue cmdq;
      BinaryTable binaries;

      void loadBinary ( const std::string& pathToBinary,
                        const OCPI::API::PValue* binaryParams );

      void unloadBinary ( const std::string& pathToBinary );

      bool compile ( const std::string& options,
                     const std::vector<std::string>& sources,
                     const std::string& output_file );
    };

    void get_compiler_options ( std::string& options,
                                cl_device_type type )
    {
      options += "-D__STDCL__ ";

      switch  ( type )
      {
        case CL_DEVICE_TYPE_CPU:
          options += "-D__CPU__ ";
          break;
        case CL_DEVICE_TYPE_GPU:
          options += "-D__GPU__ ";
          break;
      }

      const char* user_options = std::getenv ( "OCPI_OCL_COMPILER_OPTIONS" );

      if ( user_options )
      {
        options += user_options;
      }
    }

    struct BinaryFile
    {
      BinaryFile ( const std::string& path )
        : fd ( -1 ),
          ptr ( 0 ),
          n_bytes ( 0 ),
          file ( path )
      {
        init ( file );
      }

      void init ( const std::string& path )
      {
        struct stat fs;

        if ( stat ( path.c_str ( ), &fs ) )
        {
          throw OCPI::Util::Error ( "stat(%s) failed : %s", path.c_str ( ), strerror ( errno ) );
        }

        if ( fs.st_size == 0 || !S_ISREG ( fs.st_mode ) )
        {
          throw OCPI::Util::Error ( "Invalid files size or permission" );
        }

        n_bytes = fs.st_size;

        fd = open ( path.c_str ( ), O_RDONLY );

        if ( fd == -1 )
        {
          throw OCPI::Util::Error ( "open() failed : %s", strerror ( errno ) );
        }

        // Must skip metadata
        size_t metadata_n_bytes = geMetadataSize ( path.c_str ( ) );

        n_bytes -= metadata_n_bytes;

        ptr = mmap ( 0, n_bytes, PROT_READ,MAP_SHARED, fd, 0 );

        if ( !ptr )
        {
          throw OCPI::Util::Error ( "mmap() failed : %s", strerror ( errno ) );
        }
      }

      void fini ( )
      {
        if ( ptr )
        {
          (void)munmap ( ptr, n_bytes );
        }

        if ( fd != -1 )
        {
          close ( fd );
        }
      }

      BinaryFile ( const BinaryFile& orig )
        : fd ( -1 ),
          ptr ( 0 ),
          n_bytes ( 0 ),
          file ( orig.file )
      {
        init ( file );
      }

      BinaryFile& operator= ( const BinaryFile& other )
      {
        if ( this != &other )
        {
            this->fini ( );
            this->init ( other.file );
        }

        // by convention, always return *this
        return *this;
      }

      ~BinaryFile ( )
      {
        fini ( );
      }

      /*
        Taken from OCPI::Library::CompLib::getMetadata()
      */
      size_t geMetadataSize ( const char* path_to_source )
      {
        bool found_metadata ( false );

        size_t metadata_n_bytes ( 0 );

        int fd = open ( path_to_source, O_RDONLY);
        if ( fd < 0 )
        {
          throw OCPI::Util::Error ( "open() failed : %s", strerror ( errno ) );
        }

        char buf [64 / 3+4 ]; // octal + \r + \n + null

        off_t fileLength, second, third;

        if ( fd >= 0 &&
             ( ( fileLength = lseek ( fd, 0, SEEK_END ) ) != -1 ) &&
             ( ( second = lseek ( fd, -(off_t)sizeof(buf), SEEK_CUR ) ) != -1 ) &&
               ( third = read ( fd, buf, sizeof(buf ) ) ) == sizeof ( buf ) )
        {
          for ( char* cp = &buf [ sizeof ( buf ) - 2 ]; cp >= buf; cp-- )
          {
            metadata_n_bytes++;
            if ( *cp == 'X' && isdigit ( cp [ 1 ] ) )
            {
              char *end;
              long long n = strtoll (cp + 1, &end, 10 );
              // strtoll error reporting is truly bizarre
              if ( n != LLONG_MAX && n >= 0 && cp[1] && isspace(*end ) )
              {
                off_t metaStart = fileLength - sizeof(buf) + (cp - buf) - n;
                if ( lseek ( fd, metaStart, SEEK_SET ) != -1 )
                {
                  metadata_n_bytes += n;
                }
              }
              metadata_n_bytes += 1;
              found_metadata = true;
              break;
            }
          }
        }

        if ( fd >= 0 )
          close ( fd );

        return ( found_metadata ) ? metadata_n_bytes : 0;
      }

      int fd;
      void* ptr;
      size_t n_bytes;
      std::string file;
    }; // End: class BinaryFile

    void map_source_files ( const std::vector<std::string>& sources,
                            std::vector<BinaryFile>& mapped_files )
    {
      for ( size_t n = 0; n < sources.size ( ); n++ )
      {
        {
          BinaryFile binary_file ( sources [ n ] );
          mapped_files.push_back ( binary_file );
        }
      }
    }

    void print_build_command ( const std::string& options,
                               const std::vector<std::string>& sources,
                               const std::string& output_file )
    {
      std::cout << "\nOCL build command"
                << "\n  Options: "
                << options
                << "\n  Output file: "
                << output_file
                << "\n  Sources:";
      for ( size_t n = 0; n < sources.size ( ); n++ )
      {
        std::cout << " " << sources [ n ];
      }
      std::cout << std::endl;
    }

    void Device::loadBinary ( const std::string& pathToBinary,
                              const OCPI::API::PValue* binaryParams )
    {
      ( void ) binaryParams; // FIXME - can this be used for compiler options?

      BinaryFile binary_file ( pathToBinary );

      cl_int rc;

      cl_int binary_status;

      const unsigned char* mapped_binary = reinterpret_cast<const unsigned char*>( binary_file.ptr );

      cl_program program = clCreateProgramWithBinary ( context,
                                                       1,
                                                       &id,
                                                       &binary_file.n_bytes,
                                                       &mapped_binary,
                                                       &binary_status,
                                                       &rc );
      if ( rc )
      {
        throw OCPI::Util::Error ( "clCreateProgramWithBinary() failed rc: %s", ocl_strerror ( rc ) );
      }

      if ( binary_status )
      {
        throw OCPI::Util::Error ( "clCreateProgramWithBinary() failed status: %s", ocl_strerror ( binary_status ) );
      }

      cl_int build_rc = clBuildProgram ( program,
                                         1,
                                         &id,
                                         0,
                                         0,
                                         0 );
      {
        size_t build_log_n_bytes;

        rc = clGetProgramBuildInfo ( program,
                                     id,
                                     CL_PROGRAM_BUILD_LOG,
                                     0,
                                     0,
                                     &build_log_n_bytes );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetProgramBuildInfo() failed : %s", ocl_strerror ( rc ) );
        }

        std::vector<char> build_log;

        build_log.resize ( build_log_n_bytes );

        rc = clGetProgramBuildInfo ( program,
                                     id,
                                     CL_PROGRAM_BUILD_LOG,
                                     build_log_n_bytes,
                                     &build_log [ 0 ],
                                     0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetProgramBuildInfo() failed : %s", ocl_strerror ( rc ) );
        }

        build_log.push_back ( '\0' );

        std::cout << "OCL build log : =====\n"
                  << &build_log[0]
                  << " for artifact "
                  << pathToBinary
                  << std::endl;
      }

      if ( build_rc )
      {
        throw OCPI::Util::Error ( "clBuildProgram() failed : %s", ocl_strerror ( build_rc ) );
      }

      cl_uint n_kernels;

      rc = clCreateKernelsInProgram ( program, 0, 0, &n_kernels );

      if ( rc )
      {
        throw OCPI::Util::Error ( "clCreateKernelsInProgram() failed : %s", ocl_strerror ( rc ) );
      }

      std::vector<cl_kernel> kernels ( n_kernels );

      kernels.resize ( n_kernels );

      rc = clCreateKernelsInProgram ( program, n_kernels, &kernels [ 0 ], 0 );

      if ( rc )
      {
        throw OCPI::Util::Error ( "clCreateKernelsInProgram() failed : %s", ocl_strerror ( rc ) );
      }

      Binary binary;

      for ( size_t n = 0; n < kernels.size ( ); n++ )
      {
        Kernel kernel;

        kernel.kernel = kernels [ n ];

        char kernel_name [ 1024 ];

        rc = clGetKernelInfo ( kernels [ n ],
                               CL_KERNEL_FUNCTION_NAME,
                               sizeof ( kernel_name ),
                               kernel_name,
                               0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetKernelInfo() failed : %s", ocl_strerror ( rc ) );
        }

        std::cout << "OCL discovered kernel " << kernel_name << std::endl;

        kernel.name = kernel_name;

        rc = clGetKernelInfo ( kernels [ n ],
                               CL_KERNEL_CONTEXT,
                               sizeof ( cl_context ),
                               &kernel.context,
                               0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetKernelInfo() failed : %s", ocl_strerror ( rc ) );
        }

        rc = clGetKernelInfo ( kernels [ n ],
                               CL_KERNEL_PROGRAM,
                               sizeof ( cl_program ),
                               &kernel.program,
                               0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetKernelInfo() failed : %s", ocl_strerror ( rc ) );
        }

        rc = clGetKernelWorkGroupInfo ( kernels [ n ],
                                        id,
                                        CL_KERNEL_COMPILE_WORK_GROUP_SIZE,
                                        sizeof ( kernel.work_group_size ),
                                        (void*)kernel.work_group_size,
                                        0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetKernelWorkGroupInfo() failed : %s", ocl_strerror ( rc ) );
        }

        printf ( "\n\nREMOVE work_group_size %zu %zu %zu\n",
                 kernel.work_group_size [ 0 ],
                 kernel.work_group_size [ 1 ],
                 kernel.work_group_size [ 2 ] );

        binary.kernels.push_back ( kernel );
      }

      binaries [ pathToBinary ] = binary;
    }

    void Device::unloadBinary ( const std::string& pathToBinary )
    {
      BinaryTable::iterator it = binaries.find ( pathToBinary );
      if ( it != binaries.end ( ) )
      {
        binaries.erase ( it );
      }
    }

    bool Device::compile ( const std::string& options,
                           const std::vector<std::string>& sources,
                           const std::string& output_file )
    {
      std::vector<BinaryFile> mapped_files;
      std::vector<const char*> mapped_code;
      std::vector<size_t> mapped_code_size;

      map_source_files ( sources, mapped_files );

      for ( size_t n = 0; n < mapped_files.size ( ); n++ )
      {
          mapped_code.push_back ( ( const char* ) mapped_files [ n ].ptr );
          mapped_code_size.push_back ( mapped_files [ n ].n_bytes );
      }

      cl_int rc;

      cl_program program = clCreateProgramWithSource ( context,
                                                       mapped_code.size ( ),
                                                       &mapped_code[0],
                                                       &mapped_code_size[0],
                                                       &rc );
      if ( rc )
      {
        throw OCPI::Util::Error ( "clCreateProgramWithSource() failed : %s", ocl_strerror ( rc ) );
      }

      std::string compiler_options ( options );

      get_compiler_options ( compiler_options, type );

      print_build_command ( compiler_options, sources, output_file );

      cl_int build_rc = clBuildProgram ( program,
                                         1,
                                         &id,
                                         compiler_options.c_str ( ),
                                         NULL,
                                         NULL );
      {
        size_t build_log_n_bytes;

        rc = clGetProgramBuildInfo ( program,
                                     id,
                                     CL_PROGRAM_BUILD_LOG,
                                     0,
                                     0,
                                     &build_log_n_bytes );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetProgramBuildInfo() failed : %s", ocl_strerror ( rc ) );
        }

        std::vector<char> build_log;

        build_log.resize ( build_log_n_bytes );

        rc = clGetProgramBuildInfo ( program,
                                     id,
                                     CL_PROGRAM_BUILD_LOG,
                                     build_log_n_bytes,
                                     &build_log [ 0 ],
                                     0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetProgramBuildInfo() failed : %s", ocl_strerror ( rc ) );
        }

        build_log.push_back ( '\0' );

        std::cout << "OCL compile log : =====\n"
                  << &build_log[0]
                  << std::endl;

        if ( build_rc )
        {
          std::cout << "clBuildProgram() failed : " << ocl_strerror ( build_rc ) << std::endl;
          return false;
        }

        size_t binary_n_bytes ( 0 );

        rc = clGetProgramInfo ( program,
                                CL_PROGRAM_BINARY_SIZES,
                                sizeof ( binary_n_bytes ),
                                &binary_n_bytes,
                                0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetProgramInfo() failed : %s", ocl_strerror ( rc ) );
        }

        std::vector<unsigned char> binary_data;

        binary_data.resize ( binary_n_bytes );

        const unsigned char* p_binary_data = &binary_data [ 0 ];

        rc = clGetProgramInfo ( program,
                                CL_PROGRAM_BINARIES,
                                sizeof ( p_binary_data ),
                                &p_binary_data,
                                0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetProgramInfo() failed : %s", ocl_strerror ( rc ) );
        }

        std::ofstream out ( output_file.c_str ( ) );

        if ( !out.is_open ( ) )
        {
          throw OCPI::Util::Error ( "Unable to open output file %s", output_file.c_str ( ) );
        }

        out.write ( (const char*) &binary_data [ 0 ], binary_data.size ( ) );
      }

      return true;
    }

    struct Platform
    {
      cl_platform_id id;
      std::string name;
      std::string vendor;
      std::string profile;
      std::string version;
      std::string extensions;
      std::vector<Device> devices;
    };

    class PlatformManager
    {
      public:
        PlatformManager ( );

        ~PlatformManager ( );

        Device& getDevice ( const OCPI::Util::PValue* props );

        const std::string to_str ( ) const;

      private:
        std::vector<Platform> d_platforms;

        void enumerate_devices ( );
        void enumerate_platforms ( );

        PlatformManager ( const PlatformManager& );
        PlatformManager& operator=( const PlatformManager& );

    }; // End: class PlatformManager

    PlatformManager::PlatformManager ( )
    {
      enumerate_platforms ( );
    }

    PlatformManager::~PlatformManager ( )
    {
      // Empty
    }

    void PlatformManager::enumerate_platforms ( )
    {
      cl_uint n_platforms;

      cl_int rc = clGetPlatformIDs ( 0, 0, &n_platforms );

      if ( rc )
      {
        throw OCPI::Util::Error ( "clGetPlatformIDs() failed : %s", ocl_strerror ( rc ) );
      }

      if ( n_platforms == 0 )
      {
        throw OCPI::Util::Error ( "No OpenCL platforms found" );
      }

      std::vector<cl_platform_id> platform_ids;

      platform_ids.resize ( n_platforms );

      rc = clGetPlatformIDs ( n_platforms, &platform_ids [ 0 ], 0 );

      if ( rc )
      {
        throw OCPI::Util::Error ( "clGetPlatformIDs() failed : %s", ocl_strerror ( rc ) );
      }

      for ( size_t n = 0; n < platform_ids.size ( ); n++ )
      {
        Platform platform;

        char platform_info [ 1024 ];

        platform.id = platform_ids [ 0 ];

        rc = clGetPlatformInfo ( platform_ids [ n ],
                                 CL_PLATFORM_PROFILE,
                                 sizeof ( platform_info ),
                                 platform_info,
                                 0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetPlatformInfo() failed : %s", ocl_strerror ( rc ) );
        }
        platform.profile = platform_info;

        rc = clGetPlatformInfo ( platform_ids [ n ],
                                 CL_PLATFORM_VERSION,
                                 sizeof ( platform_info ),
                                 platform_info,
                                 0 );

        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetPlatformInfo() failed : %s", ocl_strerror ( rc ) );
       }
        platform.version = platform_info;

        rc = clGetPlatformInfo ( platform_ids [ n ],
                                 CL_PLATFORM_NAME,
                                 sizeof ( platform_info ),
                                 platform_info,
                                 0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetPlatformInfo() failed : %s", ocl_strerror ( rc ) );
        }
        platform.name = platform_info;

        rc = clGetPlatformInfo ( platform_ids [ n ],
                                 CL_PLATFORM_VENDOR,
                                 sizeof ( platform_info ),
                                 platform_info,
                                 0 );

        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetPlatformInfo() failed : %s", ocl_strerror ( rc ) );
        }
        platform.vendor = platform_info;

        rc = clGetPlatformInfo ( platform_ids [ n ],
                                 CL_PLATFORM_EXTENSIONS,
                                 sizeof ( platform_info ),
                                 platform_info,
                                 0 );

        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetPlatformInfo() failed : %s", ocl_strerror ( rc ) );
        }
        platform.extensions = platform_info;

        d_platforms.push_back ( platform );

      } // End: for ( size_t n = 0; n < platform_ids.size ( ); n++ )

      enumerate_devices ( );
    }

    void PlatformManager::enumerate_devices ( )
    {
      for ( size_t n = 0; n < d_platforms.size ( ); n++ )
      {
        cl_uint n_devices;

        char device_string_info [ 1024 ];
        cl_int rc = clGetDeviceIDs ( d_platforms [ n ].id,
                                     CL_DEVICE_TYPE_ALL,
                                     0,
                                     0,
                                     &n_devices );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetDeviceIDs() failed : %s", ocl_strerror ( rc ) );
        }

        std::vector<cl_device_id> device_ids;

        device_ids.resize ( n_devices );

        rc = clGetDeviceIDs ( d_platforms [ n ].id,
                              CL_DEVICE_TYPE_ALL,
                              n_devices,
                              &device_ids [ 0 ],
                              0 );
        if ( rc )
        {
          throw OCPI::Util::Error ( "clGetDeviceIDs() failed : %s", ocl_strerror ( rc ) );
        }

        cl_context_properties ctx_props [ 3 ];

        ctx_props [ 0 ] = ( cl_context_properties ) CL_CONTEXT_PLATFORM;
        ctx_props [ 1 ] = ( cl_context_properties ) d_platforms [ n ].id;
        ctx_props [ 2 ] = ( cl_context_properties ) 0;

        for ( size_t d = 0; d < device_ids.size ( ); d++ )
        {
          Device device;

          device.id = device_ids [ d ];

          device.context = clCreateContext ( ctx_props,
                                             1,
                                             &device_ids [ d ],
                                             0,
                                             0,
                                             &rc );
          if ( ( rc ) || ( !device.context ) )
          {
            throw OCPI::Util::Error ( "clCreateContext() failed : %s", ocl_strerror ( rc ) );
          }

          device.cmdq = clCreateCommandQueue ( device.context,
                                               device_ids [ d ],
                                               0,
                                               &rc );
          if ( rc )
          {
            throw OCPI::Util::Error ( "clCreateCommandQueue() failed : %s", ocl_strerror ( rc ) );
          }

#define DEV_INFO(param_name, member) DEV_INFO_SIZE(param_name, sizeof(device.member), &device.member)
	  //#define DEV_INFO_SIZE(param_name, member, size) DEV_INFO_SIZE2(param_name, member, size * sizeof(device.member), &device.member)
#define DEV_INFO_STRING(param_name, member) do {			\
	  DEV_INFO_SIZE(param_name, sizeof(device_string_info), device_string_info); \
	  device.member = device_string_info; \
          } while (0)
#define DEV_INFO_SIZE(param_name, size, out) do {			\
	    if ((rc = clGetDeviceInfo(device_ids[d], CL_DEVICE_##param_name, size, out, 0))) \
              throw OCPI::Util::Error( "clGetDeviceInfo() failed for CL_DEVICE_" #param_name ": %s", ocl_strerror ( rc ) ); \
          } while (0)  
	  DEV_INFO(TYPE, type);
	  DEV_INFO(VENDOR_ID, vendorId);
	  DEV_INFO(MAX_COMPUTE_UNITS, nUnits);
	  DEV_INFO(MAX_WORK_ITEM_DIMENSIONS, nDimensions);
	  device.sizes = new size_t[device.nDimensions];
	  DEV_INFO_SIZE(MAX_WORK_ITEM_SIZES, device.nDimensions * sizeof(size_t), device.sizes);
	  DEV_INFO(MAX_WORK_GROUP_SIZE, groupSize);
	  DEV_INFO(MAX_PARAMETER_SIZE, argSize);
	  DEV_INFO(AVAILABLE, available);
	  DEV_INFO(COMPILER_AVAILABLE, compiler);
	  DEV_INFO(EXECUTION_CAPABILITIES, capabilities);
	  DEV_INFO(QUEUE_PROPERTIES, properties);
	  DEV_INFO_STRING(NAME, name);
	  DEV_INFO_STRING(VENDOR, vendor);
	  DEV_INFO_STRING(PROFILE, profile);
	  DEV_INFO_STRING(VERSION, version);
	  //	  DEV_INFO_STRING(OPENCL_C_VERSION, cVersion); not on Apple
	  DEV_INFO_STRING(EXTENSIONS, extensions);

          d_platforms [ n ].devices.push_back ( device );

        } // End: for ( size_t d = 0; d < device_ids.size ( ); d++ )

      } // End: for ( size_t n = 0; n < d_platforms.size ( ); n++ )
    }

    const std::string PlatformManager::to_str ( ) const
    {
      std::stringstream ss;

      ss << "\nOCL Platform Manager";

      for ( size_t n = 0; n < d_platforms.size ( ); n++ )
      {
        ss << "\n  Platform "
           << n
           << "\n    Name       "
           << d_platforms [ n ].name
           << "\n    Vendor     "
           << d_platforms [ n ].vendor
           << "\n    Profile    "
           << d_platforms [ n ].profile
           << "\n    Version    "
           << d_platforms [ n ].version
           << "\n    Extensions "
           << d_platforms [ n ].extensions;
        for ( size_t d = 0; d < d_platforms [ n ].devices.size ( ); d++ )
        {
          ss << "\n    Device "
             << d
             << "\n      Type "
             << device_type_str ( d_platforms [ n ].devices [ d ].type );
        }
        ss << "\n";
      }

      return ss.str ( );
    }

    std::ostream& operator<< ( std::ostream& os, const PlatformManager& p )
    {
      return os << p.to_str();
    }

    Device& PlatformManager::getDevice ( const OCPI::Util::PValue* props )
    {
      std::cout << *this << std::endl;
      // FIXME - implement a device lookup back on properties or something.
      ( void ) props;
      return d_platforms [ 0 ].devices [ 0 ];
    }

    PlatformManager& getPlatform ( )
    {
      return OCPI::Driver::Singleton<PlatformManager>::getSingleton();
    }

    class DeviceContext::Impl
    {
      public:
        Impl ( Device& device );

        ~Impl ( );

        void loadArtifact ( const std::string& pathToArtifact,
                            const OCPI::API::PValue* artifactParams );

        void unloadArtifact ( const std::string& pathToArtifact );

        OCPI::OCL::DeviceWorker& loadWorker ( const std::string& entryPoint );

        void unloadWorker ( OCPI::OCL::DeviceWorker& worker );

        bool compile ( const std::string& options,
                       const std::vector<std::string>& sources,
                       const std::string& output_file );
      private:
        Device& d_device;
    };

    struct Buffer
    {
      Buffer ( )
        : mem ( 0 ),
          ptr ( 0 ),
          n_bytes ( 0 )
      {
        // Empty
      }

      cl_mem mem;
      void* ptr;
      size_t n_bytes;
    };

    typedef std::map<uintptr_t,Buffer> BufferTable;

    class DeviceWorker::Impl
    {
      public:
        Impl ( const std::string& entryPoint );

        ~Impl ( );

        void run ( const Grid& grid );

        void setKernel ( cl_kernel kernel )
        {
          d_kernel = kernel;
        }

        void setContext ( cl_context context )
        {
          d_context = context;
        }

        void setCommandQueue ( cl_command_queue cmdq )
        {
          d_cmdq = cmdq;
        }

        void setKernelArg ( size_t arg_idx,
                            void* ptr );

        void setWorkGroupSize ( size_t* work_group_size )
        {
          d_work_group_size [ 0 ] = work_group_size [ 0 ];
          d_work_group_size [ 1 ] = work_group_size [ 1 ];
          d_work_group_size [ 2 ] = work_group_size [ 2 ];
        }

        void setKernelArg ( size_t arg_idx,
                            size_t sizeof_arg,
                            void* arg );

        void registerPtr ( void* ptr,
                           size_t n_bytes );

        void unregisterPtr ( void* ptr );

        void* mapPtr ( void* ptr, OCPI::OCL::DeviceWorker::MapFlags flag );

        void unmapPtr ( void* ptr );

        void syncPtr ( void* ptr, SyncDirection flag );

      private:
        std::string d_entryPoint;
        cl_kernel d_kernel;
        size_t d_work_group_size [ 3 ];
        cl_context d_context;
        cl_command_queue d_cmdq;
        Grid d_grid;
        BufferTable d_buffers;
    };

  } // End: namespace OCL

} // End: namespace OCPI

/* ---- Implementation of the DeviceContext class ------------------------ */

OCPI::OCL::DeviceContext::Impl::Impl ( Device& device )
  : d_device ( device )
{
  // Empty
}

OCPI::OCL::DeviceContext::Impl::~Impl ( )
{
  // Empty
}

void OCPI::OCL::DeviceContext::Impl::loadArtifact ( const std::string& pathToArtifact,
                                                    const OCPI::API::PValue* artifactParams )
{
  d_device.loadBinary ( pathToArtifact, artifactParams );
}

void OCPI::OCL::DeviceContext::Impl::unloadArtifact ( const std::string& pathToArtifact )
{
  d_device.unloadBinary ( pathToArtifact );
}

OCPI::OCL::DeviceWorker& OCPI::OCL::DeviceContext::Impl::loadWorker ( const std::string& entryPoint )
{
  OCPI::OCL::DeviceWorker* worker = new OCPI::OCL::DeviceWorker ( entryPoint );

  for ( BinaryTable::iterator it = d_device.binaries.begin ( );
        it != d_device.binaries.end ( );
        ++it )
  {
    for ( size_t n = 0; n < it->second.kernels.size(); n++ )
    {
      if ( entryPoint == it->second.kernels [ n ].name )
      {
        worker->d_impl->setKernel ( it->second.kernels [ n ].kernel );
        worker->d_impl->setWorkGroupSize ( it->second.kernels [ n ].work_group_size );
        worker->d_impl->setContext ( it->second.kernels [ n ].context );
        worker->d_impl->setCommandQueue ( d_device.cmdq );
        break;
      }
    }
  }

  return *worker;
}

void OCPI::OCL::DeviceContext::Impl::unloadWorker ( OCPI::OCL::DeviceWorker& worker )
{
  (void) worker;
}

bool OCPI::OCL::DeviceContext::Impl::compile ( const std::string& options,
                                               const std::vector<std::string>& sources,
                                               const std::string& output_file )
{
  return d_device.compile ( options, sources, output_file );
}


/* ---- DeviceContext wrapper class -------------------------------------- */

OCPI::OCL::DeviceContext::DeviceContext ( const OCPI::Util::PValue* props )
  : d_impl ( new Impl ( getPlatform().getDevice ( props ) ) )
{
  // Empty
}

OCPI::OCL::DeviceContext::~DeviceContext ( )
{
  // Empty
}

void OCPI::OCL::DeviceContext::loadArtifact ( const std::string& pathToArtifact,
                                              const OCPI::API::PValue* artifactParams )
{
  d_impl->loadArtifact ( pathToArtifact, artifactParams );
}

void OCPI::OCL::DeviceContext::unloadArtifact ( const std::string& pathToArtifact )
{
  d_impl->unloadArtifact ( pathToArtifact );
}

OCPI::OCL::DeviceWorker& OCPI::OCL::DeviceContext::loadWorker ( const std::string& entryPoint )
{
  return d_impl->loadWorker ( entryPoint );
}

void OCPI::OCL::DeviceContext::unloadWorker ( DeviceWorker& worker )
{
  d_impl->unloadWorker ( worker );
}

bool OCPI::OCL::DeviceContext::compile ( const std::string& options,
                                         const std::vector<std::string>& sources,
                                         const std::string& output_file )
{
  return d_impl->compile ( options, sources, output_file );
}


/* ---- Implementation of the DeviceWorker class ------------------------- */

OCPI::OCL::DeviceWorker::Impl::Impl ( const std::string& entryPoint )
  : d_entryPoint ( entryPoint ),
    d_kernel ( 0 ),
    d_grid ( )
{
  // Empty
}

OCPI::OCL::DeviceWorker::Impl::~Impl ( )
{
  // Empty
}


void OCPI::OCL::DeviceWorker::Impl::run ( const OCPI::OCL::Grid& grid )
{
  cl_event event;

  size_t gtid [ 3 ];

  for ( size_t n = 0; n < 3; n++ )
  {
    if ( grid.gtid [ n ] < d_work_group_size [ n ] )
    {
      gtid [ n ] = d_work_group_size [ n ];
    }
    else
    {
      gtid [ n ] = grid.gtid [ n ];
    }
  }

  cl_int rc = clEnqueueNDRangeKernel ( d_cmdq,
                                       d_kernel,
                                       grid.dim,
                                       grid.gtid_offset,
                                       gtid,
                                       d_work_group_size,
                                       0,
                                       0,
                                       &event );
  if ( rc )
  {
    throw OCPI::Util::Error ( "clEnqueueNDRangeKernel() failed : %s", ocl_strerror ( rc ) );
  }

  rc = clWaitForEvents( 1, &event );
  if ( rc )
  {
    throw OCPI::Util::Error ( "clWaitForEvents() failed : %s", ocl_strerror ( rc ) );
  }

  rc = clReleaseEvent ( event );
  if ( rc )
  {
    throw OCPI::Util::Error ( "clReleaseEvent() failed : %s", ocl_strerror ( rc ) );
  }
}

void OCPI::OCL::DeviceWorker::Impl::setKernelArg ( size_t arg_idx,
                                                   void* ptr )
{
  BufferTable::iterator it = d_buffers.find ( ( uintptr_t ) ptr );

  if ( it == d_buffers.end ( ) )
  {
    throw OCPI::Util::Error ( "Unregistered host pointer" );
  }

  cl_int rc = clSetKernelArg ( d_kernel,
                               arg_idx,
                               sizeof ( cl_mem ),
                               (void*) &it->second.mem );
  if ( rc )
  {
    throw OCPI::Util::Error ( "clSetKernelArg() failed : %s", ocl_strerror ( rc ) );
  }
}

void OCPI::OCL::DeviceWorker::Impl::setKernelArg ( size_t arg_idx,
                                                   size_t sizeof_arg,
                                                   void* arg )
{
  cl_int rc = clSetKernelArg ( d_kernel,
                               arg_idx,
                               sizeof_arg,
                               arg );
  if ( rc )
  {
    throw OCPI::Util::Error ( "clSetKernelArg() failed : %s", ocl_strerror ( rc ) );
  }
}

void OCPI::OCL::DeviceWorker::Impl::registerPtr ( void* ptr,
                                                  size_t n_bytes )
{

  BufferTable::iterator it = d_buffers.find ( ( uintptr_t ) ptr );

  if ( it != d_buffers.end ( ) )
  {
    throw OCPI::Util::Error ( "Pointer %p already registered", ptr );
  }

  if ( it == d_buffers.end ( ) )
  {
    cl_int rc;

    Buffer buffer;

    buffer.ptr = ptr;
    buffer.n_bytes = n_bytes;

    buffer.mem = clCreateBuffer ( d_context,
                                  CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR,
                                  buffer.n_bytes,
                                  buffer.ptr,
                                  &rc );
    if ( rc )
    {
      throw OCPI::Util::Error ( "clCreateBuffer() failed : %s", ocl_strerror ( rc ) );
    }
    d_buffers [ ( uintptr_t ) ptr ] = buffer;
  }
}

void OCPI::OCL::DeviceWorker::Impl::unregisterPtr ( void* ptr )
{
  BufferTable::iterator it = d_buffers.find ( ( uintptr_t ) ptr );

  if ( it == d_buffers.end ( ) )
  {
    throw OCPI::Util::Error ( "Unknown pointer %p in unregisterPtr()", ptr );
  }

  cl_int rc = clReleaseMemObject ( it->second.mem );

  if ( rc )
  {
    throw OCPI::Util::Error ( "clReleaseMemObject() failed : %s", ocl_strerror ( rc ) );
  }

  d_buffers.erase ( it );
}


void* OCPI::OCL::DeviceWorker::Impl::mapPtr ( void* ptr,
                                              OCPI::OCL::DeviceWorker::MapFlags flag )
{
  BufferTable::iterator it = d_buffers.find ( ( uintptr_t ) ptr );

  if ( it == d_buffers.end ( ) )
  {
    throw OCPI::Util::Error ( "Unknown pointer %p in mapPtr()", ptr );
  }

  cl_int rc;

  void* mapped_ptr = clEnqueueMapBuffer ( d_cmdq,
                                          it->second.mem,
                                          CL_TRUE,
                                          ( flag == OCPI::OCL::DeviceWorker::MAP_TO_READ ) ? CL_MAP_READ
                                                                                           : CL_MAP_WRITE,
                                          0,
                                          it->second.n_bytes,
                                          0,
                                          0,
                                          0,
                                          &rc );
  if ( rc )
  {
    throw OCPI::Util::Error ( "clEnqueueMapBuffer() failed : %s", ocl_strerror ( rc ) );
  }

  return mapped_ptr;
}

void OCPI::OCL::DeviceWorker::Impl::unmapPtr ( void* ptr )
{
  BufferTable::iterator it = d_buffers.find ( ( uintptr_t ) ptr );

  if ( it == d_buffers.end ( ) )
  {
    throw OCPI::Util::Error ( "Unknown pointer %p in unmapPtr()", ptr );
  }

  cl_int rc = clEnqueueUnmapMemObject ( d_cmdq,
                                        it->second.mem,
                                        ptr,
                                        0,
                                        0,
                                        0 );
  if ( rc )
  {
    throw OCPI::Util::Error ( "clEnqueueUnmapMemObject() failed : %s", ocl_strerror ( rc ) );
  }
}

void OCPI::OCL::DeviceWorker::Impl::syncPtr ( void* ptr,
                                              SyncDirection flag )
{
  BufferTable::iterator it = d_buffers.find ( ( uintptr_t ) ptr );

  if ( it == d_buffers.end ( ) )
  {
    throw OCPI::Util::Error ( "Unknown pointer %p in syncPtr()", ptr );
  }

  if ( flag == OCPI::OCL::DeviceWorker::HOST_TO_DEVICE )
  {
    cl_int rc = clEnqueueWriteBuffer ( d_cmdq,
                                       it->second.mem,
                                       CL_TRUE,
                                       0,
                                       it->second.n_bytes,
                                       ptr,
                                       0,
                                       0,
                                       0 );
    if ( rc )
    {
      throw OCPI::Util::Error ( "clEnqueueWriteBuffer() failed : %s", ocl_strerror ( rc ) );
    }
  }
  else /* Device to host */
  {
    cl_int rc = clEnqueueReadBuffer ( d_cmdq,
                                      it->second.mem,
                                      CL_TRUE,
                                      0,
                                      it->second.n_bytes,
                                      ptr,
                                      0,
                                      0,
                                      0 );
    if ( rc )
    {
      throw OCPI::Util::Error ( "clEnqueueReadBuffer() failed : %s", ocl_strerror ( rc ) );
    }
  }
}


/* ---- DeviceWorker wrapper class --------------------------------------- */

OCPI::OCL::DeviceWorker::DeviceWorker ( const std::string& entryPoint )
  : d_impl ( new Impl ( entryPoint ) )
{
  // Empty
}

OCPI::OCL::DeviceWorker::~DeviceWorker ( )
{
  // Empty
}

void OCPI::OCL::DeviceWorker::run ( const OCPI::OCL::Grid& grid )
{
  d_impl->run ( grid );
}

void OCPI::OCL::DeviceWorker::setKernelArg ( size_t arg_idx,
                                             void* ptr )
{
  d_impl->setKernelArg ( arg_idx, ptr );
}

void OCPI::OCL::DeviceWorker::setKernelArg ( size_t arg_idx,
                                             size_t sizeof_arg,
                                             void* arg )
{
  d_impl->setKernelArg ( arg_idx, sizeof_arg, arg );
}

void OCPI::OCL::DeviceWorker::registerPtr ( void* ptr,
                                            size_t n_bytes )
{
  d_impl->registerPtr ( ptr, n_bytes );
}

void OCPI::OCL::DeviceWorker::unregisterPtr ( void* ptr )
{
  d_impl->unregisterPtr ( ptr );
}

void* OCPI::OCL::DeviceWorker::mapPtr ( void* ptr,
                                        OCPI::OCL::DeviceWorker::MapFlags flag )
{
  return d_impl->mapPtr ( ptr, flag );
}

void OCPI::OCL::DeviceWorker::unmapPtr ( void* ptr )
{
  d_impl->unmapPtr ( ptr );
}

void OCPI::OCL::DeviceWorker::syncPtr ( void* ptr,
                                        SyncDirection flag )
{
  d_impl->syncPtr ( ptr, flag );
}

