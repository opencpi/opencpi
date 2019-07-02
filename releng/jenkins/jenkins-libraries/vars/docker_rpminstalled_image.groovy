#!groovy
/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
  This snippet will find or create a docker image using the RPMs specified.
  Usage:
    @Library('opencpi-libs')_
    dimage = docker_rpminstalled_image(this, path_to_RPMs)
    dimage = docker_rpminstalled_image(this, 'centos6/')

  Another method findimage(branch, os, build) will return a usable image name or blank.
    The name returned will still need to use docker.image() to use it.

  Notes:
   * Image naming: angryviper/<BRANCHNAME>x:<ID>-C<OS>
    * The "x" at the end is in case the branch ends with "-"
   * Calls return an IMAGE not a CONTAINER.
   * These functions use the metadata file (.jenkins_metadata.json)
   * See also: https://jenkins.io/doc/pipeline/examples/#load-from-file
*/

import groovy.transform.Field
@Field final docker_image_base = "jenkins/ocpibuild:v4-C"

def Bootstrap_example = '''
@Library('opencpi-libs')_
copyArtifacts(
  ...
  target: "rpms/",
)
dimage = docker_rpminstalled_image("rpms/centos7")
dimage.inside() {
  sh "Do stuff"
}
'''

def call(caller_in, path_in) {
  def req_path = path_in // In case we're called by a closure
  def caller = caller_in
  print "docker_rpminstalled_image: Asked for build from '${req_path}'"
  // First, parse the metadata
  def rpm_config = readJSON file: "${req_path}/.jenkins_metadata.json"
  print "docker_rpminstalled_image: These RPMs are from build ${rpm_config.original_build}"
  // Check that the branch is sane (caller.env.BRANCH_NAME could be null in some cases)
  if (caller.env.BRANCH_NAME && (rpm_config.branch_name && rpm_config.branch_name != caller.env.BRANCH_NAME))
    error "Build process from branch '${caller.env.BRANCH_NAME}' attempted to use RPMs from '${rpm_config.branch_name}'"
  def image_name = "angryviper/${rpm_config.branch_name.toLowerCase()}x:${rpm_config.original_build}-C${rpm_config.os}"
  // This lock is poorly named across branches, but acceptable
  lock("docker_rpminstalled_image_${rpm_config.os}_${rpm_config.original_build}") {
    // JENKINS-45152
    def image_id = sh (script: "docker images -q ${image_name}", returnStdout: true).trim()
    if (!image_id.isEmpty()) {
      print "docker_rpminstalled_image: Found existing image for '${image_name}': ${image_id}"
      return docker.image(image_name)
    }
    print "docker_rpminstalled_image: No image currently exists for '${image_name}'; building from '${req_path}'"
    // The container name should probably have branch name, but there is length limit
    def build_cont = "building_image_${rpm_config.os}_${rpm_config.original_build}"
    dir (req_path) {
      def dimage = docker.image("${docker_image_base}${rpm_config.os}")
      // run does not map workspace, etc. like inside does
      def this_dir = sh (script: "readlink -f .", returnStdout: true).trim()
      def cont = dimage.run("""-v ${this_dir}:/build --name ${build_cont}""")
      try {
        // Install the RPMs
        sh """docker exec ${build_cont} /bin/bash -c "yum --disablerepo='*' localinstall -y /build/*rpm" """
        // Add Jenkins user to the OpenCPI group
        sh "docker exec ${build_cont} usermod -aG opencpi jenkins"
        // Create a workspace directory
        sh """docker exec ${build_cont} /bin/bash -c "mkdir /workspace; chown -R jenkins:opencpi /workspace" """
        // Save off current container
        sh "docker commit ${build_cont} ${image_name}"
      } catch (err) {
        error "docker_rpminstalled_image: In exception handler (was building image ${build_cont})\n\tError: ${err}"
      } finally {
        cont.stop()
      }
    } // dir
    return docker.image(image_name)
  } // lock
} // call

String findimage(branch, os, build) {
    print "docker_rpminstalled_image: Asked for #${build} from '${branch.toLowerCase()}' on CentOS${os}."
    def installed_image = sh (script: "docker images -q --filter 'reference=angryviper/${branch.toLowerCase()}:${build}-C${os}'", returnStdout: true).trim()
    if (installed_image) {
      print "docker_rpminstalled_image: Found ${installed_image}."
      return installed_image
    }
    print "docker_rpminstalled_image: Did not find find requested image."
    return ''
}

return this
