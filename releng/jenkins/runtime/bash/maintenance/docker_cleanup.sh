#!/bin/bash
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

set +x
set +e

echo All containers:
docker ps -a -fname=jenkins --format "table {{.Names}}\t({{.Image}})\t{{.Status}}" | grep jenkins | sort

disk_space_print() {
  # sudo lvs vg_ex/docker-pool --no-headings -o data_percent
  # New with overlay2 and no more thin pool stuff:
  df --output=pcent /var/lib/docker | tail -1
}

docker_rm() {
  cont_name=$(docker ps -a -f id=$1 --format "{{.Names}}")
  echo "--- Removing '${cont_name}' ---"
  echo -n "Before: "
  disk_space_print
  # docker ps -a -f id=$1 --format "{{.Names}} ({{.Image}})"
  docker rm $1
  echo -n "After: "
  disk_space_print
  echo "--- Done removing '${cont_name}' ---"
}

docker_rmi() {
  echo "--- Removing image '$1' ---"
  if [[ ! "$1" =~ angryviper ]]; then
    # Echo given direct hash; tell clean name
    docker images | grep $1
  fi
  echo -n "Before: "
  disk_space_print
  timeout -k10s 30s docker rmi $1 || echo "Failed"
  echo -n "After: "
  disk_space_print
}

percentage_compare() {
  # $1 is a number with '%' suffix
  # $2 is the number to compare to
  test "$(echo $1 | cut -f1 -d%)" -gt $2
}

# Pre-check - if too high, we're going to nuke EVERYTHING
DISK_USAGE=$(df --output=pcent /var/lib/docker | tail -1)
if test "$(echo $DISK_USAGE | cut -f1 -d%)" -gt 95; then
  echo "EMERGENCY LEVELS ($DISK_USAGE): Shutting down ALL containers to reap piggies."
  export BAD=1
  docker kill "$(docker ps -q)"
fi
echo "Start: Disk level ${DISK_USAGE}"

echo "Erase 'dead' containers:"
for cont in $(docker ps -qa -f status=dead -f name=jenkins); do
  docker_rm ${cont}
done

echo "Erase older containers:"
# for cont in $(docker ps -a -f status=exited -f name=jenkins | grep -P "Exited .* [5-9] days ago" | awk '{print $1}'); do
for cont in $(docker ps -a -f status=exited -f name=jenkins | grep "days ago" | awk '{print $1}'); do
  docker_rm ${cont}
done

echo "Erase piggies and junk:"
for pig in cic_ zero_ ocpidev training_project test_junk; do
 for cont in $(docker ps -a -f status=exited -f name=jenkins | grep ${pig} | awk '{print $1}'); do
   docker_rm ${cont}
 done
done

echo "Erase stuck containers:"
for cont in $(docker ps -a -f name=jenkins | grep "Removal" | awk '{print $1}'); do
  docker_rm ${cont}
done

echo "Erase old prereq images:"
declare -A imgs
# imgs[develop-7]=99
for img in $(docker images angryviper/rpmbuild --format "{{.Tag}}" | sort -rn); do
  img_os=$(docker inspect angryviper/rpmbuild:${img} | grep jenkins/ocpibuild | perl -ne '/-C(\d)/ && print $1')
  img_branch=$(docker inspect angryviper/rpmbuild:${img} | perl -ne '/GIT_BRANCH_NAME=(.*?)"/ && print $1')
  echo "Found Image: ${img}: OS=${img_os} Branch=${img_branch}"
  if [ -z "${imgs[${img_branch}-${img_os}]}" ]; then
    eval "imgs[${img_branch}-${img_os}]=${img}"
  else
    echo "That OS/Branch combo was already found in ${imgs[${img_branch}-${img_os}]}"
    docker_rmi angryviper/rpmbuild:${img}
  fi
done
unset imgs

# A lot of copy/paste...
# These are versioned as "XXX-CX"
# Should probably only erase more than X days: https://stackoverflow.com/a/33855110
# docker inspect -f '{{.Id}} {{.Created }}' $(docker images angryviper/rpminstalled -q)
echo "Erase old installed RPM images:"
declare -A imgs
for img in $(docker images angryviper/rpminstalled --format "{{.Tag}}" | sort -rn); do
  # img_os=$(docker inspect angryviper/rpminstalled:${img} | grep jenkins/ocpibuild | perl -ne '/-C(\d)/ && print $1')
  img_os=$(echo ${img} | perl -ne '/-C(\d)/ && print $1')
  img_branch=$(docker inspect angryviper/rpminstalled:${img} | perl -ne '/GIT_BRANCH_NAME=(.*?)"/ && print $1')
  echo "Found Image: ${img}: OS=${img_os} Branch=${img_branch}"
  if [ -z "${imgs[${img_branch}-${img_os}]}" ]; then
    eval "imgs[${img_branch}-${img_os}]=${img}"
  else
    echo "That OS/Branch combo was already found in ${imgs[${img_branch}-${img_os}]}"
    docker_rmi angryviper/rpminstalled:${img}
  fi
