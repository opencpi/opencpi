ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard $(OCPI_BASE_DIR)/tools/cdk/export/),$(OCPI_BASE_DIR)/tools/cdk/ocpiocl/bin)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

all: genExpected

genExpected: genExpected.cxx
	$(Gc++_$(RccTarget)) -Wall -g -o $@ -I$(OCPI_INC_DIR) -I$(OCPI_BASE_DIR)/core/local/util/misc/include $^ $(OCPI_LD_FLAGS) 

# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.

clean::
	rm -f -r genExpected genExpected.dSYM *.*~

include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk
