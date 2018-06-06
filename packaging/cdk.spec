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
# We convert that into what is needed for the RPM file list
prepare=%{RPM_OPENCPI}/packaging/prepare-packaging-list.sh
for p in runtime devel; do
  $prepare $p %{RPM_PLATFORM} %{?RPM_CROSS:1} | while read source dest; do
    if [ -n  "$dest" ]; then
      xform="-e s=^$(dirname $source)/=$dest/="
      mkdir -p %{buildroot}%{prefix0}/$dest
    else
      xform=
      mkdir -p %{buildroot}%{prefix0}/$(dirname $source)
    fi
    # nasty special case when a link should remain a link.
    if [ $(dirname $source) = project-registry ]; then
      echo $source
      cp -R $source %{buildroot}%{prefix0}/${dest:-$source}
    else
      # echo SOURCE:$source DEST:$dest > /dev/tty
      find -L $source -type f | sed $xform -e s/foo/foo/
      cp -R -L $source %{buildroot}%{prefix0}/${dest:-$source}
    fi
  done | \
  while read file; do
    dir=$(dirname $file)
    while [ $dir != . ] ; do
      [ -z "%{?RPM_CROSS:1}" -o $(basename $dir) = "%{RPM_CROSS}" ] && echo "%%dir %%{prefix0}/$dir"
      dir=$(dirname $dir)
    done
    echo "%%{prefix0}/$file"
  done | sort -u > %{_builddir}/$p-files
done
%if !0%{?RPM_CROSS:1}
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
               %{buildroot}%{prefix1}/bash_completion.d/opencpi_complete.bash
  echo %%{prefix1}/bash_completion.d/opencpi_complete.bash >> %{_builddir}/devel-files
  # 4. Add our udev-rules into the drop-in directry for udev rules
  %{__mkdir_p} %{buildroot}%{prefix1}/udev/rules.d/
  %{__ln_s} -f %{prefix0}/udev-rules/51-opencpi-usbblaster.rules %{buildroot}%{prefix1}/udev/rules.d/
  echo %%{prefix1}/udev/rules.d/51-opencpi-usbblaster.rules >> %{_builddir}/runtime-files
  #echo "%%{prefix1}/udev/rules.d/\*.rules >> %{_builddir}/runtime-files
  echo %%dir %%{prefix0} >> %{_builddir}/runtime-files
  # A very special case that will go away at some point
  cp releng/projects/new_project_source %{buildroot}%{prefix0}/projects
  echo %%{prefix0}/projects/new_project_source >> %{_builddir}/devel-files
%endif

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

%pre
# This is exact copy from opencpi-driver.spec; if changed, change there as well.
# Check if somebody is installing on the wrong platform (AV-721)
# Starting with 1.2, we ship the EL7 version for both EL6 and EL7 when it comes to platforms
%if !0%{?RPM_CROSS:1}
if [ -n "%{dist}" ]; then
  PKG_VER=`echo %{dist} | perl -ne '/el(\d)/ && print $1'`
  THIS_VER=`perl -ne '/release (\d)/ && print $1' /etc/redhat-release`
  if [ -n "${PKG_VER}" -a -n "${THIS_VER}" ]; then
    if [ ${PKG_VER} -ne ${THIS_VER} ]; then
      for i in `seq 20`; do echo ""; done
      echo "WARNING: This RPM is for CentOS${PKG_VER}, but you seem to be running CentOS${THIS_VER}"
      echo "You might want to uninstall these RPMs immediately and get the CentOS${THIS_VER} version."
      for i in `seq 5`; do echo ""; done
    fi
  fi
fi
%endif


%post

# Since we want to make the global installation aspects optional, we deal with the opencpi
# user and group optionally after all the files are installed

# Deal with the opencpi user and group if we are installing into /etc
if [ "$RPM_INSTALL_PREFIX1" = %{prefix1} ] ; then
  # Recipe from https://fedoraproject.org/wiki/Packaging:UsersAndGroups
  # -M is don't create home dir, -r is system account, -s is shell
  # -c is comment, -n is don't create group, -g is group name/id
  getent group opencpi >/dev/null || groupadd -r opencpi
  getent passwd opencpi >/dev/null || \
      useradd -M -r -s /sbin/nologin \
      -c "OpenCPI System Account" -n -g opencpi opencpi > /dev/null
  chown -R opencpi:opencpi $RPM_INSTALL_PREFIX0
fi

# We need to relocate all the global files that point to other global files.
# The files have been installed, but we must change them now.

# Relocate the global files that point to other files to respect relocations
echo $RPM_INSTALL_PREFIX0/cdk/%{RPM_PLATFORM}/lib > $RPM_INSTALL_PREFIX1/ld.so.conf.d/opencpi.conf
ln -s -f $RPM_INSTALL_PREFIX0/cdk/env/rpm_cdk.sh $RPM_INSTALL_PREFIX1/profile.d/opencpi.sh
ln -s -f $RPM_INSTALL_PREFIX0/cdk/udev-rules/51-opencpi-usbblaster.rules \
         $RPM_INSTALL_PREFIX1/udev/rules.d/

# Since these are all symlinks we are actually changing ownership of the underlying file
if [ "$RPM_INSTALL_PREFIX1" = %{prefix1} ] ; then
  chown -R root:root %{prefix0}/cdk/env.d
  chown root:root %{prefix0}/cdk/env/rpm_cdk.sh
fi

# restore initial
%preun
# I tried this to see if it allowed directories to be removed by it didn;t
#if [ "$RPM_INSTALL_PREFIX1" = %{prefix1} ] ; then
  # chown -R root:root $RPM_INSTALL_PREFIX0
#fi
%postun
echo I AM MANUALLY REMOVING $RPM_INSTALL_PREFIX0
rm -r -f -v $RPM_INSTALL_PREFIX0

%post devel
# We need to relocate all the global files that point to other global files.
# The files have been installed, but we must change them now.
ln -s -f $RPM_INSTALL_PREFIX0/cdk/scripts/ocpidev_bash_complete \
         $RPM_INSTALL_PREFIX1/bash_completion.d/opencpi_complete.bash

