include $(OCPI_CDK_DIR)/ocpisetup.mk

all: unitTest

unitTest: unitTest.cxx
	$(Gc++_$(RccTarget)) -Wall -g -o $@ -I$(OCPI_INC_DIR) -I$(OCPI_BASE_DIR)/runtime/util/misc/include $^ $(OCPI_LD_FLAGS) 

# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.

clean::
	rm -f -r unitTest unitTest.dSYM *.*~

include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk
