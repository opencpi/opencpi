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
# This script, executed within an RPM spec file, uses the generic prepare-package-list script
# to do two things for the rpmbuild process:
# 1. create the list of files for the package and put it into <package>-files
# 2. copy the files into %{buildroot}%{prefix}
package=$1
platform=$2
cross=$3
buildroot=$4
prefix=$5
builddir=$6
mkdir -p $buildroot$prefix
./packaging/prepare-package-list.sh $package $platform $cross | while read source dest; do
  if [ -n  "$dest" ]; then
    xform="-e s=^$(dirname $source)/=$dest/=" destdir=$dest
  else
    xform=                                    destdir=$(dirname $source) dest=${source/@}
  fi
  mkdir -p ${buildroot}${prefix}/$destdir
  if [[ $source == *@ ]]; then
    source=${source/@}
    echo $source
    cp -R $source ${buildroot}${prefix}/$dest
  else
    find -L $source -type f | sed $xform -e s/foo/foo/
    cp -R -L $source ${buildroot}${prefix}/$dest
  fi
done | while read file; do
  # Emit %dir lines for any directory found anywhere, then make them uniq with sort -u
  dir=$(dirname $file)
  while [ $dir != . ] ; do
    # FIXME check why this CROSS thing is actually needed anyway.
    [ -z "$cross" -o $(basename $dir) = "$platform" ] &&
    echo "%dir %{prefix0}/$dir"
    dir=$(dirname $dir)
  done
  echo "%{prefix0}/$file"
done | sort -u > $builddir/$package-files
