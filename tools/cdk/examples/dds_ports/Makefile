ifndef OCPI_CDK_DIR
OCPI_CDK_DIR=../..
endif

LIBDIR=$(OCPI_CDK_DIR)/lib/$(HostTarget)
LIBS=interfaces rcc_container rdma_drivers msg_drivers ddsdatabase dcpsgapi dcpssacpp ddsos ddsutil ddsserialization ddskernel ddsuser ddsconf ddsconfparser
ifeq ($(OSTYPE),darwin)
LIB_PATH=export DYLD_LIBRARY_PATH=$(LIBDIR);
else
LIB_PATH=export LD_LIBRARY_PATH=$(LIBDIR);
endif

all: dds_ports

dds_ports: dds_ports.cxx
	$(LIB_PATH) g++ -g -o $@ -I$(OCPI_CDK_DIR)/include $^ -L$(OSPL_HOME)/lib -L$(LIBDIR) $(LIBS:%=-l%)

run: dds_ports workers
	$(LIB_PATH) export OCPI_LIBRARY_PATH=lib/rcc; ./hello > /dev/null	

clean::
	rm -f dds_ports

# These make the worker library for this application
Implementations = dds_producer.rcc dds_consumer.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
