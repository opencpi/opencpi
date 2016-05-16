ifndef OCPI_CDK_DIR
OCPI_CDK_DIR=$(realpath ../../exports)
endif

ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  include $(OCPI_CDK_DIR)/include/ocpisetup.mk
  export PATH:=$(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_DIR):$(PATH)
  export OCPI_LIBRARY_PATH=.:$(OCPI_CDK_DIR)/lib/components/rcc
endif

DIR=target-$(OCPI_TARGET_DIR)
PROG=$(DIR)/$(APP)
OUT= > /dev/null

INCS = -I$(OCPI_INC_DIR)

ifdef APP
all: $(PROG)
$(PROG): $(APP).cxx | $(DIR)
	$(AT)echo Building $@...
	$(AT)$(CXX) $(CXXFLAGS) $(OCPI_EXPORT_DYNAMIC) -o $@ $(INCS) $^ $(OCPI_LD_FLAGS)
endif

clean::
	$(AT)rm -r -f lib target-* *.*~ timeData.raw output_image.jpg test.output

