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

################################################################################
# This startup file is for running a minimal, relocatable, server configuration
# It is executed before running the server, usually via ssh
# It is the "server control script" that performs functions to manage the server
# in the server's system

function do_stop {
  echo ocpiserve is still running. sending SIGINT >&2
  kill -s INT $pid
  sleep 2
  kill -s CONT $pid && {
    echo ocpiserve is still running after SIGINT.  Trying SIGTERM. >&2
    kill -s TERM $pid || :
    sleep 2
    kill -s CONT $pid && {
      echo ocpiserve is still running after SIGTERM.  Trying SIGKILL. >&2
      kill -s KILL $pid || :
    }
  }
  rm ocpiserve.pid
}



# We are being run in a sandbox directory
export OCPI_CDK_DIR=`pwd`
export OCPI_TOOL_PLATFORM=$(< swplatform)
export OCPI_TOOL_PLATFORM
export OCPI_TOOL_OS=linux
export OCPI_TOOL_DIR=$OCPI_TOOL_PLATFORM
export OCPI_SYSTEM_CONFIG=`pwd`/system.xml
PATH=$OCPI_CDK_DIR/$OCPI_TOOL_PLATFORM/bin:$PATH
platform=$OCPI_TOOL_PLATFORM

echo Executing remote configuration command: $* >&2

while [[ "$1" = -* ]]; do
    case $1 in
	-u) shift; user=$1;;
	-l) shift; logopt=-l$1;;
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
case $1 in
  start)
      if [ -f ocpiserve.pid ] && kill -s CONT $(< ocpiserve.pid); then
	  echo ocpiserve is still running. 2>&1
	  exit 1
      fi
      ocpidriver unload >&2 || : # in case it was loaded from a different version
      ocpidriver load >&2
      log=$(date +%Y%m%d-%H%M%S).log
      if [ -n "$vg" ] ; then
	  export PATH=$PATH:prerequisites/valgrind/$platform/bin
          export VALGRIND_LIB=prerequisites/valgrind/$platform/lib/valgrind
      fi
      echo PATH=$PATH >&2
      echo nohup ${vg:+valgrind }ocpiserve -v $logopt -p $(<port) \> $log >&2
      nohup ${vg:+valgrind }ocpiserve -v $logopt -p $(<port) > $log 2>&1 &
      pid=$!
      sleep 1
      if kill -s CONT $pid; then
	  echo ocpiserve running with pid: $pid >&2
	  echo $pid > ocpiserve.pid
	  head $log
      else
	  wait $pid
	  echo Could not start ocpiserve: exit status $?, here is the last 10 lines of the log: >&2
	  tail -10 $log >&2
	  exit 1
      fi;;
  stop)
    if [ ! -f ocpiserve.pid ]; then
      echo No ocpiserve appears to be running: no pid file >&2
      exit 1
    fi
    pid=$(< ocpiserve.pid)
    if ! kill -s CONT $pid ; then
      echo No ocpiserve appears to be running \(pid $pid\).  Process does not exist. >&2
      exit 1
    fi
    do_stop
    ;;
   stop_if)
     [ -f ocpiserve.pid ] && {
       pid=$(<ocpiserve.pid)
       kill -s CONT $pid && (do_stop || :)
       rm ocpiserve.pid
     } || :
      ;;
   log)
    logs=($(shopt -s nullglob; echo *.log))
    if [ -z "$logs" ]; then
	echo No logs found. >&2
	exit 1
    fi
    last=${logs[${#logs[@]}-1]}
    if [ -f ocpiserve.pid ] ; then
      pid=$(<ocpiserve.pid)
      if kill -s CONT $(<ocpiserve.pid); then
        echo Log is $last, pid is $pid >&2
        tail +0 -f $last
      fi
    fi
    echo No server running, dumping last log: $last >&2
    cat $last >&2
    ;;
   status)
    if [ ! -f ocpiserve.pid ]; then
      echo No ocpiserve appears to be running: no pid file >&2
      exit 1
    fi
    pid=$(< ocpiserve.pid)
    if ! kill -s CONT $pid ; then
      echo No ocpiserve appears to be running \(pid $pid\).  Process does not exist. >&2
      exit 1
    fi
    echo Server is running with port: $(<port) and pid: $pid >&2;;
esac

