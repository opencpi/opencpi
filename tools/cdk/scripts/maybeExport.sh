#!/bin/bash --noprofile

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
