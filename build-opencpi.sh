#!/bin/sh
echo ================================================================================
echo We are running in `pwd` where the git clone of opencpi has been placed.
. ./centos6_env.sh
echo ================================================================================
echo Now we will '"make"' the core OpenCPI libraries and utilities.
make
echo ================================================================================
echo Now we will '"make"' the built-in RCC '(software)' components.
make rcc
echo ================================================================================
echo Now we will '"make"' the examples
make examples
echo ================================================================================
echo Finally, we will built the OpenCPI kernel device driver for this system.
make driver
echo ================================================================================
echo OpenCPI has been built, with software components, examples and kernel driver
trap - ERR
