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
[ "$(hostname -s | sha256sum)" == "d9d9b269b1872ae1cf6253637cfa1663dd802ec801ce666a40ad349104932453  -" ] || (echo "This should be run on Jenkins host!" && false)
[ -d /opt/Xilinx ] || (echo "/opt/Xilinx not found!" && false)

if [ ! -e jenkins_private_keys.tar.bz2 ]; then
  echo "$(basename $0): Tarring Jenkins private ssh keys..."
  sudo tar jcf jenkins_private_keys.tar.bz2 -C ~jenkins/.ssh/ id_rsa{,.pub}
  sudo chown root:docker jenkins_private_keys.tar.bz2
  sudo chmod 640 jenkins_private_keys.tar.bz2
fi

IMAGE_VER=4
TODAY=$(date +%Y%m%d)
NAME="jenkins/ocpibuild"
JENKINSUID=$(id -u jenkins)
JENKINSGID=$(id -g jenkins)
CENTOS_VERSIONS=(6.10 7.5.1804) # Update FAQ.tex if these change at all!
YUM_SERVER_URL=YOU_MUST_FILL_IN_YOUR_LOCAL_YUM_MIRROR_WITH_CENTOS_AND_EPEL_HERE
JENKINS_SERVER=YOU_MUST_PUT_YOUR_JENKINS_IP_HERE
GIT_PORT=YOUR_GIT_SERVER_PORT
GIT_SERVER=YOUR_GIT_SERVER
if [ -f ../../config_files/docker_build_config ]; then
  source ../../config_files/docker_build_config
fi


# Backups
for ver in ${CENTOS_VERSIONS[*]}; do
  CENTOS_MAJ=${ver%%.*}
  if docker images | egrep "${NAME} +v${IMAGE_VER}-C${CENTOS_MAJ}-${TODAY}\\b"; then
    echo There is already a backup from today for v${IMAGE_VER}-C${CENTOS_MAJ}. Refusing to run.
    echo To remove: \'docker rmi ${NAME}:v${IMAGE_VER}-C${CENTOS_MAJ}-${TODAY}\'
    exit 99
  fi
done

for ver in ${CENTOS_VERSIONS[*]}; do
  CENTOS_MAJ=${ver%%.*}
  CENTOS_MIN=${ver#*.}
  RAND=$$_${ver}
  PACKAGES=$(../../../projects/core/rcc/platforms/centos${CENTOS_MAJ}/centos${CENTOS_MAJ}-packages.sh yumlist | xargs echo)

  # We also need some Jenkins-specific packages:
  #  lsof - used by socket_write
  #  mlocate - for Xilinx-finding
  #  valgrind - obvious?
  #  oxygen-icon-theme jre - need by IDE
  PACKAGES+=" lsof mlocate valgrind oxygen-icon-theme jre"

  echo "$(basename $0) Package List: ${PACKAGES}"

  # Backup previous image, so we can make and commit a new one to the locally stored repo.
  echo "$(basename $0): Backing up current image... (v${IMAGE_VER}-C${CENTOS_MAJ}-${TODAY})"
  docker tag jenkins/ocpibuild:v${IMAGE_VER}-C${CENTOS_MAJ}{,-${TODAY}} || echo "No previous v${IMAGE_VER}-C${CENTOS_MAJ} existed."

  echo "$(basename $0): Updating CentOS${CENTOS_MAJ} official image..."
  docker pull centos:${CENTOS_MAJ}.${CENTOS_MIN}

  # Create our yum repo file so the installs (called out in the Dockerfile) use the correct repo!
  echo "$(basename $0): Generating approved_repos${CENTOS_MAJ}.repo to use in CentOS${CENTOS_MAJ} image..."
  sed -e "s/@VER@/${CENTOS_MAJ}/g; s|@SERVER@|${YUM_SERVER_URL}|g" < yum.repos.template > approved_repos_${CENTOS_MAJ}.repo

  # Build a temp image with our docker file
  echo "$(basename $0): Building base CentOS${CENTOS_MAJ} image..."
  docker build -t image_${RAND} \
  --build-arg CENTOS_MAJ=${CENTOS_MAJ} \
  --build-arg CENTOS_MIN=${CENTOS_MIN} \
  --build-arg GIT_PORT=${GIT_PORT} \
  --build-arg GIT_SERVER=${GIT_SERVER} \
  --build-arg JENKINSUID=${JENKINSUID} \
  --build-arg JENKINSGID=${JENKINSGID} \
  --build-arg JENKINS_SERVER=${JENKINS_SERVER} \
  --build-arg PACKAGES="${PACKAGES}" \
  --build-arg NAME=${NAME} \
  --build-arg VERSION=v${IMAGE_VER}-C${CENTOS_MAJ} \
  -f Dockerfile.main .

  # Run the temp image, setting up Xilinx mount. We can't do this in the Dockerfile,
  # the mount has to be specified at runtime (with docker run -v ...)
  echo "$(basename $0): Launching container (self-reap in 12 hours)..."
  docker run -d -v /opt/Xilinx/:/opt/Xilinx/:ro --name=container_${RAND} image_${RAND} /bin/sleeper 12h
  echo "$(basename $0): Xilinx toolset locations..."
  docker exec container_${RAND} updatedb

  # Save a new image, now that we've recorded Xilinx locations (and done everything else in the actual Dockerfile).
  # When we actually use this final image, later, the Xilinx stuff will be mounted again in the run command.
  echo "$(basename $0): Taking down container..."
  docker kill container_${RAND}
  echo "$(basename $0): Snapshotting modifications..."
  docker commit container_${RAND} ${NAME}:v${IMAGE_VER}-C${CENTOS_MAJ}

  # Remove temp image, container, Dockerfiles
  echo "$(basename $0): Cleaning intermediate containers and images..."
  docker rm container_${RAND}
  docker rmi image_${RAND}
  rm approved_repos_${CENTOS_MAJ}.repo

  # Do any fixups needed if we find a 'extra' file.
  if [ -f Dockerfile.main.C${CENTOS_MAJ}extra ]; then
    echo "$(basename $0): Rebuilding CentOS${CENTOS_MAJ} image with Dockerfile.main.C${CENTOS_MAJ}extra"
    docker build -t ${NAME}:v${IMAGE_VER}-C${CENTOS_MAJ} \
    --build-arg NAME=${NAME} \
    --build-arg IMAGE_VER=${IMAGE_VER} \
    -f Dockerfile.main.C${CENTOS_MAJ}extra .
  fi
done
