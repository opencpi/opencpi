# If there is a "mynetsetup.sh" script in this directory it will run it after the
# other setup items, and arrange for it to be run in any login scripts later
# e.g. ssh logins
if test $# != 5; then
  echo You must supply 5 arguments to this script.
  echo Usage is: zednetsetup.sh '<nfs-ip-address> <nfs-share-name> <opencpi-dir> <time-server> <timezone>'
  echo A good example timezone is: EST5EDT,M3.2.0,M11.1.0
else
  if ifconfig | grep -v 127.0.0.1 | grep 'inet addr:' > /dev/null; then
     echo An IP address was detected.
  else
     echo No IP address was detected! No network or no DHCP.
     break;
  fi
  echo Setting the time from time server: $4
  rdate $4
  # Mount the opencpi development system as an NFS server, onto /mnt/net
  mount -t nfs -o udp,nolock,soft,intr $1:$2 /mnt/net
  # Make $zed point to the zed platform directory in the OpenCPI dev tree
  zed=/mnt/net/$3/platforms/zed
  # Copy the (missing) C++ runtime environment library into the current RAM rootFS
  cp $zed/libstdc++.so.6 /lib
  # Make sure the hostname is in the host table
  myipaddr=`ifconfig | grep -v 127.0.0.1 | sed -n '/inet addr:/s/^.*inet addr: *\([^ ]*\).*$/\1/p'`
  myhostname=`hostname`
  echo My IP address is: $myipaddr, and my hostname is: $myhostname
  if ! grep -q $myhostname /etc/hosts; then echo $myipaddr $myhostname >> /etc/hosts; fi
  # Run the generic script to setup the OpenCPI environment
  # Note the ocpidriver load command is innocuous if run redundantly
  export OCPI_BASE_DIR=/mnt/net/$3
  cat <<EOF > $HOME/.profile
    export OCPI_BASE_DIR=$OCPI_BASE_DIR
    source $OCPI_BASE_DIR/ocpi/ocpisetup.sh $OCPI_BASE_DIR/ocpi/ocpisetup.sh
    ocpidriver load
    export TZ=$5
    export LD_LIBRARY_PATH+=$OCPI_BASE_DIR/ocpi/lib/linux-zynq-arm
    echo OpenCPI ready for zed.
    if test -r /mnt/card/opencpi/mynetsetup.sh; then
       source /mnt/card/opencpi/mynetsetup.sh
    else
       echo Error: enable to find /mnt/card/opencpi/mynetsetup.sh
    fi
EOF
  source $HOME/.profile
fi

