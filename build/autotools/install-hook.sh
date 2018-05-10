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
# This script is run in the automake install-data-hook.
# It cannot run in the install-exec-hook since that is too early.
# It is cleaner to write it in a bash script than in a make recipe.
# It is run in the "$DESTDIR/$prefix" directory
# It is passed in a bunch of information so that it is not dependent on or dragging in any
# other configuration information.
# Args are:
# 1: dynamic suffix (including period)
# 2: ocpi_dynamic (0 or 1)
# 3: platform target string
# 4: host platform
# 5: drivers
# 6: swigs
# 7: prereqs
# 8: prereqs dir
# Environment variables are:
# V for automake 0 for quiet, 1 for verbose
#
# We use this hook to "post process" the installation staging directory $(DESTDIR) and do things
# that libtool cannot do due to its broad historical portability mandates (being nice here)
# The actions taken are:
# 0. Remove libtool relics, leaving only real libraries that we need
# 1. On static builds:
#    -- remove DT_NEEDEDs from plugin/driver libraries that reference our libs or prereqs
#    -- remove dynamic libraries that were only built for error checking (e.g. stubs).
#    The only dynamic libraries left are:
#    - driver libraries with no DT_NEEDED entries
#    - swig libraries with no-DT_NEEDED except for dynamic prereqs
# 2. On dynamic builds: for all dynamic libraries and executables:
#    -- change DT_NEEDEDs to not be absolute
#    -- use rpath $ORIGIN/../lib (or mac equivalent)

# For now we have "linux with ELF" and "mac" modes so its easy enough switch here,
# but there may be a more comprehensive solution it we get more variations.

is_mac=$(uname -s | grep Darwin)
dynsuff=$1
dynamic=$2
target=$3
host=$4
drivers=$5
swigs=$6
prereqs=$7
prereq_inst=$8
case $prereq_inst in /*);;*)prereq_inst=$(cd ../$prereq_inst; pwd);; esac
PATCHELF=$prereq_inst/patchelf/$host/bin/patchelf

# arg 1: driver file, arg 2: prereqs, arg3: dynamic suffix
function fix_static_driver {
  local newfile=$(basename $1 $3)_s$3
  if [ -n "$is_mac" ]; then
    libs=`ed -s <<-EOF
	r !otool -l $1
	g/LC_LOAD_DYLIB/.+2s/^ *name *\(.*\) (offset.*$/\1/p
	EOF`
    # for l in $libs; do
    #   if [[ $l = *libocpi_* ]]; then
    #     install_name_tool -change $l /usr/lib/libc++.1.dylib $1
    #   fi
    #   for p in $2; do
    #     [[ $l = */lib$p.* ]] && 
    #       install_name_tool -change $l /usr/lib/libc++.1.dylib $1
    #   done
    # done
    install_name_tool -id $newfile $1
    install_name_tool -add_rpath @loader_path/../lib $1
  else
      # Note that for prerequisites, they may have version suffixes, so we don't actually know
      # what the entire "needed" actually is
    local -a remove
    for n in $($PATCHELF --print-needed $1); do
      [[ $n = libocpi_* ]] && remove+=($n)
      for p in $2; do
        [[ $n = lib$p.* ]]  && remove+=($n)
      done
    done
    $PATCHELF --remove-rpath ${remove[@]/#/--remove-needed } $1
  fi
  echo Renaming driver library $1 adding a _s$3 suffix.
  mv $1 $(dirname $1)/$newfile
}
# Make a relative link from the install dir to the build dir
# args are the two args to ln (to from)
# replace the link if it exists
function relative_link {
  local from=$2
  [ -d $2 ] || from=$(dirname $2)
 # [ -d $1 ] && echo Bad relative symlink to a directory: $1 && exit 1
  local to=$(python -c "import os.path; print os.path.relpath('$(dirname $1)', '$from')")
  [ -d $2 -a -L $2/$(basename $1) ] && rm $2/$(basename $1)
  ln -s -f $to/$(basename $1) $2
}
# arg 1: driver file arg2: prereqs arg3: dynamic suffix
function fix_dynamic {
  if [ -n "$is_mac" ]; then
    # allow for both bin next to lib, and executables in bin subdirs
    install_name_tool -add_rpath @loader_path/../lib $1
    install_name_tool -add_rpath @loader_path/../../lib $1
    base=$(basename $1)
    #echo BASE:$base PI:${prereq_inst} DIR:$(dirname $1)
    if [[ $base == lib* ]]; then
      install_name_tool -id @rpath/$base $1
    fi	
    for l in $libs; do
      if [ $l = $base ]; then
        install_name_tool -id $l $1
        continue
      fi
      if install_name_tool -change "/staging/lib/$l" @rpath/$l $1; then
        : #echo "$1: /staging/lib/$l -> $l"
      else
        echo "install_name_tool failed on $1 (lib $l)"
        exit 1
      fi
    done
    # Change all the libraries we depend on to go through rpath
    libs=`otool -L $1 | tail +2 | grep /staging/ | sed 's=^[	 ]*/staging/lib/\([^ 	]*\).*$=\1='`
    for l in $libs; do
      base=$(basename $l)
      if install_name_tool -change "$prereq_inst/$l" @rpath/$base $1; then
        if [ ! -e lib/$base ]; then
	  relative_link $prereq_inst/$l lib
        fi
      else
        echo "install_name_tool failed on $1 (lib $l)"
        exit 1
      fi
    done
  else
    rpath='$ORIGIN/../lib:$ORIGIN/../../lib'
    for n in $($PATCHELF --print-needed $1); do
      lname=$(sed 's/lib\([^.]*\)\..*/\1/' <<<$n)
      for p in $prereqs; do
        [ $p = "$lname" -a ! -e lib/$n ] && relative_link $prereq_inst/$p/$target/lib/$n lib
      done
    done
    $PATCHELF --set-rpath "$rpath" $1
  fi
}
echo Running install-exec-hook.  Args are: $*
echo Removing libtool libraries, leaving only the required \"real\" libraries.
rm lib/*.la
if [ $dynamic = 1 ]; then
  echo Processing the libraries and executables for a dynamic configuration 1>&2
  rm lib/*_s.a
  # change the rpath of all dynamic libraries and executables to be relocatable
  for i in lib/*$dynsuff `find bin -type f`; do # look for all dynamic libraries we created
    fix_dynamic $i
  done
else
  echo Processing the libraries and executables for a static configuration 1>&2
  for l in lib/*_s.a; do mv $l ${l/_s.a/.a}; done
  for i in lib/*$1; do # look for all dynamic libraries we created
    me=$(basename $i $dynsuff | sed 's/^libocpi_//')
    found=
    # See if its a driver library. If so Remove DT_NEEDED and rename to _s
    for d in $drivers; do
      [ "$d" = "$me" ] && {
        echo Removing OpenCPI DT_NEEDED entries from driver library: $d
	fix_static_driver $i "$prereqs" $1
	continue 2
      }
    done
    # See if its a swig library. If so just leave it alone in this static case
    for s in $swigs; do
      [ _$s = "$me" ] && continue 2
    done
    for p in $prereqs; do
      relative_link $prereq_inst/$p/$target/lib/lib$p.a lib
    done
    # Its not a driver, and not a swig and we're doing a static install.  Remove it.
    echo Removing $i since it was only used for error checking.
    rm $i
  done
fi
echo $drivers > lib/driver-list
exit 0
