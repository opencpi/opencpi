ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -Wall -I../ossie/include -I../ossie/parser

bin_PROGRAMS = deployer

deployer_SOURCES = deployer.cpp

LDADD = ../ossie/idl/cpp/libossieidl.la ../ossie/parser/libossieparser.la ../ossie/framework/libossiecf.la
