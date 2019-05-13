# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

##########################################################################################
# This file defines the macos10_13 software platform.
# It sets platform variables as necessary to override the defaults in the file:
#   include/platform-defaults.mk file.
# See that file for a description of valid variables and their defaults.

OcpiRequiredCFlags:=$(patsubst -frecord-gcc-switches,,$(OcpiRequiredCFlags))
OcpiRequiredCXXFlags:=$(patsubst -frecord-gcc-switches,,$(OcpiRequiredCXXFlags))
OcpiRequiredCFlags+=-Werror=unknown-warning-option
OcpiRequiredCXXFlags+=-Werror=unknown-warning-option
OcpiLibraryPathEnv=DYLD_LIBRARY_PATH
OcpiRpathOrigin=@executable_path
OcpiDynamicLibrarySuffix=.dylib
OcpiDynamicLibraryFlags=-dynamiclib
OcpiPluginFlags=-Wl,-flat_namespace -Wl,-dylib
OcpiDynamicProgramFlags=-shared -no-undefined -Wl,-rpath,@executable_path
OcpiRccLDFlags:=-g -dynamiclib -Xlinker -undefined -Xlinker dynamic_lookup
#OcpiOpenclLib=/System/Library/Frameworks/OpenCL.framework/Versions/A/OpenCL
OcpiStaticSwigFlags=
OcpiCanRemoveNeeded=0
# -install_name @rpath/$(notdir $@)
# MacOS uses CLANG/LLVM
OcpiCXX=c++
OcpiCXXLD=c++
OcpiExtraLibs=
#export OCPI_OCL_LIBS=-locpi_ocl -framework OpenCL
OcpiAsNeeded=
OcpiUnknownWarningsError=-Werror=unknown-warning-option
OcpiStaticProgramFlags=-Xlinker -export_dynamic -Wl,-no_pie
#export OCPI_OPENCL_LIB=/System/Library/Frameworks/OpenCL.framework/Versions/A/OpenCL
OcpiSWIG=$(wildcard /opt/local/bin/swig)
OcpiPlatformOs=macos
OcpiPlatformOsVersion=10_14
OcpiPlatformArch=x86_64
# This is where the officially installed python headers go.
OcpiPythonInclude=/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7
