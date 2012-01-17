ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libsigproc.la
libsigproc_la_SOURCES =             \
    filters.cpp                     \
    butter.cpp                      \
    pll.cpp                         \
    scaling.cpp                     \
    sequencing.cpp                  \
    sources.cpp                     \
    utility.cpp                     \
    fec_conv.cpp                    \
    fixed.c                         \
    modem.cpp

pkginclude_HEADERS = SigProc.h fixed.h

libsigproc_la_LDFLAGS = -version-info 0:7:0

AM_CXXFLAGS = -Wall -g -O3 $(FPM)
AM_CFLAGS = -Wall -g -O3 $(FPM)

pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = sigproc.pc


# --- autotest ---

#check_PROGRAMS = cxxtest autotest
#autotest_SOURCES = autotest.cpp
#autotest_LDADD =                    \
#    filters.o                       \
#    butter.o                        \
#    pll.o                           \
#    scaling.o                       \
#    sequencing.o                    \
#    sources.o                       \
#    utility.o                       \
#    modem.o                         \
#    fec_conv.o

#TESTS = autotest

#cxxtest:
#	cxxtestgen.py --error-printer -o autotest.cpp \
#	autotest_sources/FIRPolyphaseFilterBank_testsuite.h \
#	autotest_sources/IIRFilter_testsuite.h \
#	autotest_sources/CircularBuffer_testsuite.h \
#	autotest_sources/Modem_testsuite.h \
#	autotest_sources/DesignRRCFilter_testsuite.h \
#	autotest_sources/dot_product_testsuite.h \
#	autotest_sources/trig_testsuite.h \
#	autotest_sources/butter_testsuite.h \
#	autotest_sources/sequencing_testsuite.h \
#	autotest_sources/pack_testsuite.h \
#	autotest_sources/Fec_conv_testsuite.h

# --- documentation ---
.PHONY: docs documentation

docs :
	./genDocs.py

uninstall-hook:
	-rm -rf $(includedir)/sigproc
