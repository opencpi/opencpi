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

@Library('opencpi-libs')_
import groovy.transform.Field

// Note: This does NOT detect if a base image, e.g. centos:7 has been updated, so the
// image with "sudo" installed probably needs to be manually deleted!

// TODO: Fix above as well as any locks on docker image creation / pulling

// First is default
@Field final Docker_Source_Images = [ "centos:6", "centos:6.6", "centos:6.7", "centos:6.8", "centos:6.9", "centos:6.10", "centos:7", "centos:7.0.1406", "centos:7.1.1503", "centos:7.3.1611", "centos:7.4.1708", "centos:7.5.1804", "centos:7.6.1810", "jenkins/ocpibuild:v4-C6", "jenkins/ocpibuild:v4-C7" ]

@Field final results_emails = []

@Field final mount_tools = '-v /opt/Xilinx/:/opt/Xilinx/:ro -v /opt/Altera/:/opt/Altera/:ro -v /opt/Modelsim:/opt/Modelsim:ro'
@Field final mount_sudoers = '-v /etc/passwd:/etc/passwd:ro -v /etc/shadow:/etc/shadow:ro -v /etc/sudoers:/etc/sudoers:ro -v /var/lib/jenkins:/var/lib/jenkins:ro'

properties properties: [ // This is ugly https://stackoverflow.com/a/35471196
  buildDiscarder(
    logRotator(artifactDaysToKeepStr: '',
      artifactNumToKeepStr: '3',
      daysToKeepStr: '',
      numToKeepStr: '25')),
  disableConcurrentBuilds(),
  parameters([
    choice(choices: Docker_Source_Images.join("\n"), description: 'OS image to start with', name: 'docker_image_in'),
    string(defaultValue: '', description: 'Parameter to pass to install-opencpi.sh', name: 'run_cmd'),
  ]),
]

import groovy.json.JsonOutput

@Field stage_name
currentBuild.description = params.docker_image_in+" "
if (run_cmd) currentBuild.description += "(${run_cmd})"

// mattermost_send(this, [msg: "OSS from-scratch build of `${env.BRANCH_NAME}` (${params.docker_image_in}) started"])

try { // The whole job is within this block (see "end of job")
  timeout(time: 3, unit: 'HOURS') {
    def sudo_image = params.docker_image_in+"_sudo"
    node('docker-general') {
      stage_name = 'Setup'
      stage(stage_name){
        error_email = "${BUILD_URL}\n"
        // AV-4110: deleteDir()
        checkout scm // does some cleanup
        sh """git clean -ffdx -e ".jenkins-*/" || :""" // Cleanup (see JENKINS-31924 for formula)
        echo "Attempting to spin up container based on image requested ${params.docker_image_in}"
        def dimage = docker.image(params.docker_image_in)
        dimage.inside { sh "pwd" }
        // Now that we know that exists, try one with the "sudo" in it:
        if (params.docker_image_in.contains('ocpibuild')) {
          sudo_image = params.docker_image_in
        } else {
          try {
            docker.image(sudo_image).inside { sh "pwd" }
          } catch (err) {
            echo "Did not have sudo-enabled version of ${params.docker_image_in} - building now"
            def built = false
            // TODO: Fedora, Ubuntu, etc
            if (params.docker_image_in.contains('centos')) {
              built = true
              dir (env.WORKSPACE+"/releng/jenkins/docker_support") {
                docker.build(sudo_image, "--build-arg UPSTREAM=${params.docker_image_in} -f Dockerfile_OSS_From_Scratch_centos --pull .")
              }
              echo "Built. Testing again."
              docker.image(sudo_image).inside { sh "pwd" }
            }
            if (!built)
              error "I do not know how to add 'sudo' to ${params.docker_image_in}"
          }
        } // our images
      } // end of stage

      stage_name = 'Run'
      stage(stage_name){
        def dimage = docker.image(sudo_image)
        dimage.inside("${mount_sudoers} ${mount_tools} --name ${BUILD_TAG}") {
          sh """#!/bin/bash -e
          export OCPI_XILINX_LICENSE_FILE=${global_config.license_server()}
          export OCPI_ALTERA_LICENSE_FILE=${global_config.license_server()}
          export OCPI_MODELSIM_LICENSE_FILE=${global_config.license_server()}
          ./scripts/install-opencpi.sh ${run_cmd}
          """.trim()
        } // Inside container
      } // end of stage
    } // node
  } // timeout
  mattermost_send(this, [msg: "OSS from-scratch build of `${env.BRANCH_NAME}` (${params.docker_image_in}) succeeded"])
} catch (err) {
  // An error has happened (NOTE: doesn't seem to catch all errors, e.g. java.lang.NoSuchMethodError?)
  mattermost_send(this, [icon: "https://jenkins.io/images/logos/fire/fire.png"])
  echo "Preparing email notifying failure in ${stage_name} stage"
  error_email = err.toString() + "\n\n${error_email}"
  def recip = results_emails.join(',')
  emailext body: error_email, subject: "${JOB_NAME}: ${stage_name} is failing", to: recip
  currentBuild.description += "\nFailed in ${stage_name} stage"
  currentBuild.result = 'FAILURE'
  error "Error in ${stage_name} stage\n\tError: ${err.toString()}"
} /* finally {
  // An error may or may not have happened
  // If an error happens SUPER early, previous_changesets may not be set yet:
  if (!previous_changesets) previous_changesets = [ buildNumber: 0 ]
  echo "Determining if moved to success: Current: ${currentBuild.currentResult}, Previous success: ${previous_changesets.buildNumber} vs. ${(env.BUILD_NUMBER as int)-1}"
  if ('STABLE' == currentBuild.currentResult) { // haven't failed yet
    if (((env.BUILD_NUMBER as int)-1) != previous_changesets.buildNumber) { // last success is not the one before
      echo "Preparing email notifying happiness"
      def recip = results_emails.join(',')
      emailext body: error_email, subject: "${JOB_NAME}/${BRANCH_NAME}) is back to normal", to: recip
    }
  } // not failed
} // finally
*/
