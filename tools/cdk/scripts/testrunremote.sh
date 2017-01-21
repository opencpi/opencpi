#!/bin/bash

# internal script to run tests on a remote system from a foo.test directory
# usage is: ./testrunremote.sh <host> <user> <passwd> <cmd>

pwfile=$1.pw
inrun=$(basename $(dirname $PWD))
if [ $inrun != run ]; then
  mkdir -p gen
  pwfile=gen/$pwfile
fi
rm -f $pwfile
echo echo $3 > $pwfile
chmod a+x $pwfile
SSH_ASKPASS=./$pwfile $OCPI_CDK_DIR/scripts/setsid.py ssh -o ConnectTimeout=10 $2@$1 sh -l -c \'$4\'
