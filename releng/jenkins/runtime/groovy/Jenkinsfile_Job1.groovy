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

// Editing Notes:
// There are headings in each section that are of a great benfit if you use a minimap
// The "flow" is top to bottom until the UTILITIES header
// Everything beneath that is in support of the main flow, e.g. transformXXX calls
// There used to be "transformMainClassicBuildStep" which showed launching a job and not caring - see 1e43350

@Library('opencpi-libs')_
// Field ~~ global variable
import groovy.transform.Field

/*
 ██████  ██████  ███    ██ ███████ ██  ██████
██      ██    ██ ████   ██ ██      ██ ██
██      ██    ██ ██ ██  ██ █████   ██ ██   ███
██      ██    ██ ██  ██ ██ ██      ██ ██    ██
 ██████  ██████  ██   ████ ██      ██  ██████
*/
// This is anything that could possibly be configured within a stage of this job. You probably don't need to change anything:
@Field final TargetOSs = [6,7] // Changing this will likely blow up in 100 places
@Field final Quick_Test_Sim = 'xsim' // This job will build and test "core" BUT THROWN AWAY; minimal and Job_2 re-does it anyway
@Field final IDE_Build_Job = 'IDE/Full_Build'
@Field final CROSS_DIR_zynq = '/opt/Xilinx/14.7/ISE_DS/EDK/gnu/arm/lin/bin/'
@Field final CROSS_DIR_picoflexor = '/opt/CodeSourcery/Sourcery_G++_Lite/bin/'
// JENKINS-47927 cannot combine fields
@Field final quick_failure_mode = false // Set to true if debugging Jenkins to fail faster

/// Docker options:
@Field final docker_image_base = "jenkins/ocpibuild:v4-C"
@Field final docker_image_pdf = "jenkins/ocpi_pdfgen:v4"
@Field final mount_tools = '-v /opt/Xilinx/:/opt/Xilinx/:ro'
// mount_tools previously had "--volumes-from tools_arm_cs:ro" - don't want to lose that syntax
// if we do something similar again for Docker volumes containing tools
@Field final mount_sudoers = '-v /etc/sudoers:/etc/sudoers:ro'
@Field final docker_options = '--group-add opencpi'

///Other config:
// Local prereq copy stuff
@Field local_prereq_vars
// The default for test platforms is "None" unless this is the "develop" or a release branch.
@Field release_branch = false
release_branch = (env.BRANCH_NAME =~ /^v[\d\.]+$/).matches()
@Field test_platforms = "None\nSimulators\nAll\n"
if ("develop" == env.BRANCH_NAME)
  test_platforms = "Simulators\nAll\nNone\n"
if (release_branch)
  test_platforms = "All\nSimulators\nNone\n"

properties properties: [ // This is ugly https://stackoverflow.com/a/35471196
  buildDiscarder(
    logRotator(artifactDaysToKeepStr: '',
      artifactNumToKeepStr: '10',
      daysToKeepStr: '',
      numToKeepStr: '25')),
  disableConcurrentBuilds(),
  parameters([
    string(defaultValue: '', description: 'Debug freeform text. Usually left blank.', name: 'Description'),
    // This "cheat" mode is NOT vigorously tested and will likely end up with weird things like
    // mismatched Docker image versions (new build number image created, but some things that read metadata
    // using the old number), C6- or C7- only builds breaking things, etc.
    booleanParam(defaultValue: true, description: 'Build main RPMs (unchecked will copy from previous success)', name: 'Build Main'),
    booleanParam(defaultValue: true, description: "Run Tests (only this job's tests)", name: 'Run Tests'),
    booleanParam(defaultValue: true, description: 'Launch Job 2 upon completion', name: 'Launch Job2'),
    choice(choices: test_platforms, description: 'Platform(s) to build and run unit tests on (passed to Job 2)', name: 'Test Platforms'), // First = default
    string(defaultValue: '', description: 'RPM Release tag (e.g. "beta"). Usually left blank.', name: 'RPM Release'),
    string(defaultValue: '', description: 'Target(s) or platform(s) for downstream jobs. Usually left blank.', name: 'Target Platforms'),
    booleanParam(defaultValue: false, description: 'Force rebuild of prerequisites (special case; unchecked will copy from previous successful build if no code changes are detected)', name: 'Build Prereq'),
    booleanParam(defaultValue: false, description: 'Force Rebuild (Job normally aborts for various reasons, e.g. no applicable source changes)', name: 'Force Rebuild'),
  ]),
  [$class: 'ThrottleJobProperty', categories: [], limitOneJobWithMatchingParams: false, maxConcurrentPerNode: 0, maxConcurrentTotal: 0, paramsToUseForLimit: '', throttleEnabled: false, throttleOption: 'project'],
  [$class: 'EnvInjectJobProperty', info: [loadFilesFromMaster: false, secureGroovyScript: [classpath: [], sandbox: false, script: '']], keepBuildVariables: true, keepJenkinsSystemVariables: true, on: true],
  /* not needed? pipelineTriggers([pollSCM('@hourly')]), */
]

import groovy.json.JsonOutput
// Need Slurper to parse .jenkins_metadata.json
import groovy.json.JsonSlurper
@Field branch_config
@Field previous_changesets
@Field build_prereq // Copy of incoming with possible override in Setup
@Field PlatformBSPRPMs
PlatformBSPRPMs = [] as Set
@Field error_email
@Field stage_name
currentBuild.description = "" // Ensure after this point it's at least defined
// These next two are for vendor repo support (AV-4279)
@Field vendor_repos_imported
vendor_repos_imported = [:] // Hash where key = workspace and value is set
@Field vendor_repos_exported
vendor_repos_exported = [] as Set // Exporting doesn't care about workspace
// Map software platforms to hardware platforms
@Field hw_mappings

