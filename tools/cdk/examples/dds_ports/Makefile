ifndef OCPI_OPENSPLICE_HOME
$(info Set OCPI_OPENSPLICE_HOME to the OpenSplice install location to use this example (e.g. /usr/local))
$(info OpenSplice/DDS not available.)
all:
run:
else

ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

OPENSPLICE_LIBS= ddsdatabase dcpsgapi dcpssacpp ddsos ddsutil ddsserialization ddskernel ddsuser ddsconf ddsconfparser
OPENSPLICE_LIB_DIR=$(OCPI_OPENSPLICE_HOME)/lib

all: dds_ports

dds_ports: dds_ports.cxx
	g++ -g -o $@ -I$(OCPI_INC_DIR) $^ -L$(OPENSPLICE_LIB_DIR) $(OCPI_LD_FLAGS) $(OPENSPLICE_LIBS:%=-l%)

run: dds_ports workers
	OCPI_LIBRARY_PATH=lib/rcc ./hello > /dev/null	

clean::
	rm -r -f dds_ports *.dSYM *.o

# These make the worker library for this application
Implementations = dds_producer.rcc dds_consumer.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
endif
