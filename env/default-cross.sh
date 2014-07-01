#!/bin/sh
# This sourced script sets up a default environment based on an automatic determination
# of the currently running platform, but cross compiling to the platform provided
# in arg1.
# It also avoids the setup if OCPI_BASE_DIR is already set.
if test "$OCPI_BASE_DIR" != ""; then
  echo Using existing OpenCPI environment, since OCPI_BASE_DIR is set to: $OCPI_BASE_DIR
  return 0
fi
if test ! -d env -o ! -d platforms; then
  echo Error: this script \($0\) is not being run at the top level of OpenCPI.
  return 1
fi
default_env_vars=($(platforms/getPlatform.sh))
if test $? != 0; then
  echo Failed to determine runtime platform.
  return 1
fi
p=$1
if test ! -f platforms/$p/$p-env.sh; then
  echo Platform \"$p\" has no default environment setup script.
  return 1
fi
trap "trap - ERR; break" ERR; for i in 1; do
source ./env/start.sh
p=$1
source ./platforms/$p/$p-env.sh
source ./env/finish.sh
done; trap - ERR