try { // The whole job is within this block (see "end of job")
// A top-level timeout doesn't take into account Jenkins congestion waiting for executors.
// If certain parts start hanging, we can add individual timeouts to stages as needed.
// Note that some tests have them already.
// timeout(time: 1, unit: 'HOURS') {
/*
███████ ███████ ████████ ██    ██ ██████
██      ██         ██    ██    ██ ██   ██
███████ █████      ██    ██    ██ ██████
     ██ ██         ██    ██    ██ ██
███████ ███████    ██     ██████  ██
*/
stage_name = 'Setup'
stage(stage_name) {
  echo "Job configuration: " + JsonOutput.prettyPrint(JsonOutput.toJson(params))
  // To bring in the snippets, we need to be running on a real node
  // TODO: There are Jenkins plugins that can fix this. Library stuff.
  node('master') {
    error_email = "${env.BUILD_URL}\n"
    if (params["Description"])
      currentBuild.description = "${params["Description"]}\n"
    clear_workspace()
    cached_checkout this, scm
    // sh """git clean -ffdx -e ".jenkins-*/" || :""" // Cleanup (see JENKINS-31924 for formula)
    previous_changesets = find_last_success()
    def import_config = load("releng/jenkins/runtime/groovy/lib/import_config.groovy")
    branch_config = import_config()
    // Build Platform*RPMs by walking the branch configuration
    def BSPURLs = [] as Set // If multiple platforms share a repo, only build it once
    branch_config.hdl_platforms.each { pf, dat ->
      if (dat.bsp_repo && dat.enabled) {
        if (dat.bsp_repo in BSPURLs) {
          echo "Already scheduled to build RPM for BSP from ${pf}'s repo"
        } else {
          BSPURLs << dat.bsp_repo
          PlatformBSPRPMs << pf
        }
      }
    } // hdl_platforms
  } // node

  abort_fast() // defined below

  // Auto-decide if prereqs need to be [re]built
  build_prereq = params["Build Prereq"]
  if (!build_prereq) { // Look for "prerequisites/" being changed (AV-4194)
    previous_changesets.changeSets.each { cs ->
      if (cs.paths.findAll{ it.contains('prerequisites/') }) { // If anything is returned from this search closure...
        echo "Found a changeset that affected prerequisites and forcing enable of prereq builds:"
        echo JsonOutput.prettyPrint(JsonOutput.toJson(cs))
        build_prereq = true
      } // if
    } // each
  } // if no prereq
} // end of stage

// Some global variables and general job description
if (build_prereq)
  currentBuild.description += "With prereqs"
if (!params["Run Tests"])
  currentBuild.description += " (UNTESTED!)"
local_prereq_vars = """
  # Never go to the internet
  export OCPI_PREREQUISITES_DOWNLOAD_PATH=cache:local
  export OCPI_PREREQUISITES_LOCAL_PATHNAME=${global_config.local_prereq_path()}
  export OCPI_PREREQUISITES_LOCAL_SERVER=${global_config.local_prereq_mirror()}
  # The prereq name is gtest, the actual file is release-1.8.0.zip, but we have:
  export OCPI_PREREQUISITES_LOCAL_FILE_gtest=googletest-release-1.8.0.zip
  export OCPI_PREREQUISITES_LOCAL_FILE_ad9361=no-os.git
  export OCPI_PREREQUISITES_LOCAL_FILE_liquid=liquid-dsp-v1.3.1.tar.gz
  """

// The way this job works is it constantly reuses this variable:
// See https://github.com/jenkinsci/pipeline-examples/blob/master/pipeline-examples/parallel-from-list/parallelFromList.groovy
// We populate that variable in different ways and then fire it off with a "parallel" command
@Field stepsForParallel = [:]

/*
██████  ██████  ███████ ██████  ███████  ██████
██   ██ ██   ██ ██      ██   ██ ██      ██    ██
██████  ██████  █████   ██████  █████   ██    ██
██      ██   ██ ██      ██   ██ ██      ██ ▄▄ ██
██      ██   ██ ███████ ██   ██ ███████  ██████
                                            ▀▀
*/
stage_name = 'Prereq Tarball'
stage(stage_name) {
  if (build_prereq) { // Build them (meat below in transformPrereqBuildStep)
    TargetOSs.each { os ->
      def final add_str = (7 == os)?' and Embedded RCC Platforms':''
      stepsForParallel["CentOS ${os}${add_str}: Prereq Build"] = transformPrereqBuildStep(os)
    }
    stepsForParallel['failFast'] = true
    parallel stepsForParallel
  } else { // Need to copy prereqs from a previous successful build or develop (AV-4195, AV-5256)
    node('master') {
      clear_workspace()
      def project_to_copy = env.JOB_NAME
      def id_to_copy = 0
      // We use the "run selector" plugin to find the exact build ID
      try {
        last_all = selectRun job: env.JOB_NAME, selector: status('STABLE') // , verbose: true
        id_to_copy = last_all.getId() as int
      } catch (err) {}
      if (0 == id_to_copy) {
        // Cannot find one in our history; steal from develop
        project_to_copy = project_to_copy.replace(env.BRANCH_NAME,'develop')
        try {
          last_all = selectRun job: project_to_copy, selector: status('STABLE') // , verbose: true
          id_to_copy = last_all.getId() as int
        } catch (err) {}
          currentBuild.description += "With prereqs from develop #${id_to_copy}"
      }
      if (0 == id_to_copy) {
        def final errstr = "Self-aborted (No previous prereqs found)"
        error_email += "\n\n" + errstr
        currentBuild.description = errstr
        error("Could not find previous job or successful 'develop' run with built prereqs")
      }
      TargetOSs.each { os ->
        copyArtifacts(
          projectName: project_to_copy,
          filter: "support/prereqs-C${os}.tar",
          fingerprintArtifacts: true,
          target: ".",
          flatten: true,
          selector: specific("${id_to_copy}") // JENKINS-47916
        )
        sh "tar xf prereqs-C${os}.tar && rm -f prereqs-C${os}.tar"
        dir("prerequisites") {
          stash "prereq-C${os}"
        }
      } // each TargetOSs
    } // master node
  } // build prereq or not

  // Archive our built or copied prereq into "support" (either results in a stash)
  node('master') {
    TargetOSs.each { os ->
      clear_workspace()
      dir("prerequisites") {
        unstash "prereq-C${os}"
      }
      sh "XZ_OPT='-1 -v -T4' tar Jcf prereqs-C${os}.tar prerequisites"
      dir ("support") {
        sh "cp ../prereqs-C${os}.tar ."
      }
      archiveArtifacts artifacts: "support/prereqs-C${os}.tar", fingerprint: true, onlyIfSuccessful: true
    } // each OS
  } // node
} // stage

/*
███    ███  █████  ██ ███    ██     ██████  ██████  ███    ███ ███████
████  ████ ██   ██ ██ ████   ██     ██   ██ ██   ██ ████  ████ ██
██ ████ ██ ███████ ██ ██ ██  ██     ██████  ██████  ██ ████ ██ ███████
██  ██  ██ ██   ██ ██ ██  ██ ██     ██   ██ ██      ██  ██  ██      ██
██      ██ ██   ██ ██ ██   ████     ██   ██ ██      ██      ██ ███████
*/
stage_name = 'Main RPMs'
stage(stage_name) {
  if (params["RPM Release"].trim())
    currentBuild.description += "\nRelease ${params["RPM Release"]}"
  if (!params["Build Main"])
    currentBuild.description += " (quick copied from #${previous_changesets.buildNumber})"

  stepsForParallel = ['failFast':true]
  TargetOSs.each { os ->
    stepsForParallel["CentOS ${os}: Main RPM Build"] = transformMainBuildStep(os)
  }
  // Might as well build these at the same time (AV-4223, AV-4366)
  if (params["Build Main"]) {
    stepsForParallel["Build IDE"] = BuildIDE()
    stepsForParallel["Build PDF Documentation"] = BuildPDF()
  }
  parallel stepsForParallel
} // end of stage

/*
 ██████ ██████   ██████  ███████ ███████     ██████  ██████  ███    ███ ███████
██      ██   ██ ██    ██ ██      ██          ██   ██ ██   ██ ████  ████ ██
██      ██████  ██    ██ ███████ ███████     ██████  ██████  ██ ████ ██ ███████
██      ██   ██ ██    ██      ██      ██     ██   ██ ██      ██  ██  ██      ██
 ██████ ██   ██  ██████  ███████ ███████     ██   ██ ██      ██      ██ ███████
*/
stage_name = 'Cross RPMs'
stage(stage_name) {
  if (params["Build Main"]) {
    stepsForParallel = ['failFast':true]
    // These arrays are already filtered for enabled above
    PlatformBSPRPMs.each {
      stepsForParallel["BSP RPM Build: ${it}"] = transformBSPBuildStep(it)
    }
    branch_config.rcc_platforms.each { pf, dat ->
      if (dat.enabled)
        stepsForParallel["CDK RPM Build: ${pf}"] = transformBSPCDKBuildStep(pf)
    }
    // NOTE: Previously (1e43350) there was a "virtual platform" to make the assets RPM in parallel.
    // With 1.4+, assets are part of the standard RPM build.
    parallel stepsForParallel
  }
} // end of stage

/*
████████  █████  ██████  ██████   █████  ██      ██      ███████
   ██    ██   ██ ██   ██ ██   ██ ██   ██ ██      ██      ██
   ██    ███████ ██████  ██████  ███████ ██      ██      ███████
   ██    ██   ██ ██   ██ ██   ██ ██   ██ ██      ██           ██
   ██    ██   ██ ██   ██ ██████  ██   ██ ███████ ███████ ███████
*/
stage_name = 'Extracted Tarballs'
stage(stage_name) {
  // Create "fully-installed" tarballs for non-Docker usage
  stepsForParallel = ['failFast':true]
  TargetOSs.each { os ->
    stepsForParallel["CentOS ${os}: Tarball"] = transformTarballBuildStep(os)
  } // each OS
  parallel stepsForParallel
} // end of stage

/*
███████ ███████ ██      ███████  ████████ ███████ ███████ ████████ ███████
██      ██      ██      ██          ██    ██      ██         ██    ██
███████ █████   ██      █████ █████ ██    █████   ███████    ██    ███████
     ██ ██      ██      ██          ██    ██           ██    ██         ██
███████ ███████ ███████ ██          ██    ███████ ███████    ██    ███████
*/
stage_name = 'Self-Tests'
stage(stage_name) {
  if (params["Run Tests"]) {
    stepsForParallel = ['failFast':quick_failure_mode]
    TargetOSs.each { os ->
      stepsForParallel["CentOS ${os}: Container Tests"] = transformCTestsStep(os)
      stepsForParallel["CentOS ${os}: Google Self-Tests"] = transformGTestsStep(os)
      stepsForParallel["CentOS ${os}: OCPI DDS Self-Test"] = transformDDSTestsStep(os)
      stepsForParallel["CentOS ${os}: Misc AV Tests"] = transformAVTestsStep(os)
      stepsForParallel["CentOS ${os}: RCC Examples"] = transformRCCExampleStep(os)
      // Disabled; moved to BuildHDLCore: stepsForParallel["CentOS ${os}: ocpidev tests"] = transformOcpiDevTestsStep(os)
    } // TargetOSs loop
    // The quick tests will only be done on C7 to minimize time since its output is wasted.
    // (After the core is built, a handful of tests will run, like pytests)
    stepsForParallel["HDL Core Build"] = BuildHDLCore()
    parallel stepsForParallel
  } // param check

  // Test that always run
  stepsForParallel = ['failFast':quick_failure_mode]
  TargetOSs.each { os ->
    stepsForParallel["CentOS ${os}: RPM Manifest"] = transformManifestBuildStep(os)
  } // TargetOSs loop
  parallel stepsForParallel
} // end of stage

/*
██████  ███████ ██████  ██       ██████  ██    ██
██   ██ ██      ██   ██ ██      ██    ██  ██  ██
██   ██ █████   ██████  ██      ██    ██   ████
██   ██ ██      ██      ██      ██    ██    ██
██████  ███████ ██      ███████  ██████     ██
*/
stage_name = 'Deploy'
stage(stage_name) {
  stepsForParallel = ['failFast':false]
  TargetOSs.each { os ->
    stepsForParallel["CentOS ${os}: Yum Deploy"] = transformDeployStep(os)
  } // TargetOSs loop
  parallel stepsForParallel
} // Deploy

stage_name = 'Launch Job2' // Fix error emails
if (params["Launch Job2"]) {
  echo "Launching Job 2: Target Platforms: '${params["Target Platforms"]}'"
  echo "Debug: 'Upstream_Job'='${env.BUILD_NUMBER}', 'Target Platforms'='${params["Target Platforms"]}', 'RPM Release'='${params["RPM Release"]}' 'Test Platforms'='${params['Test Platforms']}' "
  build job: "${env.JOB_NAME}".replace('Job_1','Job_2'), parameters: [
    string(name: 'Upstream_Job', value: "${env.BUILD_NUMBER}"),
    string(name: 'Target Platforms', value: params["Target Platforms"]),
    string(name: 'RPM Release', value: params["RPM Release"]),
    string(name: 'Test Platforms', value: params['Test Platforms']),
    booleanParam(name: 'Force Rebuild', value: true), // Always build if it got this far
  ], quietPeriod: 0, wait: false
}

/*
███████ ███    ██ ██████       ██████  ███████          ██  ██████  ██████
██      ████   ██ ██   ██     ██    ██ ██               ██ ██    ██ ██   ██
█████   ██ ██  ██ ██   ██     ██    ██ █████            ██ ██    ██ ██████
██      ██  ██ ██ ██   ██     ██    ██ ██          ██   ██ ██    ██ ██   ██
███████ ██   ████ ██████       ██████  ██           █████   ██████  ██████
*/
// } // timeout
} catch (org.jenkinsci.plugins.workflow.steps.FlowInterruptedException err) {
  // If aborted, who did it?
  // Might be org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout if timed out
  //   which is why we need to check if any of the getCauses() includes UserInterruption
  def found = false
  err.getCauses().each { e ->
    if (e instanceof jenkins.model.CauseOfInterruption.UserInterruption) {
      def final errstr = "Aborted by ${e.getUser().getDisplayName()}"
      mattermost_send(this, [msg: "${env.JOB_NAME}: Aborted by @${e.getUser().getId()}", icon: "https://image.flaticon.com/icons/svg/631/631681.svg"])
      // error_email = "${errstr}\n\n${error_email}"
      currentBuild.result = 'ABORTED'
      currentBuild.description += "\n${errstr}"
      found = true
    }
  }
  if (!found) // Must be something else...
    report_error(err)
} catch (err) {
  // An error has happened (NOTE: doesn't seem to catch all errors, e.g. java.lang.NoSuchMethodError?)
  report_error(err)
  error "Error in ${stage_name} stage\n\tError: ${err}"
} finally {
  // An error may or may not have happened
  // If an error happens SUPER early, previous_changesets may not be set yet (this may not ever happen again):
  if (!previous_changesets) previous_changesets = [ buildNumber: 0, abortedBuilds: [] as Set ]
  if (!branch_config) error "Reached 'finally' block too early to know whom to email!\n\n${error_email}"
  echo "Determining if moved to success: Current: ${currentBuild.currentResult}, Previous success: ${previous_changesets.buildNumber} vs. ${(env.BUILD_NUMBER as int)-1}"
  if (currentBuild.resultIsBetterOrEqualTo('SUCCESS')) { // haven't failed yet
    def final this_build = env.BUILD_NUMBER as int
    if ((this_build-1) != previous_changesets.buildNumber) { // last success is not the one before
      // Walk all intervening builds to see if ABORTED or not. Could probably do something with a set intersection.
      def all_aborted = true
      for (int bld = previous_changesets.buildNumber+1; bld < this_build; ++bld)
        if (!previous_changesets.abortedBuilds.contains(bld)) all_aborted = false
      if (all_aborted) {
        echo "(Skipping email; all intervening builds were aborted)"
      } else {
        echo "Preparing email notifying happiness"
        def recip = branch_config.notifications.join(',')
        recip += (env.BRANCH_NAME in branch_config.team_notify)? ",${branch_config.team_email}":''
        if (recip.trim() != '""')
          emailext body: error_email, subject: "Job 1 (${env.BRANCH_NAME}) is back to normal", to: recip
        else
          echo "(No email recipients)"
      }
    } // New success
  } // not failed
}

/*
██    ██ ████████ ██ ██      ██ ████████ ██ ███████ ███████
██    ██    ██    ██ ██      ██    ██    ██ ██      ██
██    ██    ██    ██ ██      ██    ██    ██ █████   ███████
██    ██    ██    ██ ██      ██    ██    ██ ██           ██
 ██████     ██    ██ ███████ ██    ██    ██ ███████ ███████
*/
// Function to decide if we should self-abort the build
// TODO: Combine with Job2 version in external call?
void abort_it(err1, err2) {
  error_email += "\n\n" + err1
  currentBuild.result = 'ABORTED'
  currentBuild.description = err1
  error(err2)
} // abort_it

