# Disable the built-in post-processing of files, namely compressing man pages and stripping
# %global __os_install_post %{nil}
Name:    opencpi-cdk
#Requires:
#Obsoletes:
#BuildArch:
#AutoReqProv:
#Source:
Version: 1.0%{?RPM_VERSION}
Release: rel%{?RPM_RELEASE}%{?RELEASE_TAG}%{?COMMIT_TAG}%{?dist}
Summary: A framework to simplify and enable code portability of real-time systems
Group:   Applications/Engineering
License: LGPLv3+
Prefix:  /opt/opencpi

%description 
test

%install
cd $OCPI_CDK_DIR
# Create the list of files for the %files section below, and copy the same files to BUILD_ROOT
eval find -L . -type f $OCPI_EXCLUDE_FOR_FIND | sed "s=^\./=%%{prefix}/=" > $RPM_BUILD_DIR/files
mkdir -p $RPM_BUILD_ROOT/%{prefix}
# Copy through the export links into the build root.
cp -R -L $(ls -d1 *| egrep -v /$OCPI_EXCLUDE_FOR_CP) $RPM_BUILD_ROOT/%{prefix}

%files -f files
