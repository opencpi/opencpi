include ../setup.mk

all:

run: all 
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc $(VG) ocpirun -d -v $(ARGS) ptest.xml > /dev/null

clean::

