APP=ext2file
include ../setup.mk

run: all
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc $(VG) $(PROG) && \
	echo Finished.  File contents are:
	cat hello.file

clean::
	rm -f -r hello.file

