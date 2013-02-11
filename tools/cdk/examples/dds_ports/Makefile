ifneq (OCPI_HAVE_OPENSPLICE,1)
#$(info Set OCPI_OPENSPLICE_HOME to the OpenSplice install location to use this example (e.g. /usr/local))
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

all: dds_ports SATests

SATests:
	pushd ./standalone_dds/Test/C++/Standalone/Posix; \
	make; \
	popd;

dds_ports: dds_ports.cxx
	g++ -g -o $@ -I$(OCPI_INC_DIR) $^ $(OPENSPLICE_LD_FLAGS) $(OCPI_LD_FLAGS) $(OPENSPLICE_LIBS:%=-l%)

run: dds_ports workers SATests
	@if test "$(OCPI_HAVE_OPENSPLICE)" == ""; then \
	  echo "*******Cannot execute OpenSplice on this machine for dds testing. Skipping execution." ; \
	else \
	. $(OCPI_OPENSPLICE_HOME)/release.com; ospl start; \
	( OCPI_LIBRARY_PATH=lib/rcc ./dds_ports & )   ; \
	sleep 2; \
	./standalone_dds/Test/C++/Standalone/Posix/exec/TestPublisher  ; \
	fi

clean::
	rm -r -f dds_ports *.dSYM *.o ; \
	pushd ./standalone_dds/Test/C++/Standalone/Posix; \
        make clean; \
        popd;

# These make the worker library for this application
Implementations = dds_producer.rcc dds_consumer.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
endif
