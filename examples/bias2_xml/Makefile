APP=biasFile
include ../setup.mk

good.output test.input: mkTestFile.cxx
	g++ -Wall -o $(DIR)/a.out mkTestFile.cxx
	$(DIR)/a.out $(FILE_SIZE) > test.input 2> good.output

run: all test.input
	$(VG) $(PROG) $(ARGS) > /dev/null	
	cmp test.output good.output

runxml: all test.input
	$(VG) ocpirun -d -v $(ARGS) bias2.xml > /dev/null
	cmp test.output good.output

runhdl1: all test.input
	$(PROGS) $(ARGS)
	cmp test.output good.output

runhdl2: all test.input
	$(PROGS) $(ARGS)
	cmp test.output good.output

clean::
	rm -f -r test.output test.input good.output
