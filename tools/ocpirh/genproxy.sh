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

#/bin/sh --noprofile
a=$1
t=$2
rhcg=$3
# Run the REDHAWK C++ component generator based on our SCA XML to create the component
# package directory structure. Generate everything normally except the "base class" files,
# which we have implemented separately and copy into the component's directory

# We use RH to generate all the normally generated build files
files="cpp/reconf build.sh cpp/Makefile.am.ide cpp/build.sh cpp/configure.ac cpp/Makefile.am"
files+=" cpp/main.cpp cpp/$a.h cpp/$a.cpp"
set -e
source /etc/profile.d/redhawk.sh
cd $t
$rhcg -f -C . --impl cpp --impldir cpp --template redhawk.codegen.jinja.cpp.component.pull \
      $a.spd.xml $files
# Patch the top level CPP file to simply pass through to base class
ed -s cpp/$a.cpp <<EOF
/::constructor()\$/
/^}/i
${a}_base::constructor();
.
/::serviceFunction()/
/return/c
return ${a}_base::serviceFunction();
.
wq
EOF
# We need to add internal OpenCPI headers, libraries, and linker options to the Makefile.am
oincs="application library container util/misc util/property util/parentChild util/ezxml \
       dataplane/transport/impl util/vfs dataplane/rdma_driver_interface/interfaces util/res \
       util/driver util/assembly util/list util/timeEmit"
OCPI_INCS="-I$OCPI_CDK_DIR/include/aci \
           $(for i in $oincs; do echo -n ' '-I$OCPI_CDK_DIR/../runtime/$i/include; done) \
           -I$OCPI_CDK_DIR/../os/interfaces/include"
pdir=${OCPI_PREREQUISITES_DIR:-/opt/opencpi/prerequisites}
olibs="application container library transport rdma_driver_interface rdma_utils rdma_smb \
       msg_driver_interface util os"
plibs="gmp lzma"
OCPI_LIB_DIR=$OCPI_CDK_DIR/lib/$OCPI_TARGET_DIR
OCPI_LIBS="$(for l in $olibs; do echo -n ' '-locpi_$l; done) \
           $(for p in $plibs; do echo -n ' '$pdir/$p/$OCPI_TARGET_PLATFORM/lib/lib$p.a; done)"
OCPI_LDFLAGS="-Xlinker --export-dynamic"
# Patch the automake makefile to include our headers, libraries and linker options
ed -s cpp/Makefile.am <<EOF
/${a}_CXXFLAGS/s|\$| $OCPI_INCS|
/${a}_LDADD/s|\$| -lbulkio-2.0 -lbulkioInterfaces -L$OCPI_LIB_DIR $OCPI_LIBS -ldl -lrt|
/${a}_LDFLAGS/s|\$| $OCPI_LDFLAGS|
wq
EOF
