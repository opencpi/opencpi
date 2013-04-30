APP=catHello
include ../setup.mk

# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.
run: all
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc $(VG) $(PROG) hello.file > /dev/null	

