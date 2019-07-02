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
Builds jobs to scan git repos and puts them under /Branch_Checks/
Needs to be re-run whenever releng/jenkins/runtime/bash/git_branch_check.sh is modified.

NOTE: This is in "Job DSL" format and NOT "pipeline" / "workflow" like other Jenkins groovy files!
https://jenkinsci.github.io/job-dsl-plugin/
*/

/* NOTE: The "repos" below must be filled in with git URLs to the repos you want to check.
   No examples are included for anonymization, sorry.
*/

def final repos = [
  'ssh://.../',
]

def final spamlist = 'user@domain' // Replace with email address for entire team

folder("/Branch_Checks") { displayName "Merged branch reports" }

def final branch_script = readFileFromWorkspace('releng/jenkins/runtime/bash/git_branch_check.sh')

repos.each { repo ->
  // Reduce to basic name
  def reponame = (repo =~ /.*\/(.*)\.git/)[0][1]
  def jobname = reponame.replace('-','_').replace('.','_dot_')
  println "Processing ${reponame}"
  def my_job = freeStyleJob("/Branch_Checks/${jobname}") {
    description('Checks what remote branches have and have not merged into "develop"')
    logRotator { daysToKeep(90) }
    label('master')
    scm {
      git {
        remote {
          url(repo)
        }
        branches("refs/remotes/origin/develop")
        extensions {
          cleanBeforeCheckout()
          cloneOptions {
            noTags()
            // shallow()
          }
          pruneBranches()
        }
      } // git
    } // scm
    triggers { cron('H H * * 7') }
    steps {
      shell {
        command branch_script
        unstableReturn 2
      }
    }
    publishers {
      extendedEmail {
        recipientList('') // Default to NOBODY
        defaultSubject("Git Branch Report - ${reponame}")
        defaultContent("See attached log")
        attachmentPatterns('branch_report.log')
        contentType('text/plain')
        triggers {
          failure {
            recipientList(spamlist)
            sendTo { recipientList() }
          }
          unstable {
            recipientList(spamlist)
            sendTo { recipientList() }
          }
        }
      } // extendedEmail
    } // publishers
  } // my_job
  // queue(my_job)
} // each repo
