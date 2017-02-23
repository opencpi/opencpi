# This script should be customized to do what you want.
# It is used in two contexts:
# 1. The core setup has not been run, so run it with your specific parameters
#    (mount point on development host, etc.), and supply the IP address as arg
# 2. The core setup HAS been run and you are just setting up a shell or ssh session

trap "trap - ERR; break" ERR; for i in 1; do
if test "$OCPI_CDK_DIR" = ""; then
  # CUSTOMIZE THIS LINE FOR YOUR ENVIRONENT
  # First argument is time server for the (old) time protocol used by the rdate command
  # Second argument is timezone spec - see "man timezone" for the format.
  source /mnt/card/opencpi/zynq_setup.sh time.nist.gov EST5EDT,M3.2.0,M11.1.0
  break # this script will be rerun recursively by setup.sh
fi
# Tell the ocpihdl utility to always assume the FPGA device is the zynq PL.
export OCPI_DEFAULT_HDL_DEVICE=pl:0
# The system config file sets the default SMB size
export OCPI_SYSTEM_CONFIG=/mnt/card/opencpi/system.xml
export OCPI_SUPPRESS_HDL_NETWORK_DISCOVERY=1
# Get ready to run some test xml-based applications
cd $OCPI_CDK_DIR/xml
# Shorten the default shell prompt
PS1='% '
# Print the available containers as a sanity check
echo Discovering available containers...
ocpirun -C
# Since we are sourcing this script we can't use "exit", do "done" is for "break"
done
trap - ERR
