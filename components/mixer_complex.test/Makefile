include $(OCPI_CDK_DIR)/include/ocpisetup.mk

all: mixerTest
	rm -f utTime.sh; ln -s ../unit.test/utTime.sh .

mixerTest: mixerTest.cxx
	$(Gc++_$(RccTarget)) -Wall -g -o $@ -I$(OCPI_INC_DIR) -I$(OCPI_BASE_DIR)/runtime/util/misc/include $^ $(OCPI_LD_FLAGS) 

# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.

run: mixerTest
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc ./mixerTest

clean::
	rm -f -r mixerTest mixerTest.dSYM *.*~ deltaDataOut.dat testDataOut.dat *.csv utTime.sh

include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk
