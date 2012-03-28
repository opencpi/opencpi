ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

all: appHello

appHello: appHello.cxx
	g++ -Wall -g -o $@ -I$(OCPI_INC_DIR) $^ $(OCPI_LD_FLAGS)

# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.
run: all
	OCPI_LIBRARY_PATH=lib/rcc ./appHello > /dev/null	

clean::
	rm -f -r appHello appHello.dSYM

# These make the worker library for this application
Implementations = hello.rcc
include $(OCPI_INC_DIR)/lib.mk
