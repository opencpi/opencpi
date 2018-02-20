# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

# Build process, in a nutshell:
#  1. (Set up exports regardless of targets - see "@AUTOGUARD@"" section below)
#  2. Build individual libraries using libtool
#    - "util" is special and has a "1p" vs "3p" half with different warning/error flags
#    - "drivers" are built as libtool "shared modules" which are expected to use dlopen()
#      - ~~by static linking executables with all shared drivers, ensures all needed symbols are linked in~~ (see below)
#      - all other libraries are forced to be static
#  3. Standard automake rules install the libraries we compiled
#    - If non-RPM build, "make install" automagically called
#    - Hooks are used to modify the installed files:
#      - drivers are renamed to not have "_s" suffix in their internal names
#      - static linked executables are edited to NOT need the driver shared libraries

# Other notes:
#  ~~static linking executables with all shared drivers~~:
#    This was retired. It worked on ocpirun, but unless we change the way ACI executables are built, they won't work.
#    Instead, we just piggyback off of the OSS "dumb1" solution at the end of ContainerManager.cxx.
#    In the future, we may do this "better" so the details are still noted here.
#  libtool LDFLAGS used:
#    -static makes a library static-only (instead of libtool's default of both)
#    -shared SOMETIMES makes a library shared-only (instead of libtool's default of both)
#    -export-dynamic allows plugins to see our internals for callbacks (see AV-355)
#    -module means we want a .so that you can call dlopen() on it (like our drivers)
#    -avoid-version means we do not want .so.0.0.0, .so.0.0, .so.0, etc.

mybin_ldflags = -static -export-dynamic
my_so_ldflags = -module -avoid-version -shared

##> Different sections are broken down with double hash marks followed by greater-than, like this line

##> "Main" Makefile.am
LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

pkgincludedir=$(temp_cdk_dir)/include
lib_LTLIBRARIES =
bin_PROGRAMS =
CLEANFILES =
# Enable things like xxx_HEADERS:
# EXTRA_HEADERS=

if !OCPI_RPMBUILD
# By default, "make" will populate the proto-CDK in non-RPM building usage (AV-592)
# It seems to be called after "make check" which will define am__remaking_logs. Don't do anything if that's the case.
@AUTOGUARD@ifndef am__remaking_logs
install_nonrpm_install=install_nonrpm_install
install_nonrpm_install: install
	$(AM_V_at)$(MAKE) exports
@AUTOGUARD@else
install_nonrpm_install:
@AUTOGUARD@endif
endif

# Stop av_config.h.in from being trashed:
$(srcdir)/av_config.h.in:
	@true

# For Debugging:
# AM_DEFAULT_VERBOSITY = 1

.PHONY: ocpi
.DEFAULT_GOAL = ocpi

# This is spread out into two calls to force host_check to finish (AV-488)
ocpi: host_check
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) libtool $(compat_mkfile) all-am $(install_nonrpm_install)

if OCPI_CROSSCOMPILING
compat_mkfile = tools/cdk/include/autoconfig_import-$(ocpi_target_platform).mk
PATH := $(PATH):$(ocpi_cross_build_bin_dir)
else
compat_mkfile = tools/cdk/include/autoconfig_import.mk
endif

# This sed stuff removes RPATH issues; see https://fedoraproject.org/wiki/Packaging:Guidelines#Beware_of_Rpath
.PHONY: rpath_check
rpath_check: libtool
	$(AM_V_at)if [ 0$(shell grep -c DIE_RPATH_DIE libtool) -eq 0 ]; then \
	  echo "Patching libtool"; \
	  @SED@ -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool; \
	  @SED@ -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool; \
	fi

.PHONY: host_check
# This is phony because automake complains it if is a real file declared .PRECIOUS
# And we don't want to lose it.
CURR_BUILD=`cat .build_platform 2>/dev/null`
host_check: rpath_check
	$(AM_V_at)if test -n "${CURR_BUILD}" && test x"$(ocpi_target_platform)" != x"${CURR_BUILD}"; then \
	  echo Previous build was for \"$(CURR_BUILD)\". Attempting to build for \"$(ocpi_target_platform)\".; \
	  echo Erasing all $(CURR_BUILD) objects and libraries.; \
	  find . -type d -name .libs -print0 | xargs -0r rm -rf; \
	  find . -iregex '.*\.l?[oa]i?' -not -iregex '.*/target-.*' -not -iregex '.*/export/lib/linux-.*' -delete; \
	fi
	$(AM_V_at)echo $(ocpi_target_platform) > .build_platform

.IGNORE: cleandriver cleandrivers cleaneverything distclean-local

# TIME=/usr/bin/time --verbose (AV-432)

