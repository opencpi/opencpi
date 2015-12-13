# settings when targeting this platform.
export OcpiLibraryPathEnv=LD_LIBRARY_PATH
export OCPI_OCL_LIBS="-lOpenCL"
export OCPI_EXTRA_LIBS="rt dl pthread"
export OCPI_EXPORT_DYNAMIC="-Xlinker --export-dynamic"
export OcpiAsNeeded="-Xlinker --no-as-needed"
export CC=gcc
export CXX=c++
export LD=c++
