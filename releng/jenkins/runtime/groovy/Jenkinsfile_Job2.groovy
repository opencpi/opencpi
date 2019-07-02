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

// TODO: Add CentOS 6 support! (Should be able to extract a platform workspace and re-run just RCC building)

// Editing Notes:
// There are headings in each section that are of a great benfit if you use a minimap
// The "flow" is top to bottom until the UTILITIES header
// Everything beneath that is in support of the main flow, e.g. transformXXX calls

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
// Note: JENKINS-47927 - you cannot use "env" in these.
@Field final licensed_retries = 5 // How many retries to run build command if license needed

/// Docker options:
// We have to mount /sys as read-only into the container for Quartus 17+
@Field final mount_tools = '-v /sys:/sys:ro -v /opt/Xilinx/:/opt/Xilinx/:ro -v /opt/Altera/:/opt/Altera/:ro -v /opt/Modelsim:/opt/Modelsim:ro'
@Field final mount_sudoers = '-v /etc/sudoers:/etc/sudoers:ro'
@Field final docker_options = '--group-add opencpi'

@Field final fake_it = false
@Field final internal_tracing = false

// The default for test platforms is "None" unless this is the "develop" or a release branch.
@Field release_branch = false
release_branch = (env.BRANCH_NAME =~ /^v[\d\.]+$/).matches()
@Field test_platforms = "None\nSimulators\nAll\n"
if ("develop" == env.BRANCH_NAME)
  test_platforms = "All\nSimulators\nNone\n"
if (release_branch)
  test_platforms = "All\nSimulators\nNone\n"

properties properties: [ // This is ugly https://stackoverflow.com/a/35471196
  buildDiscarder(
    logRotator(artifactDaysToKeepStr: '17',
      artifactNumToKeepStr: '',
      daysToKeepStr: '',
      numToKeepStr: '34')), // If this changes, see abort_fast
  disableConcurrentBuilds(),
  parameters([
    string(defaultValue: '', description: 'Upstream job ID.', name: 'Upstream_Job'), // Don't rename variable - used in many places
    booleanParam(defaultValue: false, description: 'Build Assemblies', name: 'Build Asm'), // TODO: Add a field as well?
    // booleanParam(defaultValue: true, description: 'Run Tests', name: 'Run_Tests'),
    choice(choices: test_platforms, description: 'Platform(s) to build and run unit tests on', name: 'Test Platforms'),
    // AV-5359 choice(choices: "Test Native\nForce HDL File IO\nForce RCC File IO", description: 'Override File I/O in unit tests', name: 'Test File IO'),
    string(defaultValue: '', description: 'Target or platform for downstream jobs. Usually left blank.', name: 'Target Platforms'),
    string(defaultValue: '', description: 'RPM Release tag (informational only)', name: 'RPM Release'),
    /* choice(choices: platform_list_text, description: 'Which target(s) / platform(s)?', name: 'platforms'), */
    string(defaultValue: '', description: 'Additional projects to build serially e.g. inactive, another_project_after_inactive, etc', name: 'Additional Projects'),
    booleanParam(defaultValue: false, description: 'Build Assemblies of Additional Projects', name: 'Build Additional Projects Asm'),
    booleanParam(defaultValue: false, description: 'Force Rebuild (Job normally aborts for various reasons, e.g. no applicable source changes)', name: 'Force Rebuild'),
  ]),
  [$class: 'ThrottleJobProperty', categories: [], limitOneJobWithMatchingParams: false, maxConcurrentPerNode: 0, maxConcurrentTotal: 0, paramsToUseForLimit: '', throttleEnabled: false, throttleOption: 'project'],
  [$class: 'EnvInjectJobProperty', info: [loadFilesFromMaster: false, secureGroovyScript: [classpath: [], sandbox: false, script: '']], keepBuildVariables: true, keepJenkinsSystemVariables: true, on: true],
  /* not needed? pipelineTriggers([pollSCM('@hourly')]), */
]

import groovy.json.JsonOutput
@Field branch_config
@Field previous_changesets
@Field error_email
@Field stage_name

@Field dimage
@Field dimages
dimages = [:]
currentBuild.description = "" // Ensure after this point it's at least defined

@Field additional_projects
@Field job1_git_hash
@Field hdl_targets
@Field hdl_platforms
@Field hdl_licenses
@Field bsp_projs
@Field bsp_prereqs
@Field bsp_shares
@Field bsp_queue
// Keep track of vendor repos that we may have stashed (AV-4279)
@Field vendor_stashes
@Field build_assemblies_standard_projs   // Need a mutable copy for below
@Field build_assemblies_additional_projs // Need a mutable copy for below

