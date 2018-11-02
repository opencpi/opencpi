$(if $(realpath $(OCPI_CDK_DIR)),,\
  $(error The OCPI_CDK_DIR environment variable is not set correctly.))
# To restrict the HDL assemblies that are built, you can set the Assemblies
# variable to the specific list of which ones you want to build, e.g.:
# Assemblies=assy1 assy3
# Otherwise all assemblies will be built
# Alternatively, you can set ExcludeAssemblies to list the ones you want to exclude
include $(OCPI_CDK_DIR)/include/hdl/hdl-assemblies.mk
