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

# Functions to help test system usage

# Input: (env) OCPI_REMOTE_TEST_SYSTEMS
# Outputs: list of (sanitized) hosts parsed in "remotes"
#          various variables related: X_host X_user X_passwd X_dir X_sshversion for each X in remotes
# Return: 0 if OCPI_REMOTE_TEST_SYSTEMS non-empty; 1 otherwise
function parse_remote_test_systems () {
  unset remotes
  if [ -n "${OCPI_REMOTE_TEST_SYSTEMS}" ]; then
    remotesystems=(${OCPI_REMOTE_TEST_SYSTEMS//:/ })
    for s in ${remotesystems[*]}; do
      x=(${s//=/ })
      h=${x[0]//./p}
      if expr $h : "[[:digit:]]" > /dev/null || : ; then h=h$h; fi
      export ${h}_host=${x[0]} ${h}_user=${x[1]} ${h}_passwd=${x[2]} ${h}_dir=${x[3]} ${h}_sshversion=${x[4]}
      export remotes="${remotes} ${h}"
    done
    return 0;
  fi
  return 1; # Empty OCPI_REMOTE_TEST_SYSTEMS
}

# Input: Arg 1: Command to run remotely
#        (env) OCPI_REMOTE_TEST_SYSTEMS or "remotes" from parse_remote_test_systems
# Outputs: X_result for each X in remotes; holds a file name with all output from ssh command
#          (you should delete these)
# Return: 0 if all ssh calls worked; non-zero if one or more failed
function foreach_remote () {
  if [ -z "$remotes" ]; then parse_remote_test_systems; fi
  local retval=0
  for h in $remotes; do
    eval "rhost=\${${h}_host} ruser=\${${h}_user} rpasswd=\${${h}_passwd} rdir=\${${h}_dir} rsshversion=\${${h}_sshversion}"
    if [ -n "$rsshversion" ]; then
      sshversion="-"$rsshversion
    fi
    pwfile=$(mktemp ./ocpi_remote_pwfile_XXXXXX)
    logfile=$(mktemp ./ocpi_remote_logfile_XXXXXX)
    chmod 700 $pwfile
    chmod 600 $logfile
    export ${h}_result=$logfile
    echo echo $rpasswd > $pwfile
    SSH_ASKPASS=$pwfile $OCPI_CDK_DIR/scripts/setsid.py ssh $sshversion $ruser@$rhost \
      -o "GSSAPIAuthentication no" -o "StrictHostKeyChecking no" -o "UserKnownHostsFile /dev/null" \
      sh -l -c \'$1\' > $logfile 2>&1
    retval=$(expr $retval + $? || :)
    rm -f $pwfile
  done
  return $retval
}

# Input: Arg 1: File to push
#        Arg 2: Directory to push to (def X_rdir)
#        (env) OCPI_REMOTE_TEST_SYSTEMS or "remotes" from parse_remote_test_systems
# Outputs: None
# Return: 0 if all ssh calls worked; non-zero if one or more failed
function foreach_remote_push () {
  if [ -z "$remotes" ]; then parse_remote_test_systems; fi
  local retval=0
  for h in $remotes; do
    eval "rhost=\${${h}_host} ruser=\${${h}_user} rpasswd=\${${h}_passwd} rdir=\${${h}_dir} rsshversion=\${${h}_sshversion}"
    if [ -n "$rsshversion" ]; then
      sshversion="-"$rsshversion
    fi
    if [ -n "$2" ]; then rdir=$2; fi
    pwfile=$(mktemp ./ocpi_remote_pwfile_XXXXXX)
    chmod 700 $pwfile
    echo echo $rpasswd > $pwfile
    SSH_ASKPASS=$pwfile $OCPI_CDK_DIR/scripts/setsid.py scp $sshversion\
      -o "GSSAPIAuthentication no" -o "StrictHostKeyChecking no" -o "UserKnownHostsFile /dev/null" \
      $1 $ruser@$rhost:$rdir > /dev/null 2>&1
    retval=$(expr $retval + $? || :)
    rm -f $pwfile
  done
  return $retval
}

if [ "$1" == __test__ ] ; then
  # env | sort > /tmp/testutil_varstart
  export OCPI_REMOTE_TEST_SYSTEMS="host=name=passwd=dirname:host2=name2=passwd2=dirname2"
  # set -x
  parse_remote_test_systems
  # set +x
  for h in $remotes; do
    eval echo "$h: \${${h}_user} \${${h}_passwd} \${${h}_dir} \${${h}_sshversion}"
  done
  if false; then
    # set -x
    export OCPI_REMOTE_TEST_SYSTEMS="192.168.xxx.xxx=root=root=/tmp"
    parse_remote_test_systems
    echo Pushing
    foreach_remote_push ../../../releng/jenkins/runtime/mynetsetup_matchstiq-z1.sh
    # foreach_remote 'printf "192.168.xxx.xxx\tmymachine\tmymachine.tld\n" >> /etc/hosts'
    echo Run1:
    foreach_remote hostname
    for h in $remotes; do
      echo "$h output:"
      eval cat "\${${h}_result}"
      eval rm -f "\${${h}_result}"
    done
    echo Run2:
    foreach_remote "RESET_NFS=1 source /tmp/mynetsetup_matchstiq-z1.sh x tempo"
    res=$?
    for h in $remotes; do
      echo "$h output:"
      eval cat "\${${h}_result}"
      eval rm -f "\${${h}_result}"
    done
    # return $res
  fi
  # env | sort > /tmp/testutil_varend
  # diff -u /tmp/testutil_varstart /tmp/testutil_varend
  # rm /tmp/testutil_varstart /tmp/testutil_varend
fi