try { // The whole job is within this block (see "end of job")
// Setup that is in prep of timeouts:
// If we are a release branch, always enable assemblies
build_assemblies_standard_projs = release_branch?true:params["Build Asm"]
build_assemblies_additional_projs = params["Build Additional Projects Asm"]
// On 2018-1-11 8 hours failed with ALST4 at M* and ML605 only at E*
// On 2018-2-15, 16 hours ML605 made it to W*
def timeout_time = (build_assemblies_standard_projs | build_assemblies_additional_projs)?23:8 // TODO fix?
timeout(time: timeout_time, unit: 'HOURS') {

/*
███████ ███████ ████████ ██    ██ ██████
██      ██         ██    ██    ██ ██   ██
███████ █████      ██    ██    ██ ██████
     ██ ██         ██    ██    ██ ██
███████ ███████    ██     ██████  ██
*/
stage_name = 'Setup'
stage (stage_name) {
echo "Job configuration: " + JsonOutput.prettyPrint(JsonOutput.toJson(params))
error_email = "${env.BUILD_URL}\n"
// Quit fast if no Upstream_Job set (e.g. found on repo scan)
if (params.Upstream_Job == '') {

  def final errstr = "Self-aborted (no upstream defined)"
  error_email += "\n\n" + errstr
  currentBuild.result = 'ABORTED'
  currentBuild.description = errstr
  error("No upstream job given.")
}

additional_projects = params["Additional Projects"].tokenize(", ")

// To bring in the snippets, we need to be running on a real node
node('master') {
  def bld_id = 'error'
  try {
    bld_id = params.Upstream_Job as int
  } catch (err) {
    error "Could not convert '${params.Upstream_Job}' to integer!: ${err}"
  }
  dir ('releng') { deleteDir() }
  echo "Fetching 'support/jenkins_runtime.tar' from 'Job_1/${env.BRANCH_NAME}' #${bld_id}"
  copyArtifacts(
    projectName: "Job_1/${env.BRANCH_NAME}",
    filter: "support/jenkins_runtime.tar",
    fingerprintArtifacts: true,
    target: "releng/",
    flatten: true,
    selector: specific(params.Upstream_Job)
  )
  archiveArtifacts allowEmptyArchive: false, artifacts: 'releng/jenkins_runtime.tar'
  dir ('releng') { sh "tar xf jenkins_runtime.tar" } // Some of the imported code REQUIRES this to live in "releng"
  def import_config = load("releng/jenkins/runtime/groovy/lib/import_config.groovy")
  branch_config = import_config()

  previous_changesets = find_last_success()

  abort_fast(true) // defined below

  currentBuild.description += "RPMs from Job 1 #${bld_id}\n"
  if (params["RPM Release"].trim())
    currentBuild.description += "\nRelease ${params["RPM Release"]}\n"
  dir ('rpms') { deleteDir() }
  copyArtifacts(
    projectName: "Job_1/${env.BRANCH_NAME}",
    filter: "centos7/**",
    fingerprintArtifacts: true,
    target: "rpms/",
    flatten: false,
    selector: specific(params.Upstream_Job)
  )

  // Store off the commit hash of Job 1 so we can grab any
  // additional projects from that commit, later, if needed.
  job1_metadata = readJSON file: "rpms/centos7/.jenkins_metadata.json"
  job1_git_hash = job1_metadata.git_commit

  // vendor support (optional)
  copyArtifacts(
    projectName: "Job_1/${env.BRANCH_NAME}",
    filter: "support/bsp_*_vendor.tar",
    optional: true,
    fingerprintArtifacts: false,
    target: "vendor/",
    flatten: true,
    selector: specific(params.Upstream_Job)
  )
  dimage = docker_rpminstalled_image(this, "rpms/centos7")
/* This is useless until JENKINS-48069
  def ocpidev_hdl_targets
  dimage.inside() {
    ocpidev_hdl_targets = readJSON text: sh (script: "bash -lc 'ocpidev show hdl targets --json'", returnStdout: true).trim()
  }
*/
  // Iterate over our configured targets and platforms:
  hdl_targets = [] as Set
  hdl_platforms = [] as Set
  hdl_licenses = [:]
  def req_pfns = params["Target Platforms"].tokenize(", ")
  branch_config.hdl_platforms.each { pfn, dat ->
    if (branch_config.hdl_platforms[pfn].enabled && branch_config.hdl_platforms[pfn].hdl_enabled) {
      if (!params["Target Platforms"] || (pfn in req_pfns)) {
        if (dat?.target_only) {
          hdl_targets << pfn
        } else {
          hdl_platforms << pfn
        }
        if (dat?.base_target)
          hdl_targets << dat.base_target
      } else {
        echo "Target/Platform ${pfn} was skipped - not in requested '${params["Target Platforms"]}'"
      }
      // Capture licensing info. Default is unlimited.
      if (null == dat?.tool_license) dat.tool_license = "unlimited"
      if (dat.tool_license != "unlimited") {
        def lic_entry = branch_config.tools[dat.tool_license]
        if (!lic_entry?.never_license) {
          // Set this platform
          hdl_licenses[pfn] = ["always": lic_entry?.always_license?:false, "lock_name": lic_entry.lock_name, "node_label": lic_entry.node_label, "phases": lic_entry?.phases?:[] ]
          // If this platform is based on an implied platform, extend the license to cover implied. Ex: alst4 implies stratix4 exists (not defined otherwise); they both require license
          if (dat?.base_target && !branch_config.hdl_platforms[dat.base_target])
            hdl_licenses[dat.base_target] = ["always": lic_entry?.always_license?:false, "lock_name": lic_entry.lock_name, "node_label": lic_entry.node_label, "phases": lic_entry?.phases?:[] ]
        }
      } // license
    } // enabled
  } // each branch_config.hdl_platforms
  // Ensure all platforms requested were legit
  req_pfns.each {
    if ( !hdl_platforms.contains(it) && !hdl_targets.contains(it) )
      error "User requesting HDL target/platform that build system does not recognize: ${it}"
  }
  if (params["Target Platforms"])
    currentBuild.description += "Limited to "+req_pfns.sort().join(', ')
  if (build_assemblies_standard_projs)
    currentBuild.description += "\nWith Assemblies"
  if (additional_projects) {
    currentBuild.description += "\nAdditional Projects: "+additional_projects.join(', ')
    if (build_assemblies_additional_projs)
      currentBuild.description += " (With Assemblies)"
    else
      currentBuild.description += " (No Assemblies)"
  }

/*
  // Iterate over ocpidev reported targets:
  def ocpidev_hdl_targets_map = [:]
  ocpidev_hdl_targets.each { vendor, tgts ->
    ocpidev_hdl_targets[vendor].each{ pfn, dat ->
      ocpidev_hdl_targets_map[pfn] = true
    }
  }
  // Report differences: ( http://groovyconsole.appspot.com/script/364002 )
  // Note: We do Set => List because of Jenkins security blocks on Set...?
  // JENKINS-48069 - this worked once, I swear!

  def theirKeys = ocpidev_hdl_targets_map.keySet() as List
  def removedKeys = (theirKeys - hdl_targets - hdl_platforms).sort()
  def addedKeys = ((hdl_targets as List) - theirKeys).sort()
  if (removedKeys) {
    def missing_warning = "WARNING: Build system knows about these targets that are not being built: ${removedKeys.join(', ')}"
    echo missing_warning
    error_email += missing_warning
  }
  if (addedKeys) {
    error "Branch configuration requesting HDL target(s) that build system does not recognize: " + addedKeys.join(", ")
  }
*/
  echo "Going to build for target(s): '${hdl_targets.join(', ')}' and platform(s): '${hdl_platforms.join(', ')}'"

  // Set up bsp_projs
  bsp_projs = [] as Set
  bsp_prereqs = [:]
  bsp_shares = [:]
  bsp_queue = [] as Set
  branch_config.hdl_platforms.each { pfn, dat ->
    if (branch_config.hdl_platforms[pfn].enabled
     /* && branch_config.hdl_platforms[pfn].hdl_enabled */
        && dat?.bsp_repo) {
      bsp_projs << pfn
      // Determine if this platform uses a shared BSP
      def shared_bsp = dat.bsp_repo.replaceAll(/^.*\.bsp\.(.*)\.git/, '$1')
      if (dat.bsp_repo != shared_bsp) {
        echo "Detected that platform ${pfn} uses shared BSP repo '${shared_bsp}'"
        bsp_shares[pfn] = shared_bsp // store off the mapping
      }
      // Determine if this BSP has prereq requirements (AV-4416)
      if (dat?.bsp_prereqs)
        bsp_prereqs[pfn] = dat.bsp_prereqs
    } // enabled
  } // hdl_platforms.each

  vendor_stashes = [] as Set
  findFiles(glob: "vendor/bsp_*_vendor.tar").each {
    // {"directory":false,"length":571725060,"lastModified":1530549134000,"path":"vendor/bsp_picoflexor_s1t6a_vendor.tar","name":"bsp_picoflexor_s1t6a_vendor.tar"}
    def matcher = it.name =~ /bsp_(.*)_vendor.tar/
    if (!matcher.matches()) error "Strange Regex/Glob Mismatch Error - '${it.name}'"
    def vpfn = matcher[0][1] // 0 is first match, within that 0 would hold whole match, 1 is first capture
    if (vpfn in bsp_projs) {
      echo "Found vendor support tarball '${it.name}' for '${vpfn}'. Stashing it for later."
      stash includes: "vendor/bsp_${vpfn}_vendor.tar", name: "vendor-${vpfn}"
      vendor_stashes << vpfn
    } else {
      echo "WARNING: Ignoring vendor support tarball '${it.name}' for '${vpfn}' that wasn't in expected BSP list '${groovy.json.JsonOutput.toJson(bsp_projs)}'"
    }
  }

  // Store off metadata re: what we built and how we did it
  // Jenkins DSL cannot read config.json, so we'll put as much as we can in there too
  writeFile file: '.jenkins_metadata_job2.json', text: groovy.json.JsonOutput.toJson([
    branch_config: branch_config,
    bsp_projs: bsp_projs,
    bsp_shares: bsp_shares,
    hdl_platforms: hdl_platforms,
    hdl_targets: hdl_targets,
    params: params,
  ])
  archiveArtifacts allowEmptyArchive: false, artifacts: '.jenkins_metadata_job2.json', defaultExcludes: false

// This is for testing - bypasses 99% of the job.
if (fake_it) {
  copyArtifacts(
    projectName: JOB_NAME,
    filter: "workspaces/**",
    target: ".",
    flatten: false,
    selector: specific("366") // 366 has xsim, isim, zed
  )
  archiveArtifacts allowEmptyArchive: false, artifacts: 'workspaces/**', defaultExcludes: false
}

} // node
} // end of stage

// Some global variables and general job description

// The way this job works is it constantly reuses this variable:
// See https://github.com/jenkinsci/pipeline-examples/blob/master/pipeline-examples/parallel-from-list/parallelFromList.groovy
// We populate that variable in different ways and then fire it off with a "parallel" block
@Field stepsForParallel = [:]

if (!fake_it) {

/*
██ ███    ██ ███████ ████████  █████  ██      ██
██ ████   ██ ██         ██    ██   ██ ██      ██
██ ██ ██  ██ ███████    ██    ███████ ██      ██
██ ██  ██ ██      ██    ██    ██   ██ ██      ██
██ ██   ████ ███████    ██    ██   ██ ███████ ███████
*/
stage_name = 'Installation'
echo "Waiting for master node"
node ("master") {
  // For convenience, going to only do all Docker image building on master under docker.
  // In the future, going to use more than one machine and may need to split up this
  stage (stage_name) {
    deleteDir() // always start clean
    // Install all projects
    // Note: This will be within the Jenkins disk space NOT the virtual container one
    // However, changes outside of "workspace" are dropped when the container quits,
    //   so we use "reg_projects.sh" to handle the things needed each time somebody spins
    //   up a container in this workspace.

    // Grab any additional projects to build (e.g. inactive) and stash them.
    // Later, we'll unstash and put them in /opt/opencpi/projects, where
    // they'll be treated like any other standard project, and built just by
    // being present there. The standard projects (core, assets, assets_ts) are
    // installed from an RPM, while additional projects don't exist in the RPMs,
    // so need to be grabbed from git.
    if (additional_projects) {
      dir ("temp_add_projs") {
        // We will now look for ALL additional projects in the git commit that
        // generated the RPMs in Job 1. If we can't find them all there, we
        // quit!
        // We'd prefer to do a shallow checkout, but we don't know the depth
        // value to use. If the depth isn't set to a high enough number, we won't
        // find the commit_hash we are interested in. So, unfortunately, we
        // checkout the whole thing.
        checkout(changelog: false, poll: false, scm: [$class: 'GitSCM', branches: [[name: job1_git_hash]], submoduleCfg: [], userRemoteConfigs: scm.userRemoteConfigs])

        //                 *job1_git_hash                  *HEAD
        //                /                               /
        //               /                               /
        //              /                               /
        //  GIT------>[X]---->[ ]---->[ ]---->[ ]---->[ ]

        def missingProjects = ""
        additional_projects.each {
          if (!fileExists("projects/${it}")) {
            missingProjects = missingProjects ? missingProjects + ", ${it}": "${it}"
          }
        }
        if (missingProjects) {
          echo "Could not find Additional Project(s) (${missingProjects}) after syncing to the git hash that generated Job_1's RPMs:"
          echo "--> Job 1 #" + params.Upstream_Job
          echo "--> Branch: " + env.BRANCH_NAME
          echo "--> Commit: " + job1_git_hash
          echo "Executing ls command; is (${missingProjects}) here?"
          sh "ls projects"
          error "Quitting: Additional Project(s) not found."
        }

        additional_projects.each {
          dir('projects') {
            stash includes: "${it}/**", name: "additional_projects__${it}"
          }
        }
      }
    }

    dir ("workspace") {
      gen_jenv()
      def installed_prereqs = [] as Set

      dimage.inside("${mount_tools} ${mount_sudoers} ${docker_options} --name ${env.BUILD_TAG}") {

        // Unstash any additional projects to build (e.g. inactive), and
        // move them to /opt/opencpi/projects/ .
        if (additional_projects) {
          // jenkins user is opencpi group; give it write access here.
          sh 'sudo chmod -R g+w /opt/opencpi/projects'

          additional_projects.each {
            dir('temper') {
              deleteDir()
              unstash "additional_projects__${it}"
              sh "mv ${it} /opt/opencpi/projects/"
              deleteDir()
            }
            echo "Put additional project ${it} into /opt/opencpi/projects"
          }
        }

        sh """#!/bin/bash -el
          . ./jenv.sh; set -x
          rm -f reg_projects.sh
          echo "set +x" >> reg_projects.sh
          # Install all projects and BSPs provided
          # TODO: Would a local registry solve some of our problems?
          ocpi-copy-projects tempo_proj tempo_reg
          # Set up project register script
          cd tempo_proj
          for proj in *; do
            echo "Setting reg_projects to register \${proj}:"
            # Fix Jenkins stash problem: it puts files not symlinks in exports
            echo "make -C \${proj} cleanexports" >> ../reg_projects.sh
            echo "ocpidev register project \${proj}" >> ../reg_projects.sh
          done
          mv * ..
          cd ..
          rm -rf tempo_proj tempo_reg
          chmod a+x reg_projects.sh
          """.trim()
        def prereq_needs_init = true
        bsp_projs.each { pfn ->
          if (bsp_prereqs.containsKey(pfn)) { // Import prereqs as needed AV-4416
            echo "Prereqs for ${pfn} need to be imported"
            // TODO: Should this be in "launch_container" since we're not making a new docker image?
            if (prereq_needs_init) {
              prereq_needs_init = false
              sh """
                echo "sudo chgrp -R opencpi /opt/opencpi/prerequisites/" >> reg_projects.sh
                echo "sudo chmod g+w /opt/opencpi/prerequisites/" >> reg_projects.sh
              """
              copyArtifacts(
                projectName: "Job_1/${env.BRANCH_NAME}",
                filter: "support/prereqs-C7.tar",
                fingerprintArtifacts: false,
                target: "prereq_installer/",
                selector: specific(params.Upstream_Job)
              )
            }
            bsp_prereqs[pfn].each {
              if (!(it in installed_prereqs)) {
                echo "Importing prereq ${it}"
                sh """
                  echo "echo Importing prereq ${it}" >> reg_projects.sh
                  echo "time tar -C /opt/opencpi -xf /workspace/prereq_installer/support/prereqs-C7.tar prerequisites/${it}/" >> reg_projects.sh
                """
                installed_prereqs << it
              } else {
                echo "Prereq ${it} installed previously; skipping"
              }
            } // each prereq
          } // has prereqs
        } // each bsp
        // Find (optional) vendor stuff
        // Also checks if there is a file named "opencpi_required_files*.tar*" at top-level and extracts it
        vendor_stashes.each { vpfn ->
          dir ("vendor/${vpfn}") {
            echo "Unstashing vendor-${vpfn}"
            unstash "vendor-${vpfn}"
            sh """
              tar xf vendor/bsp_${vpfn}_vendor.tar
              for tf in opencpi_required_files*.tar*; do
                tar xf \${tf}
                rm \${tf}
              done
            """
          } // vendor dir
        } // vpfn
        // "Stash" ignores empty directories, so let's make them not empty (AV-3792):
        sh "find . -type d -empty -print0 | xargs -0rt -IXXX touch XXX/.emptydir"
      } // inside() container
    } // dir
    // They say not to use stash for large files... takes about three minutes now :(
    stash includes: "workspace/**", name: 'workspace'
    deleteDir() // Done for now
  } // stage
} // node

/*
██████  ██    ██ ██ ██      ██████  ██ ███    ██  ██████
██   ██ ██    ██ ██ ██      ██   ██ ██ ████   ██ ██
██████  ██    ██ ██ ██      ██   ██ ██ ██ ██  ██ ██   ███
██   ██ ██    ██ ██ ██      ██   ██ ██ ██  ██ ██ ██    ██
██████   ██████  ██ ███████ ██████  ██ ██   ████  ██████
*/

stage_name = 'Building'
stage (stage_name) {
  // TODO: Licensing is BROKEN!!! :(
  /* Here we take into account dependencies to figure out what to build in what order.
  For example: targets 'zynq, stratix4, virtex6' and platforms 'modelsim, isim, xsim, matchstiq_z1, zed, alst4, ml605' should build (time downwards):
  .       zynq      || stratix4 || virtex6 || modelsim || isim || xsim
  .  mq_z1 || zed   ||  alst4   ||  ml605

    Phase 1: {RCC} {ACI Applications} {Primitives}
    Phase 2: {Components} {Adapters} {Devices}
    Phase 3: {Cards} {Platform Devices (hdl/platforms/ any subdir named "devices") }
    Phase 4: {make hdlplatforms} // TODO: "ocpidev build hdl platforms"
    Phase 5: {final pass top-level (optionally with assemblies) and then archive results}

    It's my understanding that after Phase 3 of Core, I should be able to do Phase 1 of Assets.
    Same restriction / limitation after Assets for Assets_TS and then BSPs.
    If assemblies are built, Phase 5 can depend on another Phase 4 (e.g. Assets's assemblies need BSP's platforms).
    TODO: Eventually this should be made more generic in case BSP3 needs BSP2 etc.

    "Time" 1: {Core Phases 1-3}
    < Synchronization Point 1 >
    "Time" 2: {Core Phases 4-5}     {Assets Phases 1-3}
                                    < Synchronization Point 2 (does NOT apply to core) >
    "Time" 3: {Core Phases 4-5 (?)} {Assets Phase 4} {Assets_ts Phases 1-3}
    < Synchronization Point 3 (applies to ALL) >
    "Time" 4:                                        {Assets_ts Phase 4} {BSP1 Phases 1-4} {BSP2 Phases 1-4} {BSP3 Phases 1-4}
    < Synchronization Point 4 (applies to ALL, at this point ONLY assemblies are left to do for everything) >
    "Time" 5:                       {Assets Phase 5} {Assets_ts Phase 5} {BSP1 Phase 5}    {BSP2 Phase 5}    {BSP3 Phase 5}

    THE "TIME" CHART ABOVE IS ONLY VALID WITHIN A GIVEN TARGET/PLATFORM.

    This is handled below by "ordered_build" which handles combining the phases. The individual
    phases are contained in "build_phase"
  */

  stepsForParallel['failFast'] = true // Any platform failing will stop all
  // First queue up platforms without bases (e.g. simulators):
  hdl_platforms.each { hdlpf ->
    if (!branch_config.hdl_platforms[hdlpf]?.base_target)
      stepsForParallel["HDL Platform ${hdlpf}"] = ordered_build("platform", hdlpf)
  } // each hdlpf
  // And all targets and then their platforms:
  hdl_targets.each { hdltgt ->
    stepsForParallel["HDL Target ${hdltgt}"] = {
      // Build the target fully "now"
      ordered_build("target", hdltgt)()
      // Now queue up platforms that are based on this target
      def pf_builds = ["failFast": true]
      hdl_platforms.each { hdlpf ->
        if (hdltgt == branch_config.hdl_platforms[hdlpf]?.base_target) {
          echo "${hdltgt} is done building and now spawning off ${hdlpf} build"
          pf_builds["HDL Platform ${hdlpf}"] = ordered_build("platform", hdlpf)
        }
      } // each hdlpf
      if (pf_builds.size() > 1)
        parallel pf_builds
    } // Closure for stepsForParallel entry
  } // each hdltgt
  parallel stepsForParallel
} // stage

} // fake_it

/*
███████ ███    ██ ██████       ██████  ███████          ██  ██████  ██████
██      ████   ██ ██   ██     ██    ██ ██               ██ ██    ██ ██   ██
█████   ██ ██  ██ ██   ██     ██    ██ █████            ██ ██    ██ ██████
██      ██  ██ ██ ██   ██     ██    ██ ██          ██   ██ ██    ██ ██   ██
███████ ██   ████ ██████       ██████  ██           █████   ██████  ██████
*/

stage_name = 'Launch Tests'
// TODO: We always want to run av-test job if "zed" was in our platforms AV-5519
stage (stage_name) {
  def our_platforms = []
  switch (params["Test Platforms"]) {
    case "Simulators":
      hdl_platforms.each { if (it.find(/sim$/)) our_platforms.push(it) }
      break
    case "All": our_platforms = hdl_platforms; break;
  }

  if (our_platforms.size()) {
    /* Things that launcher_for_testing.groovy expects:
      Job needs to be restricted to master
      Copy Artifacts from Job 2's JOB_BUILD_NUMBER into /testing_support/built_area/
    Config:
      Disable removed jobs
      Names relative to Jenkins Root
      Unstable when deprecated
    Artifacts Archived:
      Only if success
      build_configs/**, built_area/** (needed for config locations and the extracted projects)
    */
    node ('master') {
      echo "Run_Tests set; launching DSL launcher: "+our_platforms.join(", ")
      dir('releng') {
        deleteDir()
        unarchive mapping: ['releng/jenkins_runtime.tar' : 'jenkins_runtime.tar']
        sh "tar xf jenkins_runtime.tar"
      }
      dir ('testing_support') {
        deleteDir()
        dir ('built_area') {
          unarchive mapping: ['**' : '.']
        }
/*        copyArtifacts(
          projectName: "Job_1/${BRANCH_NAME}",
          filter: "support/ocpi_rpms_extracted_C7.tar",
          fingerprintArtifacts: true,
          target: "extracted_cdk",
          selector: specific(params.Upstream_Job) */
      }
      try {
        jobDsl targets: "releng/jenkins/runtime/groovy/launcher_for_testing.groovy",
          lookupStrategy: 'JENKINS_ROOT', // Default I think
          additionalParameters: [
            'JOB_NAME': JOB_NAME,
            'JOB_BUILD_NUMBER': BUILD_NUMBER,
            'HDL_PLATFORMS': our_platforms.join(" "),
            'COMPONENTS': '',
            'GIT_SELECTED_BRANCH_stripped': BRANCH_NAME,
            // 'VALGRIND': false,
            'WORKSPACE': WORKSPACE,
            ],
            removedJobAction: 'DISABLE',
            sandbox: true
        // Now all the test jobs are polling to see if we have finished.
        archiveArtifacts allowEmptyArchive: false, artifacts: 'testing_support/**', defaultExcludes: false, excludes: '**/*@tmp/' // JENKINS-53438
      } catch (err) {
        echo "Error launching tests (${err}); this is not fatal to this job because tests can be generated later."
        currentBuild.result = 'UNSTABLE'
      }
      try {
        if (release_branch) {
          mattermostSend (
            icon: 'https://jenkins.io/images/logos/formal/formal.png', // Doesn't seem to work
            message: "Job2 of `${env.BRANCH_NAME}` ${params["RPM Release"].trim()?'('+params["RPM Release"].trim()+') ':''}completed and beginning to build test assemblies: (<${env.BUILD_URL}|${env.BUILD_NUMBER}>)",
            text: '@all'
          )
        } // release_branch
      } catch (err) {
        echo "Error announcing results; this is not fatal."
      }
    } // node
  } // params check for running test
} // stage
} // timeout
} catch (org.jenkinsci.plugins.workflow.steps.FlowInterruptedException err) {
  // If aborted, who did it? (See Job1 for details)
  def found = false
  err.getCauses().each { e ->
    if (e instanceof jenkins.model.CauseOfInterruption.UserInterruption) {
      def final errstr = "Aborted by ${e.getUser().getDisplayName()}"
      mattermost_send(this, [msg: "${env.JOB_NAME}: Aborted by @${e.getUser().getId()}", icon: "https://image.flaticon.com/icons/svg/631/631681.svg"])
      currentBuild.result = 'ABORTED'
      currentBuild.description += "\n${errstr}"
      found = true
    }
  }
  if (!found) // Must be something else like a timeout...
    report_error(err)
} catch (err) {
  // An error has happened (NOTE: doesn't seem to catch all errors, e.g. java.lang.NoSuchMethodError?)
  report_error(err)
  error "Error in ${stage_name} stage\n\tError: ${err}"
} finally {
  // An error may or may not have happened
  if ('STABLE' == currentBuild.currentResult) { // haven't failed yet
    if (((env.BUILD_NUMBER as int)-1) != previous_changesets.buildNumber) { // last success is not the one before
      echo "Preparing email notifying happiness"
      if (branch_config) {
        def recip = branch_config.notifications.join(',')
        recip += (env.BRANCH_NAME in branch_config.team_notify)? ",${branch_config.team_email}":''
        if (recip.trim() != '""')
          emailext body: error_email, subject: "Job 2 (${env.BRANCH_NAME}) is back to normal", to: recip
      } else {
        echo "Reached 'finally' block without knowing whom to email!\n\n${error_email}"
      }
    }
  }
  // Docker cleanup
  dimages.each{ tgt, dat ->
    // Don't want to use Container.stop() (which also removes) in case there's something useful
    def c_status = dat['done']?"Removing completed":"Stopping and preserving incomplete"
    echo "${c_status} container for ${tgt} (${dat['node']}:${dat['id']})"
    node (dat['node']) {
      if (dat['done']) {
        // Would be nice if we could tell Jenkins this node is "lightweight"
        sh "docker rm -f ${dat['id']} || :"
      } else {
        deleteDir()
        // Before stopping, try to get some files out
        // Not using host_workspace or we would need to parse "docker inspect" - might not get same workspace again!
        sh "docker exec ${dat['id']} mkdir -p /tmp/failure_logs/${tgt} || :"
        sh "docker exec ${dat['id']} rsync -aW --include='*.log' --include='*rpt' --include='*.out' --include='*/' --exclude='*' --prune-empty-dirs /workspace/ /tmp/failure_logs/${tgt}/ || :"
        sh "docker cp -a ${dat['id']}:/tmp/failure_logs . || :"
        archiveArtifacts allowEmptyArchive: true, artifacts: 'failure_logs/**', defaultExcludes: false, excludes: '**/*@tmp/' // JENKINS-53438
        deleteDir()
        sh "docker stop --time 1 ${dat['id']} || :"
        // sh "docker rm ${dat['id']} || :"
      } // done
    } // node
  } // each
} // finally

