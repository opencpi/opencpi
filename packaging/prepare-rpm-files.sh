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
# This script, executed within an RPM spec file, uses the generic prepare-package-list script,
# and does two things for the rpmbuild process, in the rpmbuild %install context
# 1. create the list of files and directories for the package and put it into <package>-files,
#    converting the generic file/dir list into an RPM file/dir list
# 2. copy the files into %{buildroot}%{prefix} for RPM packaging
package=$1
platform=$2
cross=
[ "$3" = 1 ] && cross=1 
buildroot=$4
prefix=$5
builddir=$6
[ -z "${builddir}" ] && echo "Don't run this by hand." && exit 1
mkdir -p $buildroot$prefix
set -o pipefail
./packaging/prepare-package-list.sh $package $platform $cross | while read source dest; do
  if [[ $source == */ ]] ; then
    [ -z "$dest" ] && dest=${source/%\/}
    echo %dir %{prefix0}/$dest
    mkdir -p ${buildroot}${prefix}/$dest
    continue;
  fi
  xform=s/1/1/
  destdir=$(dirname $source)
  if [ -n "$dest" ]; then
    if [ $destdir = . ]; then
      xform="s=^=%{prefix0}/$dest/="
      destdir=
    else
      xform="s=^$destdir/=%{prefix0}/$dest/="
      destdir=$dest
    fi
  else
    xform="s=^=%{prefix0}/="
    dest=${source/@}
  fi
  mkdir -p ${buildroot}${prefix}/$destdir
  # If source needs to stay a link
  if [[ $source == *@ ]]; then
    source=${source/@}
    find $source ! -type d | sed $xform
    cp -R $source ${buildroot}${prefix}/$dest
  else
    find -L $source -type f | sed $xform
    cp -R -L $source ${buildroot}${prefix}/$dest
    # Normally we are simply doing a deep copy here, but there is an exception:
    # We need to preserve symlinks that are in the same directory (i.e. for alternative names)
    # So we do a post-pass to fix this.
    for l in `find $source -type l`; do
      [[ $(readlink $l) != */* ]] && {
        subdir=
	[ -d $source ] && subdir=/$(echo $l | sed s=$(dirname $source)==)
	cp -R -P $l ${buildroot}${prefix}/$destdir$subdir
      } || :
    done
  fi
done | sort -u |
  # Add the necessary permissions here on the exceptional files (for security)
  # These are nicer in the spec file, but this suppresses the warnings.  A tradeoff.
  # These files will only be present for dev platforms
  sed \
    -e 's=%{prefix0}/cdk/env/rpm_cdk.sh$=%attr(755,root,root) &=' \
    -e 's=%{prefix0}/cdk/env.d$=%attr(755,root,root) %config(noreplace) &=' \
    -e 's=%{prefix0}/cdk/env.d/.*\.sh\.example$=%attr(644,root,root) &=' \
    -e 's=%{prefix0}/cdk/opencpi-setup.sh$= %attr(644,root,root) &=' \
    -e 's=%{prefix0}/project-registry$= %attr(775,opencpi,opencpi) &=' \
  > $builddir/$package-files
