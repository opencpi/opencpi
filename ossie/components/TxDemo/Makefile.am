ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -Wall -I../../system/ossie/include -I../../system/ossie/include/ossie -I../../system/standardInterfaces/include

LDADD = ../../system/ossie/framework/libossiecf.la \
	../../system/ossie/parser/libossieparser.la \
	../../system/ossie/idl/cpp/libossieidl.la \
	../../system/standardInterfaces/libstandardInterfaces.la

bin_PROGRAMS = TxDemo

TxDemo_SOURCES = TxDemo.cpp TxDemo.h main.cpp

ossieName = TxDemo
xmldir = $(prefix)/dom/xml/$(ossieName)
bindir = $(prefix)/dom/bin
dist_xml_DATA = TxDemo.prf.xml TxDemo.scd.xml \
TxDemo.spd.xml TxDemo_v4.prf.xml

