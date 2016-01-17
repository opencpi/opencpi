APP=appHello
include ../setup.mk

run: all
	OCPI_LIBRARY_PATH=lib/rcc $(VG) $(PROG) $(OUT)

# These make the worker library for this application
Implementations=hello.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
