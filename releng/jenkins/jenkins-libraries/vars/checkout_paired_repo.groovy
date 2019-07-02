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


// Used to try to check out another repo with the same branch name as current, with a fallback
// Will DELETE and check out into CURRENT directory, so wrap with dir() call as needed
// Writes out a file ".githash" with the current hash
import groovy.transform.Field
@Field final default_jenkins_credentials = 'REDACTED' // This comes from snippet generator

void call(caller_in, url_in, config_in=[:]) {
  def final usage_example = """
  checkout_paired_repo(this, 'ssh://server/opencpi.git')
  // or
  checkout_paired_repo this, 'ssh://server/opencpi.git', ["fallback": "develop"]

  Configuration options (map in second parameter):
    changelog: If changes should be included in Jenkins changelog (default is false)
    fallback: If defined, which branch to try a second time (default is fail)
    credentials: Alternate credentials. Use the Jenkins snippet generator.
  """
  def url = url_in
  def config = config_in
  def caller = caller_in
  // Set defaults
  config.changelog = config.changelog?:false
  config.fallback = config.fallback?:'' // Kinda pointless...
  config.credentials = config.credentials?:default_jenkins_credentials
  def my_scm = [$class: 'GitSCM', branches: [[name: "*/${caller.env.BRANCH_NAME}"]], doGenerateSubmoduleConfigurations: false, extensions: [], submoduleCfg: [], userRemoteConfigs: [[credentialsId: config.credentials, url:url]]]
  def checked_out = false
  def scmVars

  deleteDir()
  cached_checkout caller, my_scm, false
  if (config.fallback) {
    try {
      scmVars = checkout changelog: config.changelog, poll: false, scm: my_scm
      checked_out = true
    } catch (err) {
      echo """checkout_paired_repo: [WARNING] Could not checkout "${caller.env.BRANCH_NAME}" branch of ${url} repo: '${err}'"""
      echo """checkout_paired_repo: Falling back to "${config.fallback}" """
    }
  } else {
    config.fallback = caller.env.BRANCH_NAME
  }
  if (!checked_out) {
    my_scm["branches"] = [[name: "*/${config.fallback}"]]
    scmVars = checkout changelog: config.changelog, poll: false, scm: my_scm
  }
  writeFile file: '.githash', text: scmVars.GIT_COMMIT // Store current hash
}

return this
