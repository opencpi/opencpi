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

// This file is a stub library.
// Like others, it defines the OpenCL entry points we use so that we can link against it
// to test for unresolved symbols even when we have no real implementation of OpenCL present.
#include <stdint.h>

#define CL_DEVICE_NOT_AVAILABLE                     -2
// We return this error for the first function that will be called in case the build
// machinery messes up and actually uses this file at runtime.
typedef int32_t cl_int;
cl_int clBuildProgram(){return 0;}
cl_int clCreateBuffer(){return 0;}
cl_int clCreateCommandQueue(){return 0;}
cl_int clCreateContext(){return 0;}
cl_int clCreateKernel(){return 0;}
cl_int clCreateKernelsInProgram(){return 0;}
cl_int clCreateProgramWithBinary(){return 0;}
cl_int clCreateProgramWithSource(){return 0;}
cl_int clCreateSubDevices(){return 0;}
cl_int clEnqueueMapBuffer(){return 0;}
cl_int clEnqueueNDRangeKernel(){return 0;}
cl_int clEnqueueTask(){return 0;}
cl_int clEnqueueUnmapMemObject(){return 0;}
cl_int clFinish(){return 0;}
cl_int clFlush(){return 0;}
cl_int clGetDeviceIDs(){return 0;}
cl_int clGetDeviceInfo(){return 0;}
cl_int clGetEventInfo(){return 0;}
cl_int clGetKernelInfo(){return 0;}
cl_int clGetKernelWorkGroupInfo(){return 0;}
cl_int clGetPlatformIDs(){return CL_DEVICE_NOT_AVAILABLE;}
cl_int clGetPlatformInfo(){return 0;}
cl_int clGetProgramBuildInfo(){return 0;}
cl_int clGetProgramInfo(){return 0;}
cl_int clReleaseCommandQueue(){return 0;}
cl_int clReleaseContext(){return 0;}
cl_int clReleaseKernel(){return 0;}
cl_int clReleaseMemObject(){return 0;}
cl_int clReleaseProgram(){return 0;}
cl_int clSetKernelArg(){return 0;}