void abort_fast(debug_abort = false) {
  // For debug:
  echo "Job build causes: " + JsonOutput.prettyPrint(JsonOutput.toJson(currentBuild.getBuildCauses()))
  if (params["Force Rebuild"]) // Workaround for JENKINS-43754 and JENKINS-41272
    return
  def nojenkins_flags = 0
  def ignored_path_only_changes = 0
  previous_changesets.changeSets.each { this_change ->
    // We check each changeset for NoJenkins (only the first line is checked, unfortunately)
    if (this_change.message.toLowerCase().contains("nojenkins")) {
      ++nojenkins_flags
      return // Don't want this changeset to count twice if paths match as well
    }
    // And check if all paths were something we should ignore
    def ign_paths = this_change.paths.findAll {
      it.contains('/doc/') ||
      false
    }
    if (debug_abort) {
      echo "Ignore paths: "+ign_paths.join(':')
      echo "Versus:"+this_change.paths.join(':')
    }
    if (ign_paths.size() == this_change.paths.size())
      ++ignored_path_only_changes
    if (debug_abort)
      echo "After processing ${this_change.commit} (from build ${this_change.build}): nojenkins_flags: ${nojenkins_flags} ignored_path_only_changes: ${ignored_path_only_changes}"
  } // changeSets for loop
  echo "After processing ${previous_changesets.changeSets.size()} changesets, found ${nojenkins_flags} flagged NoJenkins and ${ignored_path_only_changes} that only modified ignored paths"
  if (!previous_changesets.changeSets.size())
    abort_it "Self-aborted (no changes)", "No changes since last successful build; use 'Force Rebuild' option"
  if (nojenkins_flags == previous_changesets.changeSets.size())
    abort_it "Self-aborted (NoJenkins flags)", "All previous ${nojenkins_flags} changesets flagged with NoJenkins"
  if (ignored_path_only_changes == previous_changesets.changeSets.size())
    abort_it "Self-aborted (all paths ignored)", "All previous ${ignored_path_only_changes} changesets only dealt with ignored paths"
  if ((nojenkins_flags + ignored_path_only_changes) == previous_changesets.changeSets.size())
    abort_it "Self-aborted (NoJenkins flags + paths)", "All previous ${nojenkins_flags + ignored_path_only_changes} changesets indicated not to build"
} // abort_fast

// Used to combine catch blocks - TODO: move to utility library to share w/ Job 1 and 2
void report_error(err) {
  echo "Preparing email notifying failure in ${stage_name} stage"
  error_email = "Error: '${err}'\n\n${error_email}"
  if ('ABORTED' != currentBuild.result)
    mattermost_send(this, [icon: "https://jenkins.io/images/logos/fire/fire.png"])
  // If an error happens SUPER early, branch_config may not be set yet:
  if (!branch_config) error "Too early to know whom to email!\n\n${error_email}"
  def recip = branch_config.notifications.join(',')
  recip += (env.BRANCH_NAME in branch_config.team_notify)? ",${branch_config.team_email}":''
  if (recip.trim() != '""') {
    if ('ABORTED' == currentBuild.result) { // Might not be needed any more...
      echo "(Skipping email; build was aborted)"
    } else {
      currentBuild.result = 'FAILURE'
      currentBuild.description += "\nFailed in ${stage_name} stage"
      emailext body: error_email, subject: "Job 1: ${stage_name} (${env.BRANCH_NAME}) is failing", to: recip
    }
  } else {
    echo "(No email recipients)"
  }
}

// Used by the various tests to set up workspace and pull in a docker image
// Returns the docker image that you use to call "inside()"
def test_setup(in_OS) {
  def OS=in_OS
  def installed_image
  dir ('temp_support') {
    // Get the main RPMs and the prereq docker image
    unarchive mapping: ["centos${OS}/**" : '.'] // https://stackoverflow.com/a/29751794/836748
    unstash "support-C${OS}"
    installed_image = docker_rpminstalled_image this, "centos${OS}"
    deleteDir()
  }
  return installed_image
}

// Used by the various stages to set up workspace and extract RPMs
// It is expected to be run on ANY node
// The "true Jenkins" way would be to use withEnv() but there's other stuff that can't
//  easily be ported, like the env.d stuff.
// This assumes the workspace has been cleaned to some point before calling.
void workspace_setup(in_OS) {
  def OS=in_OS
  dir ('support') {
    // Get the RPMs, etc
    unarchive mapping: ["support/ocpi_rpms_extracted_C${OS}.tar" : './ocpi_rpms_extracted.tar']
    unstash "support-C${OS}"
    sh "mv jenkins .."
    sh "mkdir ../cdk && cd ../cdk && tar xf ../support/ocpi_rpms_extracted.tar"
    deleteDir()
  }
  // NOTE: This is copied to other jobs as well... search for OCPI_BUILD_SHARED_LIBRARIES?
  // Find local resources on this node
  sh '''
    set +x
    echo "set +x" >> jenv.sh
    if [ -d /opt/Xilinx ]; then
      XIL_VER=$(ls -1 /opt/Xilinx/ | sort | grep -E '^[0-9]+.[0-9]+$' | tail -1)
      echo "Found Xilinx Version: ${XIL_VER}"
      echo "export OCPI_XILINX_DIR=/opt/Xilinx" >> jenv.sh
      echo "export OCPI_XILINX_VERSION=${XIL_VER}" >> jenv.sh
      echo "export XILINXD_LICENSE_FILE=/opt/Xilinx/${XIL_VER}/WebPACK.lic" >> jenv.sh
    fi
    for d in /opt/{A,a}ltera /home/*/altera ; do
      if [ -d $d ]; then
        ALT_HOME=$d
        ALT_VER=$(ls -1 ${ALT_HOME} | sort | grep -E '^[0-9]+.[0-9]+' | tail -1)
        echo "Found Altera Version: ${ALT_VER} in ${ALT_HOME}"
        echo "export OCPI_ALTERA_DIR=${ALT_HOME}" >> jenv.sh
        echo "export OCPI_ALTERA_VERSION=${ALT_VER}" >> jenv.sh
      fi
    done
    if [ -d /opt/Altera/intelFPGA_pro/ ]; then
      ALT_HOME=/opt/Altera/intelFPGA_pro/
      ALT_VER=$(ls -1 ${ALT_HOME} | sort | grep -E '^[0-9]+.[0-9]+' | tail -1)
      echo "Found Quartus Pro Version: ${ALT_VER} in ${ALT_HOME}"
      echo "export OCPI_ALTERA_PRO_DIR=${ALT_HOME}" >> jenv.sh
      echo "export OCPI_ALTERA_PRO_VERSION=${ALT_VER}" >> jenv.sh
    fi
    if [ -d /opt/Modelsim ]; then
      echo "Found Modelsim"
      echo "export OCPI_MODELSIM_DIR=/opt/Modelsim/modelsim_dlx/" >> jenv.sh
    fi
  '''
  // The above cannot be double quoted because of the grep with the $ sign.
  // This needs to be double quoted to fill in the blanks.
  sh """
    set +x
    # echo "export OCPI_LOG_LEVEL=11 V=1 AT='' OCPI_DEBUG_MAKE=1 OCPI_HDL_VERBOSE_OUTPUT=1" >> jenv.sh
    printf "\
        export OCPI_XILINX_LICENSE_FILE=${global_config.license_server()}\n\
        export OCPI_ALTERA_LICENSE_FILE=${global_config.license_server()}\n\
        export OCPI_MODELSIM_LICENSE_FILE=${global_config.license_server()}\n\
        export OCPI_BUILD_SHARED_LIBRARIES=0\n\
        set -o pipefail\n\
        " >> jenv.sh
    shopt -s nullglob
    # These next few lines are the equivalent of env/rpm_cdk.sh:
    echo 'echo Importing opencpi-setup.sh' >> jenv.sh
    echo "source ${env.WORKSPACE}/cdk/opt/opencpi/cdk/opencpi-setup.sh -" >> jenv.sh
    for f in ${env.WORKSPACE}/cdk/opt/opencpi/cdk/env.d/*.sh; do
      echo "Importing \$f into jenv.sh"
      cat \$f >> jenv.sh
    done

    echo "echo jenv.sh imported" >> jenv.sh
    echo "set -x" >> jenv.sh
  """
} // workspace_setup

// Automatically download bsp
// This method is called from bring_in_external_repos
void automatic_bsp_download(pf, ws) {
  def final bsp_repo = branch_config.hdl_platforms[pf]?.bsp_repo
  if ( bsp_repo ) {
    echo "automatic_bsp_download(${pf}::${ws}): Bringing in BSP repo"
    // If URL name matches "XXXXX.bsp.YYYY.git" then use YYYY as platform name
    def shared_bsp = bsp_repo.replaceAll(/^.*\.bsp\.(.*)\.git/, '$1')
    if (bsp_repo != shared_bsp) {
      echo "automatic_bsp_download(${pf}::${ws}): Detected platform ${pf} uses shared BSP repo '${shared_bsp}'"
      branch_config.hdl_platforms[pf].shared_bsp = shared_bsp
    }
    if (!fileExists("projects/bsps/${shared_bsp}")) {
      dir("projects/bsps/${shared_bsp}") {
        clear_workspace()
        checkout_paired_repo this, bsp_repo, ["fallback": branch_config.fallback]
      } // dir
    } else {
      echo "automatic_bsp_download(${pf}::${ws}): projects/bsps/${shared_bsp} already existed"
    }
  } // bsp_repo
} // automatic_bsp_download

