# Apparently rpmbuild does not do this for you? just %{prefix}
Name:        %{RPM_NAME}
Version:     %{RPM_VERSION}
Release:     %{RPM_RELEASE}%{?dist}
Summary:     A framework to simplify and enable code portability of real-time systems
Group:       Applications/Engineering
License:     LGPLv3+
%global      prefix0 /opt/opencpi
Prefix:      %{prefix0}
# Aparently rpmbuild does not do this for you, only RPM_INSTALL_DIR0/1/2 etc.
%global      prefix1 /etc
Prefix:      %{prefix1}
Vendor:      ANGRYVIPER Team
Packager:    ANGRYVIPER team <discuss@lists.opencpi.org>
%if !0%{?RPM_CROSS:1}
##########################################################################################
# Native/development host package
%description 
Open Component Portability Infrastructure (OpenCPI)

An open source software (OSS) framework to simplify complexity and enable
code portability of real-time systems

 *   Creates a hardware abstraction layer for embedded real-time systems
 *   Real-time middleware for embedded systems
 *   Middleware for Waveform-Ready™ Processing Platforms leading to improved
     waveform code portability with standards-based interfaces
 *   Increased interoperability using container technology
 *   Real-time virtualization
 *   Component Portability Infrastructure (CPI) is a real-time embedded (RTE)
     middleware framework that simplifies programming of heterogeneous
     processing applications requiring, in a system or across a tech refresh,
     a mix of processing and interconnect technologies.
%else
##########################################################################################
# Cross build package
AutoReqProv: no  # This must preceed the %description.  Go figure.
%global      __strip ${OCPI_CROSS_COMPILE}-strip
BuildArch:   noarch
%define _binaries_in_noarch_packages_terminate_build 0
#Requires(pre,postun): %{RPM_BASENAME}
Obsoletes:   %{RPM_BASENAME}-platform-%{RPM_CROSS}
%description
This package contains the OpenCPI static libraries for cross-compiling
for the %{RPM_PLATFORM} target platform, along with core components.
%endif

%{?RPM_HASH:ReleaseID: %{RPM_HASH}}

# suppress generic post processing that is done for all packagers, like python bytecompile,
# stripping, etc. This does not suppress things like check-buildroot
%global __os_install_post %{nil}
%install
cd %{RPM_OPENCPI}
set -e
rm -r -f %{buildroot}%{prefix0}
mkdir -p %{buildroot}%{prefix0}
# The prepare script outputs source/dest appropriate for cp -R
prepare=%{RPM_OPENCPI}/packaging/prepare-packaging-list.sh
for p in runtime devel; do
  cp /dev/null %{_builddir}/$p-files
  $prepare $p %{RPM_PLATFORM} | while read source dest; do
    if [ -n  "$dest" ]; then
      xform="-e s=^$(dirname $source)/=$dest/="
      mkdir -p %{buildroot}%{prefix0}/$dest
    else
      xform=
      mkdir -p %{buildroot}%{prefix0}/$(dirname $source)
    fi
    (find -L $source -type f | sed $xform -e "s=^=%%{prefix0}/=";
     find -L $source -type d | sed $xform -e "s=^=%dir %%{prefix0}/=") >> %{_builddir}/$p-files
    cp -R -L $source %{buildroot}%{prefix0}/${dest:-$source}
  done > %{_builddir}/$p-files
done
##########################################################################################
# Enable the globally-installed files, trying to symlink them to their "real" copies if possible
# 1. Tell ld.so to look in our lib dir when looking for dynamic libraries
#    Really only needed with a dynamic library installation
#    But we always have this directory (due to plugins and swigs), so it probably does no harm
#    We could make it conditional for a slight optimization
%{__mkdir_p} %{buildroot}%{prefix1}/ld.so.conf.d
echo %{prefix0}/%{RPM_PLATFORM}/lib > %{buildroot}%{prefix1}/ld.so.conf.d/opencpi.conf
echo %%{prefix1}/ld.so.conf.d/opencpi.conf >> %{_builddir}/runtime-files
# 2. Enable a global login .profile script by dropping ours into the directory that is used
#    globally for all bash login scripts.  This drop-in is a symlink.
%{__mkdir_p} %{buildroot}%{prefix1}/profile.d
%{__ln_s} -f %{prefix0}/env/rpm_cdk.sh %{buildroot}%{prefix1}/profile.d/opencpi.sh
echo %%{prefix1}/profile.d/opencpi.sh >> %{_builddir}/runtime-files
# 3. Enable bash completion of our comments by dropping a completion script into a directory that
#    is used when interactive bash scripts startup.  Only for the devel package.
%{__mkdir_p} %{buildroot}%{prefix1}/bash_completion.d
%{__ln_s} -f %{prefix0}/scripts/ocpidev_bash_complete \
             %{buildroot}%{prefix1}/bash_completion.d/ocpidev_bash_complete.sh
echo %%{prefix1}/bash_completion.d/ocpidev_bash_complete.sh >> %{_builddir}/devel-files
# 4. Add our udev-rules into the drop-in directry for udev rules
%{__mkdir_p} %{buildroot}%{prefix1}/udev/rules.d/
%{__ln_s} -f %{prefix0}/udev-rules/51-opencpi-usbblaster.rules %{buildroot}%{prefix1}/udev/rules.d/
#echo %%{prefix1}/udev/rules.d/51-opencpi-usbblaster.rules >> %{_builddir}/runtime-files
echo "%%attr(644,root,root)" %%{prefix1}/udev/rules.d/\*.rules >> %{_builddir}/runtime-files


%files -f runtime-files

##########################################################################################
# The development (sub) package, that adds to what is installed after the runtime package.
%package devel
AutoReqProv: no
Requires:   %{name} = %{version}-%{release}
Requires(pre,postun): %{name} = %{version}-%{release}
Summary:    The OpenCPI development package
Group:      Development/Libraries
Prefix:     %{prefix0}
Prefix:     %{prefix1}
%description devel
This package ensures that all requirements for OpenCPI development are
installed. It also provides a useful development utilities.
%{?RPM_HASH:ReleaseID: %{RPM_HASH}}

%files devel -f devel-files

%post
# We need to relocate all the global files that point to other global files.
# The files have been installed, but we must change them now.

# Relocate the global files that point to other files to respect relocations
echo $RPM_INSTALL_PREFIX0/cdk/%{RPM_PLATFORM}/lib > $RPM_INSTALL_PREFIX1/ld.so.conf.d/opencpi.conf
ln -s -f $RPM_INSTALL_PREFIX0/cdk/env/rpm_cdk.sh $RPM_INSTALL_PREFIX1/profile.d/opencpi.sh
ln -s -f $RPM_INSTALL_PREFIX0/cdk/scripts/ocpidev_bash_complete \
         $RPM_INSTALL_PREFIX1/bash_completion.d/ocpidev_bash_complete.sh
ln -s -f $RPM_INSTALL_PREFIX0/cdk/udev-rules/51-opencpi-usbblaster.rules \
         $RPM_INSTALL_PREFIX1/udev/rules.d/

%post devel
# We need to relocate all the global files that point to other global files.
# The files have been installed, but we must change them now.
ln -s -f $RPM_INSTALL_PREFIX0/cdk/scripts/ocpidev_bash_complete \
         $RPM_INSTALL_PREFIX1/bash_completion.d/ocpidev_bash_complete.sh
