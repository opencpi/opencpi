ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -Wall -I../ossie/include -I../ossie/parser

bin_PROGRAMS = c_wavLoader

c_wavLoader_SOURCES = c_wavLoader.cpp tinystr.cpp tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp

LDADD = ../ossie/idl/cpp/libossieidl.la ../ossie/parser/libossieparser.la ../ossie/framework/libossiecf.la
