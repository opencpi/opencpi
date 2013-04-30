APP=appHello
include ../setup.mk

run: all
	OCPI_LIBRARY_PATH=lib/rcc $(PROG) $(OUT)

# These make the worker library for this application
Implementations=hello.rcc
include $(OCPI_INC_DIR)/lib.mk
