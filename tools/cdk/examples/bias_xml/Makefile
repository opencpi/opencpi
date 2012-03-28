ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

all: biasFile

test.input:
	g++ -Wall mkTestFile.cxx
	./a.out > test.input

biasFile: biasFile.cxx
	g++ -Wall -g -o $@ -I$(OCPI_INC_DIR) $^ $(OCPI_LD_FLAGS)


run: all test.input
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc ./biasFile '$(ARGS)' > /dev/null	
	od -X test.output

clean::
	rm -f -r biasFile biasFile.dSYM test.output test.input a.out
