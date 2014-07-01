# Centos 6 target environment

# #### Compiler/flags ############################################## #

export OCPI_CXXFLAGS="$OCPI_CXXFLAGS -Wno-dangling-else -std=c++0x"

# #### Shared library build settings ###################################### #

export OCPI_SHARED_LIBRARIES_FLAGS="-m64 -m elf_x86_64"

if test "$OCPI_BUILD_SHARED_LIBRARIES" = ""; then
  export OCPI_BUILD_SHARED_LIBRARIES=0
fi
