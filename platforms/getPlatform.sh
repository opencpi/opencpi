#!/bin/sh
# This script determines the runtime platform and target variables
# The four variables are: OS OSVersion Processor Platform
# If a platform is added, this script should be updated to recognize it
# If it returns nothing (""), that is an error
# It depends on being located in the directory where platforms live

HostSystem=`uname -s | tr A-Z a-z`
HostProcessor=`uname -m | tr A-Z a-z`
if test -f /etc/redhat-release; then
  # redhat/centos is just based on the major release number,
  # with minor releases assumed to be binary compatible.
  read v0 v1 <<EOF
`sed < /etc/redhat-release 's/^\(.\).*release \([0-9]\)\..*/\1 \2/' | tr A-Z a-z`
EOF
#  rhr=(`sed < /etc/redhat-release 's/^\(.\).*release \([0-9]\)\..*/\1 \2/' | tr A-Z a-z`)
# echo info: 1"${rhr}"x 2"${rhr[*]}"x 3"${#rhr[*]}"x 1>&2
  if test "$v0" = "" -o "$v1" = ""; then
    echo Cannot parse redhat/centos release from /etc/redhat-release 1>&2
    exit 1
  fi
  HostVersion=$v0$v1
elif test -f /etc/lsb-release; then
  . /etc/lsb-release
  HostVersion=`echo $DISTRIB_ID|sed 's/^\(.\).*/\1/' | tr A-Z a-z``echo $DISTRIB_RELEASE|sed 's/^\([^.]*\).*/\1/'`
elif test -f /etc/os-release; then
  # We'll assume debian
  . /etc/os-release
  HostVersion=d$VERSION_ID
elif test $HostSystem = darwin -a "`which sw_vers`" != ""; then
  HostSystem=macos
  HostVersion=`sw_vers -productVersion | sed 's/\(.*\.[0-9]*\).*/\1/' | tr . _`
#  HostVersion=`sw_vers -productVersion | sed 's/\.[0-9][0-9]*$//'`
elif test "$HostSystem" = linux -a -f /etc/rootfs_version; then
  HostVersion=iv
  HostProcessor=arm
#  HostVersion=`grep Version /etc/rootfs_version|sed 's/.*Version *\([.0-9]*\).*$/\1/'`
elif test "$HostSystem" = linux -a "$HostProcessor" = armv7l; then
  HostVersion=zynq
  HostProcessor=arm
else
  echo Cannot determine runtime host'!!' 1>&2
  exit 1
fi
Target=$HostSystem-$HostVersion-$HostProcessor
Platform=
for i in $(dirname $0)/*; do
  if test -d $i -a -f $i/target; then
     if test $(<$i/target) = $Target; then
       if test "$Platform" != ""; then
         echo Platform is ambiguous, both $Platform and $(basename $i) have target: $Target 1>&2
         exit 1
       fi
       Platform=$(basename $i)
     fi
  fi
done
if test "$Platform" = ""; then
  echo Cannot determine platform from runtime host: $Target 1>&2
  exit 1
fi
echo $HostSystem $HostVersion $HostProcessor $HostPlatform $Target $Platform
exit 0