done
unset imgs

# Now the same with the Jenkins 2 stuff
# Images will have format of angryviper/BRANCHNAMEx or angryviper/BRANCHNAME-prereq :(bldid)-C{6,7}
# (I don't think the prereq ones exist any more)
echo "Erase old install RPM images (Jenkins 2):"
for branch in $(docker images 'angryviper/*' --format "{{.Repository}}" | sed -e 's/-prereq//g' | sort -u | cut -f2 -d/); do
  # Might have an extra 'x' at the end
  branch=${branch%x}
  echo "Checking branch: ${branch}"
  # Check if branch exists now or not...
  rmi_limit="2" # Delete all but the last two images by default
  # if ! git ls-remote --heads --exit-code /data/jenkins_2tb/jenkins_workspaces/git_sync/opencpi/ ${branch}; then
  if ! git ls-remote --heads /data/jenkins_2tb/jenkins_workspaces/git_sync/opencpi/ | grep -iq "refs/heads/${branch}"; then
    echo "Branch ${branch} seems to be obsolete. Deleting all images."
    rmi_limit="0" # Delete all
  fi
  for os in 6 7; do
    for tag in $(docker images angryviper/${branch}-prereq --format "{{.Tag}}" | sort -n | grep C${os} | head -n -${rmi_limit}); do
      # echo "Removing prereq: angryviper/${branch}-prereq:${tag}"
      docker_rmi angryviper/${branch}-prereq:${tag}
    done
    for tag in $(docker images angryviper/${branch} --format "{{.Tag}}" | sort -n | grep C${os} | head -n -${rmi_limit}); do
      # echo "Removing non-prereq: angryviper/${branch}:${tag}"
      docker_rmi angryviper/${branch}:${tag}
    done
  done # os
  echo "The following images remain for the branch ${branch}:"
  docker images angryviper/${branch}* --format "{{.Repository}}:{{.Tag}}*{{.CreatedSince}}*({{.Size}})" | column -t -s '*' | sort | egrep "\b${branch}\b"
done # branch

echo "Erasing dangling images:"
DANG=$(docker images --filter "dangling=true" -q)
if [ -n "${DANG}" ]; then
  docker images --filter "dangling=true"
  for img in $(docker images --filter "dangling=true" -q); do
    docker_rmi ${img}
  done
fi

echo "Erasing old images:"
# Now check for super old images
for img in $(docker images 'angryviper/*' | grep 'months ago' | column -t -o'*' | cut -f3 -d'*'); do
  docker_rmi ${img}
done

echo "Checking disk space:"
# sudo lvs vg_ex/docker-pool
df -hP /var/lib/docker
DISK_USAGE_DOCKER=$(df --output=pcent /var/lib/docker | tail -1)
DISK_USAGE_JENKINS_LTS=$(df --output=pcent /data/jenkins_2tb/ | tail -1)
DISK_USAGE_JENKINS_WORKSPACES=$(df --output=pcent /data/jenkins_12tb/ | tail -1)
echo "Disk Usage: Docker =${DISK_USAGE_DOCKER} Jenkins =${DISK_USAGE_JENKINS_LTS} Scratch =${DISK_USAGE_JENKINS_WORKSPACES}" | tee final_usage.log

# Fail if...
if [ -n "${BAD}" ]; then
  exit 99
fi

if percentage_compare ${DISK_USAGE_DOCKER} 90 || \
   percentage_compare ${DISK_USAGE_JENKINS_LTS} 90 || \
   percentage_compare ${DISK_USAGE_JENKINS_WORKSPACES} 90; then
  exit 99
fi

# Unstable if...
if percentage_compare ${DISK_USAGE_DOCKER} 85 || \
   percentage_compare ${DISK_USAGE_JENKINS_LTS} 85 || \
   percentage_compare ${DISK_USAGE_JENKINS_WORKSPACES} 85; then
  exit 1
fi
