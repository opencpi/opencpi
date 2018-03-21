# Set up the prereq install and build directories.
# This file is sourced either from install-prerequisites.sh or setup-install.sh
[ -z "$OCPI_PREREQUISITES_INSTALL_DIR" ] &&
    export OCPI_PREREQUISITES_INSTALL_DIR=$OCPI_PREREQUISITES_DIR
[ -z "$OCPI_PREREQUISITES_BUILD_DIR" ] && {
  # default the build directory to something separate from the install dir so we can nuke it
  export OCPI_PREREQUISITES_BUILD_DIR=$(dirname $OCPI_PREREQUISITES_INSTALL_DIR)/prerequisites-build
}

function ask {
  ans=''
  until [[ "$ans" == [yY] || "$ans" == [nN] ]]; do
    read -p "Are you sure you want to $* (y or n)? " ans
  done
  [[ "$ans" == [Nn] ]] && exit 1
  return 0
}

function checkdir {
  if [ ! -d $1 ]; then
    if mkdir -p $1; then
      echo Created $1 since it did not exist.
    else  
      echo Could not create $1 or its parents without sudo.
      ask try to create $1 as root	
      [ sudo mkdir -p $1 ] || exit 1
    fi
  elif [ ! -w $1 ]; then
      echo You do not have permission for writing to $1.
      ask try to change permissions $1 as root	
      [ sudo chmod a+w $1 ] || exit 1
  fi
}
checkdir $OCPI_PREREQUISITES_BUILD_DIR
checkdir $OCPI_PREREQUISITES_INSTALL_DIR
