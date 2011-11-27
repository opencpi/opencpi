ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

# These linker flags tell the linker:
# 1. When executed at runtime, look in OCPI_LIB_DIR to resolve shared libraries
# 2. At link time, look in OCPI_LIB_DIR to find explicitly mentioned libraries
# 3. At link time, look in OCPI_API_LIBS for functions called from the program
LDFLAGS=-Xlinker -rpath -Xlinker "$(OCPI_LIB_DIR)" \
         -L"$(OCPI_LIB_DIR)" \
         $(OCPI_API_LIBS:%=-l%)

all: hello

hello: hello.cxx
	g++ -Wall -g -o $@ -I$(OCPI_INC_DIR) $(LDFLAGS) $^

# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.
run: hello workers
	OCPI_LIBRARY_PATH=lib/rcc ./hello > /dev/null	

clean::
	rm -f -r hello hello.dSYM

# These make the worker library for this application
Implementations = hello.rcc
include $(OCPI_INC_DIR)/lib.mk
