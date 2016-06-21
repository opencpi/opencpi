include ../setup.mk

all:

CMP= && cmp test.input test.output

run:
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) bias 2>&1 | tee xml_bias_run.log
	$(VG) ocpirun -v -d -pbias=biasValue=0 $(OPTS) $(FR) $(FW) bias 2>&1 | tee xml_bias_withvalue_run.log $(CMP)
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) copy 2>&1 | tee xml_copy_run.log $(CMP)
	$(VG) ocpirun -v -d $(OPTS) $(FR) file-bias-capture 2>&1 | tee xml_file-bias-capture_run.log
	$(VG) ocpirun -v -d $(OPTS) hello 2>&1 | tee xml_hello_run.log
	$(VG) ocpirun -v -d $(OPTS) $(FW) pattern-bias-file 2>&1 | tee xml_pattern-bias-file_run.log
	$(VG) ocpirun -v -d $(OPTS) $(FW) pattern 2>&1 | tee xml_pattern_run.log
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) proxybias 2>&1 | tee xml_proxybias_run.log
	$(VG) ocpirun -v -d -pproxy=proxybias=0 $(OPTS) $(FR) $(FW) proxybias 2>&1 | tee xml_proxybias_withvalue_run.log $(CMP)
	$(VG) ocpirun -v -d $(OPTS) tb_bias 2>&1 | tee xml_tb_bias_run.log
	$(VG) ocpirun -v -d $(OPTS) $(FR) tb_bias_file 2>&1 | tee xml_tb_bias_file_run.log
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) testbias 2>&1 | tee xml_testbias_run.log
	$(VG) ocpirun -v -d -pbias=biasValue=0 $(OPTS) $(FR) $(FW) testbias 2>&1 | tee xml_testbias_withvalue_run.log $(CMP)
	$(VG) ocpirun -v -d $(OPTS) $(FR) $(FW) testbias2 2>&1 | tee xml_testbias2_run.log
	$(VG) ocpirun -v -d -pbias0=biasValue=0 -pbias1=biasValue=0 $(OPTS) $(FR) $(FW) testbias2 2>&1 | tee xml_testbias2_withvalue_run.log $(CMP)
	$(VG) ocpirun -v -d ptest 2>&1 | tee xml_ptest_run.log

test:

clean::
	rm -f test.out test.output test.outputwrong time_test.out *_run.log
