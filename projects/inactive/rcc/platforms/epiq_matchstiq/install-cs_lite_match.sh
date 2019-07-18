#!/bin/bash --noprofile
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
# This script installs the DRS Code Sourcery compiler as a platform-specific prerequisite,
# but sourced from the vendor repo pointed at via a local symlink: vendor

[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
me=cs_lite_pico
dir=opt/CodeSourcery/Sourcery_G++_Lite
vendor=$AV_PICO_VENDOR_REPO
[ -z "$vendor" ] && vendor=vendor
source $OCPI_CDK_DIR/scripts/setup-prerequisite.sh \
       "$1" \
       $me \
       "tool chain for picoflexor" \
       $vendor \
       compiler.tar \
       $dir \
       0

# We are operating in the "build" directory since that is where setup-install leaves us.
# We aren't really building anything BUT, since these binaries are 32-bit binaries that are
# intolerant of 64-bit-inode file systems, we create little trampolines, in bin64/ here
# to preload the 64-to-32-inode adapter library (inode64) before executing the 32 bit program.
# A global (not sandboxed) installation could force this library to be preloaded for everything
# but we simply control it here for this 32-bit package.
# The mess with "param" is because kernel building does things like
# "-DKBUILD_MODNAME=KBUILD_STR(opencpi)" which causes the "eval" to explode without escaping.
mkdir -p bin64
for b in ../bin/*; do
  base=$(basename $b)
  cat > bin64/$base <<-EOF
	#!/bin/bash --noprofile
	preload=\$OCPI_CDK_DIR/\$OCPI_TOOL_DIR/lib/inode64.so
	[ -f \$preload ] || preload=\$OCPI_PREREQUISITES_DIR/inode64/\$OCPI_TOOL_PLATFORM/lib/inode64.so
	[ -f \$preload ] && DO_PRELOAD=LD_PRELOAD=\$preload\${LD_PRELOAD:+:\$LD_PRELOAD}
	eval \$DO_PRELOAD exec \$(dirname \$(dirname \$0))/bin/$base '"\$@"'
	EOF
  chmod a+x bin64/$base
done
relative_link bin64 $OcpiInstallExecDir
cd ..
relative_link bin $OcpiInstallExecDir
relative_link libexec $OcpiInstallExecDir
relative_link lib $OcpiInstallExecDir
relative_link arm-none-linux-gnueabi $OcpiInstallExecDir
