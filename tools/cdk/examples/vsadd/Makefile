
ifndef OCPI_HAVE_OPENCL
$(info The vsadd example requires OpenCL to be present, and it isn't)
$(info It is usually found as an libOpenCL.so in the standard library path)
all:
run:
clean:
else
APP=test_vsadd
include ../setup.mk

OCPI_INC_DIR+=\
	-I$(OCPI_BASE_DIR)/core/local/util/misc/include \
	-I$(OCPI_BASE_DIR)/adapt/os/ocpios/interfaces/include \
	-I$(OCPI_BASE_DIR)/core/container/ocl_container/include

OCPI_LD_FLAGS+= $(OCPI_OCL_LIBS)

run: test_vsadd
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/ocl $(PROGS)

endif