/*
██    ██ ████████ ██ ██      ██ ████████ ██ ███████ ███████
██    ██    ██    ██ ██      ██    ██    ██ ██      ██
██    ██    ██    ██ ██      ██    ██    ██ █████   ███████
██    ██    ██    ██ ██      ██    ██    ██ ██           ██
 ██████     ██    ██ ███████ ██    ██    ██ ███████ ███████
*/
// Function to decide if we should self-abort the build
// TODO: Combine with Job1 version in external call?
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
    def ign_paths = this_change.paths.findAll{
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
  // Special case - we only keep records for 34. If the last success was more than 34 ago, we cannot
  // reasonably decide whether to abort or not
  if (((env.BUILD_NUMBER as int)-34) > previous_changesets.buildNumber) {
    echo "The last success was so far back that we cannot reasonably decide to abort or not based on changesets. Continuing."
    return
  }
  echo "After processing ${previous_changesets.changeSets.size()} changesets, found ${nojenkins_flags} flagged NoJenkins and ${ignored_path_only_changes} that only modified ignored paths"
  if (!previous_changesets.changeSets.size()) {
    // Job2 special rule: if NEVER built and we have an upstream defined, then run
    if (previous_changesets.buildNumber)
      abort_it "Self-aborted (no changes)", "No changes since last successful build; use 'Force Rebuild' option"
  } else { // there are changesets to parse
    if (nojenkins_flags == previous_changesets.changeSets.size())
      abort_it "Self-aborted (NoJenkins flags)", "All previous ${nojenkins_flags} changesets flagged with NoJenkins"
    if (ignored_path_only_changes == previous_changesets.changeSets.size())
      abort_it "Self-aborted (all paths ignored)", "All previous ${ignored_path_only_changes} changesets only dealt with ignored paths"
    if ((nojenkins_flags + ignored_path_only_changes) == previous_changesets.changeSets.size())
      abort_it "Self-aborted (NoJenkins flags + paths)", "All previous ${nojenkins_flags + ignored_path_only_changes} changesets indicated not to build"
  } // had changesets
} // abort_fast

