# Settings when targeting this platform
export OcpiLibraryPathEnv=DYLD_LIBRARY_PATH
export OCPI_OCL_LIBS="-locpi_ocl -framework OpenCL"
export OcpiAsNeeded=
export OCPI_EXPORT_DYNAMIC=
export OCPI_CXXFLAGS+=-Wno-sign-conversion
export OCPI_HAVE_OPENCL=1
export OCPI_OPENCL_OBJS=/System/Library/Frameworks/OpenCL.framework/Versions/A/OpenCL
export OCPI_DYNAMIC=1
