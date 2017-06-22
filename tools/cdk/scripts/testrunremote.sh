#!/bin/bash

# internal script to run tests on a remote system from a foo.test directory
# usage is: ./testrunremote.sh <host> <user> <passwd> <cmd> <ssh-protocol-version>

pwfile=$1.pw
inrun=$(basename "$(dirname $PWD)")
if [ "$inrun" != run ]; then
  mkdir -p gen
  pwfile=gen/$pwfile
fi
if [ "$5" ]; then
  sshversion="-"$5
fi
rm -f $pwfile
echo echo $3 > $pwfile
chmod a+x $pwfile
SSH_ASKPASS=./$pwfile $OCPI_CDK_DIR/scripts/setsid.py ssh $sshversion \
  -o "GSSAPIAuthentication no" -o "StrictHostKeyChecking no" -o "UserKnownHostsFile /dev/null" -o "ConnectTimeout=30" \
  $2@$1 sh -l -c \'$4\'