# Various rules to be backwards compatible
ProjectPrefix=ocpi
# These are -include because they are erased in distclean but then re-enter this Makefile:
-include $(compat_mkfile)
-include $(OCPI_CDK_DIR)/include/util.mk
export OCPI_PROJECT_PATH:=$(OCPI_PROJECT_PATH):$(subst $(Space),:,$(wildcard $(OcpiProjectRegistryDir)/*)):$(OCPI_CDK_DIR)/../projects/core/
@AUTOGUARD@ifeq ($(filter clean%,$(MAKECMDGOALS))$(filter distclean%,$(MAKECMDGOALS)),)
@AUTOGUARD@  ifeq ($(wildcard exports),)
@AUTOGUARD@    $(info Exports have never been set up for this tree. Doing it now.)
@AUTOGUARD@  endif
@AUTOGUARD@  $(info $(shell ./scripts/makeExportLinks.sh - $(ProjectPrefix)_ ocpi.cdk xxx))
@AUTOGUARD@include exports/include/ocpisetup.mk
@AUTOGUARD@PlatformDir=$(call OcpiGetRccPlatformDir,$(OCPI_TARGET_PLATFORM))
@AUTOGUARD@    $(info $(shell mkdir -p $(PlatformDir)/include))
@AUTOGUARD@    $(info $(shell cp av_config.h $(PlatformDir)/include/ || :))
@AUTOGUARD@endif
.PHONY: everything exports export_cdk

everything: all exports

# No dependencies for av_config.h because it is generated with the Makefile
exports:
	$(AM_V_at)./scripts/makeExportLinks.sh $(ocpi_target_host) $(ProjectPrefix)_ -

export_cdk:
	file=opencpi-cdk-exported.tar; \
	(cd exports; find . -follow -type l) > $$file.exclude; \
	echo Creating export file: $$file; \
	tar -h -f $$file -c -X $$file.exclude -C exports .

.PHONY: driver cleandriver cleandrivers
driver: exports $(compat_mkfile)
if OCPI_CROSSCOMPILING
	$(AM_V_at)set -e; if test -d os/$(ocpi_target_os)/driver; then \
	  $(LN_S) ../../$(compat_mkfile) exports/include/ || : ; \
	  $(MAKE) -C os/$(ocpi_target_os)/driver \
	    OCPI_CDK_DIR=../../../exports \
	    OCPI_TARGET_PLATFORM=$(OCPI_TARGET_PLATFORM) \
	    OCPI_TARGET_ARCH=$(OCPI_TARGET_ARCH) \
	    OCPI_CROSS_BUILD_BIN_DIR=$(ocpi_cross_build_bin_dir) \
	    OCPI_TARGET_KERNEL_DIR=$(OCPI_TARGET_KERNEL_DIR) \
	    all; \
	else \
	  echo No driver for the OS '"'$(ocpi_target_os)'"', so none built. ; \
	fi
else
	$(AM_V_at)if test -d os/$(ocpi_target_os)/driver; then \
	  $(MAKE) -C os/$(ocpi_target_os)/driver OCPI_CDK_DIR=../../../exports all; \
	else \
	  echo No driver for the OS '"'$(ocpi_target_os)'"', so none built. ; \
	fi
endif
	$(AM_V_at)$(MAKE) exports

cleandriver:
	@echo This target does nothing. Perhaps you meant cleandrivers?

cleandrivers:
	-$(AM_V_at)for d in os/*/driver; do $(TIME) $(MAKE) -C $$d topclean OCPI_TARGET_HOST=$(ocpi_target_host); done

.PHONY: cleaneverything

#cleaneverything: cleandrivers cleanhdl cleancomponents cleanexports
cleaneverything: cleandrivers cleanexports
	-$(AM_V_at)rm -f .build_platform .config.envvars*
	-$(AM_V_at)find . -depth -name 'timeData.raw' -delete
	-$(AM_V_at)find . -depth -name 'target-*' -delete
	-$(AM_V_at)find . -depth -name 'gen' | grep -v /kernel-headers/ | xargs -r rm -rf
	-$(AM_V_at)find . -depth -name "lib" -a ! -path "*export*" -a ! -path "*/platforms/*" -a -type d -a -delete

# Want to clean the exports and their source last, so can't use distclean-local
distclean:
	$(AM_V_at)$(MAKE) distclean-am
	$(AM_V_at)$(MAKE) clean
	$(AM_V_at)find . -name '*~' -delete
	$(AM_V_at)rm -f Makefile.in
	$(AM_V_at)rm -f Makefile
	$(AM_V_at)rm -f av_config.h platforms/*/include/av_config.h tools/cdk/include/autoconfig_import*.mk
	$(AM_V_at)rm -f configure
	$(AM_V_at)rm -rf build-aux
	$(AM_V_at)rm aclocal.m4
	$(AM_V_at)rm -rf $(compat_mkfile) target-cdk-staging
	$(AM_V_at)rm -f $(am__CONFIG_DISTCLEAN_FILES)
	$(AM_V_at)find . -name $(DEPDIR) -type d | xargs -r rm -rf

# Want to clean the exports and their source last, so can't use clean-local
#clean: cleancomponents cleanexamples cleandrivers
clean: cleandrivers
	$(AM_V_at)rm -rf TAGS
	$(AM_V_at)rm -rf target-$(ocpi_target_host)
	$(AM_V_at)$(MAKE) cleanexports
	$(AM_V_at)$(MAKE) clean-am

cleanexports:
	-$(AM_V_at)rm -r -f exports

.PHONY: etags
etags: ETAGS=etags
etags:
	-$(AM_V_at)rm TAGS
	$(AM_V_at)find . -name '*.cpp' -o -name '*.h' -o -name '*.c' -print0 \
	| xargs -0 $(ETAGS) --extra=+q --fields=+fksaiS --c++-kinds=+px --append

# Make compat_mkfile findable:
export OCPI_CDK_DIR?=$(ocpi_cdk_dir)

# PATCHELF=PATCHELF_DEBUG=1 /opt/opencpi/prerequisites/patchelf/bin/patchelf
PATCHELF=/opt/opencpi/prerequisites/patchelf/bin/patchelf

##> Patch installed files
# Remove libtool metadata files
# Internally rename all _s.so => .so
# Patch all executables to not require any of our shared objects (not currently any - disabled)
install-exec-hook: DRIVER_SO_FILES=$(foreach lib,$(basename $(notdir $(ocpi_driver_libs))),${lib}.so)
install-exec-hook: $(install_exec_hook_swig)
	@echo "Checking internal driver SONAMEs:"
	$(AM_V_at)cd $(DESTDIR)$(libdir) && \
	  rm -rf *.la; \
	  for lib in $(DRIVER_SO_FILES); do \
	    echo "Fixing internal name for $${lib} => $${lib/_s./.}"; \
      $(PATCHELF) --ocpi-fix-soname $${lib}; \
    done
	@echo "Checking for SONEEDED of dynamic drivers:"
	$(AM_V_at)cd $(DESTDIR)$(bindir) && \
    for exe in $$(find -type f -executable -not -name "*.sh"); do \
      OLIBS=$$($(PATCHELF) --print-needed $${exe} | grep libocpi | sort | xargs -n1 -r echo -n " --remove-needed "); \
			if [ -n "$${OLIBS}" ]; then \
        echo "Removing unnecessary driver requirement(s) from $${exe}"; \
        $(PATCHELF) $${OLIBS} $${exe}; \
			fi; \
    done

# SWIG/GNU Radio Stuff
if SWIG_CHECK
if !OCPI_CROSSCOMPILING
install_exec_hook_swig=install-exec-hook-swig
install-exec-hook-swig: SWIG_SO_FILES=$(foreach lib,$(basename $(notdir $(ocpi_swig_libs))),${lib}.so)
install-exec-hook-swig:
	$(AM_V_at)cd $(DESTDIR)$(libdir) && \
	for lib in $(SWIG_SO_FILES); do \
	  echo "Fixing up internal dependency names for $${lib}"; \
	  for dep in $(DRIVER_SO_FILES); do \
	    newname=`echo $${dep} | sed 's/_s\././'`; \
	    echo "Fixing internal name for $${dep} => $${newname}"; \
        $(PATCHELF) --replace-needed $${dep} $${newname} $${lib}; \
      done \
	done
endif
endif

install-data-hook: $(install_data_hook_zynq) exports

if OCPI_CROSSCOMPILING_ZYNQ
  install_data_hook_zynq=install-data-hook-zynq
  OCPI_TARGET_KERNEL_DIR ?= $(abs_top_srcdir)/platforms/zed/release/kernel-headers
endif

# Make the zynq driver and copy the platform directory
# There is not "uninstall" hook for this because it is only used in RPM building
install-data-hook-zynq: OCPI_CROSS_HOST=$(ocpi_cross_host)
install-data-hook-zynq: driver

##> Top-level list of all include directories you might need
prereq_INC = $(lzma_dir)/include/ /opt/opencpi/prerequisites/gmp/include/ $(gtest_dir)/include/

# Instead of declaring every header, just add to header path... (AV-769)
# This doesn't follow "The GNU Autotools Way" to list every header, but we also have
# no plans on "make dist" ever working, and libtool captures the dependencies.
incpath = $(prereq_INC)

# Bring in platform-specific header(s) like av_config.h
incpath += $(OCPI_CDK_DIR)/platforms/$(OCPI_TARGET_PLATFORM)/include/
#TODO: is this needed now
#incpath += $(foreach p, $(subst :, ,$(OCPI_PROJECT_PATH)), $(p)/rcc/platforms/$(OCPI_TARGET_PLATFORM)/include/)


INIT_CFLAGS += $(foreach path, $(incpath), -I $(path))
INIT_CXXFLAGS += $(foreach path, $(incpath), -I $(path))
AM_CFLAGS_NOSTRICT = $(INIT_CFLAGS)
AM_CXXFLAGS_NOSTRICT = $(INIT_CXXFLAGS)
AM_CFLAGS = $(AM_CFLAGS_NOSTRICT) $(STRICT_FLAGS)
AM_CXXFLAGS = $(AM_CXXFLAGS_NOSTRICT) $(STRICT_FLAGS)
AM_LDFLAGS = $(INIT_LDFLAGS) -avoid-version

##> Top-level list of all libraries that are NOT drivers
# This order is IMPORTANT. libtool can't handle circular dependencies! This list taken from Makefile.ocpi.imports:

ocpi_all_libs = $(lib_ocpi_msg_drivers) $(lib_ocpi_ocpidds) $(lib_ocpi_ocpigen) $(lib_ocpi_application) $(lib_ocpi_tests) $(lib_ocpi_ctests) $(lib_ocpi_container) $(lib_ocpi_msg_driver_interface) $(lib_ocpi_library) $(lib_ocpi_transport) $(lib_ocpi_xfer) $(lib_ocpi_cdkutils) $(lib_ocpi_util) $(lib_ocpi_os)

# The drivers, which should not be statically linked:

ocpi_driver_libs =  $(lib_ocpi_xfer_drivers) $(lib_ocpi_hdl) $(lib_ocpi_ofed) $(lib_ocpi_rcc) $(lib_ocpi_remote) $(lib_ocpi_ocl)

ocpi_swig_libs = $(lib_ocpi_swig_api)

# "Install" both sets of libraries:
lib_LTLIBRARIES += $(ocpi_all_libs) $(ocpi_driver_libs) $(ocpi_swig_libs)
# Only a "convenience"
noinst_LTLIBRARIES = $(lib_ocpi_util_1p) $(lib_ocpi_util_3p)

LDADD =
# All executables should be able to see drivers (and link/export functions they need):
# (Currently disabled; see intro paragraphs)
# LDADD += $(ocpi_driver_libs) $(lib_ofed_stub)
# Then all our static libraries and prerequisites:
LDADD += $(ocpi_all_libs) -lpthread -ldl @GTEST_STATIC_LIBS@ @LZMA_STATIC_LIBS@
# Add in our copy of GMP (AV-1881) - TODO: Make this GMP_STATIC_LIBS
LDADD += $(foreach lib,gmpxx gmp, /opt/opencpi/prerequisites/gmp/$(ocpi_target_host)/lib/lib$(lib).a)

# AV-592 we moved these to have the makeExportLinks handle it:
temp_cdk_dir=$(abs_top_srcdir)/target-cdk-staging/
bindir = $(temp_cdk_dir)/bin/$(ocpi_target_host)
libdir = $(temp_cdk_dir)/lib/$(ocpi_target_host)
datadir = $(temp_cdk_dir)/bin/$(ocpi_target_host)
incdir = $(temp_cdk_dir)/include
noinst_PROGRAMS =

##> os/linux
os_linux=os/linux/
__ocpi_build_dir__libocpi_os_la_CFLAGS = $(AM_CFLAGS) -D_XOPEN_SOURCE=600 -D_FILE_OFFSET_BITS=64
__ocpi_build_dir__libocpi_os_la_CXXFLAGS = $(AM_CXXFLAGS) -D_XOPEN_SOURCE=600 -D_FILE_OFFSET_BITS=64
__ocpi_build_dir__libocpi_os_la_LDFLAGS = $(INIT_LDFLAGS) -ldl -avoid-version -static

lib_ocpi_os = $(ocpi_build_dir)/libocpi_os.la

__ocpi_build_dir__libocpi_os_la_SOURCES = \
			os/linux/src/OcpiOsAssert.cxx \
			os/linux/src/OcpiOsDebug.cxx \
			os/linux/src/OcpiOsEther.cxx \
			os/linux/src/OcpiOsEvent.cxx \
			os/linux/src/OcpiOsFileIterator.cxx \
			os/linux/src/OcpiOsFileSystem.cxx \
			os/linux/src/OcpiOsLoadableModule.cxx \
			os/linux/src/OcpiOsMisc.cxx \
			os/linux/src/OcpiOsMutex.cxx \
			os/linux/src/OcpiOsPosixError.cxx \
			os/linux/src/OcpiOsPosixSocket.cxx \
			os/linux/src/OcpiOsProcessManager.cxx \
			os/linux/src/OcpiOsRWLock.cxx \
			os/linux/src/OcpiOsSemaphore.cxx \
			os/linux/src/OcpiOsServerSocket.cxx \
			os/linux/src/OcpiOsSocket.cxx \
			os/linux/src/OcpiOsSpinLock.cxx \
			os/linux/src/OcpiOsThreadManager.cxx \
			os/linux/src/OcpiOsTimer.cxx

incpath += os/interfaces/include/ os/linux/include/

##> runtime/util
# The "_3p" stuff is imported third-party code that we are not as strict about.
# The "_1p" is ours.
# They are combined into a single static library for distribution.
lib_ocpi_util = $(ocpi_build_dir)/libocpi_util.la
lib_ocpi_util_1p = $(ocpi_build_dir)/libocpi_util_1p.la
lib_ocpi_util_3p = $(ocpi_build_dir)/libocpi_util_3p.la

# Note: this name "miscutilexec" MUST have "exec" in the string for install-exec-hook to run patchelf
# See "The Two Parts of Install" in the automake manual
miscutilexecdir = $(bindir)/utils
miscutilexec_PROGRAMS = $(ocpi_build_dir)/fasttimed $(ocpi_build_dir)/fasttime_test $(ocpi_build_dir)/fpgaTimeTest $(ocpi_build_dir)/gen-uuid $(ocpi_build_dir)/HttpClientDemo $(ocpi_build_dir)/HttpFsDemo $(ocpi_build_dir)/HttpServerDemo $(ocpi_build_dir)/iordump $(ocpi_build_dir)/LwLoggerDemo $(ocpi_build_dir)/parentChildTest $(ocpi_build_dir)/resTest $(ocpi_build_dir)/timeCompare $(ocpi_build_dir)/timeCvt $(ocpi_build_dir)/TimeEmitTest $(ocpi_build_dir)/vcdMerge $(ocpi_build_dir)/ZipReaderDemo $(ocpi_build_dir)/ZipWriterDemo

__ocpi_build_dir__fasttimed_SOURCES = runtime/util/bin/src/fasttimed_main.cxx
__ocpi_build_dir__fasttime_test_SOURCES = runtime/util/bin/src/fasttime_test_main.cxx
__ocpi_build_dir__fpgaTimeTest_SOURCES = runtime/util/bin/src/fpgaTimeTest_main.cxx
__ocpi_build_dir__gen_uuid_SOURCES = runtime/util/bin/src/gen-uuid_main.cxx
__ocpi_build_dir__HttpClientDemo_SOURCES = runtime/util/bin/src/HttpClientDemo_main.cxx
__ocpi_build_dir__HttpFsDemo_SOURCES = runtime/util/bin/src/HttpFsDemo_main.cxx
__ocpi_build_dir__HttpServerDemo_SOURCES = runtime/util/bin/src/HttpServerDemo_main.cxx
__ocpi_build_dir__iordump_SOURCES = runtime/util/bin/src/iordump_main.cxx
__ocpi_build_dir__LwLoggerDemo_SOURCES = runtime/util/bin/src/LwLoggerDemo_main.cxx
__ocpi_build_dir__parentChildTest_SOURCES = runtime/util/bin/src/parentChildTest_main.cxx
__ocpi_build_dir__resTest_SOURCES = runtime/util/bin/src/resTest_main.cxx
__ocpi_build_dir__timeCompare_SOURCES = runtime/util/bin/src/timeCompare_main.cxx
__ocpi_build_dir__timeCvt_SOURCES = runtime/util/bin/src/timeCvt_main.cxx
__ocpi_build_dir__TimeEmitTest_SOURCES = runtime/util/bin/src/TimeEmitTest_main.cxx
__ocpi_build_dir__vcdMerge_SOURCES = runtime/util/bin/src/vcdMerge_main.cxx
__ocpi_build_dir__ZipReaderDemo_SOURCES = runtime/util/bin/src/ZipReaderDemo_main.cxx
__ocpi_build_dir__ZipWriterDemo_SOURCES = runtime/util/bin/src/ZipWriterDemo_main.cxx

__ocpi_build_dir__libocpi_util_la_SOURCES = # Nothing

__ocpi_build_dir__libocpi_util_la_LDFLAGS = -static

__ocpi_build_dir__libocpi_util_la_LIBADD = $(lib_ocpi_util_1p) $(lib_ocpi_util_3p)

__ocpi_build_dir__libocpi_util_1p_la_SOURCES = \
			runtime/util/ezxml/src/OcpiUtilEzxml.cxx \
			runtime/util/ezxml/src/OcpiXmlEmbedded.cxx \
			runtime/util/assembly/src/OcpiUtilAssembly.cxx \
			runtime/util/driver/src/OcpiDriverManager.cxx \
			runtime/util/farmhash/src/farmhash.cxx \
			runtime/util/fasttime/src/calibration.cxx \
			runtime/util/fasttime/src/fasttime.cxx \
			runtime/util/filefs/src/OcpiUtilFileFs.cxx \
			runtime/util/http/src/OcpiUtilHttpClient.cxx \
			runtime/util/http/src/OcpiUtilHttpFs.cxx \
			runtime/util/http/src/OcpiUtilHttpMisc.cxx \
			runtime/util/http/src/OcpiUtilHttpServer.cxx \
			runtime/util/ior/src/OcpiUtilCDR.cxx \
			runtime/util/ior/src/OcpiUtilIIOP.cxx \
			runtime/util/ior/src/OcpiUtilIOP.cxx \
			runtime/util/list/src/OcpiList.cxx \
			runtime/util/logger/src/OcpiLoggerDebugLogger.cxx \
			runtime/util/logger/src/OcpiLoggerFallback.cxx \
			runtime/util/logger/src/OcpiLoggerLogger.cxx \
			runtime/util/logger/src/OcpiLoggerNullOutput.cxx \
			runtime/util/logger/src/OcpiLoggerOStreamAdapter.cxx \
			runtime/util/logger/src/OcpiLoggerOStreamOutput.cxx \
			runtime/util/logger/src/OcpiLoggerPrefixInserter.cxx \
			runtime/util/logger/src/OcpiLoggerTee.cxx \
			runtime/util/memfs/src/OcpiUtilStaticMemFile.cxx \
			runtime/util/memfs/src/OcpiUtilStaticMemFs.cxx \
			runtime/util/misc/src/CmdOption.cxx \
			runtime/util/misc/src/OcpiExprEvaluator.cxx \
			runtime/util/misc/src/OcpiUtilAutoMutex.cxx \
			runtime/util/misc/src/OcpiUtilAutoRDLock.cxx \
			runtime/util/misc/src/OcpiUtilAutoWRLock.cxx \
			runtime/util/misc/src/OcpiUtilCommandLineConfiguration.cxx \
			runtime/util/misc/src/OcpiUtilException.cxx \
			runtime/util/misc/src/OcpiUtilHash.cxx \
			runtime/util/misc/src/OcpiUtilMisc.cxx \
			runtime/util/misc/src/OcpiUtilPci.cxx \
			runtime/util/misc/src/OcpiUtilRefCounter.cxx \
			runtime/util/misc/src/OcpiUtilTest.cxx \
			runtime/util/misc/src/OcpiUtilUUID.cxx \
			runtime/util/misc/src/OcpiUtilUri.cxx \
			runtime/util/parentChild/src/OcpiParentChild.cxx \
			runtime/util/property/src/OcpiUtilDataTypes.cxx \
			runtime/util/property/src/OcpiUtilWorker.cxx \
			runtime/util/property/src/OcpiUtilMemory.cxx \
			runtime/util/property/src/OcpiUtilPValue.cxx \
			runtime/util/property/src/OcpiUtilPort.cxx \
			runtime/util/property/src/OcpiUtilProperty.cxx \
			runtime/util/property/src/OcpiUtilProtocol.cxx \
			runtime/util/property/src/OcpiUtilValue.cxx \
			runtime/util/property/src/ValueReader.cxx \
			runtime/util/property/src/ValueWriter.cxx \
			runtime/util/pwq/src/api.c \
			runtime/util/pwq/src/load.c \
			runtime/util/pwq/src/manager.c \
			runtime/util/pwq/src/platform.c \
			runtime/util/pwq/src/thread_info.c \
			runtime/util/pwq/src/thread_rt.c \
			runtime/util/pwq/src/witem_cache.c \
			runtime/util/res/src/OcpiAlloc.cxx \
			runtime/util/streamfs/src/OcpiUtilStreamFsReader.cxx \
			runtime/util/streamfs/src/OcpiUtilStreamFsWriter.cxx \
			runtime/util/tcp/src/OcpiUtilLwLoggerOutput.cxx \
			runtime/util/tcp/src/OcpiUtilTcpClient.cxx \
			runtime/util/tcp/src/OcpiUtilTcpConnector.cxx \
			runtime/util/tcp/src/OcpiUtilTcpServer.cxx \
			runtime/util/tcp/src/OcpiUtilTcpStream.cxx \
			runtime/util/thread/src/OcpiThread.cxx \
			runtime/util/thread/src/OcpiUtilWorkerThread.cxx \
			runtime/util/time/src/OcpiTime.cxx \
			runtime/util/timeEmit/src/OcpiTimeEmit.cxx \
			runtime/util/vfs/src/OcpiUtilFilterFs.cxx \
			runtime/util/vfs/src/OcpiUtilLoadableModule.cxx \
			runtime/util/vfs/src/OcpiUtilReadOnlyFs.cxx \
			runtime/util/vfs/src/OcpiUtilUriFs.cxx \
			runtime/util/vfs/src/OcpiUtilVfs.cxx \
			runtime/util/vfs/src/OcpiUtilVfsIterator.cxx \
			runtime/util/vfs/src/OcpiUtilVfsMd5.cxx \
			runtime/util/vfs/src/OcpiUtilVfsUtil.cxx \
			runtime/util/zipfs/src/OcpiUtilZipFs.cxx \
			runtime/util/zipfs/src/OcpiUtilZipStream.cxx

__ocpi_build_dir__libocpi_util_1p_la_LDFLAGS = -static

incpath += \
			runtime/util/assembly/include \
			runtime/util/driver/include \
			runtime/util/ezxml/include \
			runtime/util/farmhash/include \
			runtime/util/fasttime/include \
			runtime/util/fpgaTime/include \
			runtime/util/filefs/include \
			runtime/util/http/include \
			runtime/util/ior/include \
			runtime/util/list/include \
			runtime/util/logger/include \
			runtime/util/md5/include \
			runtime/util/memfs/include \
			runtime/util/misc/include \
			runtime/util/parentChild/include \
			runtime/util/property/include \
			runtime/util/pwq/include \
			runtime/util/res/include \
			runtime/util/streamfs/include \
			runtime/util/tcp/include \
			runtime/util/thread/include \
			runtime/util/thread/include \
			runtime/util/time/include \
			runtime/util/timeEmit/include \
			runtime/util/uuid/include \
			runtime/util/vfs/include \
			runtime/util/zipfs/include \
			runtime/util/zlib/include

__ocpi_build_dir__libocpi_util_3p_la_SOURCES = \
			runtime/util/ezxml/src/ezxml.c \
			runtime/util/md5/src/md5.c \
			runtime/util/uuid/src/OcpiUuid.cxx \
			runtime/util/uuid/src/uuid.c \
			runtime/util/uuid/src/uuid_dce.c \
			runtime/util/uuid/src/uuid_mac.c \
			runtime/util/uuid/src/uuid_md5.c \
			runtime/util/uuid/src/uuid_prng.c \
			runtime/util/uuid/src/uuid_sha1.c \
			runtime/util/uuid/src/uuid_str.c \
			runtime/util/uuid/src/uuid_time.c \
			runtime/util/uuid/src/uuid_ui128.c \
			runtime/util/uuid/src/uuid_ui64.c \
			runtime/util/zlib/src/adler32.c \
			runtime/util/zlib/src/compress.c \
			runtime/util/zlib/src/crc32.c \
			runtime/util/zlib/src/deflate.c \
			runtime/util/zlib/src/gzio.c \
			runtime/util/zlib/src/infback.c \
			runtime/util/zlib/src/inffast.c \
			runtime/util/zlib/src/inflate.c \
			runtime/util/zlib/src/inftrees.c \
			runtime/util/zlib/src/ioapi.c \
			runtime/util/zlib/src/trees.c \
			runtime/util/zlib/src/uncompr.c \
			runtime/util/zlib/src/unzip.c \
			runtime/util/zlib/src/zip.c \
			runtime/util/zlib/src/zutil.c

__ocpi_build_dir__libocpi_util_3p_la_LDFLAGS = -static

__ocpi_build_dir__libocpi_util_1p_la_LIBADD = $(lib_ocpi_os)

__ocpi_build_dir__libocpi_util_3p_la_CFLAGS = $(AM_CFLAGS_NOSTRICT) -DEZXML_NOMMAP
__ocpi_build_dir__libocpi_util_3p_la_CXXFLAGS = $(AM_CXXFLAGS_NOSTRICT) -DEZXML_NOMMAP

##> runtime/dataplane

##> runtime/dataplane/rdma_utils
lib_ocpi_xfer = $(ocpi_build_dir)/libocpi_xfer.la

__ocpi_build_dir__libocpi_xfer_la_SOURCES = \
				runtime/dataplane/xfer/base/src/XferEndPoint.cxx \
				runtime/dataplane/xfer/base/src/XferFactory.cxx \
				runtime/dataplane/xfer/base/src/XferManager.cxx \
				runtime/dataplane/xfer/base/src/XferPio.cxx \
				runtime/dataplane/xfer/base/src/XferPioInternal.cxx \
				runtime/dataplane/xfer/base/src/XferServices.cxx

__ocpi_build_dir__libocpi_xfer_la_LDFLAGS = -static

incpath += runtime/dataplane/xfer/base/include

##> runtime/dataplane/rdma_drivers
# OLD: They will be checked in this order, so dependencies should be implicit, e.g. DMA needs PIO.
# NEW: No interdependencies any more
lib_ocpi_xfer_drivers = $(lib_ocpi_datagram) $(lib_ocpi_pio) $(lib_ocpi_dma) $(lib_ocpi_socket)

lib_ocpi_datagram = $(ocpi_build_dir)/libocpi_datagram_s.la
lib_ocpi_dma = $(ocpi_build_dir)/libocpi_dma_s.la
lib_ocpi_pio = $(ocpi_build_dir)/libocpi_pio_s.la
lib_ocpi_socket = $(ocpi_build_dir)/libocpi_socket_s.la
lib_ocpi_ofed = $(ocpi_build_dir)/libocpi_ofed_s.la

__ocpi_build_dir__libocpi_datagram_s_la_SOURCES = \
				runtime/dataplane/xfer/drivers/datagram/src/DtDataGramEtherDriver.cxx \
				runtime/dataplane/xfer/drivers/datagram/src/DtDataGramUDPDriver.cxx \
				runtime/dataplane/xfer/drivers/datagram/src/DtDataGramXfer.cxx

__ocpi_build_dir__libocpi_dma_s_la_SOURCES = \
				runtime/dataplane/xfer/drivers/dma/src/DtDmaXfer.cxx

__ocpi_build_dir__libocpi_pio_s_la_SOURCES = \
	                        runtime/dataplane/xfer/drivers/pio/src/HostSmemServices.cxx \
	                        runtime/dataplane/xfer/drivers/pio/src/HostPosixFileMappingServices.cxx \
				runtime/dataplane/xfer/drivers/pio/src/DtPioXfer.cxx

__ocpi_build_dir__libocpi_socket_s_la_SOURCES = \
				runtime/dataplane/xfer/drivers/socket/src/DtSocketXfer.cxx

# Retired library - runtime/dataplane/rdma_drivers/udp/src/DtUDPSocketXfer.cxx

__ocpi_build_dir__libocpi_ofed_s_la_SOURCES = \
				runtime/dataplane/xfer/drivers/ofed/src/DtOfedXfer.cxx

__ocpi_build_dir__libocpi_datagram_s_la_LDFLAGS = $(my_so_ldflags)
__ocpi_build_dir__libocpi_dma_s_la_LDFLAGS = $(my_so_ldflags)
__ocpi_build_dir__libocpi_pio_s_la_LDFLAGS = $(my_so_ldflags)
__ocpi_build_dir__libocpi_socket_s_la_LDFLAGS = $(my_so_ldflags)
__ocpi_build_dir__libocpi_ofed_s_la_LDFLAGS = $(my_so_ldflags)

incpath += \
				runtime/dataplane/xfer/drivers/datagram/include \
				runtime/dataplane/xfer/drivers/pio/include \
				runtime/dataplane/xfer/drivers/socket/include \
				runtime/dataplane/xfer/drivers/ofed/include

# Retired - runtime/dataplane/rdma_drivers/udp/include/DtUDPSocketXfer.h

# Build a stub that defines the missing OFED library functions.
# http://stackoverflow.com/a/5674708
lib_ofed_stub = $(ocpi_build_dir)/libocpi_ofed_stub.la
__ocpi_build_dir__libocpi_ofed_stub_la_SOURCES = \
				runtime/dataplane/xfer/drivers/ofed/src/OFED_Stubs.cxx

# The rpath is to make it a shared library; otherwise "-shared" is ignored unless added to lib_LTLIBRARIES!
# http://stackoverflow.com/a/8278784
__ocpi_build_dir__libocpi_ofed_stub_la_LDFLAGS = $(my_so_ldflags) -rpath $(abs_builddir)
noinst_LTLIBRARIES += $(lib_ofed_stub)

##> runtime/dataplane/transport
lib_ocpi_transport = $(ocpi_build_dir)/libocpi_transport.la

__ocpi_build_dir__libocpi_transport_la_SOURCES = \
			runtime/dataplane/transport/client_server/src/OcpiMessageCircuit.cxx \
			runtime/dataplane/transport/client_server/src/OcpiMessageEndpoint.cxx \
			runtime/dataplane/transport/ddnp/src/OcpiIntBlockPartition.cxx \
			runtime/dataplane/transport/ddnp/src/OcpiIntDataDistribution.cxx \
			runtime/dataplane/transport/ddnp/src/OcpiIntDataPartition.cxx \
			runtime/dataplane/transport/ddnp/src/OcpiIntIndivisiblePartition.cxx \
			runtime/dataplane/transport/ddnp/src/OcpiIntParallelDataDistribution.cxx \
			runtime/dataplane/transport/ddnp/src/OcpiIntSequentialDataDistribution.cxx \
			runtime/dataplane/transport/impl/src/OcpiAFCTransferController.cxx \
			runtime/dataplane/transport/impl/src/OcpiBuffer.cxx \
			runtime/dataplane/transport/impl/src/OcpiCircuit.cxx \
			runtime/dataplane/transport/impl/src/OcpiConnectionMetaData.cxx \
			runtime/dataplane/transport/impl/src/OcpiDataDistribution.cxx \
			runtime/dataplane/transport/impl/src/OcpiDataTxHandlerPattern1.cxx \
			runtime/dataplane/transport/impl/src/OcpiInputBuffer.cxx \
			runtime/dataplane/transport/impl/src/OcpiOutputBuffer.cxx \
			runtime/dataplane/transport/impl/src/OcpiParallelDataDistribution.cxx \
			runtime/dataplane/transport/impl/src/OcpiPort.cxx \
			runtime/dataplane/transport/impl/src/OcpiPortMetaData.cxx \
			runtime/dataplane/transport/impl/src/OcpiPortSet.cxx \
			runtime/dataplane/transport/impl/src/OcpiPortSetMetaData.cxx \
			runtime/dataplane/transport/impl/src/OcpiTemplateGenerators.cxx \
			runtime/dataplane/transport/impl/src/OcpiTransferController.cxx \
			runtime/dataplane/transport/impl/src/OcpiTransferTemplate.cxx \
			runtime/dataplane/transport/impl/src/OcpiTransport.cxx \
			runtime/dataplane/transport/impl/src/OcpiTransportGlobal.cxx \
			runtime/dataplane/transport/impl/src/TransportMailBox.cxx

__ocpi_build_dir__libocpi_transport_la_LDFLAGS = -static

incpath += \
			runtime/dataplane/transport/client_server/include \
			runtime/dataplane/transport/ddnp/include \
			runtime/dataplane/transport/impl/include

##> runtime/dataplane/msg_driver_interface
lib_ocpi_msg_driver_interface = $(ocpi_build_dir)/libocpi_msg_driver_interface.la

__ocpi_build_dir__libocpi_msg_driver_interface_la_SOURCES = runtime/dataplane/msg_driver_interface/src/DtMsgDriver.cxx

__ocpi_build_dir__libocpi_msg_driver_interface_la_LDFLAGS = -static

incpath += runtime/dataplane/msg_driver_interface/include

##> runtime/dataplane/msg_drivers
lib_ocpi_msg_drivers = $(ocpi_build_dir)/libocpi_msg_drivers.la

__ocpi_build_dir__libocpi_msg_drivers_la_SOURCES = \
				runtime/dataplane/msg_drivers/corba/src/OcpiCORBATransport.cxx \
				runtime/dataplane/msg_drivers/dds/src/OcpiDDSTransport.cxx

__ocpi_build_dir__libocpi_msg_drivers_la_LDFLAGS = -static

incpath += runtime/dataplane/msg_drivers/dds/include

##> runtime/dataplane/rdma_tests
#lib_ocpi_rdma_tests = $(ocpi_build_dir)/libocpi_xfer_tests.la

#__ocpi_build_dir__libocpi_rdma_tests_la_SOURCES = runtime/dataplane/xfer/tests/stub/src/stub.cxx

#__ocpi_build_dir__libocpi_rdma_tests_la_LDFLAGS = -static

# Same rule as "miscutilexec" above:
xferexecdir = $(bindir)/xfer_tests

# rdmaexec_PROGRAMS = $(ocpi_build_dir)/ClientServerTest $(ocpi_build_dir)/transportTest
xferexec_PROGRAMS = $(ocpi_build_dir)/transportTest

__ocpi_build_dir__transportTest_SOURCES = runtime/dataplane/xfer/tests/src/transportTest_main.cxx

##> runtime/library
lib_ocpi_library = $(ocpi_build_dir)/libocpi_library.la

__ocpi_build_dir__libocpi_library_la_SOURCES = \
				runtime/library/src/LibrarySimple.cxx  \
				runtime/library/src/OcpiComponentLibrary.cxx  \
				runtime/library/src/OcpiLibraryAssembly.cxx  \
				runtime/library/src/OcpiLibraryManager.cxx

__ocpi_build_dir__libocpi_library_la_LDFLAGS = -static

incpath += runtime/library/include

##> runtime/container
lib_ocpi_container = $(ocpi_build_dir)/libocpi_container.la

__ocpi_build_dir__libocpi_container_la_SOURCES = \
				runtime/container/src/Container.cxx \
				runtime/container/src/ContainerApplication.cxx \
				runtime/container/src/ContainerArtifact.cxx \
				runtime/container/src/ContainerBasicPort.cxx \
				runtime/container/src/ContainerLauncher.cxx \
				runtime/container/src/ContainerLocalPort.cxx \
				runtime/container/src/ContainerManager.cxx \
				runtime/container/src/ContainerMemory.cxx \
				runtime/container/src/ContainerPort.cxx \
				runtime/container/src/ContainerProperty.cxx \
				runtime/container/src/RemoteClient.cxx \
				runtime/container/src/ContainerWorker.cxx

__ocpi_build_dir__libocpi_container_la_LDFLAGS = -static

incpath += runtime/container/include

##> runtime/rcc
lib_ocpi_rcc= $(ocpi_build_dir)/libocpi_rcc_s.la

__ocpi_build_dir__libocpi_rcc_s_la_SOURCES = \
				runtime/rcc/src/RccApplication.cxx \
				runtime/rcc/src/RccContainer.cxx \
				runtime/rcc/src/RccDriver.cxx \
				runtime/rcc/src/RccPort.cxx \
				runtime/rcc/src/RccWorker.cxx

__ocpi_build_dir__libocpi_rcc_s_la_LDFLAGS = $(my_so_ldflags)

incpath += runtime/rcc/include

##> runtime/remote and runtime/remote-support
lib_ocpi_remote = $(ocpi_build_dir)/libocpi_remote_s.la

# Any executables within remote or remote-support get the Remote driver statically linked in and NOT as a shared object
REMOTE_LDADD = $(__ocpi_build_dir__libocpi_remote_s_la_OBJECTS) $(filter-out $(lib_ocpi_remote), $(LDADD))

__ocpi_build_dir__libocpi_remote_s_la_SOURCES = \
				runtime/remote/src/RemoteDriver.cxx \
				runtime/remote/src/RemoteLauncher.cxx

__ocpi_build_dir__libocpi_remote_s_la_LDFLAGS = $(my_so_ldflags)

incpath += runtime/remote/include runtime/remote-support/include

bin_PROGRAMS += $(ocpi_build_dir)/ocpiremote $(ocpi_build_dir)/ocpiserve

__ocpi_build_dir__ocpiremote_SOURCES = runtime/remote-support/src/ocpiremote_main.cxx
__ocpi_build_dir__ocpiremote_LDFLAGS = $(mybin_ldflags)
__ocpi_build_dir__ocpiremote_LDADD = $(REMOTE_LDADD)
__ocpi_build_dir__ocpiserve_SOURCES = runtime/remote-support/src/ocpiserve_main.cxx \
                                      runtime/remote-support/src/OcpiServer.cxx \
                                      runtime/remote-support/src/RemoteServer.cxx
__ocpi_build_dir__ocpiserve_LDFLAGS = $(mybin_ldflags)
__ocpi_build_dir__ocpiserve_LDADD = $(LDADD)

##> tools/cdkutils
lib_ocpi_cdkutils = $(ocpi_build_dir)/libocpi_cdkutils.la

__ocpi_build_dir__libocpi_cdkutils_la_SOURCES = \
			tools/cdkutils/src/cdkutils.cxx \
			tools/cdkutils/src/platforms.cxx

__ocpi_build_dir__libocpi_cdkutils_la_LDFLAGS = -static

incpath += tools/cdkutils/include

##> tools/cdk
# Q: Why do we put many of the same headers in both /include/ and /include/rcc?
# A: "We need to control what the rcc workers can include/see. The top level include is for main programs etc.. I could even imagine having a "aci" subdir next to "rcc", to make it even clearer." (AV-1460)

##> tools/cdk/ocpidds/bin
bin_PROGRAMS += $(ocpi_build_dir)/ocpidds

__ocpi_build_dir__ocpidds_SOURCES = tools/cdk/ocpidds/bin/src/ocpidds_main.cxx
__ocpi_build_dir__ocpidds_LDFLAGS = $(mybin_ldflags)

##> tools/cdk/ocpidds/ocpidds
lib_ocpi_ocpidds = $(ocpi_build_dir)/libocpi_ocpidds.la

__ocpi_build_dir__libocpi_ocpidds_la_SOURCES = \
			tools/cdk/ocpidds/ocpidds/src/dataTypeTest.cxx \
			tools/cdk/ocpidds/ocpidds/src/ocpidds.cxx

__ocpi_build_dir__libocpi_ocpidds_la_LDFLAGS = -static

incpath += tools/cdk/ocpidds/ocpidds/include

##> tools/cdk/ocpixml
bin_PROGRAMS += $(ocpi_build_dir)/ocpixml

__ocpi_build_dir__ocpixml_SOURCES = runtime/util/ezxml/src/ocpixml_main.cxx

##> tools/ocpigen
lib_ocpi_ocpigen = $(ocpi_build_dir)/libocpi_ocpigen.la

bin_PROGRAMS += $(ocpi_build_dir)/ocpigen
__ocpi_build_dir__ocpigen_SOURCES = runtime/application/include/av_team.h tools/ocpigen/src/ocpigen_main.cxx
__ocpi_build_dir__ocpigen_LDFLAGS = -static

__ocpi_build_dir__libocpi_ocpigen_la_SOURCES = \
				tools/ocpigen/src/assembly.cxx \
				tools/ocpigen/src/data.cxx \
				tools/ocpigen/src/dev.cxx \
				tools/ocpigen/src/hdl-assembly.cxx \
				tools/ocpigen/src/hdl-card.cxx \
				tools/ocpigen/src/hdl-config.cxx \
				tools/ocpigen/src/hdl-container.cxx \
				tools/ocpigen/src/hdl.cxx \
				tools/ocpigen/src/hdl-device.cxx \
				tools/ocpigen/src/hdl-platform.cxx \
				tools/ocpigen/src/hdl-slot.cxx \
				tools/ocpigen/src/hdl-worker.cxx \
				tools/ocpigen/src/ocl.cxx \
				tools/ocpigen/src/ocp.cxx \
				tools/ocpigen/src/parameters.cxx \
				tools/ocpigen/src/parse.cxx \
				tools/ocpigen/src/port.cxx \
				tools/ocpigen/src/rcc.cxx \
				tools/ocpigen/src/sdp.cxx \
				tools/ocpigen/src/tests.cxx \
				tools/ocpigen/src/wci.cxx \
				tools/ocpigen/src/wmemi.cxx \
				tools/ocpigen/src/wmi.cxx \
				tools/ocpigen/src/wsi.cxx \
				tools/ocpigen/src/wti.cxx

__ocpi_build_dir__libocpi_ocpigen_la_LDFLAGS = -static

incpath += tools/ocpigen/include

##> runtime/hdl and runtime/hdl-support (split while porting to autotools)
lib_ocpi_hdl = $(ocpi_build_dir)/libocpi_hdl_s.la

# Any executables within hdl or hdl-support get the HDL driver statically linked in and NOT as a shared object
# In OSS build, they use "EXTRAOBJS" (AV-2655)
HDL_LDADD = $(__ocpi_build_dir__libocpi_hdl_s_la_OBJECTS) $(filter-out $(lib_ocpi_hdl), $(LDADD))

__ocpi_build_dir__libocpi_hdl_s_la_SOURCES = \
				runtime/hdl/src/HdlAccess.cxx \
				runtime/hdl/src/HdlBusDriver.cxx \
				runtime/hdl/src/HdlContainer.cxx \
				runtime/hdl/src/HdlDevice.cxx \
				runtime/hdl/src/HdlDriver.cxx \
				runtime/hdl/src/HdlDummyWorker.cxx \
				runtime/hdl/src/HdlEtherDriver.cxx \
				runtime/hdl/src/HdlLSimDevice.cxx \
				runtime/hdl/src/HdlNetDriver.cxx \
				runtime/hdl/src/HdlPciDriver.cxx \
				runtime/hdl/src/HdlSimDriver.cxx \
				runtime/hdl/src/HdlSdp.cxx \
				runtime/hdl/src/HdlWciControl.cxx

__ocpi_build_dir__libocpi_hdl_s_la_LDFLAGS = $(my_so_ldflags)

incpath += runtime/hdl/include

# getPci.h is in /src/ and there is no /include/ subdir:
incpath += runtime/hdl-support/src

bin_PROGRAMS += $(ocpi_build_dir)/ocpihdl $(ocpi_build_dir)/ocfrp_check
# AV-1783: Removed ocpinft but keep ability to return
# bin_PROGRAMS += $(ocpi_build_dir)/ocpinft
# AV-2913: Same
# bin_PROGRAMS += $(ocpi_build_dir)/testRpl

if OCPI_CROSSCOMPILING_ZYNQ
  bin_PROGRAMS += $(ocpi_build_dir)/ocpizynq
endif

__ocpi_build_dir__ocfrp_check_SOURCES = runtime/hdl-support/src/ocfrp_check_main.cxx
__ocpi_build_dir__ocfrp_check_LDFLAGS = $(mybin_ldflags)
__ocpi_build_dir__ocfrp_check_LDADD = $(HDL_LDADD)

__ocpi_build_dir__ocpihdl_SOURCES = runtime/hdl-support/src/ocpihdl_main.cxx
__ocpi_build_dir__ocpihdl_LDFLAGS = $(mybin_ldflags)
__ocpi_build_dir__ocpihdl_LDADD = $(HDL_LDADD)

# AV-1783 __ocpi_build_dir__ocpinft_SOURCES = runtime/hdl-support/src/ocpinft_main.c
# AV-1783 __ocpi_build_dir__ocpinft_LDFLAGS = $(mybin_ldflags)
# HDL_LDADD too

__ocpi_build_dir__ocpizynq_SOURCES = runtime/hdl-support/src/ocpizynq_main.cxx
__ocpi_build_dir__ocpizynq_LDFLAGS = $(mybin_ldflags)
__ocpi_build_dir__ocpizynq_LDADD = $(HDL_LDADD)

# AV-2913 __ocpi_build_dir__testRpl_SOURCES = runtime/hdl-support/src/testRpl_main.cxx
# AV-2913 __ocpi_build_dir__testRpl_LDFLAGS = $(mybin_ldflags)
# AV-2913 __ocpi_build_dir__testRpl_LDADD = $(HDL_LDADD)

##> runtime/ocl and runtime/ocl-support (split while porting to autotools)
lib_ocpi_ocl = $(ocpi_build_dir)/libocpi_ocl_s.la

# Any executables within ocl or ocl-support get the OCL driver statically linked in and NOT as a shared object
OCL_LDADD = $(__ocpi_build_dir__libocpi_ocl_s_la_OBJECTS) $(filter-out $(lib_ocpi_ocl), $(LDADD))

__ocpi_build_dir__libocpi_ocl_s_la_SOURCES = \
				runtime/ocl/src/OclContainer.cxx

__ocpi_build_dir__libocpi_ocl_s_la_LDFLAGS = $(my_so_ldflags)

incpath += runtime/ocl/include runtime/ocl/include/CL

bin_PROGRAMS += $(ocpi_build_dir)/ocpiocl

__ocpi_build_dir__ocpiocl_SOURCES = runtime/ocl-support/src/OclCompile.cxx \
                                    runtime/ocl-support/src/ocpiocl_main.cxx

__ocpi_build_dir__ocpiocl_LDFLAGS = -Xlinker --warn-unresolved-symbols $(mybin_ldflags)
__ocpi_build_dir__ocpiocl_LDADD = $(OCL_LDADD)

bin_PROGRAMS += $(ocpi_build_dir)/ocpiocltest

__ocpi_build_dir__ocpiocltest_SOURCES = runtime/ocl-support/src/ocpiocltest_main.cxx

__ocpi_build_dir__ocpiocltest_LDFLAGS = $(mybin_ldflags)
__ocpi_build_dir__ocpiocltest_LDADD = $(LDADD)

##> runtime/application
lib_ocpi_application = $(ocpi_build_dir)/libocpi_application.la

__ocpi_build_dir__libocpi_application_la_SOURCES = \
				runtime/application/src/OcpiApplication.cxx

__ocpi_build_dir__libocpi_application_la_LDFLAGS = -static

incpath += runtime/application/include

bin_PROGRAMS += $(ocpi_build_dir)/ocpirun

if !OCPI_CROSSCOMPILING
noinst_PROGRAMS += $(ocpi_build_dir)/builddrivertest

# Test application to see if all symbols are linked in properly.
# Note the extra space at the end of DRIVERLIST is required.
__ocpi_build_dir__builddrivertest_SOURCES = runtime/application/src/builddrivertest_main.cxx
__ocpi_build_dir__builddrivertest_LDFLAGS = $(mybin_ldflags)
__ocpi_build_dir__builddrivertest_CXXFLAGS =  $(AM_CXXFLAGS) \
  -DDRIVERLIST="\"$(notdir $(basename $(ocpi_driver_libs))) \"" \
  -DCDTO="\"$(ocpi_build_dir)\""
TESTS = $(ocpi_build_dir)/builddrivertest $(ocpi_build_dir)/rttest $(ocpi_build_dir)/test_advanced_port_mgmt $(ocpi_build_dir)/test_control_errors $(ocpi_build_dir)/test_optional_ports $(ocpi_build_dir)/test_shutdown $(ocpi_build_dir)/test_timeout_worker $(ocpi_build_dir)/test_transfer_roles
# This test requires build components, which we might not have: $(ocpi_build_dir)/test_xml_worker

# We will do even more tests when building on Jenkins
# (ones that take more than a few second, but not TOO long, since have post-RPM test jobs as well):
# if ON_JENKINS
# TESTS += $(ocpi_build_dir)/test_zero_copy_io
# nope - way too long $(ocpi_build_dir)/ocpitests
# endif
endif

__ocpi_build_dir__ocpirun_SOURCES = runtime/application/src/ocpirun_main.cxx
__ocpi_build_dir__ocpirun_LDFLAGS = $(mybin_ldflags)

##> tests
lib_ocpi_tests = $(ocpi_build_dir)/libocpi_tests.la

bin_PROGRAMS += $(ocpi_build_dir)/ocpitests
__ocpi_build_dir__ocpitests_SOURCES = \
			tests/src/ocpitests_main.cxx \
			$(__ocpi_build_dir__libocpi_tests_la_SOURCES)

__ocpi_build_dir__ocpitests_CXXFLAGS = $(AM_CXXFLAGS) @GTEST_CXXFLAGS@

incpath += tests/test-logger/include

__ocpi_build_dir__ocpitests_LDFLAGS = -static

__ocpi_build_dir__ocpitests_LDADD = $(LDADD) @GTEST_STATIC_LIBS@

__ocpi_build_dir__libocpi_tests_la_SOURCES = \
			tests/test-hdlbag/src/test-fixture1.cxx \
			tests/test-logger/src/message-keeper.cxx \
			tests/test-logger/src/test-fallback.cxx \
			tests/test-logger/src/test-null-output.cxx \
			tests/test-logger/src/test-ostream.cxx \
			tests/test-logger/src/test-prefix-inserter.cxx \
			tests/test-logger/src/test-tee.cxx \
			tests/test-os/src/test-event.cxx \
			tests/test-os/src/test-file-system.cxx \
			tests/test-os/src/test-mutex.cxx \
			tests/test-os/src/test-process-manager.cxx \
			tests/test-os/src/test-socket.cxx \
			tests/test-os/src/test-thread-manager.cxx \
			tests/test-os/src/test-timer.cxx

__ocpi_build_dir__libocpi_tests_la_LDFLAGS = -static

##> runtime/ctests
lib_ocpi_ctests = $(ocpi_build_dir)/libocpi_ctests.la

# Same rule as "miscutilexec" above:
ctestexecdir = $(bindir)/ctests

# Removed: GPPContainerTest RCCLbTest RTxTest test_user_port
ctestexec_PROGRAMS = $(ocpi_build_dir)/rttest $(ocpi_build_dir)/test_advanced_port_mgmt $(ocpi_build_dir)/test_control_errors $(ocpi_build_dir)/test_optional_ports $(ocpi_build_dir)/test_shutdown $(ocpi_build_dir)/test_timeout_worker $(ocpi_build_dir)/test_transfer_roles $(ocpi_build_dir)/test_xml_worker $(ocpi_build_dir)/test_zero_copy_io

#__ocpi_build_dir__GPPContainerTest_SOURCES = runtime/ctests/src/GPPContainerTest_main.cxx \
#				runtime/ctests/src/test_utilities.cxx
#__ocpi_build_dir__GPPContainerTest_LDFLAGS = $(mybin_ldflags)

#__ocpi_build_dir__RCCLbTest_SOURCES = runtime/ctests/src/RCCLbTest_main.cxx \
#			runtime/ctests/src/test_utilities.cxx
#__ocpi_build_dir__RCCLbTest_LDFLAGS = $(mybin_ldflags)

__ocpi_build_dir__rttest_SOURCES = runtime/ctests/src/rttest_main.cxx \
		runtime/ctests/src/test_utilities.cxx
__ocpi_build_dir__rttest_LDFLAGS = $(mybin_ldflags)

#__ocpi_build_dir__RTxTest_SOURCES = runtime/ctests/src/RTxTest_main.cxx \
#		runtime/ctests/src/test_utilities.cxx
#__ocpi_build_dir__RTxTest_LDFLAGS = $(mybin_ldflags)

__ocpi_build_dir__test_advanced_port_mgmt_SOURCES = runtime/ctests/src/test_advanced_port_mgmt_main.cxx \
					runtime/ctests/src/test_utilities.cxx
__ocpi_build_dir__test_advanced_port_mgmt_LDFLAGS = $(mybin_ldflags)

__ocpi_build_dir__test_control_errors_SOURCES = runtime/ctests/src/test_control_errors_main.cxx \
				runtime/ctests/src/test_utilities.cxx
__ocpi_build_dir__test_control_errors_LDFLAGS = $(mybin_ldflags)

__ocpi_build_dir__test_optional_ports_SOURCES = runtime/ctests/src/test_optional_ports_main.cxx \
				runtime/ctests/src/test_utilities.cxx
__ocpi_build_dir__test_optional_ports_LDFLAGS = $(mybin_ldflags)

__ocpi_build_dir__test_shutdown_SOURCES = runtime/ctests/src/test_shutdown_main.cxx \
			runtime/ctests/src/test_utilities.cxx
__ocpi_build_dir__test_shutdown_LDFLAGS = $(mybin_ldflags)

__ocpi_build_dir__test_timeout_worker_SOURCES = runtime/ctests/src/test_timeout_worker_main.cxx \
				runtime/ctests/src/test_utilities.cxx
__ocpi_build_dir__test_timeout_worker_LDFLAGS = $(mybin_ldflags)

__ocpi_build_dir__test_transfer_roles_SOURCES = runtime/ctests/src/test_transfer_roles_main.cxx \
				runtime/ctests/src/test_utilities.cxx
__ocpi_build_dir__test_transfer_roles_LDFLAGS = $(mybin_ldflags)

#__ocpi_build_dir__test_user_port_SOURCES = runtime/ctests/src/test_user_port_main.cxx \
#				runtime/ctests/src/test_utilities.cxx
#__ocpi_build_dir__test_user_port_LDFLAGS = $(mybin_ldflags)

__ocpi_build_dir__test_xml_worker_SOURCES = runtime/ctests/src/test_xml_worker_main.cxx \
			  runtime/ctests/src/test_utilities.cxx
__ocpi_build_dir__test_xml_worker_LDFLAGS = $(mybin_ldflags)

__ocpi_build_dir__test_zero_copy_io_SOURCES = runtime/ctests/src/test_zero_copy_io_main.cxx \
			  runtime/ctests/src/test_utilities.cxx
__ocpi_build_dir__test_zero_copy_io_LDFLAGS = $(mybin_ldflags)

__ocpi_build_dir__libocpi_ctests_la_SOURCES = \
				runtime/ctests/src/ConsumerWorker.c \
				runtime/ctests/src/LoopbackWorker.c \
				runtime/ctests/src/ProdWorker.c \
				runtime/ctests/src/UtGenericLoopBackWorkers.c \
				runtime/ctests/src/UtZeroCopyIOWorkers.c

__ocpi_build_dir__libocpi_ctests_la_LDFLAGS = -static

incpath += runtime/ctests/include

ctestexec_SCRIPTS =			runtime/ctests/src/run_tests.sh

# SWIG/GNU Radio Stuff
if SWIG_CHECK
if !OCPI_CROSSCOMPILING
##> tools/ocpigr/bin
bin_PROGRAMS += $(ocpi_build_dir)/ocpigr

__ocpi_build_dir__ocpigr_SOURCES = tools/ocpigr/src/ocpigr_main.cxx
__ocpi_build_dir__ocpigr_LDFLAGS = $(mybin_ldflags)

# SWIG OcpiApi.i => Wrapper
swig/OcpiApi_wrap.cxx: exports $(compat_mkfile)
	swig -c++ -python -I$(OCPI_CDK_DIR)/include/rcc swig/OcpiApi.i
swig/OcpiApi.py: swig/OcpiApi_wrap.cxx ;
CLEANFILES += swig/OcpiApi_wrap.cxx swig/OcpiApi.py

# SWIG Wrapper => .so / .pyc
lib_ocpi_swig_api = $(ocpi_build_dir)/_OcpiApi.la

__ocpi_build_dir___OcpiApi_la_SOURCES  = swig/OcpiApi_wrap.cxx
__ocpi_build_dir___OcpiApi_ladir       = $(DESTDIR)$(libdir)
__ocpi_build_dir___OcpiApi_la_DATA     = swig/OcpiApi.py
__ocpi_build_dir___OcpiApi_la_CFLAGS   = $(AM_CFLAGS) @PYTHON_CFLAGS@
__ocpi_build_dir___OcpiApi_la_CXXFLAGS = $(AM_CXXFLAGS) @PYTHON_CFLAGS@
__ocpi_build_dir___OcpiApi_la_LDFLAGS  = \
				$(my_so_ldflags) \
				$(lib_ocpi_application) \
				$(lib_ocpi_container) \
				$(lib_ocpi_library) \
				$(lib_ocpi_xfer) \
				$(lib_ocpi_transport) \
				$(lib_ocpi_util) \
				$(lib_ocpi_os) \
				$(lib_ocpi_rcc) \
				$(lib_ocpi_remote) \
				$(lib_ocpi_hdl) \
				$(lib_ocpi_pio) \
				$(lib_ocpi_dma) \
				$(lib_ocpi_socket) \
				/opt/opencpi/prerequisites/lzma/$(ocpi_target_host)/lib/liblzma.a \
				$(foreach lib,gmpxx gmp, /opt/opencpi/prerequisites/gmp/$(ocpi_target_host)/lib/lib$(lib).a)
__ocpi_build_dir___OcpiApi_la_DEPENDENCIES = \
				$(lib_ocpi_application) \
				$(lib_ocpi_container) \
				$(lib_ocpi_library) \
				$(lib_ocpi_xfer) \
				$(lib_ocpi_transport) \
				$(lib_ocpi_util) \
				$(lib_ocpi_os) \
				$(lib_ocpi_rcc) \
				$(lib_ocpi_remote) \
				$(lib_ocpi_hdl) \
				$(lib_ocpi_pio) \
				$(lib_ocpi_dma) \
				$(lib_ocpi_socket)
endif
endif
