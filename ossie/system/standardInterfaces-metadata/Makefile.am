%.pp.idl : %.idl
	@CC@ -E -x c++ $< > $@

%SK.cpp %.h %DynSK.cpp : %.idl
	@IDL@ @IDL_FLAGS@ -bcxx -Wba -Wbd=DynSK.cpp -Wbh=.h -Wbs=SK.cpp -Wbkeep_inc_path -Wbuse_quotes $<
	cp $*.h include/standardinterfaces-metadata

SUBDIRS = include

BUILT_SOURCES =             \
    realCharMDSK.cpp          \
    complexShortMDSK.cpp      \
    RadioMetaDataSK.cpp

nodist_pkginclude_HEADERS = \
    realCharMD.h              \
    complexShortMD.h          \
    RadioMetaData.h         

AM_CXXFLAGS = -Wall -g

INCLUDES = -I./include

pkginclude_HEADERS =        \
    realCharMD.idl            \
    complexShortMD.idl        \
    RadioMetaData.idl

pkgconfigdir = $(libdir)/pkgconfig

dist_pkgconfig_DATA = standardInterfaces-metadata.pc

lib_LTLIBRARIES = libstandardInterfaces_metadata.la

libstandardInterfaces_metadata_la_SOURCES =              \
    realCharMD_u.cpp      realCharMD_p.cpp          \
    complexShortMD_u.cpp  complexShortMD_p.cpp      \
    RadioMetaData.cpp     \
    RadioMetaData_u.cpp RadioMetaData_p.cpp     

nodist_libstandardInterfaces_metadata_la_SOURCES =       \
    realCharMDSK.cpp          \
    complexShortMDSK.cpp      \
    RadioMetaDataSK.cpp

libstandardInterfaces_metadata_la_LDFLAGS = -version-info 0:6:0

CLEANFILES =                                    \
    realCharMD.h          realCharMDSK.cpp          \
    complexShortMD.h      complexShortMDSK.cpp      \
    RadioMetaData.h     RadioMetaDataSK.cpp     

# ---------- documentation ---------
.PHONY: docs documentation

docs :
	./genDocs.py


