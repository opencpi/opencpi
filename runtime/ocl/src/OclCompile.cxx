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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "OclContainer.h"
#include "OclDevice.h"
namespace OCPI {
  namespace OCL {
    
    // Compile the provided, in-memory source files.
    void compile(size_t nSources, const char **mapped_sources, off_t *sizes,
		 const char **includes, const char **defines, const char *output,
		 const char *target, bool /*verbose*/) {
      ocpiDebug("OCL Compile %zu sources to target %s", nSources, target);
      Device &device = OCPI::OCL::Driver::getSingleton().find(target);
      ocpiDebug("OCL Compile for device %s", device.name().c_str());
      cl_int rc;
      cl_program program;
      OCL_RC(program,
	     clCreateProgramWithSource(device.context(), OCPI_UTRUNCATE(cl_uint, nSources),
				       mapped_sources, (size_t *)sizes, &rc));
      std::string options;
      OU::format(options, "-g -D__STDCL__ -D__%s__", device.isCPU() ? "CPU" : "GPU");
      for (const char **ap = defines; ap && *ap; ap++)
	OU::formatAdd(options, " -D%s", *ap);
      for (const char **ap = includes; ap && *ap; ap++)
	OU::formatAdd(options, " -I%s", *ap);
      ocpiDebug("Compiler options: %s\n", options.c_str());
      if ((rc = clBuildProgram(program, 1, &device.id(), options.c_str(), NULL, NULL)) &&
	  rc != CL_BUILD_PROGRAM_FAILURE)
	throwOclError(rc, "Compiling worker");
      size_t n, n1;
      OCL(clGetProgramBuildInfo(program, device.id(), CL_PROGRAM_BUILD_LOG, 0, 0, &n));
      std::vector<char> log(n + 1, 0);
      OCL(clGetProgramBuildInfo(program, device.id(), CL_PROGRAM_BUILD_LOG, n, &log[0], 0));
      if (log[0]) {
	fprintf(stderr, "%s", &log[0]);
	if (log[strlen(&log[0]) - 1] != '\n')
	  fprintf(stderr, "\n");
      }
      if (rc)
	throw OU::Error("OpenCL compilation had errors");
      OCL(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(n), &n, &n1));
      assert(n1 == sizeof(n));
      std::vector<unsigned char> binary(n);
      unsigned char *ucp = &binary[0];
      OCL(clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(ucp), &ucp, 0));
      int fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd < 0 || write(fd, &binary[0], n) != (ssize_t)n || close(fd))
	throw OU::Error("Error creating or writing output file: %s", output);
    }
  }
}
