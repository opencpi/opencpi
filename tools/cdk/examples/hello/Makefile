ifndef OCPI_CDK_DIR
OCPI_CDK_DIR=../..
endif

LIBDIR=$(OCPI_CDK_DIR)/lib/$(OcpiHostTarget)
LIBS=interfaces rcc_container rdma_drivers util
LIB_PATH=export $(OcpiLibraryPathEnv)=$(LIBDIR);

all: hello

hello: hello.cxx
	$(LIB_PATH) g++ -Wall -g -o $@ -I$(OCPI_CDK_DIR)/include $^ -L$(LIBDIR) $(LIBS:%=-l%)

run: hello workers
	$(LIB_PATH) export OCPI_LIBRARY_PATH=lib/rcc; ./hello > /dev/null	

clean::
	rm -f hello

# These make the worker library for this application
Implementations = hello.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
