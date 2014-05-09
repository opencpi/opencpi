# This setup is based on what the default (14.6) xilinx root fs is capable of.
# Namely it can do NFS clients, but no NTP client.
# If there is a "mysetup.sh" script in this directory it will run it after the
# other setup items, and arrange for it to be run in any login scripts later
# e.g. ssh logins
if test $# != 5; then
  echo You must supply 5 arguments to this script.
  echo Usage is: setup.sh '<nfs-ip-address> <nfs-share-name> <opencpi-dir> <ntp-server> <timezone>'
else
  # Mount the opencpi development system as an NFS server, onto /mnt/net
  mount -t nfs -o udp,nolock,soft,intr $1:$2 /mnt/net
  # Make $zed point to the zed platform directory in the OpenCPI dev tree
  zed=/mnt/net/$3/platforms/zed
  # Run an NTP client so the Zed board knows what time it is
  $zed/ntpclient -h $4 -s
  # Copy the (missing) C++ runtime environment library into the current RAM rootFS
  cp $zed/libstdc++.so.6 /lib
  export OCPI_ROOT_DIR=/mnt/net/$3
  # Run the generic script to setup the OpenCPI environment
  # Note the ocpidriver load command is innocuous if run redundantly
  cat <<EOF > $HOME/.profile
    export OCPI_ROOT_DIR=$OCPI_ROOT_DIR
    source $OCPI_ROOT_DIR/ocpi/ocpisetup.sh $OCPI_ROOT_DIR/ocpi/ocpisetup.sh
    ocpidriver load
    export TZ=$5
    if test -r $OCPI_ROOT_DIR/platforms/zed/mysetup.sh; then
       source $OCPI_ROOT_DIR/platforms/zed/mysetup.sh
    fi
EOF
  source $HOME/.profile
fi

