rifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

all:

CMP= && cmp test.input test.output

run:
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) bias
	$(VG) ocpirun -v -d -pbias=biasValue=0 $(OPTS) $(FR) $(FW) bias $(CMP)
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) copy $(CMP)
	$(VG) ocpirun -v -d $(OPTS) $(FR) file-bias-capture
	$(VG) ocpirun -v -d -t 5 $(OPTS) hello
	$(VG) ocpirun -v -d $(OPTS) $(FW) pattern-bias-file
	$(VG) ocpirun -v -d $(OPTS) $(FW) pattern
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) proxybias
	$(VG) ocpirun -v -d -pproxy=proxybias=0 $(OPTS) $(FR) $(FW) proxybias $(CMP)
	$(VG) ocpirun -v -d $(OPTS) tb_bias
	$(VG) ocpirun -v -d $(OPTS) $(FR) tb_bias_file
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) testbias
	$(VG) ocpirun -v -d -pbias=biasValue=0 $(OPTS) $(FR) $(FW) testbias $(CMP)
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) testbias2
	$(VG) ocpirun -v -d -pbias0=biasValue=0 -pbias1=biasValue=0 $(OPTS) $(FR) $(FW) testbias2 $(CMP)

test:

clean::
