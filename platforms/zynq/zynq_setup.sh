# If there is a "mysetup.sh" script in this directory it will run it after the
# other setup items, and arrange for it to be run in any login scripts later
# e.g. ssh logins
if test $# != 2; then
  echo You must supply 2 arguments to this script.
  echo Usage is: zynq_setup.sh '<ntp-server> <timezone>'
  echo A good example timezone is: EST5EDT,M3.2.0,M11.1.0
  echo If the ntp-server is '"-"', no ntpclient will be started.
else
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
  # Make sure the hostname is in the host table
  myhostname=`hostname`
  if ! grep -q $myhostname /etc/hosts; then echo 127.0.0.1 $myhostname >> /etc/hosts; fi
  # Run the generic script to setup the OpenCPI environment
  # Note the ocpidriver load command is innocuous if run redundantly
  OCPI_CDK_DIR=/mnt/card/opencpi
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
    # There is no multimode support when running standalone
    export OCPI_TOOL_DIR=\$OCPI_TOOL_HOST
    export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/artifacts
    export PATH=$OCPI_CDK_DIR/bin:\$PATH
    # This is only for explicitly-linked driver libraries.  Fixed someday.
    export LD_LIBRARY_PATH=$OCPI_CDK_DIR/lib/\$OCPI_TOOL_DIR:\$LD_LIBRARY_PATH
    ocpidriver load
    export TZ=$2
    echo OpenCPI ready for zynq.
    if test -r $OCPI_CDK_DIR/mysetup.sh; then
       source $OCPI_CDK_DIR/mysetup.sh
    fi
EOF
  echo Running login script. OCPI_CDK_DIR is now $OCPI_CDK_DIR.
  source $HOME/.profile
fi