// Launches a container for a given target/platform, copying from base "target" if applicable
// Currently uses licensed node if set to "always" until JENKINS-44141
void launch_container(hdltgt) {
  // node ("docker-C7") { // JENKINS-44141
  // TODO: In theory, the "target" container could be on a different node than the "platform" that it is based on. Need to handle that eventually!
  node (license_node(0,hdltgt)) { // If "always" then just request that node now
    deleteDir() // always start clean
    def build_cont = "${env.BUILD_TAG}-tgt-${hdltgt}"
    dir ("workspace") {
//        def this_dir = sh (script: "readlink -f .", returnStdout: true).trim()
// -v ${this_dir}:/host_workspace
      def cont_id = dimage.run("""${mount_tools} ${mount_sudoers} --name ${build_cont}""", "/bin/sleeper 2d")
      dimages[hdltgt] = [ "node": env.NODE_NAME, "id": cont_id.id, "done": false ]
      echo "Container launched: ${dimages[hdltgt].node}:${dimages[hdltgt].id}"
      // Two scenarios: 1. Totally new container.
      //                2. Built target container copied to become a platform container.
      if (branch_config.hdl_platforms[hdltgt]?.base_target) {
        echo "Platform ${hdltgt} has a base target of ${branch_config.hdl_platforms[hdltgt].base_target}"
        // Platform has a base
        if (!dimages[branch_config.hdl_platforms[hdltgt].base_target]) {
          // TODO: Eventually if we have multiple docker hosts, this needs to be handled:
          error "Could not find container for ${branch_config.hdl_platforms[hdltgt].base_target} to build from for ${hdltgt} when there should be one"
        }
        def base_id = dimages[branch_config.hdl_platforms[hdltgt].base_target].id
        // Need to copy from an existing container
        // Docker doesn't allow container-to-container as single command
        sh "docker cp -a ${base_id}:/workspace - | docker cp -a - ${cont_id.id}:/"
        sh "docker exec -u jenkins ${cont_id.id} bash -lc 'cd /workspace && ./reg_projects.sh'"
        return
      } // platform has base target
      unstash 'workspace'
      gen_jenv()
      // Copy workspaces into container
      sh "docker cp workspace ${cont_id.id}:/"
      sh "docker exec ${cont_id.id} chown -R jenkins:opencpi /workspace"
      sh "docker exec -u jenkins ${cont_id.id} bash -lc 'cd /workspace && ./reg_projects.sh'"
      deleteDir()
    } // workspace
  } // node (TODO: split this later more?)
}

