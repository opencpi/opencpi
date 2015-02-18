. ./env/tilera-cross.sh

export OCPI_TARGET_PLATFORM=tilegx

# Build static libraries for zynq so we don't have to install a directory
# full of shared libraries.
export OCPI_BUILD_SHARED_LIBRARIES=0






