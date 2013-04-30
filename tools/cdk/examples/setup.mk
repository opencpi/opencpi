ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

PROG=$(OCPI_TARGET_DIR)/$(APP)
OUT= > /dev/null

all: $(PROG)

$(PROG): $(APP).cxx | $(OCPI_TARGET_DIR)
	g++ -g -Wall -o $@ -I$(OCPI_INC_DIR) $^ $(OCPI_LD_FLAGS)

clean::
	rm -r -f lib target-* *.*~ timeData.raw output_image.jpg

