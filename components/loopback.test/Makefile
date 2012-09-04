ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard $(OCPI_BASE_DIR)/tools/cdk/export/),$(OCPI_BASE_DIR)/tools/cdk/ocpiocl/bin)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

all: 
	cd ../unit.test; make
	rm -f unitTest; ln -s ../unit.test/unitTest .
	rm -f genExpected; ln -s ../genExpected.test/genExpected .



# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.
run: all
	OCPI_LIBRARY_PATH=$(OCPI_BASE_DIR)/components/lib/rcc ./test.sh 

clean::
	rm -f -r unitTest unitTest.dSYM *.*~ *~ *.raw

# These make the worker library for this application
Implementations =
include $(OCPI_INC_DIR)/lib.mk


