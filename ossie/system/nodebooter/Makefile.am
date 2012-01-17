ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -Wall -I../ossie/include -I../ossie/include/ossie -I../ossie/parser -I../standardInterfaces/include \
-I/home/jek/clone1/opencpi/core/local/util/vfs/include \
-I/home/jek/clone1/opencpi/core/local/util/filefs/include \
-I/home/jek/clone1/opencpi/core/sca/cf/cf222/idl \
-I/home/jek/clone1/opencpi/core/sca/cf_util/vfs/include \
-I/home/jek/clone1/opencpi/adapt/os/ocpios/interfaces/include

bin_PROGRAMS = nodeBooter

nodeBooter_SOURCES = nodebooter.cpp

LDADD = ../ossie/framework/libossiecf.la ../ossie/parser/libossieparser.la ../ossie/idl/cpp/libossieidl.la
