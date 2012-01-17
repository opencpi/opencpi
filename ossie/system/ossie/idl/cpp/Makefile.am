IDL_BIN = $(shell which omniidl)

%SK.cpp %.h %DynSK.cpp : ../%.idl
	$(IDL_BIN) -bcxx -Wba -Wbd=DynSK.cpp -Wbh=.h -Wbs=SK.cpp $<
	cp $*.h ../../include/ossie
	cp ../$*.idl ../../include/ossie

install-data-hook :
	touch $(DESTDIR)$(pkgincludedir)/*.idl
	touch $(DESTDIR)$(pkgincludedir)/*.h

AM_CXXFLAGS = -Wall -g

BUILT_SOURCES = cfSK.cpp PortTypesSK.cpp StandardEventSK.cpp cfDynSK.cpp PortTypesDynSK.cpp StandardEventDynSK.cpp
CLEANFILES = cfSK.cpp PortTypesSK.cpp StandardEventSK.cpp cf.h PortTypes.h StandardEvent.h cfDynSK.cpp PortTypesDynSK.cpp StandardEventDynSK.cpp

nodist_pkginclude_HEADERS = cf.h PortTypes.h StandardEvent.h
pkginclude_HEADERS = ../cf.idl ../PortTypes.idl ../StandardEvent.idl

lib_LTLIBRARIES = libossieidl.la
nodist_libossieidl_la_SOURCES = cfSK.cpp PortTypesSK.cpp StandardEventSK.cpp cfDynSK.cpp PortTypesDynSK.cpp StandardEventDynSK.cpp
libossieidl_la_LDFLAGS = -version-info 0:4:0

