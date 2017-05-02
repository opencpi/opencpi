#!/bin/sh
# This script determines the runtime platform and target variables
# The four variables are: OS OSVersion Processor Platform
# If a platform is added, this script should be updated to recognize it
# If it returns nothing (""), that is an error
# It depends on being located in the directory where platforms live

# These are universally available so far so we do this once and pass then to all probes.
HostSystem=`uname -s | tr A-Z a-z`
HostProcessor=`uname -m | tr A-Z a-z`
# Each recognizable platform has a script <platform>-check.sh
# If there is a file platform-list, then we look for them in that order.
# Otherwise, we just look in alphabetical order
mydir=$(dirname $0)
declare -a platforms
if test -f $mydir/platform-list; then
  platforms=($(<$mydir/platform-list))
  [ -n "$1" ] && echo Found these platforms in the platforms-list file: ${platforms[@]} 1>&2
else
  for i in $mydir/*; do
    if test -d $i; then
      p=$(basename $i)
      if test -f $i/$p-check.sh; then
        platforms="$platforms $(basename $i)"
      fi
    fi
done
fi
[ -n "$1" ] && echo Considering platforms: ${platforms[@]} 1>&2
for p in ${platforms[@]}; do
  [ -n "$1" ] && /bin/echo -n Checking if we are running on platform $p:'  ' 1>&2
  [ -r $mydir/$p/$p-check.sh ] || {
    echo platform $p is in the $mydir/platforms-list file, but $mydir/$p/$p-check.sh does not exist. 1>&2
    exit 0
  }
  vars=($(sh $mydir/$p/$p-check.sh $HostSystem $HostProcessor))
  if test ${#vars[@]} = 3; then
    [ -n "$1" ] && echo succeeded.  Target is ${vars[0]}-${vars[1]}-${vars[2]}. 1>&2
    echo ${vars[@]}  ${vars[0]}-${vars[1]}-${vars[2]} $p
    exit 0
  fi
  [ -n "$1" ] && echo failed. 1>&2
done
echo Cannot determine platform we are running on.  1>&2
exit 1
