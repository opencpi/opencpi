ifndef OCPI_CDK_DIR
OCPI_CDK_DIR=$(realpath ../../exports)
endif

all:

ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  export PATH:=$(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_DIR):$(PATH)
  export OCPI_LIBRARY_PATH=.:$(OCPI_CDK_DIR)/lib/components/rcc:$(OCPI_CDK_DIR)/lib/hdl/assemblies
endif

OUT= > /dev/null
DIR=target-$(OCPI_TOOL_DIR)
ifdef APP
OCPI_DEBUG=1
OcpiAppNoRun=1
PROG=$(DIR)/$(APP)
# override the name since these are not created by ocpidev (yet)
OcpiApp=$(APP)
include $(OCPI_CDK_DIR)/include/application.mk
endif

clean::
	$(AT)rm -r -f lib target-* *.*~ timeData.raw output_image.jpg test.output

