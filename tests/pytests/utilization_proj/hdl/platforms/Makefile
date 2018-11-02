$(if $(realpath $(OCPI_CDK_DIR)),,\
  $(error The OCPI_CDK_DIR environment variable is not set correctly.))
# To restrict the HDL platforms that are built, you can set the Platforms
# variable to the specific list of which ones you want to build, e.g.:
# Platforms=pf1 pf3
# Otherwise all platforms will be built
# Alternatively, you can set ExcludePlatforms to list the ones you want to exclude
include $(OCPI_CDK_DIR)/include/hdl/hdl-platforms.mk