// example hdl_licenses: {"modelsim":{"always":true,"lock_name":"Modelsim"},"alst4":{"always":true,"lock_name":"Altera"},"stratix4":{"always":true,"lock_name":"Altera"},"ml605":{"always":false,"lock_name":"Xilinx"},"virtex6":{"always":false,"lock_name":"Xilinx"}}
// Would return the lock name to put into a lock stanza
String license_check(in_phase, in_tgt) {
  echo "license_check: License locking ignored for phase ${in_phase} of target ${in_tgt} (not yet implemented)"
  return '' // Fake placeholder until JENKINS-44141
}

String license_check_POST_44141(in_phase, in_tgt) {
  // TODO: Sync up to latest version below!
  echo "license_check(${in_phase}, ${in_tgt})"
  if (!hdl_licenses.containsKey(in_tgt))
    return ''
  echo "Licensing info exists: ${hdl_licenses[in_tgt]}"
  if (hdl_licenses[in_tgt]["always"])
    return "License_${hdl_licenses[in_tgt]["lock_name"]}"
  // All licenses needed in Phase 5 if they are defined
  if (in_phase == 5)
    return "License_${hdl_licenses[in_tgt]["lock_name"]}"
  return ''
}

// Returns a node label to ensure licenses respected
// TODO: Use license_check_POST_44141 to get a lock name if JENKINS-44141 is ever fixed...
String license_node(in_phase, in_tgt, in_proj = '') {
  echo "license_node(${in_phase}, ${in_tgt}, ${in_proj?:"''"})"
  def tgt = in_tgt
  def final def_str = 'docker-C7'
  // If we are building a BSP for a platform, assume we need to license based on that platform (if it is not a simulator)
  if (in_proj.startsWith("bsp_") && !tgt.find(/sim$/)) {
    // def bsp_pfs = [ in_proj.subString(4) ] // JENKINS-32661?
    def bsp_pfs = [ in_proj.replace('bsp_','') ]
    echo "license_node(${in_phase}, ${tgt}): Building BSP '${in_proj}' (${bsp_pfs[0]})"
    // Need to do reverse lookup of bsp_shares to see if bsp_pf is shared
    // FIXME: The BSP should probably be a top-level element in config.json, since it is repeated now
    def shared_pfs = bsp_shares.findAll { k,v -> bsp_pfs[0] == v }.keySet()
    if (shared_pfs) bsp_pfs = shared_pfs // Replace single value with N results
    bsp_pfs << tgt // Add the requested as well
    // Now bsp_pfs contains 1 or more platforms used by this BSP and the requested platform
    // From here to --ENDCHECK-- below is all just sanity checking to ensure the licenses are the same iff given...
    def pfs_with_licenses = bsp_pfs.collect{ hdl_licenses.containsKey(it)?it:'' } // Finds pfs that have hdl_licenses
    def pf_licenses = pfs_with_licenses.collect{ branch_config.hdl_platforms[it]?.tool_license } // Find out their licenses
    // At this point, we would want to pick the most restrictive one...
    // sandbox won't allow: def filtered_pf_licenses = pf_licenses.unique().findAll() // This should make them unique and remove blanks
    def filtered_pf_licenses = pf_licenses.unique().findAll{it}
    // ...but we have no reasonable way to compare...
    if (filtered_pf_licenses.size() > 1) echo "Could not prioritize mismatched licenses: ${JsonOutput.toJson(filtered_pf_licenses)}"
    assert (filtered_pf_licenses.size() <= 1)
    // --ENDCHECK-- and now change the tgt to the match the most restrictive one
    if (filtered_pf_licenses) // There is at least one license required; now find the platform that required it
      if (!hdl_licenses.containsKey(tgt)) // Not easiest case - if tgt itself needed it, leave it alone
        tgt = bsp_pfs.find{ hdl_licenses.containsKey(it) } // Find the first platform that has a license
    if (tgt != in_tgt)
      echo "license_node(${in_phase}, ${in_tgt}, ${in_proj}): Set new target '${tgt}' to enforce implied licensing requirements in BSP"
  } // Parsing a BSP
  if (!hdl_licenses.containsKey(tgt))
    return def_str
  def final node_str = def_str + " && " + hdl_licenses[tgt]["node_label"]
  echo "Licensing info for ${tgt} exists: ${hdl_licenses[tgt]}"
  if (hdl_licenses[tgt]["always"]) // one caller actually asks for Phase 0 for this...
    return node_str
  if (hdl_licenses[tgt]["phases"])
    return (in_phase as int) in hdl_licenses[tgt]["phases"] ? node_str : def_str
  // All licenses needed in Phase 5 if they are defined at all
  if (in_phase == 5)
    return node_str
  return def_str
}