// Used by the BSP and CDK stuff to bring in external vendor support and BSP repos.
// Handles all the config stuff from 'bsp_repo' and 'vendor_repo'
// Also fixes executable file problems as well as tarring up anything from the vendor repo needed in HDL builds
// Is NOT explicitly thread-safe! That might be a problem with the globals.
// TODO: Move to Makefile?
// in_pf_hw_mapping is an array of hardware platforms that correspond to the pf being passed in.
// If the array is provided this means we need to bring in all of the provided hardware platforms
void bring_in_external_repos(in_pf, in_ws, in_pf_hw_mapping = []) {
  def pf=in_pf
  def ws=in_ws
  def pf_hw_mapping=in_pf_hw_mapping
  // Add pf to platforms to be automatically downloaded
  pf_hw_mapping << pf
  echo "bring_in_external_repos(${pf}::${ws}): Called"
  if (!vendor_repos_imported.containsKey(ws)) vendor_repos_imported[ws] = [] as Set // Initialize
  if (branch_config.rcc_platforms?."${pf}"?.hdl_project) { // Map back to a project, if it exists
    def old_pf = pf
    pf = branch_config.rcc_platforms[pf].hdl_project
    echo "bring_in_external_repos(${old_pf}::${ws}): Remapping self to ${pf} for external repo"
  }

  pf_hw_mapping.each {
    automatic_bsp_download(it, ws)
  }

  def final vendor_repo_this_pf = branch_config.hdl_platforms[pf]?.vendor_repo

  // If it is a SHARED BSP, then we want to ensure we create a vendor support file for EVERY platform that uses that BSP so that base_target builds of the BSP will work.
  def vendor_repos = [:]
  if (branch_config.hdl_platforms[pf]?.base_target && branch_config.hdl_platforms[pf]?.shared_bsp) {
    branch_config.hdl_platforms.each { pfn, dat ->
      if (dat?.base_target == branch_config.hdl_platforms[pf].base_target &&
          dat?.vendor_repo && dat?.vendor_repo == vendor_repo_this_pf)
        vendor_repos[pfn] = dat.vendor_repo
    } // each hdl_platform
  }

  echo "bring_in_external_repos(${pf}::${ws}): Bringing in vendor repo(s)"
  vendor_repos.each { pfn, vendor_repo ->
    if (pfn in vendor_repos_imported[ws]) {
      echo "Skipping vendor repo import of ${pfn} (called as ${pf}) because already done"
    } else {
      echo "Performing vendor repo import of ${pfn} (called as ${pf})"
      vendor_repos_imported[ws] << pfn
      // This should give reasonable errors if parameters are missing.
      def vendor_build = selectRun filter: parameters("GIT_BRANCH_NAME=${vendor_repo.git_branch}"), job: vendor_repo.project_name, selector: status('STABLE'), verbose: true
      copyArtifacts(
        projectName: vendor_repo.project_name,
        filter: vendor_repo.file_filter,
        fingerprintArtifacts: true,
        target: 'vendor',
        selector: specific(vendor_build.getId())
      )
      // Now if there is a file "executable_files.txt" we need to parse it to get around JENKINS-13128 / JENKINS-14269
      if (fileExists('vendor/executable_files.txt')) {
        echo "Processing executable_files.txt:"
        sh """
          set +e; set +x
          cd vendor
          while read -r file; do
            [ -e \${file} ] && echo "Setting executable bit on \${file}" && chmod a+x \${file}
          done < executable_files.txt
          rm executable_files.txt
        """
      } // executable_files.txt
    } // vendor_repos_imported
    // Now, if user asked for HDL support, archive the requested files
    // Reminder: pfn is ANY platform, pf is the requested platform and we need all that match base
    if (pfn in vendor_repos_exported) {
      echo "Skipping vendor repo export of ${pfn} (called as ${pf}) because already done"
    } else {
      echo "Performing vendor repo export of ${pfn} (called as ${pf})"
      vendor_repos_exported << pfn
      if (branch_config.hdl_platforms[pfn]?.vendor_repo.job2_support) {
        dir("support/bsp_${pfn}_vendor") {
          // Copy the files into temporary location
          branch_config.hdl_platforms[pfn].vendor_repo.job2_support.each {
            echo "Processing job2_support file list: ${it}"
            assert !it.contains('..') // a little security check on the path... easily fooled
            sh "cp -R ../../vendor/${it} ."
          }
        } // dir
        // Archive them into support/ for export with a unique name
        echo "bring_in_external_repos(${pf}): Creating HDL support: bsp_${pfn}_vendor.tar"
        sh """cd support && XZ_OPT="-1 -v -T4" tar Jcf bsp_${pfn}_vendor.tar -C bsp_${pfn}_vendor ."""
        archiveArtifacts artifacts: "support/bsp_${pfn}_vendor.tar", fingerprint: true, onlyIfSuccessful: true
      } // job2_support
    } // vendor_repos_exported
  } // vendor_repos.each
} // bring_in_external_repos

// Used to clear a workspace (tells bring_in_external_repos that it was deleted)
void clear_workspace() {
  vendor_repos_imported.remove("${env.NODE_NAME}:${env.WORKSPACE}")
  deleteDir()
}

/*
████████ ███████    ██████  ██████  ███████ ██████  ███████  ██████
   ██    ██         ██   ██ ██   ██ ██      ██   ██ ██      ██    ██
   ██    █████      ██████  ██████  █████   ██████  █████   ██    ██
   ██    ██         ██      ██   ██ ██      ██   ██ ██      ██ ▄▄ ██
   ██    ██ ███████ ██      ██   ██ ███████ ██   ██ ███████  ██████
                                                                ▀▀
*/
// Returns a closure that will build and stash prereq RPMs for a given OS (includes node assignment)
Closure transformPrereqBuildStep(in_OS) {
  return {
    // Variables need to be locally cached because otherwise they are a reference
    def OS = in_OS
    node ("rpmbuild && docker-C${OS}") {
      clear_workspace()
      def dimage = docker.image(docker_image_base+OS)
      def scmVars = cached_checkout this, scm
      def OLD_PARALLEL_BUILD = ''' // Might come back in the future.
      def prereq_par = [:]
      dir('releng/prereq') {
        sh "make libmyhostname" // Make sure one copy built before going parallel
        def prereq_packages = sh (script: "egrep '^PACKAGES' Makefile | cut -f2 -d=", returnStdout: true).trim().split()
        prereq_packages.each { pkg ->
          prereq_par["CentOS ${OS}: Prereq: ${pkg}"] = transformPrereqPkgBuildStep(dimage, OS, pkg)
        }
      } // prereq dir
      prereq_par['failFast'] = quick_failure_mode
      parallel prereq_par
      // Copy out the RPMs to /prereq/centosX/
      dir("prereq/centos${OS}") {
        sh "mv ../../releng/prereq/*rpm ."
      }
      '''
      // Build2 does all prereqs at once
      def pfs = ["centos${OS}"]
      if (7 == OS) // Only parallelize (and stash) other platforms under C7
        branch_config.rcc_platforms.each { pf, dat ->
          if (dat.enabled) {
            pfs.push(pf)
            if (dat?.hdl_project)
              bring_in_external_repos(dat.hdl_project, "${env.NODE_NAME}:${env.WORKSPACE}")
          }
        }
      sh "time cd ${global_config.local_prereq_path()} && pwd" // automount
      dir("prereq_src_${OS}") { sh "cp -v ${global_config.local_prereq_path()}/* ." } // copy so containers don't need to do NFS
      if (false) { // Cannot be used until framework has a way to download-only all packages ahead of time. Otherwise timing issues between them. Also, non-host platforms require host built first, so all hell breaks loose. Maybe some day...
        def prereq_par = [:]
        prereq_par['failFast'] = true // quick_failure_mode
        // Before things run in parallel, do top-level exports first and download all source (TODO!!!!)
        dimage.inside() { sh "make exports" }
        pfs.each { pf ->
          prereq_par["Prereqs for ${pf}"] = transformPrereqPkgBuildStep(dimage, pf, "${env.WORKSPACE}/prereq_src_${OS}")
        }
        parallel prereq_par
      } else { // not parallel
        dimage.inside("${mount_tools} -v ${env.WORKSPACE}/prereq_src_${OS}:${global_config.local_prereq_path()} --name ${env.BUILD_TAG}-prereq-C${OS}") {
          sh """set +x
                ${local_prereq_vars}
                shopt -s nullglob
                for bsp in projects/bsps/*; do
                  echo "Importing BSP \${bsp} into OCPI_PROJECT_PATH"
                  export OCPI_PROJECT_PATH="\$(pwd)/\${bsp}:\${OCPI_PROJECT_PATH}"
                done
                set -x
                make prerequisites Platforms="${pfs.join(' ')}" """
        } // inside
      } // not parallel
      // Stash the results in "prerequisites"
      dir("prerequisites") {
        // Dereference any symlinks that point outside of this tree (keeping ones like XX.so -> XX.so.1)
        // And remove extra locale info (e.g. for pico, 1.4G ⟶ 387M)
        sh """
          for l in `find . -type l`; do
            real=\$(readlink \$l)
            if [[ \$real = */prerequisites-build/* ]]; then
              mv \$l \$l.link
              cp -R -L \$l.link \$l
              rm \$l.link
            fi
          done
          set +e
          find . -type f \\( ! -wholename '*/boost/locale/*' -wholename '*/locale/*' ! -wholename '*/locale/en_US*' \\) -delete
          find . -type d -empty -delete
        """
        stash "prereq-C${OS}"
        // Remove source tarballs
        dir("prereq_src_${OS}") { deleteDir() }
      }
    } // node
  } // closure
} // transformPrereqBuildStep

// Returns a closure that will build all prereqs for a given platform within an already existing node's checkout
Closure transformPrereqPkgBuildStep(in_dimage, in_pf, in_src) {
  return {
    def dimage = in_dimage
    def pf = in_pf
    def srcdir = in_src
    // Name not needed, but nice to see in "docker ps"
    dimage.inside("${mount_tools} -v ${srcdir}:${global_config.local_prereq_path()}:ro --name ${env.BUILD_TAG}-prereq-${pf}") {
      sh """${local_prereq_vars}
            make prerequisites Platforms="${pf}" """
    } // inside() container
  } // closure
} // transformPrereqPkgBuildStep

