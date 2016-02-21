include ../setup.mk

all:

CMP= && cmp test.input test.output
SCALE=$(VG) ocpirun -v -d -pbias=biasValue=0 $(OPTS) $(FR) $(FW) \
                    -pfile_write=messagesize=200 -pfile_read=messagesize=200 \
                    -Nfile_read=$1 -Nbias=$2 -Nfile_write=$3 \
                      testbias $(CMP)

run:
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) bias
	$(VG) ocpirun -v -d -pbias=biasValue=0 $(OPTS) $(FR) $(FW) bias $(CMP)
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) copy $(CMP)
	$(VG) ocpirun -v -d $(OPTS) $(FR) file-bias-capture
	$(VG) ocpirun -v -d $(OPTS) hello
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

scale:
	$(call SCALE,1,1,1)
	$(call SCALE,1,1,2)
	$(call SCALE,1,2,1)
	$(call SCALE,1,2,2)
	$(call SCALE,2,1,1)
	$(call SCALE,2,1,2)
	$(call SCALE,2,2,1)
	$(call SCALE,2,2,2)

test:

clean::
	rm -f test.out test.output test.outputwrong time_test.out