// Returns 1 if this phase doesn't need a license, and licensed_retries if it does
def license_retry(in_phase, in_tgt) {
  ('docker-C7' == license_node(in_phase, in_tgt))?1:licensed_retries
}

// Used to combine catch blocks
void report_error(err) {
  echo "Preparing email notifying failure in ${stage_name} stage"
  error_email = "Error: '${err}'\n\n${error_email}"
  mattermost_send(this, [icon: "https://jenkins.io/images/logos/fire/fire.png"])
  // If an error happens SUPER early, branch_config may not be set yet:
  if (!branch_config) error "Too early to know whom to email!\n\n${error_email}"
  def recip = branch_config.notifications.join(',')
  recip += (env.BRANCH_NAME in branch_config.team_notify)? ",${branch_config.team_email}":''
  if ('ABORTED' == currentBuild.result) {
    echo "(Skipping email; build was aborted)"
  } else {
    currentBuild.result = 'FAILURE'
    currentBuild.description += "\nFailed in ${stage_name} stage"
    if (recip.trim() != '""') {
      emailext body: error_email, subject: "Job 2: ${stage_name} (${env.BRANCH_NAME}) is failing", to: recip
    } else {
      echo "(No email recipients)"
    }
  } // not ABORTED
}

/*
     ██ ███████ ███    ██ ██    ██   ███████ ██   ██
     ██ ██      ████   ██ ██    ██   ██      ██   ██
     ██ █████   ██ ██  ██ ██    ██   ███████ ███████
██   ██ ██      ██  ██ ██  ██  ██         ██ ██   ██
 █████  ███████ ██   ████   ████  ██ ███████ ██   ██
*/
void gen_jenv() {
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
    echo "echo jenv.sh imported" >> jenv.sh
    echo "set -x" >> jenv.sh
  """
  // job2_envvars should probably be here, but we don't know our platform at this point
} // gen_jenv

/*
 ██████  ██████  ██████  ███████ ██████  ███████ ██████          ██████  ██    ██ ██ ██      ██████
██    ██ ██   ██ ██   ██ ██      ██   ██ ██      ██   ██         ██   ██ ██    ██ ██ ██      ██   ██
██    ██ ██████  ██   ██ █████   ██████  █████   ██   ██         ██████  ██    ██ ██ ██      ██   ██
██    ██ ██   ██ ██   ██ ██      ██   ██ ██      ██   ██         ██   ██ ██    ██ ██ ██      ██   ██
 ██████  ██   ██ ██████  ███████ ██   ██ ███████ ██████  ███████ ██████   ██████  ██ ███████ ██████
*/
// See notes under "Building" above for theory
Closure ordered_build(in_type, in_tgt) {
  return { // a closure that will perform all the build phases on all the projects for a given type + target
    def type = in_type
    def hdltgt = in_tgt
    // Run build_phase(1..3) "now" on core
    build_phase(["tgt":hdltgt, "proj":"core", "type":type, "phases": [1,2,3]])()
    echo "< Synchronization Point 1 : ${type} ${hdltgt}: Core Phases 1-3 Complete. >"

    /*------------------------Synchronization Point 1-----------------------*/

    def next_steps = ["failFast": true]
    next_steps["HDL ${type} ${hdltgt}: Core (Phases 4 and 5)"] =
      build_phase(["tgt":hdltgt, "proj":"core", "type":type, "phases": [4,5], "do_assemblies": build_assemblies_standard_projs])
    next_steps["HDL ${type} ${hdltgt}: Assets (Phases 1 to 4) and Assets_ts (Phases 1 to 3)"] = {
      // Run build_phase(1..3) "now" on assets
      build_phase(["tgt":hdltgt, "proj":"assets", "type":type, "phases": [1,2,3]])()
      echo "< Synchronization Point 2 : ${type} ${hdltgt}: Core build (unbound). Assets Phases 1-3 Complete. >"
      /*----------------------Synchronization Point 2-----------------------*/
      def next_steps_assets = ["failFast": true]
      next_steps_assets["HDL ${type} ${hdltgt}: Assets (Phase 4)"] =
          build_phase(["tgt":hdltgt, "proj":"assets", "type":type, "phases": [4]])
      next_steps_assets["HDL ${type} ${hdltgt}: Assets_ts (Phases 1 to 3)"] =
        build_phase(["tgt":hdltgt, "proj":"assets_ts", "type":type, "phases": [1,2,3]])
      parallel next_steps_assets
    } // Closure of Start Assets and Assets_ts
    parallel next_steps
    echo "< Synchronization Point 3 : ${type} ${hdltgt}: Core Complete. Assets Phase 4 Complete. Assets_ts Phases 1-3 Complete. >"

    /*------------------------Synchronization Point 3-----------------------*/

    next_steps = ["failFast": true]
    next_steps["HDL ${type} ${hdltgt}: Assets_ts (Phase 4)"] =
      build_phase(["tgt":hdltgt, "proj":"assets_ts", "type":type, "phases": [4]])
    def my_BSPs = [] as Set
    bsp_projs.find { pfn ->
      // Skip if pfn doesn't match hdltgt and hdltgt isn't a simulator when platform (AV-3788)
      // https://stackoverflow.com/a/3050218/836748
      if (("platform" == type) && (hdltgt != pfn) && !hdltgt.find(/sim$/) ) {
        echo "Skipping BSP for platform ${pfn} while building for ${hdltgt}"
        return false
      }
      def pfn_bsp = bsp_shares[pfn]?:pfn // Possibly override name from global lookup
      if ("${type}.${hdltgt}.${pfn_bsp}" in bsp_queue) {
        echo "Skipping BSP ${pfn_bsp} build for ${type}.${hdltgt} - already queued elsewhere in job"
      } else {
        bsp_queue << "${type}.${hdltgt}.${pfn_bsp}" // global
        my_BSPs << pfn_bsp
        next_steps["HDL ${type} ${hdltgt}: ${pfn_bsp} BSP Build (Phases 1 to 4)"] = build_phase(["tgt":hdltgt, "proj":"bsp_${pfn_bsp}", "type":type, "phases": [1,2,3,4]])
      }
      return false
    } // each bsp
    parallel next_steps
    echo "< Synchronization Point 4 : ${type} ${hdltgt}: Standard Projects' Phases 1-4 Complete. >"

    /*------------------------Synchronization Point 4-----------------------*/

    next_steps = ["failFast": true]
    next_steps["HDL ${type} ${hdltgt}: Assets (Phase 5)"] =
      build_phase(["tgt":hdltgt, "proj":"assets", "type":type, "phases": [5], "do_assemblies": build_assemblies_standard_projs])
    next_steps["HDL ${type} ${hdltgt}: Assets_ts (Phase 5)"] =
      build_phase(["tgt":hdltgt, "proj":"assets_ts", "type":type, "phases": [5], "do_assemblies": build_assemblies_standard_projs])
    my_BSPs.each { bsp ->
      next_steps["HDL ${type} ${hdltgt}: ${bsp} BSP Build (Phase 5)"] =
        build_phase(["tgt":hdltgt, "proj":"bsp_${bsp}", "type":type, "phases": [5], "do_assemblies": build_assemblies_standard_projs])
    }
    parallel next_steps
    echo "< Synchronization Point 5 : ${type} ${hdltgt}: Standard Projects Complete. >"

    /*------------------------Synchronization Point 5-----------------------*/

    if (additional_projects) {
      echo "Will now build Additional Project(s): "+additional_projects.join(", ") + " : ${type} ${hdltgt}"

      additional_projects.each {
        echo "Additional Project: ${it}: ${type} ${hdltgt}: Begin."
        build_phase(["tgt":hdltgt, "proj":it, "type":type, "phases": [1,2,3,4,5], "do_assemblies": build_assemblies_additional_projs])() // execute now
        echo "Additional Project: ${it}: ${type} ${hdltgt}: Complete."
      }
      echo "All Additional Project(s) for ${type} ${hdltgt}: Complete."
    }
    /*-------------------------Additional Project(s)------------------------*/

    if (type == "platform")
      currentBuild.description += "\nPlatform ${hdltgt} complete (${currentBuild.durationString.replace(' and counting', '')})"
    dimages[hdltgt]["done"] = true
  }
}

/*
██████  ██    ██ ██ ██      ██████          ██████  ██   ██  █████  ███████ ███████
██   ██ ██    ██ ██ ██      ██   ██         ██   ██ ██   ██ ██   ██ ██      ██
██████  ██    ██ ██ ██      ██   ██         ██████  ███████ ███████ ███████ █████
██   ██ ██    ██ ██ ██      ██   ██         ██      ██   ██ ██   ██      ██ ██
██████   ██████  ██ ███████ ██████  ███████ ██      ██   ██ ██   ██ ███████ ███████
*/
// Returns a closure that will build phases of a project as requested
/*
Inputs (in a map): ["tgt":hdlpf, "proj":"core", "type":"platform"]
  tgt: target (e.g. zynq)
  proj: project to build (assumed to be in /workspace/XXX)
  type: "target" or "platform"
  phases: array saying what phases to run, e.g. [1,2,3,4,5]
    - This is needed to keep things as parallel as possible
*/
Closure build_phase(in_config) {
  return {
    // We keep the config in a large map to call phases repeatedly
    def my_config = in_config
    // Then use short names for sanity
    def tgt = my_config['tgt']
    def proj = my_config['proj']
    def type = my_config['type']

    if (!dimages[tgt])
      launch_container(tgt)

    my_config['run_cmd'] = "docker exec -u jenkins ${dimages[tgt]['id']} bash -lc"
    my_config['prefix'] = "cd /workspace/${proj} && source ../jenv.sh && "
    // Only define alternate platform if in platform mode; in target mode it will build for host
    my_config['rcc_hdl_pf'] = (type == "platform")?"--rcc-hdl-platform ${tgt}":''
    // Process any job2_envvars here
    // It would make more sense in gen_jenv, but that is platform agnostic. At this point, we have a known platform
    if ("platform" == type || proj.startsWith("bsp_") ) {
      def tgt_pfs = [tgt] as Set // Always look with actual name
      // Need to map "backwards" and find the platform(s) from the base target
      // Should this take into account the actual BSP including shared ability? Probably, but shouldn't matter.
      // If we are a platform, we need to check the target not the platform:
      def base_tgt = (branch_config.hdl_platforms[tgt]?.base_target)?:tgt
      tgt_pfs << base_tgt
      branch_config.hdl_platforms.each { pfn, dat ->
        if (dat?.base_target == base_tgt) tgt_pfs << pfn // add any platform that depends on current target
      }
      // If we are targeting _any_ platform (like a sim) when building a BSP, import all users of the BSP
      if (proj.startsWith("bsp_")) {
        echo "Debug: in BSP project"
        // Need to do reverse lookup of bsp_shares to see if bsp_pf is shared
        // FIXME: The BSP should probably be a top-level element in config.json, since it is repeated now
        def shared_pfs = bsp_shares.findAll { k,v -> proj - /bsp_/ == v }.keySet()
        shared_pfs.each {
          tgt_pfs << it
          tgt_pfs << branch_config.hdl_platforms[it]?.base_target
          echo "Debug: in BSP project and pushed ${it} and ${branch_config.hdl_platforms[it]?.base_target} into list to look for envvars"
        }
      }
      tgt_pfs.removeAll([null]) // could have empty base targets inserted, etc.
      tgt_pfs.each { tgt_pf ->
        if (branch_config.hdl_platforms[tgt_pf] &&             // Some targets don't even have entries, e.g. virtex6
            branch_config.hdl_platforms[tgt_pf].hdl_enabled && // Should always exist thanks to import_config
            branch_config.hdl_platforms[tgt_pf]?.job2_envvars) {
          branch_config.hdl_platforms[tgt_pf].job2_envvars.each {
            assert (it.replaceAll(/[$`&|;]/,'') == it) // Check for naughty things
            // The "ls -alF" is probably not needed, but it did catch a bug where Job1 wasn't copying the vendor stuff...
            my_config['prefix'] += "export ${it.replace('@VENDOR@', "/workspace/vendor/${tgt_pf}")} && ls -alF /workspace/vendor/ && "
          } // each
        } // job2_envvars defined
      } // each tgt_pf
    } // platform build and BSP

    // nodes / licenses are handled within each phase
    if (1 in my_config['phases']) phase1(my_config)
    if (2 in my_config['phases']) phase2(my_config)
    if (3 in my_config['phases']) phase3(my_config)
    if (4 in my_config['phases']) phase4(my_config)
    if (5 in my_config['phases']) phase5(my_config)
  } // closure
} // build_phase

// This will check if the node matches what the original container was launched on
// TODO: Copy/move container in some way in the future?
void check_node(tgt) {
  if (dimages[tgt]['node'] != env.NODE_NAME) {
    echo "check_node: Detected running on ${env.NODE_NAME} not ${dimages[tgt]['node']}"
    // If the docker container exists, we are OK (continue even if grep fails)
    def found_cont = sh (script: "docker ps -q --no-trunc | grep ${dimages[tgt]['id']} || :", returnStdout: true).trim()
    if (!found_cont.isEmpty())
      echo "check_node: Container ${found_cont} is available on this host"
    else
      error "Jenkins moved job from ${dimages[tgt]['node']} to ${env.NODE_NAME}, which does not have the container we were using!"
  } // no match
} // check_node

/*
██████  ██   ██  █████  ███████ ███████ ███████
██   ██ ██   ██ ██   ██ ██      ██      ██
██████  ███████ ███████ ███████ █████   ███████
██      ██   ██ ██   ██      ██ ██           ██
██      ██   ██ ██   ██ ███████ ███████ ███████
*/
// Phase 1: {RCC} {Primitives}
void phase1(in_config) {
  def tgt = in_config['tgt']
  def proj = in_config['proj']
  def type = in_config['type']
  def run_cmd = in_config['run_cmd']
  def prefix = in_config['prefix']
  def rcc_hdl_pf = in_config['rcc_hdl_pf']

  echo "Phase1: Project=${proj} hdl-${type}=${tgt}: waiting for node with label '${license_node(1, tgt, proj)}'"
  if (internal_tracing) return
  node (license_node(1, tgt, proj)) {
    check_node(tgt)
    lock(license_check(1,tgt)) {
      retry(license_retry(1,tgt)) {
      parallel \
        "Build ${proj} for ${type} ${tgt} ⟶ (1) RCC" :
          { sh """${run_cmd} "${prefix} ocpidev build --rcc ${rcc_hdl_pf} | tee build_rcc_${type}.log 2>&1" # Phase 1 RCC """ },
        /* temporary disable
         "Build ${proj} for ${type} ${tgt} ⟶ (1) ACI Applications" :
          { sh """${run_cmd} "${prefix} if [ -d applications ]; then ocpidev build applications --rcc ${rcc_hdl_pf} | tee build_aci_${type}.log 2>&1; fi" """ }, */
        "Build ${proj} for ${type} ${tgt} ⟶ (1) Primitives" :
          { sh """${run_cmd} "${prefix} if [ -d hdl/primitives ]; then ocpidev build hdl primitives --hdl --hdl-${type} ${tgt} | tee build_primitives_${type}.log 2>&1; fi" # Phase 1 Primitives """ },
          failFast: true
      } // retry
    } // lock
  } // node
}

