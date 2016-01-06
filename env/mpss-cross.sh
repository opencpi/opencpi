# Definitions common to tilera CROSS_COMPILATION platforms.

#binary for the compiler
export OCPI_CROSS_BUILD_BIN_DIR=/usr/linux-k1om-4.7/bin/
export OCPI_CROSS_HOST=x86_64-k1om-linux
export OCPI_TARGET_OS=linux
export OCPI_TARGET_OS_VERSION=xp
export OCPI_TARGET_ARCH=mic
export OCPI_TARGET_HOST=linux-xp-mic
export OCPI_ARCH=mic
export OCPI_CFLAGS=-fPIC
export OCPI_CXXFLAGS=-fPIC
#export OCPI_LDFLAGS="-static --export-dynamic -Bsymbolic --export-all-symbols"
#export OCPI_LDFLAGS="-dynamic --export-dynamic -Bsymbolic --export-all-symbols"

export OCPI_LDFLAGS="-dynamic -Bsymbolic"
export OCPI_SHARED_LIBRARIES_FLAGS=

