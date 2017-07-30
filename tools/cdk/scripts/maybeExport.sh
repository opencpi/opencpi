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


# This script, run inside a project, will export the file given in its argument if that file
# is an explicit export in the Project.exports file (including wildcards in exports file)
# The intention is to perform incremental exports which will happen sooner than a global
# "make exports" at the top level of a project.
# The first argument is a software target or "-", and the second argument is the file to be
# exported.
if [ ! -e $2 ]; then
  echo Unexpected non-existent file in $0: $2 1>&2
  exit 1
fi
source $OCPI_CDK_DIR/scripts/util.sh
down=$2
until [ -f Project.exports -o "$PWD" == / ]; do
  down=$(basename $PWD)/$down
  cd ..
done
if [ $PWD == / ]; then
  exit 0 # we're not in a project
fi
# FIXME: These lines are copied from makeExportLinks.sh
# When that file is moved to a better place, perhaps this function can be incorporated there.
additions=$(test -f Project.exports && grep '^[ 	]*+' Project.exports | sed 's/^[ 	]*+[ 	]*\([^ 	#]*\)[ 	]*\([^ 	#]*\).*$/\1:\2/') || true
set -f
for a in $additions; do
  declare -a both=($(echo $a | tr : ' '))
  [[ $a == *\<target\>* && $1 == - ]] && continue
  rawsrc=${both[0]//<target>/$1}
  if [[ $down == $rawsrc ]]; then
    dir=exports/${both[1]//<target>/$1}
    base=$(basename $down)
    after=
    if [[ ${both[1]} =~ /$ || ${both[1]} == "" ]]; then
      after=$base
    else
      # export link has a file name, perhaps replace the suffix
      suff=$(echo $base | sed -n '/\./s/.*\(\.[^.]*\)$/\1/p')
      dir=${dir//<suffix>/$suff}
    fi
    mkdir -p exports
    makeRelativeLink $down $dir$after
    exit 0
  fi
done
