ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -Wall -I../../system/ossie/include -I../../system/ossie/include/ossie -I../../system/standardInterfaces/include

LDADD = ../../system/ossie/framework/libossiecf.la \
	../../system/ossie/parser/libossieparser.la \
	../../system/ossie/idl/cpp/libossieidl.la \
	../../system/standardInterfaces/libstandardInterfaces.la

bin_PROGRAMS = RxDemo

RxDemo_SOURCES = RxDemo.cpp RxDemo.h main.cpp

ossieName = RxDemo
xmldir = $(prefix)/dom/xml/$(ossieName)
bindir = $(prefix)/dom/bin
dist_xml_DATA = RxDemo.prf.xml RxDemo.scd.xml \
RxDemo.spd.xml 

