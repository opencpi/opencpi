Name:        %{RPM_NAME}
Version:     %{RPM_VERSION}
Release:     %{RPM_RELEASE}%{?dist}
Summary:     A framework to simplify and enable code portability of real-time systems
Group:       Applications/Engineering
License:     LGPLv3+
Prefix:      /opt/opencpi
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
#Requires(pre,postun): %{RPM_BASENAME}
Obsoletes:   %{RPM_BASENAME}-platform-%{RPM_CROSS}
%description
This package contains the OpenCPI static libraries for cross-compiling
for the %{OCPI_TARGET_PLATFORM} target platform, along with core components.
%endif

%{?RPM_HASH:ReleaseID: %{RPM_HASH}}

# suppress generic post processing that is done for all packagers, like
# python bytecompile, stripping, etc.
# This does not suppress things like check-buildroot
%global __os_install_post %{nil}
%install
cd $OCPI_CDK_DIR # We are entering the exports tree
# Create the list of files for the %files section below, and copy the same files to BUILD_ROOT
eval find -L . -type f $OCPI_EXCLUDE_FOR_FIND | sed "s=^\./=%%{prefix}/=" > %{_builddir}/files
mkdir -p %{buildroot}/%{prefix}
# Copy through the export links into the build root.
set -vx
cp -R -L $(ls -d1 *| egrep -v /$OCPI_EXCLUDE_FOR_CP) %{buildroot}/%{prefix}
(cd %{buildroot}/%{prefix}; find . -type d | sed "s=^\.=%dir %%{prefix}=") >>%{_builddir}/files

%files -f files
