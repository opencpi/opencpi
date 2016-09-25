set -e -x
OPTS=$1
BIAS=$2
BIAS0=$3
CMP=" && cmp test.input test.output"
$VG ocpirun -v -d $OPTS $BIAS $FR $FW bias
$VG ocpirun -v -d -pbias=biasValue=0 $OPTS $BIAS $FR $FW bias $CMP
$VG ocpirun -v -d $OPTS $FR $FW copy $CMP
$VG ocpirun -v -d $OPTS $BIAS $FR file-bias-capture
$VG ocpirun -v -d $OPTS hello
$VG ocpirun -v -d $OPTS $BIAS $FW pattern-bias-file
$VG ocpirun -v -d $OPTS $BIAS $FW pattern
$VG ocpirun -v -d $OPTS $FR $FW proxybias
$VG ocpirun -v -d -pproxy=proxybias=0 $OPTS $BIAS $FR $FW proxybias $CMP
$VG ocpirun -v -d $OPTS $BIAS tb_bias
$VG ocpirun -v -d $OPTS $BIAS $FR tb_bias_file
$VG ocpirun -v -d $OPTS $BIAS $FR $FW testbias
$VG ocpirun -v -d -pbias=biasValue=0 $OPTS $BIAS $FR $FW testbias $CMP
$VG ocpirun -v -d $OPTS $BIAS0 $FR $FW testbias2
$VG ocpirun -v -d -pbias0=biasValue=0 -pbias1=biasValue=0 $OPTS $BIAS0 $FR $FW testbias2 $CMP