// Phase 2: {Components} {Adapters} {Devices}
void phase2(in_config) {
  def tgt = in_config['tgt']
  def proj = in_config['proj']
  def type = in_config['type']
  def run_cmd = in_config['run_cmd']
  def prefix = in_config['prefix']
  def rcc_hdl_pf = in_config['rcc_hdl_pf']

  echo "Phase2: Project=${proj} hdl-${type}=${tgt}: waiting for node with label '${license_node(2, tgt, proj)}'"
  if (internal_tracing) return
  node (license_node(2, tgt, proj)) {
    check_node(tgt)
    lock(license_check(2,tgt)) {
      retry(license_retry(2,tgt)) {
        parallel "Build ${proj} for ${type} ${tgt} ⟶ (2) Components" : {
          sh """${run_cmd} "${prefix} if [ -d components ]; then ocpidev build library components --hdl --hdl-${type} ${tgt} | tee build_components_${type}.log 2>&1; fi" # Phase 2 Components """
          },
          "Build ${proj} for ${type} ${tgt} ⟶ (2) Adapters": {
            sh """${run_cmd} "${prefix} if [ -d hdl/adapters ]; then ocpidev build hdl library adapters --hdl --hdl-${type} ${tgt} ${rcc_hdl_pf} | tee build_adapters_${type}.log 2>&1; fi" # Phase 2 Adapters """ },
          "Build ${proj} for ${type} ${tgt} ⟶ (2) Devices": {
            sh """${run_cmd} "${prefix} if [ -d hdl/devices ]; then ocpidev build hdl library devices --hdl --hdl-${type} ${tgt} ${rcc_hdl_pf} | tee build_devices_${type}.log 2>&1; fi" # Phase 2 Devices """ },
          failFast: true
      } // retry
    } // lock
  } // node
}

