
# Cross-build on x86 Linux for PPC Linux
export ORB=OMNI
export VIEW=
export PPP_INC=/opt/mercury/include
export PPP_LIB=/opt/mercury/linux-MPC8641D/lib/librose.a
export CPIDIR=/home/jmiller/projects/opencpi/cpi
export SYSTEM=linux
export MKDEPEND=/h/mpepe/bin/fpmkdepend
export BUILDSHAREDLIBRARIES=0
export DEBUG=1
export ASSERT=1
export USE_CPIP_SIMULATION=0
export HAVE_CORBA=1
export ACE_ROOT=/opt/TAO/5.6.6/linux-ppc86xx-gcc/ACE_wrappers
export HOST_ROOT=/opt/TAO/5.6.6/linux-x86_64-gcc/ACE_wrappers
export LD_LIBRARY_PATH=$HOST_ROOT/lib:$LD_LIBRARY_PATH
export CROSS_HOST=ppc86xx-linux
export CROSS_BUILD_BIN=/opt/timesys/toolchains/$CROSS_HOST/bin
export OMNIDIR=/usr/local/
export OUTDIR=$SYSTEM-ppc-bin
PATH=$PATH:~/bin

