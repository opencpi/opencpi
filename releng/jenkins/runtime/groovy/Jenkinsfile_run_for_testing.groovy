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

// TODO: Anything not a simulator ;)

// Note: This job does NOT email the user pass/fail - people could be BOMBARDED.
// Should we support CentOS 6 in the future?
@Library('opencpi-libs')_
import groovy.json.JsonOutput
import groovy.transform.Field

// DO NOT use "properties" here - JENKINS-43758 and JENKINS-51038

def final DEBUG=true

@Field cpulimit = 'cpulimit -il 100' // Limits to a single CPU
@Field final driver_pfs = ['alst4', 'ml605'] // List of hosted PCI platforms (need driver locked and loaded)
@Field final log_files = ['*.log', '*.out', '*.rpt']
@Field final sim_files = ['*.wdb'] // Obnoxiously large simulation files to compress when exiting
// Note: Modelsim has huge waveform files *.wlf, but they don't compress.
// The valgrind_conf has double %% because it's sent to command-line printf
@Field final valgrind_conf = '--leak-check=full --show-possibly-lost=yes --show-reachable=yes --xml=yes --xml-file=@WORKSPACE@/valgrind-reports/%%p-valgrind.xml'

try {
  echo "Job parameters: " + JsonOutput.prettyPrint(JsonOutput.toJson(params))
} catch (err) {
  echo "Something did not work printing parameters: '${err}'"
}

echo "Incoming env:" + JsonOutput.prettyPrint(JsonOutput.toJson(env))
// First thing we do is parse the job name for various variables.
// "JOB_NAME": "Test_Results/AV_2985_Jenkins_2/xsim/run_comms_comps__mfsk_mapper",
@Field git_branch
@Field platform
@Field upstream_job
@Field upstream_job_config
@Field component_to_test = ''

// Set a nice description with links so we can see this run's source in the Jenkins UI "Build History".
final descriptionDefault = "(upstream not found)"
currentBuild.description = descriptionDefault
currentBuild.upstreamBuilds.reverse().each {

  isSplit     = it.getFullProjectName().split("/").size() > 0 // Job_1/some_branch/...
  projectName = isSplit ? it.getFullProjectName().split("/")[0].replace('_',' ') : 'error' // Job 1, Test Results, aProject
  buildNumber = it.getDisplayName()     // #29
  name        = it.getProjectName()     // build_core__all
  aUrl        = it.getAbsoluteUrl()

  // Don't display the "build" job that really called us (e.g. build_core__all).
  // It's already available in "Build History" UI, and not useful.
  if (!(name =~ /^build_/ )) {
    // Will look like this:  From → Job 1 #68 → Job 2 #72
    def jobstr = "${projectName} <a href='${aUrl}'>${buildNumber}</a>"
    if (currentBuild.description == descriptionDefault)
      currentBuild.description = "From ${jobstr}"
    else
      currentBuild.description += " → ${jobstr}"
  } // not "build_"
}

// These cannot be combined with above b/c "env" is not present at field initialization
git_branch = env.JOB_NAME.split('/')[1]
platform = env.JOB_NAME.split('/')[2]
upstream_job = env.JOB_NAME.replace('/run_', '/build_')
if (platform =~ /sim$/) {
  // Need to replace our name (everything after last __) with "all" for AV-4529,
  // because simulators built in a single large "all" job.
  // e.g. Test_Results/feature__AV_4529_serial_simulators/xsim/run_assets__comms_comps__mfsk_mapper =>
  //      Test_Results/feature__AV_4529_serial_simulators/xsim/build_assets__all
  def final matcher = env.JOB_NAME =~ /(.*)\/run_(.*?)__/
  // 1 holds path leading up to us
  // 2 should be just the project
  if (2 != matcher.groupCount()) error "Strange Regex/Glob Mismatch Error"
  upstream_job = matcher[0][1]+'/build_'+matcher[0][2]+'__all'
}

