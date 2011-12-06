
ifndef OCPI_CDK_DIR
OCPI_CDK_DIR=../..
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

INC_MISC := \
	-I$(OCPI_BASE_DIR)/core/local/util/misc/include \
	-I$(OCPI_BASE_DIR)/adapt/os/ocpios/interfaces/include \
	-I$(OCPI_BASE_DIR)/core/container/ocl_container/include

LDFLAGS=-Xlinker -rpath -Xlinker "$(OCPI_LIB_DIR)" \
         -L"$(OCPI_LIB_DIR)" \
         $(OCPI_API_LIBS:%=-l%) $(OCPI_OCL_LIBS)

all: test_vsadd

test_vsadd: test_vsadd.cxx
	g++ -g -o $@ -I$(OCPI_CDK_DIR)/include $(INC_MISC) $^ $(LDFLAGS)

run: test_vsadd
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/ocl ./test_vsadd

clean::
	rm -f test_vsadd

# These make the worker library for this application
#Implementations = vsadd.ocl
#include $(OCPI_CDK_DIR)/include/lib.mk
