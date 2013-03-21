ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

all: ext2file

ext2file: ext2file.cxx
	g++ -Wall -g -o $@ -I$(OCPI_INC_DIR) $^ $(OCPI_LD_FLAGS)

# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.
run: all
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc $(VG) ./ext2file && \
	echo Finished.  File contents are:
	cat hello.file

clean::
	rm -f -r ext2file ext2file.dSYM hello.file

