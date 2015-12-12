# If there is a "mysetup.sh" script in this directory it will run it after the
# other setup items, and arrange for it to be run in any login scripts later
# e.g. ssh logins
if test $# != 2; then
  echo You must supply 2 arguments to this script.
  echo Usage is: zedsetup.sh '<ntp-server> <timezone>'
  echo A good example timezone is: EST5EDT,M3.2.0,M11.1.0
  echo If the ntp-server is '"-"', no ntpclient will be started.
else
  export OCPI_ROOT_DIR=/mnt/card/opencpi
  export OCPI_CDK_DIR=/mnt/card/opencpi
  # In case dhcp failed on eth0, try it on eth1
  if test "$1" != -; then
    echo Attempting to set the time from time server: $1
    if rdate $1; then
      echo Succeeded in setting the time from time server: $1
    else
      echo ====YOU HAVE NO NETWORK CONNECTION and NO HARDWARE CLOCK====
      echo Set the time using the '"date YYYY.MM.DD-HH:MM[:SS]"' command.
    fi
  fi
  # Copy the (missing) C++ runtime environment library into the current RAM rootFS
  # cp $OCPI_ROOT_DIR/lib/libstdc++.so.6 /lib
  # Make sure the hostname is in the host table
  myhostname=`hostname`
  if ! grep -q $myhostname /etc/hosts; then echo 127.0.0.1 $myhostname >> /etc/hosts; fi
  # Run the generic script to setup the OpenCPI environment
  # Note the ocpidriver load command is innocuous if run redundantly
  cat <<EOF > $HOME/.profile
    export OCPI_ROOT_DIR=$OCPI_ROOT_DIR
    export OCPI_CDK_DIR=$OCPI_CDK_DIR
    export PATH=$PATH:$OCPI_ROOT_DIR/bin
    export OCPI_LIBRARY_PATH=$OCPI_ROOT_DIR/artifacts
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OCPI_CDK_DIR/lib/linux-zynq-arm
    ocpidriver load
    export TZ=$2
    echo OpenCPI ready for zed.
    if test -r $OCPI_ROOT_DIR/mysetup.sh; then
       source $OCPI_ROOT_DIR/mysetup.sh
    fi
EOF
  source $HOME/.profile
fi

