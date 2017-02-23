# settings when targeting this platform.
OcpiLibraryPathEnv=LD_LIBRARY_PATH
OcpiRpathOrigin=$${ORIGIN}
OcpiDynamicSuffix=so
OCPI_OCL_LIBS=  -lOpenCL
OCPI_EXTRA_LIBS=rt dl pthread
OCPI_EXPORT_DYNAMIC=-Xlinker --export-dynamic
OcpiAsNeeded=-Xlinker --no-as-needed
OCPI_TARGET_CXXFLAGS:=$(filter-out -Wno-sign-conversion,$(OCPI_TARGET_CXXFLAGS))
OCPI_TARGET_CFLAGS:=$(filter-out -Wno-sign-conversion,$(OCPI_TARGET_CFLAGS))
CC = gcc
CXX = c++
LD = c++