/*
████████ ███████    ███    ███  █████  ██ ███    ██
   ██    ██         ████  ████ ██   ██ ██ ████   ██
   ██    █████      ██ ████ ██ ███████ ██ ██ ██  ██
   ██    ██         ██  ██  ██ ██   ██ ██ ██  ██ ██
   ██    ██ ███████ ██      ██ ██   ██ ██ ██   ████
*/
// Returns a closure that will build the main RPMs for a given OS (includes node assignment) - not platforms!
Closure transformMainBuildStep(in_OS) {
  return {
    def OS = in_OS
    node ('rpmbuild && docker-C'+OS) {
      clear_workspace()
      // Pull the repo (even if cheating, need it to stash releng, tests, etc)
      def scmVars = cached_checkout this, scm // could be on a new node / workspace
      if (!params["Build Main"]) {
        def old = previous_changesets.buildNumber
        if (old) {
          dir ('old_rpms') {
            copyArtifacts (
              projectName: env.JOB_NAME,
              filter: "centos${OS}/**, support/cdksnapshot-C${OS}.tar",
              fingerprintArtifacts: false,
              target: ".",
              flatten: false,
              selector: specific("${old}")
            )
          }
          dir ("centos${OS}") {
            sh "cp ../old_rpms/centos${OS}/{.jenkins_metadata.json,*.rpm} ."
            sh "cp ../old_rpms/support/cdksnapshot-C${OS}.tar ."
            // Read in .jenkins_metadata.json hw mappings into the global hw_mappings
            def final old_config = new JsonSlurper().parseText(readFile('.jenkins_metadata.json'))
            hw_mappings = old_config.hw_mappings
          }
          if (7 == OS) { // Copy the vendor BSPs if they exist
            dir ('support') { deleteDir() }
            copyArtifacts (
              projectName: env.JOB_NAME,
              filter: "support/bsp_*",
              fingerprintArtifacts: true,
              target: "support",
              flatten: true,
              optional: true,
              selector: specific("${old}")
            )
            archiveArtifacts (
              artifacts: "support/bsp_*",
              fingerprint: true,
              onlyIfSuccessful: true,
              optional: true
            )
            dir ('support') { deleteDir() }
          }
        } else {
          error_email += "\nCould not find old build to copy Main RPMs from!\n"
          error "Could not find old build to copy Main RPMs from!"
        }
      } else { // Not cheating
        // Start the actual build
        // Workaround for JENKINS-43077 (as is any RR to get cleaned up later)
        dir("prerequisites") { unstash "prereq-C${OS}" }
        def RR=""
        if (params["RPM Release"].trim())
          RR="export RPM_RELEASE=${params["RPM Release"].trim()};"
        // The following variables are used in the docker container and need to be initalized outside of it
        def rcc_hdl_platforms
        def loc_hw_mappings = ""
        docker.image(docker_image_base+OS).inside("${mount_tools} --name ${env.BUILD_TAG}-C${OS}") {
          withEnv(["RPM_RELEASE=${params["RPM Release"].trim()}"]) { // Doesn't work (JENKINS-43077)
            sh """
              ${RR}
              # build the framework, which needs prereq libs and tools
              make Platform=centos${OS}
              # build RCC in core, which is needed for runtime RPM minimal RCC artifacts
              make rcc Projects=core
              make rpm Platform=centos${OS} RpmVerbose=1 OcpiRelease=\$RPM_RELEASE
              # driver RPM as well
              make rpm Platform=centos${OS} RpmVerbose=1 Package=driver OcpiRelease=\$RPM_RELEASE
              """
            // Bring in all BSPS.  This is needed so that make can give use which RCC platforms correspond with which HDL platforms
            // If make does not have access to the projects then it can let us know which RCC corresponds to which HDL
            PlatformBSPRPMs.each {
              bring_in_external_repos(it, "${env.NODE_NAME}:${env.WORKSPACE}")
            }
            // Set up the HDL to RCC mapping
            loc_hw_mappings = "{"
            // Below we are getting the hardware mappings for each RCC platform
            // To get the mappings you have to set some OCPI variables to find all the platforms needed
            branch_config.rcc_platforms.each { pf, dat ->
              if (dat.enabled) {
                rcc_hdl_platforms = sh (script: """
                  shopt -s nullglob
                  for bsp in projects/bsps/*; do
                    export OCPI_PROJECT_PATH="\$(pwd)/\${bsp}:\${OCPI_PROJECT_PATH}"
                  done
                  export OCPI_CDK_DIR="\$(pwd)/cdk"
                  export OCPI_HDL_PLATFORM_PATH="\$(echo \$(make showhwdir Platform=${pf}) | tr ' ' ':')"
                  make showhw Platform=${pf}
                  """, returnStdout: true).trim()
                loc_hw_mappings += "\"${pf}\"" + ': ["' + rcc_hdl_platforms.replaceAll(' ', '","') + '"],'
              }
            }
            if (',' == loc_hw_mappings[-1]) loc_hw_mappings = loc_hw_mappings[0..-2] // remove trailing comma
            loc_hw_mappings += "}"
          } // env
        } // inside() container
        dir ("centos${OS}") {
          deleteDir()
          // save the RPMs and the CDK
          sh """
            cp ../packaging/target-centos${OS}/*.rpm .
            XZ_OPT="-1 -v -T4" tar Jcf cdksnapshot-C${OS}.tar -h -C ../exports .
            """
          hw_mappings = new JsonSlurper().parseText(loc_hw_mappings)
          // Generate metadata
          writeFile file: '.jenkins_metadata.json', text: groovy.json.JsonOutput.toJson([
            'branch_name': env.BRANCH_NAME,
            'git_commit': scmVars.GIT_COMMIT,
            'original_build': env.BUILD_NUMBER,
            'os': OS,
            'hw_mappings': hw_mappings,
          ])
        }
      } // Build Main
      // Save off stuff we need later, but end user doesn't want
      dir ("releng") {
        sh "mv ../centos${OS}/cdksnapshot-C${OS}.tar ."
          // Stash includes cdksnapshot and jenkins/runtime
          stash includes: "cdksnapshot-C${OS}.tar, jenkins/runtime/**", name: "support-C${OS}"
      }
      dir ("tests") {
        sh "rm -rf c++tests*"
      }
      stash includes: "tests/**", name: "tests-C${OS}" // C6 will never be used
      archiveArtifacts artifacts: "centos${OS}/**", fingerprint: true, onlyIfSuccessful: true
      // Have the releng/support and cdksnapshot be a deliverable for downstream support
      dir ('support') {
        deleteDir()
        unstash name: "support-C${OS}"
        sh "XZ_OPT='-1 -v -T4' tar Jcf jenkins_runtime.tar jenkins/"
      }
      def arts = "support/cdksnapshot-C${OS}.tar"
      if (7 == OS) arts += ", support/jenkins_runtime.tar"
      archiveArtifacts artifacts: arts, fingerprint: true, onlyIfSuccessful: true
      if (7 == OS && params["Build Main"]) {
        // These will be a mess for now - see JENKINS-47669
        // Compiler warnings
        warnings consoleParsers: [[parserName: 'GNU C Compiler 4 (gcc)']],
          defaultEncoding: '',
          excludePattern: '.*/rpmbuild/.*',
          healthy: '',
          includePattern: '',
          messagesPattern: '',
          unHealthy: '',
          useStableBuildAsReference: true
        // TODO list
        openTasks defaultEncoding: '',
          excludePattern: '**/kernel-headers*/**, **/zynq_kernel/**, **/tools/astyle/*, build/**, exports/**, **/exports/**, imports/**, **/imports/**, packaging/**, project-registry/**, runtime/foreign/**, prerequisites/**',
          healthy: '',
          high: 'BUG',
          low: 'XXX',
          normal: 'TODO,FIXME',
          pattern: '**/*.c, **/*.cc, **/*.cpp, **/*.h, **/*.hh',
          unHealthy: '',
          useStableBuildAsReference: true
      } // C7 only
    } // node
  } // closure
} // transformMainBuildStep

/*
████████ ███████    ██████  ███████ ██████   ██████ ██████  ██   ██
   ██    ██         ██   ██ ██      ██   ██ ██      ██   ██ ██  ██
   ██    █████      ██████  ███████ ██████  ██      ██   ██ █████
   ██    ██         ██   ██      ██ ██      ██      ██   ██ ██  ██
   ██    ██ ███████ ██████  ███████ ██       ██████ ██████  ██   ██
*/
// Returns a closure that will build BSP CDK RPMs for a given RCC or HDL platform (includes node assignment)
Closure transformBSPCDKBuildStep(in_pf) {
  return {
    def pf = in_pf
    node ('rpmbuild && docker-C7') {
      clear_workspace()
      /* def scmVars = */ cached_checkout this, scm // could be on a new node / workspace
      // Bring in special BSPs etc here:
      // Get hardware mappings from hw_mappings global variable
      def rcc_platforms = hw_mappings[pf]
      // Take the array of HDL platforms that go with the current RCC platform and turn them into a string with the current RCC platform appended
      // e.g. turn [zed, e3xx] to "zed:xilinx13_4 e3xx:xilinx13_4"
      def build_targets = ""
      if (rcc_platforms) {
        build_targets = rcc_platforms.join(":${pf} ") + ":${pf}"
      }
      bring_in_external_repos(pf, "${env.NODE_NAME}:${env.WORKSPACE}", rcc_platforms)
      // Bring in stashed built prereqs, both for dev host and for this platform
      dir('prerequisites') { unstash "prereq-C7" }
      // Bring in stashed CDK since we need it to build the rcc workers in core
      dir('support') { unstash "support-C7" }
      // Restore the dev platform CDK from "main"
      dir('exports') { sh "tar xf ../support/cdksnapshot-C7.tar" }
      def RR=""
      if (params["RPM Release"].trim())
        RR="export RPM_RELEASE=${params["RPM Release"].trim()};"
      // Starting in 1.4, we don't have prereq RPMs any more, so the base image *is* the standard base image now.
      docker.image(docker_image_base+"7").inside("${mount_sudoers} ${mount_tools} --name ${env.BUILD_TAG}-bsprpm-${pf}") {
        // not needed any more? sh "sudo updatedb -e /root"
        sh """set +x
          # Import BSPs
          shopt -s nullglob
          for bsp in projects/bsps/*; do
            echo "Importing BSP \${bsp} into OCPI_PROJECT_PATH"
            export OCPI_PROJECT_PATH="\$(pwd)/\${bsp}:\${OCPI_PROJECT_PATH}"
          done
          set -x
          # Do the build
          ${RR}
          # build the framework, which needs dev host prereqs (e.g. patchelf)
          make Platform=${pf}
          # build driver
          make driver Platform=${pf}
          # build rcc workers in core, which needs dev host cdk, which is needed for runtime RPM
          make projects Projects=core Platform=${pf}
          make rpm Platform=${pf} RpmVerbose=1 OcpiRelease=\$RPM_RELEASE
          hdl_platforms="${build_targets}"
          if [ -n "\$hdl_platforms" ]; then
            export OCPI_HDL_PLATFORM_PATH=\$(echo \$(make showhwdir Platform=${pf}) | tr ' ' ':'):\$OCPI_HDL_PLATFORM_PATH
            make rpm Platform="\$hdl_platforms" RpmVerbose=1 OcpiRelease=\$RPM_RELEASE
            # Moving rpms to the correct location so they will be archived by jenkins
            for p in \$hdl_platforms; do
              # This is needed to strip out hardware platform in case <hw-platform>:<sw-platform> is given
              p=\$(echo \$p | cut -d: -f1)
              cp ./packaging/target-\$p/*.rpm ./packaging/target-${pf} || :
            done
          fi
          """
      } // inside
      // ALL OSs get the BSP RPMs.
      TargetOSs.each { os ->
        dir("centos${os}") { sh "cp ../packaging/target-${pf}/*.rpm ." }
      }
      archiveArtifacts artifacts: 'centos*/*.rpm', fingerprint: true, onlyIfSuccessful: true
    } // node
  } // closure
} // transformBSPCDKBuildStep

/*
████████ ███████    ██████  ███████ ██████
   ██    ██         ██   ██ ██      ██   ██
   ██    █████      ██████  ███████ ██████
   ██    ██         ██   ██      ██ ██
   ██    ██ ███████ ██████  ███████ ██
*/
// Returns a closure that will build BSP RPMs for a given platform (includes node assignment)

// NOTE: Currently unused in non-picoflexor branches - may be needed after build2 merge into develop - not sure... still in flux... check post-1.4 (FIXME) AV-4373

Closure transformBSPBuildStep(in_pf) {
  return {
    def pf = in_pf
    node ('rpmbuild') {
      clear_workspace()
      /* def scmVars = */ cached_checkout this, scm // could be on a new node / workspace
      bring_in_external_repos(pf, "${env.NODE_NAME}:${env.WORKSPACE}")
      def bsp_target = pf
      if (branch_config.hdl_platforms[pf]?.shared_bsp)
        bsp_target = branch_config.hdl_platforms[pf]?.shared_bsp
      lock("rpmbuild-${env.NODE_NAME}") {
        // Workaround for JENKINS-43077 (as is any RR to get cleaned up later)
        def RR=""
        if (params["RPM Release"].trim())
          RR="export RPM_RELEASE=${params["RPM Release"].trim()};"
        withEnv(["RPM_RELEASE=${params["RPM Release"].trim()}"]) {
          dir ('releng') {
            sh "${RR} make cleanrpmbuild bsp-${bsp_target}"
          } // releng
          // ALL OSs get the BSP RPMs. (AV-4735)
          TargetOSs.each { os ->
            dir("centos${os}") { sh "cp -l ../releng/*.rpm ." }
          }
          archiveArtifacts artifacts: 'centos*/*.rpm', fingerprint: true, onlyIfSuccessful: true
        } // RPM_RELEASE env
      } // lock
    } // node
  } // closure
} // transformBSPBuildStep

