#!/bin/sh

# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

# This script assumes the following.
# 1. The caller is using a bash shell.
# 2. The OpenCPI environment script has been sourced.
# 3. The script is being invoked from the top-level of the OpenCPI source tree.

echo ""
echo "# #### Start of OpenCPI parameter dump ############################### #"
echo ""

echo ""
echo "# #### OS ############################################################# #"
echo ""

echo "# #### cat /proc/version"
cat /proc/version

echo ""
echo "# #### cat /etc/redhat-release"
cat /etc/redhat-release

echo ""
echo "# #### uname -a"
uname -a

echo ""
echo "# #### Hardware ####################################################### #"
echo ""

echo "# #### cat /proc/cpuinfo"
cat /proc/cpuinfo

echo ""
echo "# #### cat /proc/meminfo"
cat /proc/meminfo

echo ""
echo "# #### cat /proc/devices"
cat /proc/devices

echo ""
echo "# #### /sbin/lspci  -vvv"
/sbin/lspci  -vvv

echo ""
echo "# #### RPMs ########################################################## #"
echo ""

echo "# #### rpm -qa"
rpm -qa 

echo ""
echo "# #### Environment ################################################### #"
echo ""

echo "# #### env"
env

echo ""
echo "# #### Check for prerequisites ####################################### #"
echo ""

OCPI_ENV_GTEST_DIR=/opt/opencpi/$OCPI_BUILD_HOST/prerequisites/gtest
if [ -d $OCPI_ENV_GTEST_DIR ] 
then
  echo "Found Google Test prerequisite ($OCPI_ENV_GTEST_DIR)."
else
  echo "Unable to find Google Test prerequisite ($OCPI_ENV_GTEST_DIR)."
fi

echo ""
OCPI_ENV_OMNIORB_DIR=/opt/opencpi/$OCPI_BUILD_HOST/prerequisites/omniorb
if [ -d $OCPI_ENV_OMNIORB_DIR ] 
then
  echo "Found OmniORB prerequisite ($OCPI_ENV_OMNIORB_DIR)."
else
  echo "Unable to find OmniORB prerequisite ($OCPI_ENV_OMNIORB_DIR)."
fi

echo ""
if [ -d $OCPI_XILINX_TOOLS_DIR ] 
then
  echo "Found Xilinx prerequisite ($OCPI_XILINX_TOOLS_DIR)."
else
  echo "Unable to find Xilinx prerequisite ($OCPI_XILINX_TOOLS_DIR)."
fi

echo ""
echo "# #### Show OpenCPI version ######################################## #"
echo ""

echo "# #### git show --name-only HEAD"
git show --name-only HEAD

echo ""
echo "# #### End of OpenCPI parameter dump ################################# #"
echo ""

