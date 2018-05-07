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

##########################################################################################
version=7.2.1-2017.11
minor=`sed 's/\([0-9]*\.[0-9]*\)\.[0-9]*\(-.*$\)/\1\2/' <<<$version`
dir=gcc-linaro-$version-x86_64_arm-linux-gnueabihf
me=linaro-arm-gnueabihf # could be from ${0} etc.
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-prerequisite.sh \
       "$1" \
       $me \
       "Tool chain for Altera SoC" \
       https://releases.linaro.org/components/toolchain/binaries/$minor/arm-linux-gnueabihf \
       $dir.tar.xz \
       $dir \
       0
# Don't need a build dir since this is a binary distribution.
cd ..
# The tool chain finds ancillary files ok so all we need is bin.
relative_link bin $install_exec_dir
