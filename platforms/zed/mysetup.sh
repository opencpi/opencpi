# Run the standard setup script
source /mnt/card/setup.sh 10.0.1.105 /Users/jek/Business ocpi/opencpi time.apple.com EDT
# Set my OCPI path to some bitstream directories I am working on.
export OCPI_LIBRARY_PATH=$OCPI_LIBRARY_PATH:/mnt/net/ocpi/opencpi/hdl/assemblies/biascapture:/mnt/net/ocpi/opencpi/hdl/assemblies/testbias:/mnt/net/ocpi/opencpi/hdl/assemblies/patternbias
export OCPI_SMB_SIZE=100000
# Load a particular bitstream
ocpihdl -v load /mnt/net/ocpi/opencpi/hdl/assemblies/biascapture/target-biascapture_zed_base/*.gz
# Get ready to run some test xml-based applications
cd /mnt/net/ocpi/opencpi/tools/cdk/examples/xml
# Shorten the default shell prompt
PS1='% '
# Print the available containers as a sanity check
ocpirun -C
mp=/mnt/net/ocpi/opencpi/test/hdl/target-linux-zynq-arm/mem_probe
