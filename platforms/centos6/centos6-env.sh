# Centos 6 target environment

# #### Compiler/flags ############################################## #

export OCPI_TARGET_CXXFLAGS="$OCPI_TARGET_CXXFLAGS -std=c++0x"

# #### Shared library build settings ###################################### #

export OCPI_SHARED_LIBRARIES_FLAGS="-m64"

if test "$OCPI_BUILD_SHARED_LIBRARIES" = ""; then
  export OCPI_BUILD_SHARED_LIBRARIES=0
fi
