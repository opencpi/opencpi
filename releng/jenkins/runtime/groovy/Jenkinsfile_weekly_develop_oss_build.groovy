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

mattermost_send(this, [msg: "Weekly OSS Build of `develop` started:", attn: "@${global_config.jenkins_admin_username()}"])

// They will NOT be in parallel
['6', '7'].each {
  stage("Building CentOS ${it}") {
    // This will wait for completion
    build job: 'OSS_From_Scratch/develop',
          parameters: [
            string(name: 'docker_image_in', value: "centos:${it}"),
            string(name: 'run_cmd', value: '')
          ],
          quietPeriod: 0
  } // stage
} // each OS

// Don't do anything here; the job itself reports to Mattermost
