# If there is a "mynetsetup.sh" script in this directory it will run it after the
# other setup items, and arrange for it to be run in any login scripts later
# e.g. ssh logins
if test $# != 5; then
  echo You must supply 5 arguments to this script.
  echo Usage is: zynq_net_setup.sh '<nfs-ip-address> <nfs-share-name> <opencpi-dir> <time-server> <timezone>'
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
  # Make sure the hostname is in the host table
  myipaddr=`ifconfig | grep -v 127.0.0.1 | sed -n '/inet addr:/s/^.*inet addr: *\([^ ]*\).*$/\1/p'`
  myhostname=`hostname`
  echo My IP address is: $myipaddr, and my hostname is: $myhostname
  if ! grep -q $myhostname /etc/hosts; then echo $myipaddr $myhostname >> /etc/hosts; fi
  # Run the generic script to setup the OpenCPI environment
  # Note the ocpidriver load command is innocuous if run redundantly
  export OCPI_CDK_DIR=/mnt/net/$3
  cat <<EOF > $HOME/.profile
    echo Executing $HOME/.profile.
    export OCPI_CDK_DIR=$OCPI_CDK_DIR
    if test -f /etc/opencpi-release; then
      read OCPI_TOOL_PLATFORM OCPI_TOOL_HOST x < /etc/opencpi-release
    else
      echo No /etc/opencpi-release - assuming ZedBoard hardware
      OCPI_TOOL_PLATFORM=zed
      OCPI_TOOL_HOST=linux-x13_4-arm
    fi
    export OCPI_TOOL_PLATFORM
    export OCPI_TOOL_HOST
    export OCPI_TOOL_DIR=\$OCPI_TOOL_HOST\${OCPI_TOOL_MODE:+/$OCPI_TOOL_MODE}
    export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/lib/components/rcc/\$OCPI_TOOL_DIR:$OCPI_CDK_DIR/lib/hdl/assemblies
    export PATH=$OCPI_CDK_DIR/bin/\$OCPI_TOOL_DIR:\$PATH
    # This is only for explicitly-linked driver libraries.  Fixed someday.
    export LD_LIBRARY_PATH=$OCPI_CDK_DIR/lib/$OCPI_TOOL_DIR:\$LD_LIBRARY_PATH
    ocpidriver load
    export TZ=$5
    echo OpenCPI ready for zynq.
    if test -r /mnt/card/opencpi/mynetsetup.sh; then
       source /mnt/card/opencpi/mynetsetup.sh
    else
       echo Error: enable to find /mnt/card/opencpi/mynetsetup.sh
    fi
EOF
  echo Running login script. OCPI_CDK_DIR is now $OCPI_CDK_DIR.
  source $HOME/.profile
fi

