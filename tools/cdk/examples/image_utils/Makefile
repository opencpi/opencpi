ifndef OCPI_CDK_DIR
OCPI_CDK_DIR=../..
endif

LIBDIR=$(OCPI_CDK_DIR)/lib/$(HostTarget)
LIBS=interfaces rcc_container rdma_drivers
ifeq ($(OSTYPE),darwin)
LIB_PATH=export DYLD_LIBRARY_PATH=$(LIBDIR);
else
LIB_PATH=export LD_LIBRARY_PATH=$(LIBDIR);
endif

LIBRARY_PATH 	+= -L/usr/local/lib -L$(OCPI_BASE_DIR)/lib

all: image_utils

image_utils: DemoWorkerFacade.cxx
	$(LIB_PATH) g++ -c -g -o $@.o $(INCS) -I$(OCPI_CDK_DIR)/include $^ 

clean::
	rm -f image_utils.o

# These make the worker library for this application
#Implementations = hello.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
