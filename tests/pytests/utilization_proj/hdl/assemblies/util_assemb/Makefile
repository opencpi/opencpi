$(if $(realpath $(OCPI_CDK_DIR)),,\
  $(error The OCPI_CDK_DIR environment variable is not set correctly.))
# This is the HDL assembly Makefile for the "util_assemb" assembly
# The file "util_assemb.xml" defines the assembly.
# The default container for all assemblies is one that connects all external ports to
# the devices interconnect to communicate with software workers or other FPGAs.
#
# Limit this assembly to certain platforms or targets with Exclude/Only and Targets/Platforms ie:
# ExcludePlatforms=




#1752:
# If you want to modify the default MAP options for Xilinx bitstream builds, export
# the "OcpiXstMapOptions" variable here (the default options are shown below) e.g:
# export OcpiXstMapOptions=-detail -w -logic_opt on -xe c -mt 4 -register_duplication on -global_opt off -ir off -pr off -lc off -power off
# Note that the -t option should not be added in OcpiXstMapOptions as it is
# already handled by a different mechanism.
#
# If you want to modify the default PAR options for Xilinx bitstream builds, export
# the "OcpiXstParOptions" variable here (the default options are shown below) e.g:
# export OcpiXstParOptions=-mt 4 -w -xe n
#
# If you want to connect external ports of the assembly to local devices on the platform,
# you must define container XML files, and mention them in a "Containers" variable here, e.g.:
# Containers=take_input_from_local_ADC

include $(OCPI_CDK_DIR)/include/hdl/hdl-assembly.mk
