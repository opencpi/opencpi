# This script should be customized to do what you want.
# It is used in two contexts:
# 1. The core setup has not been run, so run it with your specific parameters
#    (mount point on development host, etc.), and supply the IP address as arg
# 2. The core setup HAS been run and you are just setting up a shell or ssh session

for i in x; do
if test "$OCPI_ROOT_DIR" = ""; then
  if test "$1" = ""; then
     echo It appears that the environment is not set up yet.
     echo You must supply the IP address of the OpenCPI server machine as an argument to this script.
     break
  fi
  # CUSTOMIZE THIS LINE FOR YOUR ENVIRONENT
  source /mnt/card/ocpizedsetup.sh $1 /Users/jek/Business ocpi/opencpi time.apple.com EDT
  break # this script will be rerun recursively by setup.sh
fi
# Tell the ocpihdl utility to always assume the FPGA device is the zynq PL.
export OCPI_DEFAULT_HDL_DEVICE=pl:0
# Set my OCPI path to some bitstream directories I am working on.
export OCPI_LIBRARY_PATH=$OCPI_LIBRARY_PATH:$OCPI_ROOT_DIR/hdl/assemblies/biascapture:$OCPI_ROOT_DIR/hdl/assemblies/testbias:$OCPI_ROOT_DIR/hdl/assemblies/patternbias
export OCPI_SMB_SIZE=100000
# Load a particular bitstream
#ocpihdl -v load /mnt/net/ocpi/opencpi/hdl/assemblies/biascapture/target-biascapture_zed_base/*.gz
# Get ready to run some test xml-based applications
cd $OCPI_ROOT_DIR/tools/cdk/examples/xml
# Shorten the default shell prompt
PS1='% '
# Print the available containers as a sanity check
ocpirun -C
done
