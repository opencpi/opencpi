# This sourced script is for clean environments, only for use in the core source tree,
# although if CDK is available we let it go with a warning
if test "$OCPI_CDK_DIR" != ""; then
  echo "Warning!!!!!!: "you are setting up the OpenCPI build environment when it is already set.
  echo "Warning!!!!!!: "this is not guaranteed to work.  You should probably use a new shell.
  echo You can also \"source scripts/clean-env.sh\" to start over.
else
  # We're being run in an uninitialized environment.
  if test ! -x ./scripts/makeExportLinks.sh; then
    echo Error: it appears that this script is not being run at the top level of OpenCPI.
    exit 1
  fi
  # Ensure a skeletal CDK
  ./scripts/makeExportLinks.sh - x x
fi
