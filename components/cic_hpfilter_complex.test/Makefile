ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard $(OCPI_BASE_DIR)/tools/cdk/export/),$(OCPI_BASE_DIR)/tools/cdk/ocpiocl/bin)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

all: 
	rm -f unitTest; ln -s ../unit.test/unitTest .
	rm -f utTime.sh; ln -s ../unit.test/utTime.sh .
	rm -f genExpected; ln -s ../genExpected.test/genExpected .



# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.
run: all
	OCPI_LIBRARY_PATH=$(OCPI_BASE_DIR)/components/lib/rcc ./test.sh 

clean::
	rm -f -r *.*~ *~ *.raw deltaDataOut.dat testDataOut.dat genExpected unitTest utTime.sh *.csv
