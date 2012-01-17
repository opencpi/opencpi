ACLOCAL_AMFLAGS = -I m4

AM_CXXFLAGS = -Wall \
-I/home/jek/clone1/opencpi/core/local/util/vfs/include \
-I/home/jek/clone1/opencpi/core/local/util/filefs/include \
-I/home/jek/clone1/opencpi/core/sca/cf/cf222/idl \
-I/home/jek/clone1/opencpi/core/sca/cf_util/vfs/include \
-I/home/jek/clone1/opencpi/adapt/os/ocpios/interfaces/include

ossieName = default_GPP_node

waveformdir = $(prefix)/dev/nodes/$(ossieName)
dist_waveform_DATA = DeviceManager.dcd.xml DeviceManager.spd.xml DeviceManager.scd.xml DeviceManager.prf.xml 

# install-data-hook:
#	ln -t $(waveformdir) -s ../../xml/ xml
