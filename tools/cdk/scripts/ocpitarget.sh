#!/bin/bash
# Extract the target-related variables set in the make context for use in the shell context
# This file must be sourced since its purpose is to change the environment
# This is rarey needed since these variables are almost always used in the "make" context,
# where the initialization is done using the ocpisetup.mk script.
# A single argument is required which will become OCPI_TARGET_PLATFORM.
# If the single argument is empty, and OCPI_TARGET_PLATFORM is already set, it is used.
# If the single argument is empty and OCPI_TARGET_PLATFORM is not set, OCPI_TOOL_PLATFORM is used.
if [ -z "$1" ]; then
  # Support legacy use of setting OCPI_TARGET_HOST, before platforms could be in projects
  if test "$OCPI_TARGET_PLATFORM" = ""; then
    if test "$OCPI_TARGET_HOST" != ""; then
      # For compatibility if  OCPI_TARGET_PLATFORM not set.
      for i in $OCPI_CDK_DIR/platforms/*; do
       if test -f $i/target -a "$(< $i/target)" = "$OCPI_TARGET_HOST"; then
         export OCPI_TARGET_PLATFORM=$(basename $i)
         break
       fi
      done
      if test "$OCPI_TARGET_PLATFORM" = ""; then
        echo The value of $OCPI_TARGET_HOST does not match any known platform.
        exit 1
      fi
      echo 'Warning:  the OCPI_TARGET_HOST environment variable was found set: it is deprecated; use OCPI_TARGET_PLATFORM instead, when cross-building.'
    fi
  fi
  # End of legacy support for setting OCPI_TARGET_HOST
  if [ -z "$OCPI_TARGET_PLATFORM" ]; then
    [ -z "$OCPI_TOOL_PLATFORM" ] && {
       echo "Internal error: environment not set in ocpitarget.sh"
       exit 1
    }
    export OCPI_TARGET_PLATFORM=$OCPI_TOOL_PLATFORM
  fi
else
  export OCPI_TARGET_PLATFORM=$1
fi 
# Ensure we are really starting fresh for this target
unset `env | grep OCPI_TARGET | grep -v OCPI_TARGET_PLATFORM | sed 's/=.*//'`
source $OCPI_CDK_DIR/scripts/util.sh
setVarsFromMake $OCPI_CDK_DIR/include/ocpisetup.mk ShellTargetVars=1
