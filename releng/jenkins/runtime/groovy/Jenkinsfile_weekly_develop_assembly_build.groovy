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
@Field build_res
@Field last_build_num

mattermost_send(this, [msg: "Weekly Assembly Build of `develop` started:", attn: "@${global_config.jenkins_admin_username()}"])

stage("Scanning") {
  def final http_response = httpRequest (
    url:"${global_config.our_jenkins()}/job/Job_1/job/develop/lastSuccessfulBuild/api/json",
    ignoreSslErrors: true
  )
  def last_build_data = readJSON(text: http_response.content)
  def final last_build_date = new Date(last_build_data.timestamp)
  last_build_num = last_build_data.id
  def final last_week = new Date(System.currentTimeMillis()-7*24*60*60*1000)
  echo "Last successful build: ${last_build_date}"
  if (! last_build_date.after(last_week)) {
    error "The last successful build was over a week ago!"
  }
}

stage("Building") {
  echo "Launching Job 2 with results from Job 1 #${last_build_num}"
  currentBuild.description = "Job 1 #${last_build_num}"
  // This will wait for completion (but not fail if they do - propagate=false)
  build_res = build job: 'Job_2/develop',
        parameters: [
          string(name: 'Upstream_Job', value: last_build_num),
          booleanParam(name: 'Build Asm', value: true),
          string(name: 'Test Platforms', value: 'Simulators'),
          string(name: 'Target Platforms', value: ''),
          string(name: 'RPM Release', value: ''),
          booleanParam(name: 'Force Rebuild', value: true)
        ],
        propagate: false,
        quietPeriod: 0
}

stage("Parsing Results") {
  if (build_res.resultIsWorseOrEqualTo('UNSTABLE'))
    mattermost_send(this, [msg: "Weekly Assembly Build of `develop` FAILED:", attn: "@${global_config.jenkins_admin_username()}", icon: 'https://jenkins.io/images/logos/fire/fire.png'])
}
