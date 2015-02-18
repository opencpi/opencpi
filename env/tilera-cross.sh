# Definitions common to tilera CROSS_COMPILATION platforms.

#binary for the compiler
export OCPI_CROSS_BUILD_BIN_DIR=$TILERA_ROOT/bin
export OCPI_CROSS_HOST=tile
export OCPI_TARGET_OS=linux
export OCPI_TARGET_OS_VERSION=tile
export OCPI_TARGET_ARCH=gx
export OCPI_TARGET_HOST=linux-tile-gx
export OCPI_ARCH=tilegx
# export OCPI_CFLAGS=
# export OCPI_CXXFLAGS=
#export OCPI_LDFLAGS="-static --export-dynamic -Bsymbolic --export-all-symbols"
export OCPI_LDFLAGS="-dynamic --export-dynamic -Bsymbolic --export-all-symbols"
export OCPI_SHARED_LIBRARIES_FLAGS=

