
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/.
export CPI_OCFRP_DUMMY=myshm
export CPI_DUMP_PORTS=/home/jmiller/cpi/core/dataplane/tests/linux-bin/myports

#export CPI_DUMP_PORTS=./myports
#./testRpl -l & sleep 1; ./testRpl -i 10; echo done

./testRpl -l 



