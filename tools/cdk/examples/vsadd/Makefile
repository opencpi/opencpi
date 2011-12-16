ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

ifndef OCPI_HAVE_OPENCL
$(info The vsadd example requires OpenCL to be present, and it isn't)
$(info It is usually found as an libOpenCL.so in the standard library path)
all:
run:
clean:
else

INCS := -I$(OCPI_INC_DIR) \
	-I$(OCPI_BASE_DIR)/core/local/util/misc/include \
	-I$(OCPI_BASE_DIR)/adapt/os/ocpios/interfaces/include \
	-I$(OCPI_BASE_DIR)/core/container/ocl_container/include

LDFLAGS=$(OCPI_LD_FLAGS) $(OCPI_OCL_LIBS)

all: test_vsadd

test_vsadd: test_vsadd.cxx
	g++ -g -o $@ $(INCS) $^ $(LDFLAGS)

run: test_vsadd
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/ocl ./test_vsadd

clean::
	rm -f -r *.dSYM test_vsadd

endif
