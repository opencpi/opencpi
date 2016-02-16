
ifneq ($(OCPI_HAVE_OPENCL),1)
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
$(info The vsadd example requires OpenCL to be present, and it isn't)
$(info It is usually found as an libOpenCL.so in the standard library path)
endif
all:
run:
clean:
else

APP=test_vsadd
include ../setup.mk

OCPI_LD_FLAGS+= $(OCPI_OCL_LIBS)

run: $(PROG) all
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/ocl $(VG) $(PROG)

endif
