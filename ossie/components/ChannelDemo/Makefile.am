ACLOCAL_AMFLAGS = -I m4

AM_CXXFLAGS = -Wall -I../../system/ossie/include -I../../system/ossie/include/ossie -I../../system/standardInterfaces/include

LDADD = ../../system/ossie/framework/libossiecf.la \
	../../system/ossie/parser/libossieparser.la \
	../../system/ossie/idl/cpp/libossieidl.la \
	../../system/standardInterfaces/libstandardInterfaces.la

bin_PROGRAMS = ChannelDemo

ChannelDemo_SOURCES = ChannelDemo.cpp ChannelDemo.h main.cpp

ossieName = ChannelDemo
xmldir = $(prefix)/dom/xml/$(ossieName)
bindir = $(prefix)/dom/bin
dist_xml_DATA = ChannelDemo.prf.xml ChannelDemo.scd.xml \
ChannelDemo.spd.xml 

