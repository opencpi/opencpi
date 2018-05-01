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
# This file provides defaults and documentation for platform-specific build variables.
# Any platform can override these except that the "required" flags should only be
# "translated" if needed, not removed.
# The defaults are basically what CentOS6 wants, so everything other platform definition
# is essentially empty except to the extent that it deviates from the CentOS6 baseline.
# This file might be read more than once to RESET all the variables to their defaults.
#
# Some conventions:
#  - a boolean variable is 1 for true, otherwise false.
#  - linker related flags assume the use of the compiler command for linking
#    i.e. they would typically need a -Xlinker or -Wl, etc.
#  - these variables are "immediate/simply-expanded" make variables,
#    not "deferred/recursively-expanded" make variables
#  - Variable names are camelcase to make it crystal clear they are not environment variables
#    I.e. if we DO want environment variables we use all upper case

ifndef __PLATFORM_DEFAULTS_MK__
__PLATFORM_DEFAULTS_MK__:=1

# The list of all variables defaulted and settable by platforms
# The value of this variable should not be changed lightly since it may affect all platforms.
# Each variable has a comment with the default values below.
# In case insensitive alphabetical order
OcpiAllPlatformVars:=\
  OcpiAR OcpiAsNeeded OcpiCanRemoveNeeded \
  OcpiCC OcpiCFlags OcpiCLD OcpiCrossCompile OcpiCXX OcpiCXXFlags OcpiCXXLD \
  OcpiDebugOffFlags OcpiDebugOnFlags OcpiDriverFlags \
  OcpiDynamicCompilerFlags OcpiDynamicLibraryFlags OcpiDynamicLibrarySuffix \
  OcpiDynamicProgramFlags OcpiDynamicSwigFlags \
  OcpiExtraLibs OcpiKernelDir \
  OcpiLibraryPathEnv OcpiOclLibs OcpiOptionalCWarnings OcpiOptionalCXXWarnings \
  OcpiPlatform OcpiPlatformArch OcpiPlatformDir OcpiPlatformOs OcpiPlatformOsVersion \
  OcpiPlatformPrerequisites \
  OcpiRccCXXFlags  OcpiRccLDFlags \
  OcpiRequiredCFlags OcpiRequiredCPPFlags OcpiRequiredCXXFlags OcpiRpathOrigin \
  OcpiStaticLibraryFlags OcpiStaticLibrarySuffix OcpiStaticProgramFlags OcpiStaticSwigFlags \
  OcpiStrictCFlags OcpiStrictCXXFlags OcpiSTRIP OcpiSWIG \
  OcpiUnknownWarningsError \

# The list of variables containing optional warnings needed early (for autoreconf).
OcpiAllOptionalWarningsVars:=OcpiOptionalCWarnings OcpiOptionalCXXWarnings

