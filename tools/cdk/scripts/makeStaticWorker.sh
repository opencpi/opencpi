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

if test "$#" = 0; then
  echo This script takes a dynamic RCC worker artifact as input and creates one
  echo suitable for being dynamicly loaded into a statically linked executable.
  echo Usage is:  makeStaticWorker infile [lib1...]
  echo The first argument is the input file and the rest of the arguments are the names
  echo of the needed ocpi libraries will not be required of the output. The output is a file
  echo whose name is the same as the input with a _s suffix added BEFORE the file extension.
  echo Typically this means:  foo.so as input creates foo_s.so
  exit 1
fi

# This gets around some of the RPM-based limitations
# OCPI_CDK_DIR/../bin/ may not (yet) be in PATH.
ocpixml=$(type -p ocpixml)
[ -z ${ocpixml} ] && ocpixml=${OCPI_CDK_DIR}/bin/${OCPI_TOOL_DIR}/ocpixml
[ -x ${ocpixml} ] || echo "Could not find ocpixml!"

OS=$1
shift
IN=$1
shift
OUT=$1
shift
if [ $OS = linux ]; then
  patchelf=$OCPI_PREREQUISITES_DIR/patchelf/bin/patchelf
  [ -x ${patchelf} ] || patchelf=$OCPI_PREREQUISITES_DIR/patchelf/$OCPI_TOOL_DIR/bin/patchelf
  [ -x ${patchelf} ] || echo "Could not find patchelf executable!"
fi

T1=/tmp/makeStaticWorker$$.1
T2=/tmp/makeStaticWorker$$.2
set -e
$ocpixml get $IN > $T1
$ocpixml strip $IN $T2
X=1
if ! grep '<artifact.*dynamic=.1' $T1 > /dev/null; then
  echo The file \"$1\" does not contain XML from an RCC worker.
elif file $T2 | grep ELF > /dev/null; then
  for i in $*; do PARGS="$PARGS --remove-needed $i"; done
  ${patchelf} $PARGS $T2
  X=0
elif test $OS = macos; then
  # The patchelf is unnecessary on MacOS since it does not have the DT_NEEDED entries anyway
  X=0    
else
  echo "Input file $1 is non-existent or not readable or not a suitable (ELF) file."
fi
if test $X = 0; then
  sed "s/\(<artifact.*dynamic='\)./\10/" $T1 | $ocpixml add $T2 -
  cp $T2 $OUT
  chmod a+x $OUT
fi
rm -f $T1 $T2
exit $X
