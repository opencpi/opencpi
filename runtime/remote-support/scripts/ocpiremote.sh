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


# This script is temporary until the ocpiremote tool can do this in a more thoughtful way

# prereqs are that you have deployment package (a.k.a. SDcard dir) under
# $OCPI_CDK_DIR/<hw-platform>/<sw-platform>
# and you have established a bootable system with the right OS on the SD card in the system
# You do not need to have an OpenCPI installation on the SD card
# Bootstrapping the OS on a running system is not implemented here.
# We are basically creating a minimal server installation of opencpi in a sandbox that does not
# affect the installation on the SD card.
# usage is: ./testrunremote.sh <host> <user> <passwd> <cmd> <ssh-protocol-version>

user=root
passwd=root
[[ -z $OCPI_SERVER_ADDRESSES || $OCPI_SERVER_ADDRESSES != *:* ]] &&
    echo Bad OCPI_SERVER_ADDRESSES value: $OCPI_SERVER_ADDRESSES && exit 1
host=${OCPI_SERVER_ADDRESSES%%:*}
port=${OCPI_SERVER_ADDRESSES##*:}
sshopts='-t -o "GSSAPIAuthentication no" -o "StrictHostKeyChecking no" -o "UserKnownHostsFile /dev/null" -o "ConnectTimeout=30"'
rdir=sandbox
platform=xilinx13_4
os=linux
log=
function help {
  cat <<-EOF
	Options are:
	  -u <user>         - login user - default is: root
	  -p <passwd>       - login password - default is: root
	  -a <address>      - host address - default is first address in \$OCPI_SERVER_ADDRESSES
	  -p <port>         - port to use when running server
	  -s <ssh-options>  - default is:
	                      $sshopts
	  -d <remote-dir>   - dir on remote system to create a server sandbox - default is ~/opencpi
	  -o <options>      - ocpiserve options
	  -h                - print this help (also --help, also no arguments at all)
	  -P <platform>     - Specify the software platform for the server environment
	  -n <server>       - Specify NTP server, or - for none
          -V                - Use valgrind (for load, load it, for start, use it)

	Commands are:
	  test              - test basic connectivity
	  unhost            - remove host from local known hosts file (requires non-default ssh-options)
	  load              - load a server sandbox, but not if it exists (at the directory)
	  reload            - load a server sandbox, removing the directory if it exists
	  remove            - remove a server sandbox
	  start             - start server
	  kill              - kill running server
	  status            - show status of server, like ocpirun -C
EOF
    exit 1
}


trap 'test -n "$tmpdir" -a -d "$tmpdir" && rm -r -f "$tmpdir"' EXIT
while [[ "$1" = -* ]]; do
    case $1 in
	-u) shift; user=$1;;
	-l) shift; log="-l $1";;
	-w) shift; passwd=$1;;
	-a) shift; host=$1;;
	-p) shift; port="$1";;
	-s) shift; sshopts="$1";;
	-d) shift; rdir="$1";;
	-h|--help) help;;
	-o) shift; options="$1";;
	-P) shift; platform="$1";;
        -V) vg=-V;;
	*) echo Unknown option: $1 && exit 1;;
    esac
    shift
done
[ -z "$1" ] && help
[ -z "$OCPI_CDK_DIR" ] && echo OCPI_CDK_DIR not set. && exit 1
[ -z "$host" ] && echo No host or ip-address specified in OCPI_SERVER_ADDRESSES or using the -a option. && exit 1
[ -n "$passwd" ] && {
  tmpdir=$(mktemp -d)
  chmod 700 $tmpdir
  touch $tmpdir/password
  chmod 777 $tmpdir/password
  echo echo $passwd > $tmpdir/password
  ASK="SSH_ASKPASS=$tmpdir/password"
}
function do_ssh {
    eval export $ASK
    set +e
    eval $OCPI_CDK_DIR/scripts/setsid.py ssh $sshopts $user@$host sh -c "'\"$1\"'"
}
function unload {
    do_ssh "if [ -d $rdir ]; then rm -r -f $rdir; fi"
}
checkdir="test ! -d $rdir && echo Directory $rdir does not exist && exit 1"
set -e
for op in $*; do
  case $op in
    test)
      echo Checking for connectivity by running \"pwd\" remotely on host: $host
      do_ssh pwd;;
    ls)
      do_ssh "$checkdir; cd $rdir; ls -Rl";;
    unhost)
      echo Checking for an entry for host $host in ~/.ssh/known_hosts
      [ $(grep "^$host " ~/.ssh/known_hosts|wc|(read a b && echo $a)) != 1 ] &&
	  echo Host $host not in ~/.ssh/known_hosts && exit 1
      echo Found it and removing it.
      pwd
      kn=~/.ssh/known_hosts
      cp -p $kn{,.$(date +%F.%T)}
      grep -v "^$host " ~/.ssh/known_hosts > $tmpdir/known_hosts
      cp $tmpdir/known_hosts $kn;;
    load|reload)
      [ $op = reload ] && unload
      pdir=$OCPI_CDK_DIR/$platform
      echo "Loading server package into a specific location ($rdir).  Relative path is in root home dir."
      [ -d $pdir ] || (echo No deployable software at $pdir && exit 1)
      drivers=($(<$pdir/lib/driver-list))
      driverlibs=(${drivers[*]/#/$platform\/lib\/libocpi_})
      driverlibs=(${driverlibs[*]/%/_s.so})
      cd $OCPI_CDK_DIR
      # This list is not yet an official package like "development/runtime/deploy" etc., but it will be
      files="/etc/localtime scripts/ocpi_${os}_driver scripts/ocpiserver.sh \
      	     $platform/lib/*.ko $platform/lib/*.rules ${driverlibs[*]} \
             $platform/bin/ocpidriver $platform/bin/ocpiserve $platform/bin/gdb $platform/system.xml"
      [ -n "$vg" ] && files+=" ../prerequisites/valgrind/$platform"
      tar -c -z -H -f $tmpdir/tar.tgz $files
      do_ssh "test -e $rdir && echo Directory exists && exit 1;
      	      date -u `date -u +%Y.%m.%d-%H:%M:%S`;
	      mkdir $rdir && echo $platform > $rdir/swplatform && echo $port > $rdir/port &&
	      ln -s scripts/ocpiserver.sh $rdir && ln -s $platform/system.xml $rdir
	      tar -x -z -C $rdir -f - && pwd && TZ=\`pwd\`/$rdir/etc/localtime date" < $tmpdir/tar.tgz
	;;
    unload|remove)
	reload;;
    start)
	do_ssh "$checkdir; cd $rdir && ./ocpiserver.sh $vg $log start";;
    log)
	do_ssh "$checkdir; cd $rdir && ./ocpiserver.sh log";;
    stop)
	do_ssh "$checkdir; cd $rdir && ./ocpiserver.sh stop";;
    stop_if)
	do_ssh "test ! -d $rdir || (cd $rdir && ./ocpiserver.sh stop_if)";;
    status)
	do_ssh "$checkdir; cd $rdir && ./ocpiserver.sh status";;
  esac
done
