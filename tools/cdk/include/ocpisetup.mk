# This make file fragment establishes environment variables for user Makefiles,
# all based on OCPI_CDK_DIR, which might be set already.
# The output (via $(info)), is a series of bash-style export assignments.
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
export OCPI_RUNTIME_SYSTEM:=$(shell $(OCPI_CDK_DIR)/scripts/showRuntimeHost)
#$(info export OCPI_RUNTIME_SYSTEM=$(OCPI_RUNTIME_SYSTEM))
export OCPI_LIB_DIR:=$(OCPI_CDK_DIR)/lib/$(OCPI_RUNTIME_SYSTEM)
#$(info export OCPI_LIB_DIR=$(OCPI_LIB_DIR))
export OCPI_INC_DIR:=$(OCPI_CDK_DIR)/include
#$(info export OCPI_INC_DIR=$(OCPI_INC_DIR))
#$(info OCPI_RUNTIME_SYSTEM=$(OCPI_RUNTIME_SYSTEM) =$(findstring,darwin,$(OCPI_RUNTIME_SYSTEM))=)
ifneq ($(findstring darwin,$(OCPI_RUNTIME_SYSTEM)),)
Ocpilibrarypathenv=DYLD_LIBRARY_PATH
OCPI_OCL_LIBS=-locl_container -framework OpenCL
else
OcpiLibraryPathEnv=LD_LIBRARY_PATH
OCPI_OCL_LIBS=-locl_container -lOpenCL
endif
export OCPI_SET_LIB_PATH=$(OcpiLibraryPathEnv)=$$$(OcpiLibraryPathEnv):$(OCPI_LIB_DIR)
#$(info export OCPI_SET_LIB_PATH=$(OCPI_SET_LIB_PATH))
export OCPI_API_LIBS=interfaces rcc_container rdma_drivers util # ocpios
#$(info export OCPI_API_LIBS=$(OCPI_API_LIBS))

