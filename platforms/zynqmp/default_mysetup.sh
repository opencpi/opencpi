# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

# This script should be customized to do what you want.
# It is used in two contexts:
# 1. The core setup has not been run, so run it with your specific parameters
#    (mount point on development host, etc.), and supply the IP address as arg
# 2. The core setup HAS been run and you are just setting up a shell or ssh session

trap "trap - ERR; break" ERR; for i in 1; do
if test "$OCPI_CDK_DIR" = ""; then
  source /home/root/opencpi/zynqmp_setup.sh
  # add any commands to be run only the first time this script is run

  break # this script will be rerun recursively by setup.sh
fi
alias ll='ls -lt'
# Tell the ocpihdl utility to always assume the FPGA device is the zynq_ultra PL.
export OCPI_DEFAULT_HDL_DEVICE=pl:0
# The system config file sets the default SMB size
export OCPI_SYSTEM_CONFIG=/home/root/opencpi/system.xml
# Get ready to run some test xml-based applications
cd $OCPI_CDK_DIR/applications

# add any commands to be run every time this script is run


# Print the available containers as a sanity check
echo Discovering available containers...
ocpirun -C
# Since we are sourcing this script we can't use "exit", do "done" is for "break"
done
trap - ERR
