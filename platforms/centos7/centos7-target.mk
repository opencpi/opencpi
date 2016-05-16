# settings when targeting this platform.
OcpiLibraryPathEnv=LD_LIBRARY_PATH
OcpiRpathOrigin=$${ORIGIN}
OcpiDynamicSuffix=so
OCPI_OCL_LIBS=  -lOpenCL
OCPI_EXTRA_LIBS=rt dl pthread
OCPI_EXPORT_DYNAMIC=-Xlinker --export-dynamic
OcpiAsNeeded=-Xlinker --no-as-needed
CC = gcc
CXX = c++
LD = c++
