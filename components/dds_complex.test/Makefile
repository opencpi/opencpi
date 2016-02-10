include $(OCPI_CDK_DIR)/include/ocpisetup.mk

all: ddsTest
	rm -f utTime.sh; ln -s ../unit.test/utTime.sh .

ddsTest: ddsTest.cxx
	$(Gc++_$(RccTarget)) -Wall -g -o $@ $(OCPI_EXPORT_DYNAMIC) -I$(OCPI_INC_DIR) -I$(OCPI_BASE_DIR)/runtime/util/misc/include $^ $(OCPI_LD_FLAGS) 

# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.

run: ddsTest
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc ./ddsTest

clean::
	rm -f -r ddsTest ddsTest.dSYM *.*~ deltaDataOut.dat testDataOut.dat *.csv utTime.sh

include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk
