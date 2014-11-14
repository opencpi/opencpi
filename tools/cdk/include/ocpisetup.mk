# This make file fragment establishes environment variables for user Makefiles,
# all based on OCPI_CDK_DIR, which might be set already.
# 
OcpiThisFile=$(lastword $(MAKEFILE_LIST))
#$(info OcpiThisFile set to $(OcpiThisFile), which is really $(realpath $(OcpiThisFile)))
ifndef OCPI_CDK_DIR
export OCPI_CDK_DIR:= $(abspath $(dir $(abspath $(OcpiThisFile)))..)
else
# Just to be sure it is exported
export OCPI_CDK_DIR
endif

# We run the OCPI_CDK_DIR through the shell to handle ~ (at least).
OCPI_CDK_DIR:=$(shell echo $(OCPI_CDK_DIR))
#$(info export OCPI_CDK_DIR=$(OCPI_CDK_DIR))
ifeq ($(realpath $(shell echo $(OCPI_CDK_DIR))),)
$(error The OCPI_CDK_DIR variable, "$(OCPI_CDK_DIR)", points to a nonexistent directory)
endif
ifneq ($(realpath $(OCPI_CDK_DIR)/include/ocpisetup.mk),$(realpath $(OcpiThisFile)))
ifneq ($(realpath $(OCPI_CDK_DIR)/ocpisetup.mk),$(realpath $(OcpiThisFile)))
$(error Inconsistent usage of this file ($(OcpiThisFile)) vs. OCPI_CDK_DIR ($(OCPI_CDK_DIR)))
endif
endif
#export OCPI_RUNTIME_SYSTEM:=$(shell $(OCPI_CDK_DIR)/scripts/showRuntimeHost)
export OCPI_LIB_DIR:=$(OCPI_CDK_DIR)/lib/$(OCPI_TARGET_HOST)
export OCPI_BIN_DIR:=$(OCPI_CDK_DIR)/bin/$(OCPI_TARGET_HOST)
export OCPI_INC_DIR:=$(OCPI_CDK_DIR)/include
ifneq ($(findstring macos,$(OCPI_TARGET_OS)),)
OcpiLibraryPathEnv=DYLD_LIBRARY_PATH
OCPI_OCL_LIBS=-locpi_ocl -framework OpenCL
OcpiAsNeeded=
OCPI_EXPORT_DYNAMIC=-Xlinker -export_dynamic
else
OcpiLibraryPathEnv=LD_LIBRARY_PATH
OCPI_OCL_LIBS=  -Xlinker --undefined=_ZN4OCPI3OCL6driverE -locl -lOpenCL
OCPI_EXTRA_LIBS=rt dl pthread
OCPI_EXPORT_DYNAMIC=-Xlinker --export-dynamic
# for static builds
ifneq ($(wildcard $(OCPI_LIB_DIR)/*.a),)
OCPI_DRIVER_OBJS=\
  -Xlinker --undefined=_ZN4OCPI3RCC6driverE\
  -Xlinker --undefined=_ZN4OCPI7Library7CompLib6driverE\
  -Xlinker --undefined=_ZN4OCPI3PIO6driverE\
  -Xlinker --undefined=_ZN4OCPI3HDL6driverE\
  -Xlinker --undefined=_ZN4OCPI3DMA6driverE\

endif
OcpiAsNeeded=-Xlinker --no-as-needed
endif
export OCPI_SET_LIB_PATH=$(OcpiLibraryPathEnv)=$$$(OcpiLibraryPathEnv):$(OCPI_LIB_DIR)
#$(info export OCPI_SET_LIB_PATH=$(OCPI_SET_LIB_PATH))
# Note most of these are just required for static linking
export OCPI_API_LIBS=application rcc hdl interfaces library transport rdma_driver_interface rdma_drivers rdma_utils rdma_smb util  msg_driver_interface ocpios
export OCPI_TRANSPORT_LIBS=rdma_drivers util  msg_driver_interface  msg_drivers
#$(info export OCPI_API_LIBS=$(OCPI_API_LIBS))

ifndef OCPI_PREREQUISITES_DIR
  OCPI_PREREQUISITES_DIR:=/opt/opencpi/prerequisites
endif

#FIXME  this registration should be somewhere else.
ifndef OCPI_PREREQUISITES_LIBS
  OCPI_PREREQUISITES_LIBS:=lzma
endif
# These linker flags tell the linker:
# 1. When executed at runtime, look in OCPI_LIB_DIR to resolve shared libraries
# 2. At link time, look in OCPI_LIB_DIR to find explicitly mentioned libraries
# 3. At link time, look in OCPI_API_LIBS for functions called from the program
export OCPI_LD_FLAGS= $(OCPI_DRIVER_OBJS) $(OcpiAsNeeded) -Xlinker -rpath -Xlinker "$(OCPI_LIB_DIR)" -L"$(OCPI_LIB_DIR)" $(OCPI_API_LIBS:%=-locpi_%) $(OCPI_EXTRA_LIBS:%=-l%) $(foreach l,$(OCPI_PREREQUISITES_LIBS),-l $l -L $(OCPI_PREREQUISITES_DIR)/$l/$(OCPI_TARGET_HOST)/lib)

ifeq ($(origin OCPI_HAVE_OPENCL),undefined)
OCPI_HAVE_OPENCL:=$(if $(realpath $(OCPI_BIN_DIR)/ocpiocl),$(shell $(OCPI_BIN_DIR)/ocpiocl test; if [ $$? = 0 ]; then echo 1; fi),)
endif
#$(info OCL=$(OCPI_HAVE_OPENCL)=)
ifeq ($(origin OCPI_SUDO),undefined)
OCPI_SUDO=sudo -E
endif
OCPI_TARGET_DIR=target-$(OCPI_TARGET_HOST)

ifeq ($(OCPI_CROSS_HOST),)
CC = gcc
CXX = c++
LD = c++
else
CC = $(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-gcc
CXX = $(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-c++
LD = $(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-c++
AR = $(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-ar
endif

all:
$(OCPI_TARGET_DIR):
	mkdir $@

