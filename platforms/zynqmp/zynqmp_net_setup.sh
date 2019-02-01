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

# If there is a "mynetsetup.sh" script in this directory it will run it after the
# other setup items, and arrange for it to be run in any login scripts later
# e.g. ssh logins
if test -z  "$3"; then
  echo You must supply at least 3 arguments to this script.
  echo Usage is: zynqmp_net_setup.sh '<nfs-ip-address> <nfs-share-name> <opencpi-dir> [<hdl-platform>]'
else
  if test -n "$4"; then
     echo OCPI_HDL_PLATFORM set to $4.
  fi
  if ifconfig | grep -v 127.0.0.1 | grep 'inet addr:' > /dev/null; then
     echo An IP address was detected.
  else
     echo No IP address was detected! No network or no DHCP.
     break;
  fi
  mkdir -p /mnt/net
  # Mount the opencpi development system as an NFS server, onto /mnt/net
  mount -t nfs -o udp,nolock,soft,intr $1:$2 /mnt/net
  # Make sure the hostname is in the host table
  myipaddr=`ifconfig | grep -v 127.0.0.1 | sed -n '/inet addr:/s/^.*inet addr: *\([^ ]*\).*$/\1/p'`
  myhostname=`hostname`
  echo My IP address is: $myipaddr, and my hostname is: $myhostname
  if ! grep -q $myhostname /etc/hosts; then echo $myipaddr $myhostname >> /etc/hosts; fi
  # Run the generic script to setup the OpenCPI environment
  # Note the ocpidriver load command is innocuous if run redundantly
  # Some Zynq-based SD cards are ephemeral and lose $HOME on reboots. Others don't.
  # This tries to handle both cases sanely.
  if test -d /etc/profile.d; then
    export PROFILE_FILE=/etc/profile.d/opencpi-persist.sh
  else
    export PROFILE_FILE=$HOME/.profile
  fi
  export OCPI_CDK_DIR=/mnt/net/$3
  cat <<EOF > $PROFILE_FILE
  if test -e /mnt/net/$3; then
    echo Executing $PROFILE_FILE
    export OCPI_CDK_DIR=$OCPI_CDK_DIR
    if test -f /etc/opencpi-release; then
      read OCPI_TOOL_PLATFORM x < /etc/opencpi-release
    else
      echo No /etc/opencpi-release - assuming xilinx18_2 hardware
      OCPI_TOOL_PLATFORM=xilinx18_2
    fi
    export OCPI_TOOL_PLATFORM
    export OCPI_TOOL_OS=linux
    export OCPI_TOOL_DIR=\$OCPI_TOOL_PLATFORM
    # As a default, access all built RCC artifacts from the core project
    export OCPI_LIBRARY_PATH+=:$OCPI_CDK_DIR/\$OCPI_TOOL_PLATFORM/artifacts
    # Priorities for finding system.xml:
    # 1. If is it on the local system it is considered customized for this system - use it.
    if test -r /home/root/opencpi/system.xml; then
      OCPI_SYSTEM_CONFIG=/home/root/opencpi/system.xml
    # 2. If is it at the top level of the mounted CDK, it is considered customized for all the
    #    systems that use this CDK installation (not shipped/installed by the CDK)
    elif test -r $OCPI_CDK_DIR/system.xml; then
      OCPI_SYSTEM_CONFIG=$OCPI_CDK_DIR/system.xml
    # 3. If there is one for this HDL platform, it is considered more specific than one that is
    #    specific to the RCC platform, so it should be used in preference to the RCC platform one.
    elif test -n "$OCPI_HDL_PLATFORM" -a -r $OCPI_CDK_DIR/$OCPI_HDL_PLATFORM/system.xml; then
      OCPI_SYSTEM_CONFIG=$OCPI_CDK_DIR/$OCPI_HDL_PLATFORM/system.xml
    # 4. If there is one for this RCC platform, it is more specific than the default one.
    elif test -r $OCPI_CDK_DIR/\$OCPI_TOOL_PLATFORM/system.xml; then
      OCPI_SYSTEM_CONFIG=$OCPI_CDK_DIR/\$OCPI_TOOL_PLATFORM/system.xml
    # 5. Finally use the default one that is very generic.
    else
      OCPI_SYSTEM_CONFIG=$OCPI_CDK_DIR/default-system.xml
    fi
    export OCPI_SYSTEM_CONFIG
    export PATH=$OCPI_CDK_DIR/\$OCPI_TOOL_DIR/bin:\$PATH

    # On certain ZynqMP boards, a bitstream must be loaded before ANY OpenCPI use
    # (even ocpihdl). So, if /home/root/opencpi/<hw-platform>/default_opencpi.bin
    # exists, load that now using fpga_manager
    # A temporarily exported variable was needed here or it appeared unset in the following lines
    if test -n "$OCPI_HDL_PLATFORM"; then
      export OCPI_HDL_PLATFORM_TMP=$OCPI_HDL_PLATFORM
    else
      export OCPI_HDL_PLATFORM_TMP=zcu102
    fi
    if test -e /home/root/opencpi/$OCPI_HDL_PLATFORM_TMP/default_opencpi.bin; then
      echo Loading default OpenCPI bitstream
      echo 0 > /sys/class/fpga_manager/fpga0/flags
      mkdir -p /lib/firmware
      cp /home/root/opencpi/$OCPI_HDL_PLATFORM_TMP/default_opencpi.bin /lib/firmware/
      echo default_opencpi.bin > /sys/class/fpga_manager/fpga0/firmware
    fi

    # This is only for ACI executables in special cases...
    export LD_LIBRARY_PATH=$OCPI_CDK_DIR/\$OCPI_TOOL_DIR/lib:\$LD_LIBRARY_PATH

    ocpidriver load
    echo OpenCPI ready for zynq_ultra.
    if test -r /home/root/opencpi/mynetsetup.sh; then
       source /home/root/opencpi/mynetsetup.sh
    else
       echo Error: enable to find /home/root/opencpi/mynetsetup.sh
    fi
  else
    echo NFS mounts not yet set up. Please mount the OpenCPI CDK into /mnt/net/.
  fi
EOF
  echo Running login script. OCPI_CDK_DIR is now $OCPI_CDK_DIR.
  source $PROFILE_FILE
fi
