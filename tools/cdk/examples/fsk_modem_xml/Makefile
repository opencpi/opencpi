ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

all: fsk_modem

fsk_modem: fsk_modem.cxx
	g++ -Wall -g -o $@ -I$(OCPI_INC_DIR) -I$(OCPI_BASE_DIR)/core/local/util/misc/include $^ $(OCPI_LD_FLAGS)

# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.
run: all
	OCPI_LIBRARY_PATH=$(OCPI_BASE_DIR)/components/lib/rcc ./fsk_modem > /dev/null	

clean::
	rm -f -r fsk_modem fsk_modem.dSYM timeData.* *.~

# These make the worker library for this application
Implementations =
include $(OCPI_INC_DIR)/lib.mk


