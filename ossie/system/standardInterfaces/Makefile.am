IDL_BIN = $(shell which omniidl)

ACLOCAL_AMFLAGS = -I m4

%.pp.idl : %.idl
	gcc -E -x c++ $< > $@

%SK.cpp %.h %DynSK.cpp : %.idl
	$(IDL_BIN) -I../ossie/include -bcxx -Wba -Wbd=DynSK.cpp -Wbh=.h -Wbs=SK.cpp -Wbkeep_inc_path -Wbuse_quotes $<
	cp $*.h include/standardinterfaces

SUBDIRS = include

BUILT_SOURCES =             \
    realCharSK.cpp          \
    realShortSK.cpp         \
    realLongSK.cpp          \
    realFloatSK.cpp         \
    realDoubleSK.cpp        \
    complexShortSK.cpp      \
    complexFloatSK.cpp      \
    complexDoubleSK.cpp     \
    complexCharSK.cpp       \
    complexLongSK.cpp       \
    FM_ControlSK.cpp        \
    soundControlSK.cpp      \
    stereoSoundSK.cpp       \
    Radio_ControlSK.cpp     

nodist_pkginclude_HEADERS = \
    realShort.h             \
    realLong.h              \
    realChar.h              \
    realFloat.h             \
    realDouble.h            \
    complexShort.h          \
    complexFloat.h          \
    complexDouble.h         \
    complexChar.h           \
    complexLong.h           \
    FM_Control.h            \
    soundControl.h          \
    stereoSound.h           \
    Radio_Control.h

LDADD = ../ossie/framework/libossiecf.la ../ossie/parser/libossieparser.la ../ossie/idl/cpp/libossieidl.la

AM_CXXFLAGS = -Wall -g

INCLUDES = -I./include -I../ossie/include -I../ossie/include/ossie

pkginclude_HEADERS =        \
    realChar.idl            \
    realShort.idl           \
    realLong.idl            \
    realFloat.idl           \
    realDouble.idl          \
    complexShort.idl        \
    complexFloat.idl        \
    complexDouble.idl       \
    complexChar.idl         \
    complexLong.idl         \
    Radio_Control.idl       \
    FM_Control.idl          \
    soundControl.idl        \
    stereoSound.idl         \
    sampleRateConvertorControl.idl

pkgconfigdir = $(libdir)/pkgconfig

dist_pkgconfig_DATA = standardInterfaces.pc

lib_LTLIBRARIES = libstandardInterfaces.la

libstandardInterfaces_la_SOURCES =              \
    realShort_u.cpp     realShort_p.cpp         \
    realLong_u.cpp      realLong_p.cpp          \
    realFloat_u.cpp     realFloat_p.cpp         \
    realDouble_u.cpp    realDouble_p.cpp        \
    realChar_u.cpp      realChar_p.cpp          \
    complexChar_u.cpp   complexChar_p.cpp       \
    complexShort_u.cpp  complexShort_p.cpp      \
    complexFloat_u.cpp  complexFloat_p.cpp      \
    complexDouble_u.cpp complexDouble_p.cpp     \
    complexLong_u.cpp   complexLong_p.cpp       \
    Radio_Control_u.cpp Radio_Control_p.cpp     \
    Resource_u.cpp 

nodist_libstandardInterfaces_la_SOURCES =       \
    realShortSK.cpp         \
    realLongSK.cpp          \
    realFloatSK.cpp         \
    realDoubleSK.cpp        \
    realCharSK.cpp          \
    complexShortSK.cpp      \
    complexFloatSK.cpp      \
    complexDoubleSK.cpp     \
    complexCharSK.cpp       \
    complexLongSK.cpp       \
    Radio_ControlSK.cpp

libstandardInterfaces_la_LDFLAGS = -version-info 0:6:0

CLEANFILES =                                    \
    realShort.h         realShortSK.cpp         \
    realLong.h          realLongSK.cpp          \
    realFloat.h         realFloatSK.cpp         \
    realDouble.h        realDoubleSK.cpp        \
    realChar.h          realCharSK.cpp          \
    complexShort.h      complexShortSK.cpp      \
    complexFloat.h      complexFloatSK.cpp      \
    complexDouble.h     complexDoubleSK.cpp     \
    complexChar.h       complexCharSK.cpp       \
    complexLong.h       complexLongSK.cpp       \
    Radio_Control.h     Radio_ControlSK.cpp     \
    standardIdl.pp.idl

# ---------- documentation ---------
.PHONY: docs documentation

docs :
	./genDocs.py

uninstall-hook:
	-rm -rf $(includedir)/standardInterfaces
