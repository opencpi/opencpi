ACLOCAL_AMFLAGS = -I m4

AM_CXXFLAGS = -Wall -I../../system/ossie/include -I../../system/ossie/include/ossie -I../../system/standardInterfaces/include

LDADD = ../../system/ossie/framework/libossiecf.la \
	../../system/ossie/parser/libossieparser.la \
	../../system/ossie/idl/cpp/libossieidl.la \
	../../system/standardInterfaces/libstandardInterfaces.la

bin_PROGRAMS = amplifier

amplifier_SOURCES = amplifier.cpp amplifier.h main.cpp

ossieName = amplifier
bindir = $(prefix)/dom/bin/
xmldir = $(prefix)/dom/xml/$(ossieName)
dist_xml_DATA = amplifier.prf.xml amplifier.scd.xml \
amplifier.spd.xml 

