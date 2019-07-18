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

################################################################################################
if test "$#" = 0; then
  echo This script modifies a dynamic RCC worker artifact to be suitable for being dynamicly
  echo loaded into a statically linked executable.
  echo Usage is:  makeStaticWorker OS infile [lib1...]
  echo The first argument is the OS, the second is the input file and the rest of the arguments
  echo are the names of the needed ocpi libraries that will not be required at runtime.
  echo The input file is MODIFIED IN PLACE.
  exit 1
fi
# This gets around some of the RPM-based limitations
# OCPI_CDK_DIR/../bin/ may not (yet) be in PATH.
ocpixml=$(type -p ocpixml)
[ -z "${ocpixml}" ] && ocpixml=${OCPI_CDK_DIR}/${OCPI_TOOL_DIR}/bin/ocpixml
[ ! -x ${ocpixml} ] && echo "Could not find ocpixml!" && exit 1
OS=$1
shift
IN=$1
shift
if [ $OS = linux ]; then
  patchelf=$OCPI_PREREQUISITES_DIR/patchelf/bin/patchelf
  [ -x ${patchelf} ] || patchelf=$OCPI_PREREQUISITES_DIR/patchelf/$OCPI_TOOL_DIR/bin/patchelf
  [ -x ${patchelf} ] || echo "Could not find patchelf executable!"
fi
set -e
X=1
if file $IN | grep -q ELF; then
  for i in $*; do PARGS="$PARGS --remove-needed $i"; done
  ${patchelf} $PARGS $IN
  X=0
elif [ $OS = macos ]; then
  # The patchelf is unnecessary on MacOS since it does not have the DT_NEEDED entries anyway
  X=0    
else
  echo "Input file $1 is non-existent or not readable or not a suitable (ELF) file."
fi
exit $X
