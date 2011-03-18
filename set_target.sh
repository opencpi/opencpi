#!/bin/sh
HostSystem=`uname -s | tr A-Z a-z`
HostProcessor=`uname -m | tr A-Z a-z`
if test $HostProcessor=i386; then HostProcessor=x86_64; fi
HostTarget=$HostSystem-$HostProcessor
