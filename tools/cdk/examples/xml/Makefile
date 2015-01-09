ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

all:

run:
	$(VG) ocpirun -v -d bias
	$(VG) ocpirun -v -d copy
	$(VG) ocpirun -v -d file-bias-capture
	$(VG) ocpirun -v -d -t 5 hello
	$(VG) ocpirun -v -d pattern-bias-file
	$(VG) ocpirun -v -d pattern
	$(VG) ocpirun -v -d proxybias
	$(VG) ocpirun -v -d tb_bias
	$(VG) ocpirun -v -d tb_bias_file
	$(VG) ocpirun -v -d testbias
	$(VG) ocpirun -v -d testbias2

test:

clean::