/*
████████ ███████ ████████  █████  ██████  ██████   █████  ██      ██
   ██    ██         ██    ██   ██ ██   ██ ██   ██ ██   ██ ██      ██
   ██    █████      ██    ███████ ██████  ██████  ███████ ██      ██
   ██    ██         ██    ██   ██ ██   ██ ██   ██ ██   ██ ██      ██
   ██    ██ ███████ ██    ██   ██ ██   ██ ██████  ██   ██ ███████ ███████
*/
// Returns a closure that will tar up almost everything installed
// Skips debuginfo, documentation, and HW (SD Card) packages
Closure transformTarballBuildStep(in_OS) {
  return {
    def OS = in_OS
    node('master') {
      clear_workspace()
      if (!params["Build Main"]) {
        def old = previous_changesets.buildNumber
        if (old != 0) {
          // cp -v /tmp/tempo_solutions/ocpi_rpms_extracted_C${OS}.tar .
          copyArtifacts (
            projectName: env.JOB_NAME,
            filter: "support/ocpi_rpms_extracted_C${OS}.tar",
            fingerprintArtifacts: false,
            target: ".",
            flatten: true,
            selector: specific("${old}")
          )
        } else {
          error_email += "\nCould not find old build to copy extracted RPMs from!\n"
          error "Could not find old build to copy extracted RPMs from!"
        }
      } else { // Not cheating
        // Get the RPMs
        unarchive mapping: ["centos${OS}/**" : '.']
        dir ('ws') {
          sh """#!/bin/bash -el
            set +x
            mv ../centos${OS} .
            mv centos${OS} rpms
            rm -vrf rpms/*debuginfo*rpm rpms/opencpi-doc-*rpm rpms/opencpi-hw-platform-*rpm
            for rpm in rpms/*rpm; do
              set -x
              rpm2cpio \$rpm | cpio -di
              set +x
            done
            rm -rf rpms
            echo "Fixing absolute symlinks that point to /opt/opencpi and making them relative"
            for l in \$(find -type l); do
              if ! grep -q "\$(pwd)" <(readlink -m \$l); then
                echo -n "Checking "
                ls -l \$l | cut -f9- -d' '
                if egrep -q '^/opt/opencpi' <(readlink -m \$l); then
                  echo -n "Fixed "
                  mv \${l}{,.old}
                  ln -sf \$(realpath -mL --relative-to=\$(dirname \$l) ".\$(readlink -m \${l}.old)") \$l
                  ls -l \$l | cut -f9- -d' '
                fi
              fi
            done
            export XZ_OPT="-1 -v -T4"
            time tar Jcf ../ocpi_rpms_extracted_C${OS}.tar .
          """.trim()
          deleteDir()
        }
      } // Build Main
      dir ('support') {
        sh "mv ../ocpi_rpms_extracted_C${OS}.tar ."
      }
      archiveArtifacts artifacts: "support/ocpi_rpms_extracted_C${OS}.tar", fingerprint: true, onlyIfSuccessful: true
    } // node
  } // closure
} // transformTarballBuildStep

/*
████████ ███████     █████  ██    ██ ████████ ███████ ███████ ████████ ███████
   ██    ██         ██   ██ ██    ██    ██    ██      ██         ██    ██
   ██    █████      ███████ ██    ██    ██    █████   ███████    ██    ███████
   ██    ██         ██   ██  ██  ██     ██    ██           ██    ██         ██
   ██    ██ ███████ ██   ██   ████      ██    ███████ ███████    ██    ███████
*/
// Returns a closure that will run the AV self-tests on a given OS
Closure transformAVTestsStep(in_OS) {
  return {
    def OS = in_OS
    def test_to_run = 'av-tests'
    // NOTE: In the future, these may be broken out into separate containers
    node ('docker-C'+OS) {
      def results_dir = "${env.WORKSPACE}/test_results/centos${OS}/${test_to_run}"
      clear_workspace() // This cannot be inside container (JENKINS-41894)
      try {
        unstash "tests-C${OS}"
        test_setup(OS).inside("${mount_sudoers} ${mount_tools} ${docker_options} --name ${env.BUILD_TAG}-C${OS}-${test_to_run}") {
          // JENKINS-33510 dir ("${results_dir}") { sh "pwd" } // Create results directory
          sh "mkdir -p ${results_dir}"
          sh """
            #!/bin/bash -el
            # Install the core project to have host centosX (AV-4440)
            ocpi-copy-projects workspace
            """.trim()
          /* This test was moved to BuildHDLCore
          timeout(5) { // 5 minutes
            if (fileExists("tests/pytests/run_pytests.sh")) {
              sh """
                #!/bin/bash -el
                set -o pipefail
                OCPI_LOG_LEVEL=9 ocpitest python | tee ${results_dir}/pytests.log 2>&1
                """.trim()
            }
          } // timeout
          */
          timeout(time: 3, activity: true) { // 3 minute hang restriction
            sh """
              #!/bin/bash -el
              set -o pipefail
              OCPI_LOG_LEVEL=9 ocpitest av | tee ${results_dir}/avtests.log 2>&1
              """.trim()
          } // timeout
          timeout(time: 3, activity: true) { // 3 minute hang restriction
            sh """
              #!/bin/bash -el
              set -o pipefail
              OCPI_LOG_LEVEL=9 ocpitest ocpidev | tee ${results_dir}/ocpidevtests.log 2>&1
              """.trim()
          } // timeout
        } // inside() container
      } catch (err) {
        error "Error in CentOS${OS} ${test_to_run}: ${err}"
      } finally {
        archiveArtifacts allowEmptyArchive: true, artifacts: 'test_results/**', defaultExcludes: false, fingerprint: false, onlyIfSuccessful: false
      }
    } // node
  } // closure
} // transformAVTestsStep

/*
████████ ███████     ██████ ████████ ███████ ███████ ████████ ███████
   ██    ██         ██         ██    ██      ██         ██    ██
   ██    █████      ██         ██    █████   ███████    ██    ███████
   ██    ██         ██         ██    ██           ██    ██         ██
   ██    ██ ███████  ██████    ██    ███████ ███████    ██    ███████
*/
// Returns a closure that will run ctests on a given OS (with and without Valgrind)
Closure transformCTestsStep(in_OS) {
  return {
    def OS = in_OS
    def test_to_run = 'ctests'
    node ('docker-C'+OS) {
      clear_workspace() // nuke the workspace
      try {
        def results_dir = "${env.WORKSPACE}/test_results/centos${OS}/${test_to_run}"
        dir ("${results_dir}/valgrind-reports") { sh "pwd" } // Create results directories
        dir ("${results_dir}/${test_to_run}_output_logs") { sh "pwd" }
        dir ("${results_dir}/${test_to_run}_output_logs_withvalgrind") { sh "pwd" }
        test_setup(OS).inside("${mount_tools} ${docker_options} --name ${env.BUILD_TAG}-C${OS}-${test_to_run}") {
          parallel "CentOS ${OS}: ${test_to_run} (no valgrind)": {
            // The shell command must have bash on first line or trim call JENKINS-47787
            sh """
              #!/bin/bash -el
              set -o pipefail
              export DIR=${results_dir}/${test_to_run}_output_logs
              /opt/opencpi/cdk/centos${OS}/bin/ctests/run_tests.sh | tee ${results_dir}/${test_to_run}.log 2>&1
              """.trim()
          },
           "CentOS ${OS}: ${test_to_run} (with valgrind)": {
            sh """
              #!/bin/bash -el
              set -o pipefail
              export DIR=${results_dir}/${test_to_run}_output_logs_withvalgrind
              # sudo chmod g-s /opt/opencpi/cdk/centos${OS}/bin/ctests/*
              export VG='valgrind --leak-check=full --show-possibly-lost=yes --show-reachable=yes --xml=yes --xml-file=${results_dir}/valgrind-reports/${test_to_run}-%p-valgrind.xml'
              /opt/opencpi/cdk/centos${OS}/bin/ctests/run_tests.sh | tee ${results_dir}/${test_to_run}_withvalgrind.log 2>&1
              # Bug in valgrind on C7
              # https://bugs.kde.org/show_bug.cgi?id=353660
              find ${results_dir}/valgrind-reports/ -name *-valgrind.xml | \
              xargs -r -n1 perl -i -pe '/<auxwhat>/ && (s/&/&amp;/g, s^<(?!/?auxwhat)^&lt;^g, s^(?<!auxwhat)>^&gt;^g )'
              """.trim()
          },
          failFast: quick_failure_mode
        } // inside() container
      } catch (err) {
        error "Error in CentOS${OS} ${test_to_run}: ${err}"
      } finally {
        archiveArtifacts allowEmptyArchive: true, artifacts: 'test_results/**', defaultExcludes: false, fingerprint: false, onlyIfSuccessful: false
        // Valgrind warnings
        publishValgrind (
          failBuildOnInvalidReports: false,
          failBuildOnMissingReports: false,
          pattern: '**/*-valgrind.xml',
          publishResultsForAbortedBuilds: true,
          publishResultsForFailedBuilds: true
        )
      }
    } // node
  } // closure
} // transformCTestsStep

/*
████████ ███████    ██████  ██████  ███████ ████████ ███████ ███████ ████████ ███████
   ██    ██         ██   ██ ██   ██ ██         ██    ██      ██         ██    ██
   ██    █████      ██   ██ ██   ██ ███████    ██    █████   ███████    ██    ███████
   ██    ██         ██   ██ ██   ██      ██    ██    ██           ██    ██         ██
   ██    ██ ███████ ██████  ██████  ███████    ██    ███████ ███████    ██    ███████
*/
// Returns a closure that will run ocpi dds tests on a given OS (with and without Valgrind)
Closure transformDDSTestsStep(in_OS) {
  return {
    def OS = in_OS
    def test_to_run = 'ddstests'
    node ('docker-C'+OS) {
      clear_workspace() // nuke the workspace
      def results_dir = "${env.WORKSPACE}/test_results/centos${OS}/${test_to_run}"
      dir ("${results_dir}/valgrind-reports") { sh "pwd" } // Create results directory
      try {
        // Removed: ${mount_sudoers}
        test_setup(OS).inside("${mount_tools} ${docker_options} --name ${env.BUILD_TAG}-C${OS}-${test_to_run}") {
          parallel "CentOS ${OS}: ${test_to_run} (no valgrind)": {
            // The shell command must have bash on first line or trim call JENKINS-47787
            sh """
              #!/bin/bash -el
              set -o pipefail
              export ITER=500000
              /opt/opencpi/cdk/centos${OS}/bin/cxxtests/ocpidds -t \${ITER} 2>&1 >${results_dir}/${test_to_run}_stdout.log | tee ${results_dir}/${test_to_run}.log
              xz --fast -v ${results_dir}/${test_to_run}_stdout.log
              """.trim()
          }, "CentOS ${OS}: ${test_to_run} (with valgrind)": {
            sh """
              #!/bin/bash -el
              set -o pipefail
              # sudo chmod g-s /opt/opencpi/cdk/centos${OS}/bin/cxxtests/ocpidds
              mkdir -p /tmp/${test_to_run}
              export VG='valgrind --leak-check=full --show-possibly-lost=yes --show-reachable=yes --xml=yes --xml-file=${results_dir}/valgrind-reports/${test_to_run}-%p-valgrind'
              export ITER=10000
              \${VG} /opt/opencpi/cdk/centos${OS}/bin/cxxtests/ocpidds -t \${ITER} 2>&1 >${results_dir}/${test_to_run}_withvalgrind_stdout.log | tee ${results_dir}/${test_to_run}_withvalgrind.log
              xz --fast -v ${results_dir}/${test_to_run}_withvalgrind_stdout.log
              # Bug in valgrind on C7
              # https://bugs.kde.org/show_bug.cgi?id=353660
              find ${results_dir}/valgrind-reports/ -name *-valgrind.xml | \
              xargs -r -n1 perl -i -pe '/<auxwhat>/ && (s/&/&amp;/g, s^<(?!/?auxwhat)^&lt;^g, s^(?<!auxwhat)>^&gt;^g )'
              """.trim()
          },
          failFast: quick_failure_mode
        } // inside() container
      } catch (err) {
        error "Error in CentOS${OS} ${test_to_run}: ${err}"
      } finally {
        archiveArtifacts allowEmptyArchive: true, artifacts: 'test_results/**', defaultExcludes: false, fingerprint: false, onlyIfSuccessful: false
        // Valgrind warnings
        publishValgrind (
          failBuildOnInvalidReports: false,
          failBuildOnMissingReports: false,
          pattern: '**/*-valgrind.xml',
          publishResultsForAbortedBuilds: true,
          publishResultsForFailedBuilds: true
        )
      }
    } // node
  } // closure
} // transformDDSTestsStep

