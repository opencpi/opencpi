#!/bin/bash
OCPI_REDHAWK_VERSION=2.0.5
case `uname -r` in
    (*.el6.*) osv=6;;
    (*.el7.*) osv=7;;
    (*) echo Unsupported operating system for Redhawk installation; exit 1;;
esac
source ./scripts/setup-install.sh \
       "$1" \
       redhawk \
       redhawk-yum-${OCPI_REDHAWK_VERSION}-el${osv}-x86_64.tar.gz \
       https://github.com/RedhawkSDR/redhawk/releases/download/${OCPI_REDHAWK_VERSION} \
       redhawk-${OCPI_REDHAWK_VERSION}-el${osv}-x86_64 \
       0
# Remove the build directory called by setup-install.sh
cd ..
rm -r -f ocpi-build-*
echo ========= Ensure epel-release package is installed
yum list installed | grep epel-release.noarch > /dev/null ||
  sudo yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-${osv}.noarch.rpm
sudo yum -y update epel-release
echo ========= Establishing the RedHawk yum repository, locally in
cat<<EOF|sed 's@LDIR@'`pwd`'@g'|sudo tee /etc/yum.repos.d/redhawk.repo
[redhawk]
name=REDHAWK Repository
baseurl=file://LDIR/
enabled=1
gpgcheck=0
EOF
sudo yum -y install redhawk-devel redhawk-codegen bulkioInterfaces
echo ========= This is a global operation and is '*NOT*' sand-boxed for OpenCPI.
echo ========= Partial REDHAWK installation complete for supporting ocpirh_export

