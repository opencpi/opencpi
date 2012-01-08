ifndef OCPI_OPENSPLICE_HOME
$(info Set OCPI_OPENSPLICE_HOME to the OpenSplice install location to use this example (e.g. /usr/local))
$(info OpenSplice/DDS not available.)
all:
run:
clean:
else

ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

OPENSPLICE_LIBS= dcpssacpp
OPENSPLICE_LD_FLAGS=-L$(OCPI_OPENSPLICE_HOME)/lib -Xlinker -rpath -Xlinker $(OCPI_OPENSPLICE_HOME)/lib
OCPI_API_LIBS+= msg_driver_interface msg_drivers

all: dds_ports

dds_ports: dds_ports.cxx
	g++ -g -o $@ -I$(OCPI_INC_DIR) $(OPENSPLICE_LD_FLAGS) $(OCPI_LD_FLAGS) $^ $(OPENSPLICE_LIBS:%=-l%)

run: dds_ports workers
	. $(OCPI_OPENSPLICE_HOME)/release.com; ospl start; OCPI_LIBRARY_PATH=lib/rcc ./dds_ports	

clean::
	rm -r -f dds_ports *.dSYM *.o

# These make the worker library for this application
Implementations = dds_producer.rcc dds_consumer.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
endif
