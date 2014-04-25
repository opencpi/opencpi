# This setup is based on what the default (14.6) xilinx root fs is capable of.
# Namely it can do NFS clients, but no NTP.
if test $# != 5; then
  echo You must supply 5 arguments to this script.
  echo Usage is: setup.sh '<nfs-ip-address> <nfs-share-name> <opencpi-dir> <ntp-server> <timezone>'
else
  # Set up the timezone for this shell
  export TZ=$5
  # Mount the opencpi development system as an NFS server, onto /mnt/net
  mount -t nfs -o udp,nolock $1:$2 /mnt/net
  # Make $zed point to the zed platform directory in the OpenCPI dev tree
  zed=/mnt/net/$3/platforms/zed
  # Run an NTP client so the Zed board knows what time it is
  $zed/ntpclient -h $4 -s
  # Copy the (missing) C++ runtime environment library into the current RAM rootFS
  cp $zed/libstdc++.so.6 /lib
  # Run the generic script to setup the OpenCPI environment
  source /mnt/net/$3/ocpi/ocpisetup.sh /mnt/net/$3/ocpi/ocpisetup.sh
  # Load the OpenCPI linux kernel driver into the running (Xilinx) kernel
  ocpidriver load
  # Tell the ocpihdl utility to always assume the FPGA device is the zynq PL.
  export OCPI_DEFAULT_HDL_DEVICE=pl:0
fi

