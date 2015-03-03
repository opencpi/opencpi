#!/bin/sh
# Install prerequisite packages for Centos6
echo Installing standard extra packages using "yum"
sudo yum -y groupinstall "development tools"
echo Installing packages required: tcl pax python-devel fakeroot
sudo yum -y install tcl pax python-devel fakeroot
echo Installing 32 bit libraries '(really only required for modelsim)'
sudo yum -y install glibc.i686 libXft.i686 libXext.i686 ncurses-libs.i686