/*
████████ ███████     ██████ ████████ ███████ ███████ ████████ ███████
   ██    ██         ██         ██    ██      ██         ██    ██
   ██    █████      ██   ███   ██    █████   ███████    ██    ███████
   ██    ██         ██    ██   ██    ██           ██    ██         ██
   ██    ██ ███████  ██████    ██    ███████ ███████    ██    ███████
*/
// Returns a closure that will run gtests on a given OS
Closure transformGTestsStep(in_OS) {
  return {
    def OS = in_OS
    def test_to_run = 'gtests'
    node ('docker-C'+OS) {
      clear_workspace() // nuke the workspace
      def results_dir = "${env.WORKSPACE}/test_results/centos${OS}/${test_to_run}"
      dir (results_dir) { sh "pwd" } // Create results directory
      try {
        test_setup(OS).inside("${mount_sudoers} ${mount_tools} ${docker_options} --name ${env.BUILD_TAG}-C${OS}-${test_to_run}") {
          // The shell command must have bash on first line or trim call JENKINS-47787
          sh """
            #!/bin/bash -el
            set -o pipefail
            /opt/opencpi/cdk/centos${OS}/bin/cxxtests/ocpitests | tee ${results_dir}/${test_to_run}.log 2>&1
            /opt/opencpi/cdk/centos${OS}/bin/ocpitest load-drivers | tee ${results_dir}/../load-drivers.log 2>&1
            """.trim()
        } // inside() container
      } catch (err) {
        error "Error in CentOS${OS} ${test_to_run}: ${err}"
      } finally {
        archiveArtifacts allowEmptyArchive: true, artifacts: 'test_results/**', defaultExcludes: false, fingerprint: false, onlyIfSuccessful: false
      }
    } // node
  } // closure
} // transformGTestsStep

/*
████████ ███████    ███    ███  █████  ███    ██ ██ ███████ ███████ ███████ ████████
   ██    ██         ████  ████ ██   ██ ████   ██ ██ ██      ██      ██         ██
   ██    █████      ██ ████ ██ ███████ ██ ██  ██ ██ █████   █████   ███████    ██
   ██    ██         ██  ██  ██ ██   ██ ██  ██ ██ ██ ██      ██           ██    ██
   ██    ██ ███████ ██      ██ ██   ██ ██   ████ ██ ██      ███████ ███████    ██
*/
// Returns a closure that compare and report if RPMs are shipping different files
Closure transformManifestBuildStep(in_OS) {
  return {
    def OS = in_OS
    def old = previous_changesets.buildNumber
    def new_ = env.BUILD_NUMBER
    if (old != 0) {
      node('master') {
        try {
          clear_workspace()
          def results_dir = "test_results/centos${OS}/rpm_manifest/"
          dir (results_dir) { sh "pwd" } // Create results directory
          dir ('old_rpms') {
            copyArtifacts (
              projectName: env.JOB_NAME,
              filter: "centos${OS}/**",
              fingerprintArtifacts: false,
              target: ".",
              flatten: true,
              selector: specific("${old}")
            )
          }
          dir ('new_rpms') {
            unarchive mapping: ["centos${OS}/**" : '.']
            // Flatten the rpms
            sh "find . -name '*.rpm' -print0 | xargs -0 mv --target-directory=."
          }
          sh """
            set +x
            rm *_rpms/angryviper-ide*.rpm # Not IDE RPMs
            rpm -qlp old_rpms/*rpm | grep -v /usr/lib/debug/ | sort -u > ${results_dir}B${old}_files.txt
            rpm -qlp new_rpms/*rpm | grep -v /usr/lib/debug/ | sort -u > ${results_dir}B${new_}_files.txt
            (cd old_rpms; ls -alF *rpm) >> ${results_dir}B${old}_info.txt
            (cd new_rpms; ls -alF *rpm) >> ${results_dir}B${new_}_info.txt
            diff -u ${results_dir}B{${old},${new_}}_files.txt 2>${results_dir}B${old}_vs_B${new_}_err.txt >${results_dir}B${old}_vs_B${new_}.txt || :
            if [ -s ${results_dir}B${old}_vs_B${new_}.txt ]; then
              echo "Difference(s) found."
            else
              echo "No differences found."
              rm ${results_dir}B${old}_vs_B${new_}.txt
            fi
            if [ -s ${results_dir}B${old}_vs_B${new_}_err.txt ]; then
              echo "Errors running diff:"
              cat ${results_dir}B${old}_vs_B${new_}_err.txt
            else
              rm ${results_dir}B${old}_vs_B${new_}_err.txt
            fi
            rm -rf {new,old}_rpms
            """
          dir (results_dir) {
            def rpt_file
            if (fileExists("B${old}_vs_B${new_}.txt"))
              rpt_file = "B${old}_vs_B${new_}.txt"
            if (fileExists("B${old}_vs_B${new_}_err.txt"))
              rpt_file = "B${old}_vs_B${new_}_err.txt"
            if (rpt_file) {
              def file_text = "${env.BUILD_URL}\n" + readFile(rpt_file)
              def recip = branch_config.notifications.join(',')
              recip += (env.BRANCH_NAME in branch_config.team_notify)? ",${branch_config.team_email}":''
              if (recip.trim() != '""')
                emailext body: file_text, subject: "RPM Manifest Change (C${OS}): ${env.BRANCH_NAME} #${new_}", to: recip
            }
          } // dir
        } catch (err) {
          error "Error in CentOS${OS} RPM Manifest: ${err}"
        } finally {
          archiveArtifacts allowEmptyArchive: true, artifacts: 'test_results/**', defaultExcludes: false, fingerprint: false, onlyIfSuccessful: false
        }
      } // node
    } // if non-zero previous
  } // closure
} // transformManifestBuildStep

/*
████████ ███████    ██████   ██████  ██████ ███████ ██   ██  █████  ███    ███ ██████  ██      ███████
   ██    ██         ██   ██ ██      ██      ██       ██ ██  ██   ██ ████  ████ ██   ██ ██      ██
   ██    █████      ██████  ██      ██      █████     ███   ███████ ██ ████ ██ ██████  ██      █████
   ██    ██         ██   ██ ██      ██      ██       ██ ██  ██   ██ ██  ██  ██ ██      ██      ██
   ██    ██ ███████ ██   ██  ██████  ██████ ███████ ██   ██ ██   ██ ██      ██ ██      ███████ ███████
*/
// Returns a closure that will run RCC Examples on a given OS (with and without Valgrind)
/* Note: Each test in runtests will create a logfile, but we won't capture it if a test fails,
         so we capture an overall log as well. If all tests pass, we delete the extra logs. */
Closure transformRCCExampleStep(in_OS) {
  return {
    def OS = in_OS
    def test_to_run = 'rccexamples'
    node ('docker-C'+OS) {
      clear_workspace() // nuke the workspace
      def results_dir = "${env.WORKSPACE}/test_results/centos${OS}/${test_to_run}"
      dir ("${results_dir}/valgrind-reports") { sh "pwd" } // Create results directory
      try {
        // Removed: ${mount_sudoers}
        test_setup(OS).inside("${mount_tools} ${docker_options} --name ${env.BUILD_TAG}-C${OS}-${test_to_run}") {
          sh """
            #!/bin/bash -el
            # Install the core and assets project
            ocpi-copy-projects workspace
            # Build base_comps; the minimum needed
            ocpidev build --rcc -d workspace/assets/components/base_comps
            # Do the build (TODO: ocpidev command to build all applications?)
            ocpidev build -d workspace/assets/applications/
            """.trim()
          // Unlike other tests, these tests will build their own logs
          parallel "CentOS ${OS}: ${test_to_run} (no valgrind)": {
            // The shell command must have bash on first line or trim call JENKINS-47787
            sh """
              #!/bin/bash -el
              make -C workspace/rccexamples/applications/ runtests | tee ${results_dir}/${test_to_run}_all.log 2>&1
              """.trim()
          }, "CentOS ${OS}: ${test_to_run} (with valgrind)": {
            sh """
              #!/bin/bash -el
              # sudo chmod g-s /opt/opencpi/cdk/centos${OS}/bin/ocpirun
              export OcpiRunBefore='valgrind --leak-check=full --show-possibly-lost=yes --show-reachable=yes --xml=yes --xml-file=${results_dir}/valgrind-reports/${test_to_run}-%p-valgrind.xml'
              make -C workspace/rccexamples/applications/ runtests | tee ${results_dir}/${test_to_run}_all_withvalgrind.log 2>&1
              # Bug in valgrind on C7
              # https://bugs.kde.org/show_bug.cgi?id=353660
              find ${results_dir}/valgrind-reports/ -name *-valgrind.xml | \
              xargs -r -n1 perl -i -pe '/<auxwhat>/ && (s/&/&amp;/g, s^<(?!/?auxwhat)^&lt;^g, s^(?<!auxwhat)>^&gt;^g )'
              """.trim()
	      },
          failFast: quick_failure_mode
          // Collect logs to here
          sh "find workspace/rccexamples -name *_*.log | xargs -r cp --target-directory=${results_dir}/"
          // If individual logs came out, delete extra ones
          sh "rm -f ${results_dir}/${test_to_run}_all*.log"
        } // inside() container
      } catch (err) {
        error "Error in CentOS${OS} ${test_to_run}: ${err}"
      } finally {
        archiveArtifacts allowEmptyArchive: true, artifacts: 'test_results/**', defaultExcludes: false, fingerprint: false, onlyIfSuccessful: false
        // Valgrind warnings
        publishValgrind (
          failBuildOnInvalidReports: false,
          failBuildOnMissingReports: false,
          pattern: '**/*-valgrind.xml',
          publishResultsForAbortedBuilds: true,
          publishResultsForFailedBuilds: true
        )
      }
    } // node
  } // closure
} // transformRCCExampleStep

/*
██████  ██    ██ ██ ██      ██████  ██████  ██████  ███████
██   ██ ██    ██ ██ ██      ██   ██ ██   ██ ██   ██ ██
██████  ██    ██ ██ ██      ██   ██ ██████  ██   ██ █████
██   ██ ██    ██ ██ ██      ██   ██ ██      ██   ██ ██
██████   ██████  ██ ███████ ██████  ██      ██████  ██
*/
// Returns a closure that will build PDFs
Closure BuildPDF() {
  return {
    node('master') {
      clear_workspace()
      cached_checkout this, scm
      // Get all sub-repos (try to match branch name first)
      branch_config.hdl_platforms.each { pf, dat ->
        if (dat.bsp_repo && dat.enabled) {
          echo "Checking out BSP for ${pf}"
          dir("projects/bsps/${pf}") {
            clear_workspace()
            checkout_paired_repo this, dat.bsp_repo, ["fallback": branch_config.fallback]
          } // dir
        } // has a BSP repo and is enabled (AV-4734)
      } // each platform
      // Spin up a container (the PDF image has a user/group added already for "jenkins" and "opencpi"):
      def dimage = docker.image(docker_image_pdf)
      def RR=""
      if (params["RPM Release"].trim())
        RR="export RPM_RELEASE=${params["RPM Release"].trim()};"
      dimage.inside("${mount_sudoers} --name ${env.BUILD_TAG}-pdfgen") {
        sh """
          cd releng
          ${RR}
          make doc_rpm
          """
      } // inside
      TargetOSs.each { os ->
        dir("centos${os}/") {
          sh "cp ../releng/opencpi-doc-*.noarch.rpm ."
        }
      } // each OS
      archiveArtifacts artifacts: "pdfs/**, centos*/opencpi-doc-*.noarch.rpm", fingerprint: false, onlyIfSuccessful: true
    } // node
  } // closure
} // Build PDF