// Decide type of host
def node_label = 'hdl-' + platform

if (DEBUG) echo "Determined git_branch: ${git_branch}, platform: ${platform}, upstream: ${upstream_job}, node_label: ${node_label}"
// If we support C6, would need to add labels here based on OS if simulators, as well as RPM copies

if (DEBUG) echo "Waiting for a node matching '${node_label}'"
timestamps {
  node(node_label) {
    def final ocpi_cdk_dir_var = "OCPI_CDK_DIR=${env.WORKSPACE}/workspace/opt/opencpi/cdk/"
    stage ("Setup") {
      // There could be root-owner driver installs and python caches(!) left around
      sh "sudo chown -R jenkins:jenkins workspace/opt/opencpi/ || :"
      deleteDir()
      // TODO: The next block is in 2-3 files now... library time!!!
      // Do not start until launcher job is actually complete. Otherwise it might still be archiving the files we need.
      try { // We may have been launched manually, so just let this blow up.
        wait_for_job(currentBuild.upstreamBuilds[0])
      } catch (err) {
        if (DEBUG) echo "Ignoring error when trying to wait for launcher: ${err}"
      }
      // Bring in metadata from builder
      if (DEBUG) echo "Bringing in metadata from builder"
      copyArtifacts(
        projectName: upstream_job,
        filter: ".test_metadata.json"
      )
      upstream_job_config = readJSON file: ".test_metadata.json"
      if (DEBUG) echo "upstream_job_config:" + JsonOutput.prettyPrint(JsonOutput.toJson(upstream_job_config))
      component_to_test = upstream_job_config.launcher_config.COMPONENT_TO_TEST
      if (upstream_job =~ /__all$/) {
        // Need to set the component_to_test to match our job name, because we were launched by
        // the "all" builder, which only sets upstream_job_config.launcher_config.COMPONENT_TO_TEST
        // to the first component in the project.
        def final matcher = env.JOB_NAME =~ /.*\/run_(.*?)__(.*)/
        // 1 should be project
        // 2 is component to test, which we look up in tests_list to get a path
        component_to_test = matcher[0][1]+"/"+upstream_job_config.launcher_config.tests_list[matcher[0][2]]
      }
      // Bring in workspaces
      def files_to_copy = ['releng/jenkins_runtime.tar']
      upstream_job_config.projs.each { files_to_copy.push("**/workspace_${it}_${platform}.*") }
      if (DEBUG) echo "Copying workspaces from ${upstream_job_config.upstream_job} #${upstream_job_config.upstream_job_build}"
      // Pull the extracted version of the project
      copyArtifacts(
        projectName: upstream_job_config.upstream_job,
        filter: files_to_copy.join(", "),
        target: "workspace/",
        flatten: true,
        selector: specific("${upstream_job_config.upstream_job_build}")
      )
      files_to_copy = 'support/ocpi_rpms_extracted_C7.tar'
      if (DEBUG) echo "Copying extracted RPMs from Job_1/${upstream_job_config.upstream_git_branch} #${upstream_job_config.job2_config.params.Upstream_Job}"
      copyArtifacts(
        projectName: "Job_1/${upstream_job_config.upstream_git_branch}",
        filter: files_to_copy,
        target: "rpms/",
        flatten: false,
        selector: specific(upstream_job_config.job2_config.params.Upstream_Job)
      )
      dir ('releng') {
        sh "tar xf ../workspace/jenkins_runtime.tar" // Want it to be in usual place under releng
      }
      if (NODE_NAME != "master") cpulimit = '' // Don't limit CPU usage
      cpulimit = '' // Disable CPU limiting with locks (experimental)
      // Extract our workspaces
      dir ('built_area') {
        upstream_job_config.projs.each {
          echo "Extracting tarball for ${it}"
          lock("${NODE_NAME}-workspace-${it}") {
            sh "tar xf ../workspace/workspace_${it}_*"
            sh "rm -vrf ../workspace/workspace_${it}_*"
          }
        }
        // Bring in built test artifacts
        if (DEBUG) echo "Copying built test artifacts from ${upstream_job}"
        copyArtifacts(
          projectName: upstream_job,
          filter: "*.tar"
        )
        // And now get the built stuff into the right place
        sh "${cpulimit} tar xf ${component_to_test.split('/')[-1]}.tar"
        // Clean up
        // sh "rm ${component_to_test.split('/')[-1]}.tar"
        sh "rm -vrf *.test.tar"
        log_files.each { sh "find . -name '${it}' -delete" }
      } // built_area
      dir ('workspace') {
        echo "Extracting framework from RPMs"
        lock("${NODE_NAME}-ocpi_rpms_extracted") {
          sh "tar xf ../rpms/support/ocpi_rpms_extracted_C*.tar"
          sh "rm -vrf ../rpms"
        }
        // This needs to be double quoted to fill in the blanks.
        // This is copied from Job 1 and build_for_testing and then spread out here
        def OS=7
        sh """
          set +x
          echo "source ${env.WORKSPACE}/workspace/opt/opencpi/cdk/opencpi-setup.sh -v -" >> jenv.sh
          shopt -s nullglob
          for f in ${env.WORKSPACE}/workspace/opt/opencpi/cdk/env.d/*.sh; do
            echo "Importing \$f into jenv.sh"
            cat \$f >> jenv.sh
          done
        """
        // These are for our bash script
        sh """
          echo 'export SKIP_BUILD=1' >> jenv.sh
          echo 'ulimit -n 2048 || :' >> jenv.sh
        """
        if (upstream_job_config.use_valgrind) {
          sh """printf "export VG='valgrind ${valgrind_conf.replace('@WORKSPACE@',env.WORKSPACE)}'\n" >> jenv.sh"""
          sh "mkdir ${env.WORKSPACE}/valgrind-reports/"
        }
        // Separated from above because this is from Job 1 copies...
        sh """set +x
          printf "
            export OCPI_XILINX_LICENSE_FILE=${global_config.license_server()}
            export OCPI_ALTERA_LICENSE_FILE=${global_config.license_server()}
            export OCPI_MODELSIM_LICENSE_FILE=${global_config.license_server()}
            set -o pipefail
            echo jenv.sh imported
            set -x
          " >> jenv.sh
        """
        // AV-5359 - possibly override OCPI_FORCE_HDL_FILE_IO
        if (DEBUG) echo "Processing 'Test File IO' parameter: '${params["Test File IO"]}'"
        switch (params["Test File IO"]) {
          case "Force HDL File IO":
            sh "echo 'export OCPI_FORCE_HDL_FILE_IO=1' >> jenv.sh"
            break
          case "Force RCC File IO":
            // Time is also adjusted below (if keywords ever change)
            sh "echo 'export OCPI_FORCE_HDL_FILE_IO=0' >> jenv.sh"
            break
        }
      } // workspace
    } // setup stage
    def final driver_lock = (platform in driver_pfs)?"driver-${NODE_NAME}":''
    lock(driver_lock) {
      try {
        stage ("Run Test") {
          def timeout_time = 5
          // Sadly, simulators sometimes take this long... on an unloaded Jenkins run:
          // 02:35:08.313   Executing case case00.11 using worker zero_padding-7.hdl...
          // 03:54:44.658     Execution succeeded, time was 01:19:38 (4778 seconds).
          if (platform =~ /sim$/) {
            timeout_time = 120
            if ("Force RCC File IO" == params["Test File IO"]) {
              echo "Adjusting timeout because simulating with RCC File I/O."
              timeout_time *= 2
            }
          }
          timeout(time: timeout_time, unit: 'MINUTES', activity: true) { // This is NOT absolute, but inactivity
            dir ('logs') { sh "pwd" } // Create logs directory
            dir ('built_area') {
              // NOTE: "sudo" has "env_keep" set for OCPI_CDK_DIR so it passes thru
              if (platform in driver_pfs) {
                try {
                  sh "${ocpi_cdk_dir_var} sudo ../workspace/etc/rc.d/init.d/opencpi-driver-check stop || :"
                  sh "${ocpi_cdk_dir_var} sudo ../workspace/etc/rc.d/init.d/opencpi-driver-check start"
                } catch (err) {
                  sh "mv --target-directory=${env.WORKSPACE}/logs/ /tmp/*opencpi*log ${env.WORKSPACE}/workspace/opt/opencpi/driver/*/*.log || :"
                  error "Error starting driver."
                }
              } // driver start
              // Put a symlink to jenv where build_test expects it
              sh "ln -s workspace/jenv.sh .."
              // Importing build_test may set VERBOSE (and will fix PATH, OCPI_CDK_DIR, etc)
              // Do the actual run. On non-simulators, skip the host.
              // More stupid JENKINS-13128 / JENKINS-14269 permission fixes
              sh """
              . ../releng/jenkins/runtime/bash/build_test.sh
              for f in \$(find ${component_to_test} -name '*.sh' -o -name '*.py'); do
                chmod a+x \$f
              done
              # Delete the HDL version if told to force RCC (AV-5373 work-around):
              if [ "\${OCPI_FORCE_HDL_FILE_IO}" == "0" ]; then
                rm -rf ${component_to_test}/gen/assemblies/*_frw
              fi
              make -C ${component_to_test} run ${platform.find(/sim$/)?'':'--exclude-platform centos7'} 2>&1 | tee -a run.log
              """
              // If it got this far, the test was a success, so we can clean up...
              echo "Test successful. Cleaning up."
              sh ". ../jenv.sh; for dir in \$(compgen -o dirnames | grep -v ocpi_registry); do ocpidev -d \$dir clean || :; done"
            } // built_area
          } // timeout
        } // run stage
      } catch (err) {
        echo "Something did not work: '${err}'. Capturing logs."
        error "Something did not work: '${err}'. Captured logs."
      } finally { // Always get the logs and clean up the driver
        (log_files + sim_files).each {
          echo "Compressing obnoxiously large files matching '${it}' found in built_area"
          dir ("built_area") {
            sh "find . -name '${it}' -size +10M | xargs -rn1 xz -1v || :"
          }
        }
        log_files.each {
          dir ("logs") {
            sh """(cd ../built_area && find -name '${it}' -o -name '${it}.xz') | xargs -r -IXXX bash -c 'mkdir -p \$(dirname XXX) && cp -l ../built_area/XXX \$(dirname XXX)' || :"""
          }
        }
        sh "sudo chown -R jenkins:jenkins ${env.WORKSPACE} || :" // There might be root-owned drivers, logs, etc.
        // Possibly parse Valgrind warnings
        if (upstream_job_config.use_valgrind) {
          publishValgrind (
            failBuildOnInvalidReports: false,
            failBuildOnMissingReports: false,
            pattern: "${env.WORKSPACE}/valgrind-reports/*-valgrind.xml",
            publishResultsForAbortedBuilds: true,
            publishResultsForFailedBuilds: true
          )
        } // valgrind
        timeout(time: 10, unit: 'MINUTES') {
          if (platform in driver_pfs) {
            sh "${ocpi_cdk_dir_var} sudo workspace/etc/rc.d/init.d/opencpi-driver-check stop || :"
          }
          archiveArtifacts artifacts: "logs/**, built_area/${component_to_test}/**", allowEmptyArchive: false, fingerprint: false, onlyIfSuccessful: false
          if (currentBuild.resultIsBetterOrEqualTo('SUCCESS'))
            deleteDir() // cleanup workspaces
        } // cleanup timeout
      } // finally
    } // driver lock
  } // node
} // timestamps
