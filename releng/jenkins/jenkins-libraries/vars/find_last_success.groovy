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


// This will return the build number of the last successful build and the changesets between then and now
// The returned array of maps is slightly complicated; try "JsonOutput.toJson()" on it.
// See also https://support.cloudbees.com/hc/en-us/articles/217630098-How-to-access-Changelogs-in-a-Pipeline-Job-

def usage_example = """
@Library('opencpi-libs')_
def build_data = find_last_success()
"""
import groovy.transform.Field
@Field changesets   // stores array of maps of changeset information
@Field successBuild // keeps track of last Jenkins build success

void lastSuccessfulBuild(failedBuilds, abortedBuilds, build) { // https://devops.stackexchange.com/a/2319
  // echo "Debug: lastSuccessfulBuild(X,Y,${build.number}): in"
  if ((build != null) && (build.result != 'SUCCESS')) {
      // echo "find_last_success: Build ${build.number} was not SUCCESS."
      failedBuilds.add(build)
      if ('ABORTED' == build.result)
        abortedBuilds.add(build.number)
      // echo "Debug: lastSuccessfulBuild(X,Y,${build.number}): not a success; changeset size = ${build.changeSets.size()}"
      build.changeSets.each { entries ->
        entries.each { entry ->
          // echo "find_last_success: Entry: ${entry.commitId} by ${entry.author} on ${new Date(entry.timestamp)}: ${entry.msg}"
          def file_info = [] // Collect all files
          def files = new ArrayList(entry.affectedFiles)
          files.each { file ->
            file_info.push([action: file.editType.name, file: file.getPath()])
          }
          def path_info = [] // Collect all paths
          def paths = new ArrayList(entry.affectedPaths)
          paths.each {
            path_info.push(it)
          }
          changesets.push([
            build: build.number, // Can be 1:N relationship
            commit: entry.commitId,
            author: entry.author.getFullName(), // want string not User class
            message: entry.msg, // Unfortunately, only first line
            date: new Date(entry.timestamp),
            files: file_info,
            paths: path_info,
          ])
        } // entry in a changeset
      } // changeset in a build
      lastSuccessfulBuild(failedBuilds, abortedBuilds, build.getPreviousBuild())
  }
  if ((build != null) && (build.result == 'SUCCESS'))
    successBuild = build
}

def call() {
  changesets = []
  failedBuilds = []
  abortedBuilds = [] as Set
  successBuild = null
  lastSuccessfulBuild(failedBuilds, abortedBuilds, currentBuild);

  if (successBuild)
    echo "find_last_success: Returning last success was #${successBuild.number} (${changesets.size()} changesets)"
  else
    echo "find_last_success: Could not find successful build."
  return [ buildNumber: successBuild?successBuild.number:0,
           changeSets: changesets,
           failedBuilds: failedBuilds,
           abortedBuilds: abortedBuilds,
         ]
}

return this