# Can this platform support removing dynamic library references from a linked dynamic library
# (e.g. remote SO_NEEDED entries, using patchelf)
OcpiCanRemoveNeeded:=1
# The name of the environment variable for dynamic library loading.
# Rarely used, but needed for some cases.
OcpiLibraryPathEnv:=LD_LIBRARY_PATH
# The string used in executable/dynamic-library rpath settings indicating where the
# executable lives (the directory it is in).
OcpiRpathOrigin:=$${ORIGIN}
OcpiStaticLibrarySuffix:=.a
# linker flags when linking a static executable
OcpiStaticProgramFlags:=-Xlinker -export-dynamic
# flags when linking a static library
OcpiStaticLibraryFlags:=
OcpiDynamicCompilerFlags:=-fPIC
OcpiDynamicLibrarySuffix:=.so
# linker flags when creating a dynamic library
OcpiDynamicLibraryFlags:=-shared -Xlinker --no-undefined
# linker flags when creating an executable linked with dynamic libraries
OcpiDynamicProgramFlags:=
# linker flags when creating a swig library linked against static libraries: FIXME: needed?
OcpiStaticSwigFlags:=-Xlinker -export-dynamic
# linker flags when creating a swig library linked against dynamic libraries
OcpiDynamicSwigFlags:=
# linker flags when creating a driver/plugin library
# Note this is NOT an immediate variable
OcpiDriverFlags=$(OcpiDynamicLibraryFlags)
# When linking against dynamic libraries, these options indicate that libraries on the
# linker command line that are not referenced should not be "needed" by the resulting
# executable or dynamic library
OcpiAsNeeded:=-Xlinker --no-as-needed
# The system libraries that are required on this platform for the framework.
# rt is typical for clockgettime, dl for dlopen.
# on some platforms these are all implicit
OcpiExtraLibs:=rt dl pthread
# The system's default library flag when using OpenCL
OcpiOclLibs:=-lOpenCL
# String indicating the (absolute) pathname prefix for tools for this platform.
# If blank, then it is not a cross-compiled platform
# Example for using zynq tools from xilinx
#    include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk
#    OcpiCrossCompile:=$(OcpiXilinxEdkDir)/gnu/arm/lin/bin/arm-xilinx-linux-gnueabi-
OcpiCrossCompile:=
########## The variables for "commands" and "languages" use all upper case to be slightly more
# similar to other make or autotools variables etc.
# These default -g flags are not intended to suppress optimization, just keep sym info around
# See DebugOn/Off flags below
OcpiCFlags:=-g -pipe
OcpiCXXFlags:=-g -pipe
# typical "make"
# The commands used to build for this platform
OcpiCC:=gcc
OcpiCXX:=c++
# These are the linker commands to use, per language
OcpiCLD:=gcc
OcpiCXXLD:=c++
OcpiAR:=ar
OcpiSTRIP:=strip
# Where linux kernel headers should be found for out-of-tree building of OpenCPI kernel driver
OcpiKernelDir=$(OcpiPlatformDir)/kernel-headers
# Other than -g, there is no need to force optimization off
OcpiOptimizedOffFlags:=
# FIXME: discuss whether we need to specifically offer independent assert control
# FIXME: discuss whether we should universally make the default to be debug (yes)
# This is biased for best optimizations (bipolar)
OcpiOptimizeOnFlags:=-O2 -NDEBUG=1
# For all code:
# We require these or their equivalent be supported.  If not its an error
# They should not be overridden or reset by a platform, except perhaps to change the syntax
# when some equivalent name or syntax is used.
OcpiRequiredCPPFlags:= -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
OcpiRequiredCFlags:= -Wall -Wfloat-equal -Wextra -fno-strict-aliasing -Wformat -Wuninitialized \
                     -Winit-self -Wshadow -frecord-gcc-switches -fstack-protector \
                     -Wno-conversion -Wno-sign-conversion -std=c99
OcpiRequiredCXXFlags:=$(filter-out -std%,$(OcpiRequiredCFlags)) -std=c++0x
# For all code:
# We prefer these warnings or their equivalent be supported.  It is not an error if they are not.
# If they are supported they are "error" warnings in strict cases, otherwise normal warnings
OcpiOptionalCWarnings:=maybe-uninitialized misleading-indentation null-dereference \
                       unused-result unused-but-set-variable
OcpiOptionalCXXWarnings:=$(OcpiOptionalCWarnings)
# For "native" OpenCPI code (as opposed to imported code):
# We prefer to ADD these or their equivalent.  It is not an error if they are not supported.
OcpiStrictCFlags:=-Wconversion -Werror=parentheses -Werror=unused-parameter -Werror=empty-body \
	          -Werror=write-strings -Werror=reorder -Werror=extra -Werror=comment \
                  -Werror=format -Werror=init-self -Wsign-conversion
OcpiStrictCXXFlags:=$(OcpiStrictCFlags)
# What we want to burden users with by default
OcpiRccCXXFlags:=-Wall -Wextra
OcpiRccLDFlags:=-g -shared
# Set this to the option that cause unknown warnings to be errors
# The default is that unknown warnings are expected to be warned
OcpiUnknownWarningsError:=
# Either relative to where the platform is defined in its directory, or absolute
OcpiKernelDir:=
OcpiPlatformOs:=linux
OcpiPlatformArch:=x86_64
OcpiPlatformOsVersion:=
OcpiPlatformPrerequisites:=
endif
