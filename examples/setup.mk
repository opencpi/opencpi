ifndef OCPI_CDK_DIR
OCPI_CDK_DIR=$(realpath ../../exports)
endif

include $(OCPI_CDK_DIR)/include/ocpisetup.mk

DIR=target-$(OCPI_TARGET_DIR)
PROG=$(DIR)/$(APP)
OUT= > /dev/null

INCS = -I$(OCPI_INC_DIR)

all: $(PROG)

$(DIR):
	mkdir -p $(DIR)

$(PROG): $(APP).cxx | $(DIR)
	$(AT)echo Building $@...
	$(AT)$(CXX) -g -Wall $(OCPI_EXPORT_DYNAMIC) -o $@ $(INCS) $^ $(OCPI_LD_FLAGS)

clean::
	$(AT)rm -r -f lib target-* *.*~ timeData.raw output_image.jpg test.output

