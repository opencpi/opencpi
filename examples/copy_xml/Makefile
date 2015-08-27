APP=copyFile
include ../setup.mk

run: all
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc $(VG) $(PROG) $(Transport) > /dev/null	
	cmp hello.file out.file

clean::
	rm -f -r out.file
