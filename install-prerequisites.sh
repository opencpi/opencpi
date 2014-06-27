#!/bin/sh
echo This script assumes you have already done the following:
echo 1. Installed Centos6 '(e.g. from a Centos6.5 Live CD)'.
echo 2. You are in a user account with sudo enabled.
echo 3. You have done '"sudo yum update"' followed by a reboot.
echo '*'Note, this script will create the /opt/opencpi directory.
echo ' 'That is NOT where the OpenCPI code tree will go, but where
echo ' 'various prerequisite packages will be installed, and where
echo ' 'the development kit will be installed.
echo 4. You should be in the top level directory of the OpenCPI code base repository,
echo '   usually called "opencpi"'
/bin/echo -n Is this is all OK and do you want to proceed with the installation'\? (y or n): '
read ans
if [ $ans != y ]; then
  echo OK, try again when you have done those things.
  exit 1
fi
trap "trap - ERR; return" ERR
echo ================================================================================
echo Installing standard extra packages using "yum"
sudo yum -y groupinstall "development tools"
echo Installing packages required: tcl pax python-devel
sudo yum -y install tcl pax python-devel
echo Installing 32 bit libraries '(really only required for modelsim)'
sudo yum -y install glibc.i686 libXft.i686 libXext.i686 ncurses-lib.i686
echo ================================================================================
echo All basic prerequisites are installed in the system.
echo Now we will create /opt/opencpi and make it read/write for everyone
sudo mkdir -p /opt/opencpi
sudo chmod a+rwx /opt/opencpi
echo ================================================================================
echo We are running in `pwd` where the git clone of opencpi has been placed.
echo Next, before building OpenCPI, we will install some prerequisites in /opt/opencpi.
mkdir -p /opt/opencpi/prerequisites
. ./centos6_env.sh
echo ================================================================================
echo Installing Google test '(gtest)' under /opt/opencpi/prerequisites
./install-gtest.sh
echo ================================================================================
echo All OpenCPI prerequisites have been installed.