// Phase 3: {Cards} {Platform Devices (hdl/platforms/ any subdir named "devices") }
void phase3(in_config) {
  def tgt = in_config['tgt']
  def proj = in_config['proj']
  def type = in_config['type']
  def run_cmd = in_config['run_cmd']
  def prefix = in_config['prefix']
  def rcc_hdl_pf = in_config['rcc_hdl_pf']

  echo "Phase3: Project=${proj} hdl-${type}=${tgt}: waiting for node with label '${license_node(3, tgt, proj)}'"
  if (internal_tracing) return
  node (license_node(3, tgt, proj)) {
    check_node(tgt)
    lock(license_check(3,tgt)) {
      retry(license_retry(3,tgt)) {
        parallel "Build ${proj} for ${type} ${tgt} ⟶ (3) Cards" : {
          sh """${run_cmd} "${prefix} if [ -d hdl/cards ]; then ocpidev build hdl library cards --hdl --hdl-${type} ${tgt} ${rcc_hdl_pf} | tee build_cards_${type}.log 2>&1; fi" # Phase 3 Cards """
          },
          "Build ${proj} for ${type} ${tgt} ⟶ (3) Platform Devices": {
            sh """${run_cmd} "${prefix} for pf in \\\$(find hdl/platforms/ -type d -name devices | cut -f3 -d/); do
              echo \\\${pf}:
              ocpidev build hdl library devices -P \\\${pf} --hdl-${type} ${tgt} ${rcc_hdl_pf} | tee -a build_\\\${pf}_devices_${tgt}.log 2>&1
            done" # Phase 3 Platform Devices """ },
          failFast: true
      } // retry
      sh """${run_cmd} "${prefix} ocpidev refresh project" """ // sync exports etc.
    } // lock
  } // node
}

// Phase 4: {make hdlplatforms}
void phase4(in_config) {
  def tgt = in_config['tgt']
  def proj = in_config['proj']
  def type = in_config['type']
  def run_cmd = in_config['run_cmd']
  def prefix = in_config['prefix']

  echo "Phase4: Project=${proj} hdl-${type}=${tgt}: waiting for node with label '${license_node(4, tgt, proj)}'"
  if (internal_tracing) return

  // Phase 4 doesn't make much sense unless we are a platform
  if (type != "platform") {
    echo "Phase 4 skipped for Project ${proj} because target is ${tgt}, which is not a platform"
    return
  }

  node (license_node(4, tgt, proj)) {
    check_node(tgt)
    lock(license_check(4,tgt)) {
      retry(license_retry(4,tgt)) {
        echo "Build ${proj} for ${type} ${tgt} ⟶ (4) hdlplatforms"
        sh """${run_cmd} "${prefix} make hdlplatforms HdlPlatforms=${tgt} | tee build_hdlplatforms.log 2>&1" # Phase 4 HdlPlatforms """
      } // retry
    } // lock
  } // node
}

// Phase 5: {final pass top-level} {archive results}
void phase5(in_config) {
  def tgt = in_config['tgt']
  def proj = in_config['proj']
  def type = in_config['type']
  def run_cmd = in_config['run_cmd']
  def prefix = in_config['prefix']
  def rcc_hdl_pf = in_config['rcc_hdl_pf']
  def do_asm = in_config['do_assemblies']

  // TODO: Possible future improvement - somehow get list of assemblies and then do parallel
  // across multiple nodes. That would take more coordination and merging. Or do we move assemblies
  // into different jobs?
  echo "Phase5: Project=${proj} hdl-${type}=${tgt}: waiting for node with label '${license_node(5, tgt, proj)}'"
  if (internal_tracing) return
  node (license_node(5, tgt, proj)) {
    check_node(tgt)
    lock(license_check(5,tgt)) {
      retry(license_retry(5,tgt)) {
        echo "Build ${proj} for ${type} ${tgt} ⟶ (5) Final Pass"
        def asm_flag = do_asm?'':'--no-assemblies'
        sh """${run_cmd} "${prefix} ocpidev build --hdl --hdl-${type} ${tgt} ${rcc_hdl_pf} ${asm_flag} | tee build_final_${type}.log 2>&1" # Phase 5 """
      } // retry
      sh """${run_cmd} "${prefix} ocpidev refresh project" """ // sync exports etc.
    } // lock
    // Archive the artifacts. Currently continues to tie up the node/licence until we're lock-based.
    // However, without this, when archiving was "phase 6" it has failed with timeout hitting
    //   because it was trying to run/resume on an "off-peak" node during the workday. (AV-5156)
    echo "Build ${proj} for ${type} ${tgt} ⟶ (5) Archiving"
    deleteDir()
    sh """${run_cmd} "${prefix} cd .. && tar cf workspace_${proj}_${tgt}.tar ${proj}" """
    sh """${run_cmd} "${prefix} cd .. && xz -1v workspace_${proj}_${tgt}.tar" """
    dir ("workspaces/${type}s/") {
      sh "docker cp -a ${dimages[tgt]['id']}:/workspace/workspace_${proj}_${tgt}.tar.xz ."
    }
    archiveArtifacts allowEmptyArchive: false, artifacts: 'workspaces/**', defaultExcludes: false, excludes: '**/*@tmp/' // JENKINS-53438
    deleteDir()
  } // node
}
