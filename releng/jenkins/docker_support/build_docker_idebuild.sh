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

IMAGE_VER=1
NAME="jenkins/av_idebuild"
TODAY=$(date +%Y%m%d)
SAPPHIRE_URL="http://www.eclipse.org/downloads/download.php?file=/sapphire/9.1/sapphire-repository-9.1.zip&r=1"
# We previously used Webtools 3.8, but they are not available.
WTP_URL_UNAVAILABLE="http://www.eclipse.org/downloads/download.php?file=/webtools/downloads/drops/R3.8.0/R-3.8.0-20160608130753/wtp-repo-R-3.8.0-20160608130753.zip&r=1"
WTP_URL="https://www.eclipse.org/downloads/download.php?file=/webtools/downloads/drops/R3.9.5/R-3.9.5-20180409100740/wtp-repo-R-3.9.5-20180409100740.zip&r=1"
GIT_PORT=YOUR_GIT_SERVER_PORT
GIT_SERVER=YOUR_GIT_SERVER
if [ -f ../../config_files/docker_build_config ]; then
  source ../../config_files/docker_build_config
fi

# Backups
if docker images | egrep "${NAME} +v${IMAGE_VER}-${TODAY}\\b"; then
  echo There is already a backup from today for v${IMAGE_VER}. Refusing to run.
  echo To remove: \'docker rmi ${NAME}:v${IMAGE_VER}-${TODAY}\'
  exit 99
fi

# Backup previous image, so we can make and commit a new one to the locally stored repo.
echo "$(basename $0): Backing up current image... (v${IMAGE_VER}-${TODAY})"
docker tag ${NAME}:v${IMAGE_VER}{,-${TODAY}} || echo "No previous v${IMAGE_VER} existed."

RAND=$$_${ver}x
echo "$(basename $0): Updating Fedora official image..."
docker pull fedora
echo "$(basename $0): Building base Fedora image..."
docker build -t image_${RAND} \
--build-arg GIT_PORT="${GIT_PORT}" \
--build-arg GIT_SERVER="${GIT_SERVER}" \
--build-arg SAPPHIRE_URL="${SAPPHIRE_URL}" \
--build-arg WTP_URL="${WTP_URL}" \
--build-arg NAME=${NAME} \
--build-arg VERSION=v${IMAGE_VER} \
-f ./Dockerfile.idebuild .
echo "$(basename $0): Launching container (self-reap in 6 hours)..."
docker run -d --name=container_${RAND} image_${RAND} /bin/sleeper 6h
echo "$(basename $0): Taking down container..."
docker kill container_${RAND}
echo "$(basename $0): Snapshotting modifications..."
docker commit container_${RAND} ${NAME}:v${IMAGE_VER}
echo "$(basename $0): Cleaning intermediate containers and images..."
docker rm container_${RAND}
docker rmi image_${RAND}
