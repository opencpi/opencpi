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
  if test "$1" = ""; then
     echo It appears that the environment is not set up yet.
     echo You must supply the IP address of the OpenCPI server machine as an argument to this script.
     break
  fi

  # CUSTOMIZE THIS LINE FOR YOUR ENVIRONMENT
  # Second arg is shared file system mount point on development system
  # Third argument is opencpi dir relative to mount point
  source /home/root/opencpi/zynqmp_net_setup.sh $1 /opt/opencpi cdk
  # mkdir -p /mnt/ocpi_core
  # mount -t nfs -o udp,nolock,soft,intr $1:/home/user/core /mnt/ocpi_core
  # mkdir -p /mnt/ocpi_assets
  # mount -t nfs -o udp,nolock,soft,intr $1:/home/user/ocpi_assets /mnt/ocpi_assets
  # mkdir -p /mnt/bsp_zcu111
  # mount -t nfs -o udp,nolock,soft,intr $1:/home/user/bsp_zcu111 /mnt/bsp_zcu111
  # add any commands to be run only the first time this script is run

  break # this script will be rerun recursively by setup.sh
fi
# Below this (until "done") is optional user customizations
alias ll='ls -lt'
# Tell the ocpihdl utility to always assume the FPGA device is the zynq_ultra PL.
export OCPI_DEFAULT_HDL_DEVICE=pl:0
# Only override this file if it is customized beyond what is the default for the platform
# export OCPI_SYSTEM_CONFIG=/home/root/opencpi/system.xml
# Get ready to run some test xml-based applications

# add any commands to be run every time this script is run

# Print the available containers as a sanity check
echo Discovering available containers...
ocpirun -C
# Since we are sourcing this script we can't use "exit", so "done" is for "break" from "for"
done
trap - ERR