/*
██████  ██    ██ ██ ██      ██████  ██ ██████  ███████
██   ██ ██    ██ ██ ██      ██   ██ ██ ██   ██ ██
██████  ██    ██ ██ ██      ██   ██ ██ ██   ██ █████
██   ██ ██    ██ ██ ██      ██   ██ ██ ██   ██ ██
██████   ██████  ██ ███████ ██████  ██ ██████  ███████
*/
// Returns a closure that will build IDE
Closure BuildIDE() {
  return {
    // "propagate" means we will fail if IDE build fails
    def ide_job = ''
    try {
      ide_job = build job: "${IDE_Build_Job}/${env.BRANCH_NAME}", parameters: [
        booleanParam(name: 'Clean', value: release_branch),
        string(      name: 'RPM Release', value: params["RPM Release"])
      ], propagate: true, quietPeriod: 0, wait: true
    } catch (err) {
      echo "Failed to launch IDE builder in branch '${env.BRANCH_NAME}': ${err}"
    }
    if ('' == ide_job) {
      echo "Falling back... attempting to build IDE from branch '${branch_config.fallback}'"
      ide_job = build job: "${IDE_Build_Job}/${branch_config.fallback}", parameters: [
        booleanParam(name: 'Clean',       value: release_branch),
        string(      name: 'RPM Release', value: params["RPM Release"])
      ], propagate: true, quietPeriod: 0, wait: true
    }
    // At this point ide_job is a "RunWrapper"
    node {
      // Don't want to do top-level deleteDir in case we have a cozy reusable workspace
      dir ('ide_rpms') {
        deleteDir()
        copyArtifacts (
          projectName: ide_job.getFullProjectName(),
          filter: "**/angryviper-ide-*rpm",
          fingerprintArtifacts: true,
          selector: specific(ide_job.getId()),
          //target: tgt_path,
          flatten: true
        )
      } // dir
      // Copy out the RPMs (AV-4223)
      TargetOSs.each { os ->
        dir ("centos${os}") {
          deleteDir()
          sh "mv ../ide_rpms/*el${os}*rpm ."
        }
        archiveArtifacts artifacts: "centos${os}/*rpm", fingerprint: true, onlyIfSuccessful: true
      } // os
    } // node
  } // closure
} // Build IDE

/*
██   ██ ██████  ██               ██████  ██████  ██████  ███████
██   ██ ██   ██ ██              ██      ██    ██ ██   ██ ██
███████ ██   ██ ██              ██      ██    ██ ██████  █████
██   ██ ██   ██ ██              ██      ██    ██ ██   ██ ██
██   ██ ██████  ███████ ███████  ██████  ██████  ██   ██ ███████
*/
// Returns a closure that will do minimal simulator testing
// Note: Unlike others, this is run ON THE HOST and not in a container!
Closure BuildHDLCore() {
  return {
    node("C7 && hdlbuilder-${Quick_Test_Sim}") { // TODO: Remove the hdlbuilder part
      clear_workspace() // nuke the workspace
      def results_dir = "${env.WORKSPACE}/test_results/centos7/hdl_core/"
      dir (results_dir) { sh "pwd" } // Create results directory
      def sub_stage = "Setup"
      try {
        workspace_setup(7)
        sh """
          set +x; . ./jenv.sh; echo 'jenv.sh imported'; set -x
          # Install the project
          ocpi-copy-projects workspace
          """
// BROKEN??? JENKINS-44141          lock(resource: 'License_Modelsim', quantity: 1) {
          dir ('workspace/core') {
            def jenv="source ../../jenv.sh && "
            sub_stage = "Phase 1: RCC / Primitives"
            parallel "HDL Core Build ⟶ (1) RCC" : { sh "${jenv} ocpidev build --rcc --rcc-platform centos7 | tee ${results_dir}/build_rcc.log 2>&1" },
              "HDL Core Build ⟶ (1) Primitives" : { sh "${jenv} ocpidev build hdl primitives --hdl --hdl-platform ${Quick_Test_Sim} | tee ${results_dir}/build_primitives.log 2>&1" },
              failFast: true
            // if ocpidev platform doesn't do devices: sh "${jenv} ocpidev build hdl devices --hdl --hdl-platform ${Quick_Test_Sim} | tee ${results_dir}/build_devices.log 2>&1"
            sub_stage = "Phase 2: Components / Adapters / Devices"
            parallel "HDL Core Build ⟶ (2) Components" : {
                dir ('components') {
                  sh "source ../../../jenv.sh && ocpidev build --hdl --hdl-platform ${Quick_Test_Sim} | tee ${results_dir}/build_components.log 2>&1"
                }
              },
              "HDL Core Build ⟶ (2) Adapters": { sh "${jenv} ocpidev build hdl library adapters | tee ${results_dir}/build_adapters.log 2>&1" },
              "HDL Core Build ⟶ (2) Devices": { sh "${jenv} ocpidev build hdl library devices | tee ${results_dir}/build_devices.log 2>&1" },
              failFast: true
            sub_stage = "Phase 3: Cards / Platform Devices"
            parallel "HDL Core Build ⟶ (3) Cards": { sh "${jenv} ocpidev build hdl library cards | tee ${results_dir}/build_cards.log 2>&1" },
              "HDL Core Build ⟶ (3) Platform Devices": {
                // TODO: Make this "real" parallel in other libraries
                sh """
                  ${jenv} for pf in \$(find hdl/platforms/ -type d -name devices | cut -f3 -d/); do
                    echo \${pf}:
                    ocpidev build hdl library devices -P \${pf} | tee -a ${results_dir}/build_\${pf}_devices.log 2>&1
                  done
                  """
              },
              failFast: true
            // at this point, we can parallel other projects
            sub_stage = "make hdlplatforms"
            sh "${jenv} make hdlplatforms HdlPlatform=${Quick_Test_Sim} Assemblies= | tee ${results_dir}/build_platforms.log 2>&1"
            sub_stage = "final ocpidev build"
            sh "${jenv} ocpidev build --hdl --hdl-platform ${Quick_Test_Sim} | tee ${results_dir}/build_final.log 2>&1"
          } // dir workspace/core
//          } // license lock

        // Now bring in tests that needed a built core project
        unstash "tests-C7" // Bring /tests/ from the repo
        sub_stage = "Testing"
        parallel failFast: false, // more than one might fail
          "HDL Core Build ⟶ Testing ⟶ bias": {
            dir ('workspace/core') {
              def my_results_dir = "${env.WORKSPACE}/test_results/centos7/hdl_core/"
              sh "source ../../jenv.sh && ocpidev -l components build test bias.test --hdl-platform ${Quick_Test_Sim} | tee ${my_results_dir}/build_test.log 2>&1"
              sh "source ../../jenv.sh && ocpidev run test bias --mode prep_run_verify | tee ${my_results_dir}/runtest.log 2>&1"
            } // dir
          },
          "HDL Core Build ⟶ Testing ⟶ ocpidev": { // ocpidev tests from avide repo (formerly transformOcpiDevTestsStep)
            def my_results_dir = "${env.WORKSPACE}/test_results/centos7/ocpidev-tests"
            dir (my_results_dir) { sh "pwd" } // Create results directory
            sh """
              source ./jenv.sh
              export NO_BUILD=1 # Takes too long for Job 1 - about 40 minutes
              # export OCPI_LOG_LEVEL=8
              export HDL_PLATFORM=${Quick_Test_Sim}
              # test-ocpidev needs these as well if the above is defined:
              export HDL_TARGET=${Quick_Test_Sim}
              cd tests/ocpidev_test
              ./test-ocpidev.sh | tee ${my_results_dir}/test-ocpidev.log 2>&1
              """
          },
          "HDL Core Build ⟶ Testing ⟶ pytests": {
            def my_results_dir = "${env.WORKSPACE}/test_results/centos7/av-tests"
            dir (my_results_dir) { sh "pwd" } // Create results directory
            sh """
              source ./jenv.sh
              export OCPI_LOG_LEVEL=8
              export HDL_PLATFORM=${Quick_Test_Sim}
              cd tests/pytests
              ./run_pytests.sh | tee ${my_results_dir}/pytests.log 2>&1
              """
          }
        // Some drop-ins manipulate the core repo so can't be parallel with above
        sub_stage = "Drop-In Testing"
        results_dir = "${env.WORKSPACE}/test_results/centos7/dropin-tests"
        dir (results_dir) { sh "pwd" } // Create results directory
        sh """
          source ./jenv.sh
          export OCPI_LOG_LEVEL=11
          export HDL_PLATFORM=${Quick_Test_Sim}
          cd tests/ocpidev/
          ./run-dropin-tests.sh | tee ${results_dir}/run.log 2>&1
          """
      } catch (err) {
        error "HDL Core Build failure in sub stage '${sub_stage}': ${err}"
      } finally {
        archiveArtifacts allowEmptyArchive: true, artifacts: 'test_results/**', defaultExcludes: false, fingerprint: false, onlyIfSuccessful: false
      }
    } // node
  } // closure
} // Build HDL Core

/*
████████ ███████    ██████  ███████ ██████  ██       ██████  ██    ██
   ██    ██         ██   ██ ██      ██   ██ ██      ██    ██  ██  ██
   ██    █████      ██   ██ █████   ██████  ██      ██    ██   ████
   ██    ██         ██   ██ ██      ██      ██      ██    ██    ██
   ██    ██ ███████ ██████  ███████ ██      ███████  ██████     ██
*/
// Returns a closure that will deploy to yum repo for a given OS
Closure transformDeployStep(in_OS) {
  return {
    def OS = in_OS
    lock ("rpm-deploy-C${OS}-${env.BRANCH_NAME}") {
      node ('master') {
        clear_workspace()
        // TODO, if needed - figure out how many RPMs get dumped, and then decide to keep last N sets of them and build deltarpms
        // Note: It says cdk_rpms but the IDE is in there too
        unarchive mapping: ["centos${OS}/**" : 'cdk_rpms']
        // Flatten the rpms
        sh "find cdk_rpms -name '*.rpm' -print0 | xargs -0 mv --target-directory=cdk_rpms"
        def final repo_path = "${global_config.yum_base()}${env.BRANCH_NAME}/${OS}"
        // Create target path
        sh """
          if [ -d ${repo_path} ]; then
            echo "Repo already exists for ${env.BRANCH_NAME}/${OS}:"
            ls -alF ${repo_path}
            rm -rf --one-file-system ${repo_path}
          fi
          mkdir -p ${repo_path}
          """
        // Move the RPMs, create metadata, cleanup
        sh """
          mv cdk_rpms/*rpm ${repo_path}
          cd ${repo_path}
          createrepo -v .
          cd ${env.WORKSPACE}
          rm -rf cdk_rpms *.repo
          """
        // Create downloadable .repo file
        writeFile file: "angryviper-jenkins-${env.BRANCH_NAME}.repo",
          text: """\
          [angryviper-jenkins-${env.BRANCH_NAME}]
          name=ANGRYVIPER-Jenkins-${env.BRANCH_NAME}
          baseurl=${global_config.yum_repo_url()}${env.BRANCH_NAME}/\$releasever
          cost=10000
          enabled=0
          gpgcheck=0
          repo_gpgcheck=0
          metadata_expire=5m
          skip_if_unavailable=1
          """.stripIndent()
        archiveArtifacts artifacts: "angryviper-jenkins-${env.BRANCH_NAME}.repo", fingerprint: true, onlyIfSuccessful: true
      } // node
    } // lock
  } // closure
} // transformDeployStep
