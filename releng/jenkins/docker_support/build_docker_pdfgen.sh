#!/bin/bash -e
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

# NOTE: Prior to AV-4085 (60b51ec) there was custom texmf code. If non-RPM LaTeX extensions
# are required, revisit that code!

[ "$(hostname -s | sha256sum)" == "d9d9b269b1872ae1cf6253637cfa1663dd802ec801ce666a40ad349104932453  -" ] || (echo "This should be run on Jenkins host!" && false)
IMAGE_VER=4
CENTOS_MAJ=7
NAME="jenkins/ocpi_pdfgen"
JENKINSUID=$(id -u jenkins)
JENKINSGID=$(id -g jenkins)
GIT_PORT=YOUR_GIT_SERVER_PORT
GIT_SERVER=YOUR_GIT_SERVER
if [ -f ../../config_files/docker_build_config ]; then
  source ../../config_files/docker_build_config
fi

echo "$(basename $0): Tarring Jenkins private ssh keys..."
if [ ! -e jenkins_private_keys.tar.bz2 ]; then
  sudo tar jcf jenkins_private_keys.tar.bz2 -C ~jenkins/.ssh/ id_rsa{,.pub}
  sudo chown root:docker jenkins_private_keys.tar.bz2
  sudo chmod 640 jenkins_private_keys.tar.bz2
fi

RAND=$$_${CENTOS_MAJ}
echo "$(basename $0): Updating CentOS${CENTOS_MAJ} official image..."
docker pull centos:${CENTOS_MAJ}
echo "$(basename $0): Generating Dockerfile config file..."
TEX_PKGS=$(grep "sudo yum install -y" ../../../doc/av/tex/README | cut -f6- -d' ' | xargs echo)
echo "$(basename $0): Building base CentOS${CENTOS_MAJ} image..."
docker build -t image_${RAND} \
--build-arg GIT_PORT="${GIT_PORT}" \
--build-arg GIT_SERVER="${GIT_SERVER}" \
--build-arg TEX_PKGS="${TEX_PKGS}" \
--build-arg JENKINSUID="${JENKINSUID}" \
--build-arg JENKINSGID="${JENKINSGID}" \
--build-arg NAME=${NAME} \
--build-arg VERSION=v${IMAGE_VER} \
-f Dockerfile.pdfgen .
echo "$(basename $0): Launching container (self-reap in 1 hour)..."
docker run -d --name=container_${RAND} image_${RAND} /bin/sleeper 1h
echo "$(basename $0): Taking down container..."
docker kill container_${RAND}
echo "$(basename $0): Snapshotting modifications..."
docker commit container_${RAND} ${NAME}:v${IMAGE_VER}
echo "$(basename $0): Cleaning intermediate containers and images..."
docker rm container_${RAND}
docker rmi image_${RAND}
