#!/bin/bash
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


# Extract a base project, creating it at $1, using ocpidev to do everything we can.

function bad {
  echo Failed: $*
  exit 1
}

function get_first {
  grep '^[ 	]*<[^!]' $1 | head -1 | sed 's/^[ 	]*<[ 	]*\([^ 	].*\)>/\1/' > $2
}

function get_attrs {
  get_first $1 $2
  lang=$(sed -n "s/.*[lL]anguage=['\"]\([^'\"]*\)['\"].*\$/\1/p" $2)
  spec=$(sed -n "s/.*[sS]pec=['\"]\([^'\"]*\)['\"].*\$/\1/p" $2)
  wname=$(sed -n "s/.*[nN]ame=['\"]\([^'\"]*\)['\"].*\$/\1/p" $2)
  if [ -z "$spec" ] ; then
    spec=$(sed -n '/href=/s/^.*href="\([^"]*\)".*$/\1/p' $1)
    [ -z $spec ] && {
      if grep -q -i componentspec $1; then
        spec=none
      else
        bad spec cannot be found: $(dirname $1)
      fi
    }
  fi
  return 0
}
set -e
[ "$(basename $1)" != base ] && echo Error: base project \"$1\" must end in \"base\" && exit 1
mkdir -p $(dirname $1)
ocpidev -K ocpi -d $(dirname $1) create project $(basename $1)
from=$(pwd)
t=/tmp/$(basename $0).$$
cd $1
ocpidev create library components
for i in $from/components/specs/*; do
  get_first $i $t
  x="$(sed 's/^\([a-zA-Z]*\)[^a-zA-Z]*.*$/\1/' $t | tr A-Z a-z)"
  case "$x" in
    (protocol)
      ocpidev create protocol $(basename $i)
      [ -e $from/components/specs/$(basename $i) ] || bad protocol not there for $i
      cp $i components/specs/$(basename $i)
      ;;
    (componentspec)
      ocpidev create spec $(basename $i .xml)
      [ -e $from/components/specs/$(basename $i) ] || bad spec not there for $i
      cp $i components/specs/$(basename $i)
      ;;
    (*)
      bad unexpected spec file name: $i:$x;;
 esac
done
for i in $((for x in $from/components/*.{rcc,hdl,ocl}; do echo $x ; done) | sort) ; do
  n=$(basename $i)
  x=(${n/./ })
  model=${x[1]}
  wvals=()
  workers=()
  wsf=()
  if [ -e $i/${x[0]}.xml ] ; then
    get_attrs $i/${x[0]}.xml $t
    specoption=
    [[ "$spec" == *.xml ]] && spec=${spec/.xml/}
    ospec=$spec
    [[ "$spec" == *[-_]spec ]] && spec=${spec/%[-_]spec/}
    [[ "$spec" == ${x[0]} ]] && spec=
    [ -z $spec ] || spec=$ospec
    echo Spec for $n is $ospec
  else
    # No single OWD
    multlang=
    workers=($(sed -n '/^[ 	]*Workers/s/^[ 	]*Workers[ 	]*=[ 	]*\([a-zA-Z].*\)[ 	]*$/\1/p' $i/Makefile))
    [ -z "$workers" ] && bad No OWD and cannot find workers for $i
    for w in ${workers[@]} ; do
      get_attrs $i/$w.xml $t
      [ -n "$lang" ] && {
         [ -n "$multlang" -a "$lang" != "$multlang" ] && bad language mismatch with multiple workers
         multlang=$lang
      }
      wvals=(${wvals[@]} ${w}:${spec}:${wname})
    done
    lang=$multlang
    spec=
    wname=
  fi
  others=($(sed -n '/^[ 	]*SourceFiles/s/^[ 	]*SourceFiles[ 	]*=[ 	]*\([a-zA-Z].*\)[ 	]*$/\1/p' $i/Makefile))
  includes=($(sed -n '/^[ 	]*IncludeDirs/s/^[ 	]*IncludeDirs[ 	]*=[ 	]*\(.*\)[ 	]*$/\1/p' $i/Makefile))
  cores=($(sed -n '/^[ 	]*Cores/s/^[ 	]*Cores[ 	]*=[ 	]*\([a-zA-Z].*\)[ 	]*$/\1/p' $i/Makefile))
  onlys=($(sed -n '/^[ 	]*OnlyTargets/s/^[ 	]*OnlyTargets[ 	]*:*=[ 	]*\([a-zA-Z].*\)[ 	]*$/\1/p' $i/Makefile))
  params=($(sed -n '/^[ 	]*Param/s/^[ 	]*\(Param.*\)$/\1/p' $i/Makefile))
  wsf=($(sed -n '/^[ 	]*WorkerSourceFiles/s/^[ 	]*\(WorkerSourceFiles.*\)$/\1/p' $i/Makefile))
  [ -n "$wsf" ] && params=(${params[@]} ${wsf[@]})
  echo ocpidev create worker $n \
${wname:+-N }${wname} \
${lang:+-L }${lang} \
${spec:+-S }${spec} \
${includes[@]/#/-I } \
${others[@]/#/-O } \
${cores[@]/#/-C } \
${onlys[@]/#/-T } \
${wvals[@]/#/-W } \

  ocpidev create worker $n \
${wname:+-N }${wname} \
${lang:+-L }${lang} \
${spec:+-S }${spec} \
${includes[@]/#/-I } \
${others[@]/#/-O } \
${cores[@]/#/-C } \
${onlys[@]/#/-T } \
${wvals[@]/#/-W } \

 [ -n "$others" ] && {
   for o in ${others[@]} ; do
     [ -r $i/$o ] || bad for $i missing other file $o
     cp $i/$o components/$n
   done
 }
 [ -n "$params" ] && {
   for p in ${params[@]}; do
     ed -s components/$n/Makefile <<EOF
/^[ 	]*include/
i
$p
.
w
EOF
   done
 }
 if [ -n "$workers" ] ; then
  echo Multiple from $i wsf:${wsf}
  # if specified worker sources, copy them
  [ -n "$wsf" ] && cp $i/${wsf/WorkerSourceFiles=/} components/$n
  for w in ${workers[@]} ; do
#    diff $i/$w.xml components/$n || true
     mv components/$n/$w.xml{,.gen}
     cp $i/$w.xml components/$n
     [ -z "$wsf" ] && cp $i/$w.c* components/$n
  done
 else
  echo Single from $i
#  diff  components/$n/${x[0]}.xml $i || true
  mv components/$n/${x[0]}.xml{,.gen}
  cp $i/${x[0]}.xml components/$n
  cp $i/${x[0]}.[cv]* components/$n
 fi
# grep -v '^[ 	]*#' $i/Makefile | diff components/$n/Makefile - || true
done
mkdir components/include
cp $from/components/include/signal_utils.h components/include
cp $from/components/include/stream_data_file_format.h components/include
cp $from/components/include/sym_fir.h components/include
cp $from/components/ptest.hdl/metadatarom.dat components/ptest.hdl
echo ================== Applications =====================
for x in $from/examples/xml/*.xml ; do
  app=$(basename $x .xml)
  echo Creating app $x $app
  ocpidev create application -X $app
  [ -r $x ] || bad wierd unreadable app
  [ -r applications/$app.xml ] || bad not established app
  cp $x applications/$app.xml
  cp $from/examples/xml/test.input applications
  ed applications/Makefile <<EOF
/include\/applications.mk/
i
ExcludeApplications=devbias testwidth time_test
.
w
EOF
done
echo ================== Cleaning =====================
make clean


