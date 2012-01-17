ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -Wall -I../../system/ossie/include \
-I/home/jek/clone1/opencpi/core/local/util/vfs/include \
-I/home/jek/clone1/opencpi/core/local/util/filefs/include \
-I/home/jek/clone1/opencpi/core/sca/cf/cf222/idl \
-I/home/jek/clone1/opencpi/core/sca/cf_util/vfs/include \
-I/home/jek/clone1/opencpi/adapt/os/ocpios/interfaces/include


GPP_LDADD = ../../system/ossie/parser/libossieparser.la \
	    ../../system/ossie/framework/libossiecf.la \
	    ../../system/ossie/idl/cpp/libossieidl.la

ossieName = GPP
esseName = GPP

devbindir = $(prefix)/dev/bin
devbin_PROGRAMS = GPP

xmldir = $(prefix)/dev/xml/$(ossieName)
dist_xml_DATA = GPP.scd.xml GPP.spd.xml PowerPC405.prf.xml

GPP_SOURCES = main.cpp GPP.cpp GPP.h

