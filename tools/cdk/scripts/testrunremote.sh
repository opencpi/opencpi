#!/bin/bash

# internal script to run tests on a remote system from a foo.test directory
# usage is: ./testrunremote.sh <host> <user> <passwd> <cmd>

pwfile=gen/$1.pw
echo echo $3 > $pwfile
chmod a+x $pwfile
SSH_ASKPASS=$pwfile $OCPI_CDK_DIR/scripts/setsid.py ssh $2@$1 'sh -l -c "'$4'"'
