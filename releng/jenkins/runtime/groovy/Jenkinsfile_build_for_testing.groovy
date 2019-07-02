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

// Note: This job does NOT email the user pass/fail - people could be BOMBARDED.

// TODO: Add CentOS 6 support!

@Library('opencpi-libs')_
import groovy.json.JsonOutput
import groovy.transform.Field

@Field final docker_mount_paths = [ '/sys', '/opt/Altera', '/opt/Modelsim', '/opt/Xilinx', '/opt/TotallyFakeLocationForATool' ] // TODO: Move this code to central location?
@Field final docker_label = 'docker' // Can change to test "non-docker" on server
@Field final bitz_files = ['*.bitz', '*.tar.gz', '*.bit.gz']
@Field final log_files = ['*.log', '*.out', '*.rpt']

// DO NOT use "properties" here - JENKINS-43758 and JENKINS-51038

// These jobs are intended to NOT be launched manually!
// https://stackoverflow.com/a/49981638/836748
def manualTrigger = true

// upstreamBuilds returns a "RunWrapper" https://github.com/jenkinsci/workflow-support-plugin/blob/master/src/main/java/org/jenkinsci/plugins/workflow/support/steps/build/RunWrapper.java

currentBuild.upstreamBuilds?.each { manualTrigger = false }
currentBuild.description = "" // Ensure after this point it's at least defined

if (manualTrigger) {
  currentBuild.description = "Self-aborted (do not run jobs manually)"
  error "This job is designed to only be launched by other jobs."
}

def final DEBUG=true

// Configuration stuff coming out of Setup stage:
@Field launcher
@Field launcher_config
@Field job2_config
@Field upstream_job // This will have the git branch attached to it, e.g. Job_2/develop
@Field upstream_job_build
@Field upstream_git_branch
@Field use_valgrind
@Field lic_label
@Field comps_to_test
@Field projs
@Field pf

stage("Setup") {
  // currentBuild is NULL when Fields are initialized so cannot be "final" above
  // TODO: Need to move a lot of this into a reusable library...

  // NOTE: Can we assume the "first" in the list is the last cause? All testing so far only had one caller. Eventually, Job 1 will call Job 2 will call launcher will call us... Might need to check the name?
  launcher = currentBuild.upstreamBuilds[0]

  /* Thanks to a handful of Jenkins bugs, like JENKINS-51038 and JENKINS-51144, the way this
  job gets its parameters are a total pain. It COULD be launched two different ways - from
  the console by an end user, or by Job_2 automatically. If launched manually, getBuildVariables()
  will give our information, remembering there is a "launcher" and an "upstream". If called
  automatically, ask the Jenkins RunWrapper since launcher = upstream. */

  def upstream_config = launcher.getBuildVariables()
  if (upstream_config) {
    if (DEBUG) echo "Incoming upstream_config from user launch: "+groovy.json.JsonOutput.prettyPrint(groovy.json.JsonOutput.toJson(upstream_config))
    upstream_job_build = upstream_config["JOB_BUILD_NUMBER"]
    upstream_git_branch = upstream_config["GIT_SELECTED_BRANCH_stripped"]
    upstream_job = "${upstream_config["UPSTREAM_JOB_NAME"]}/${upstream_git_branch}"
    use_valgrind = upstream_config["VALGRIND"]
  } else { // Job_2 called us directly, so we can directly reference its parameters
    upstream_job = launcher.getFullProjectName()
    upstream_job_build = launcher.getNumber()
    upstream_git_branch = upstream_job.split('/')[-1]
    use_valgrind = false
  } // Launch method

  // Do not start until launcher job is actually complete. Otherwise it might still be archiving the files we need.
  wait_for_job(launcher)

  node("master") {
    dir ('config') { deleteDir() }
/* This was not needed, but wanted to keep this snippet for the JSON tree code.
   Without limiting, crashed with too many artifacts...
    if (!upstream_config) {
      upstream_config = readJSON text:
        sh (script: "curl -ksgL ${launcher.getAbsoluteUrl()}/api/json/?tree=actions[parameters[*]]", returnStdout: true)
      if (DEBUG) echo "Incoming upstream_config from automated launch: "+groovy.json.JsonOutput.prettyPrint(groovy.json.JsonOutput.toJson(upstream_config))
    }
*/
    copyArtifacts( // Import ".env" file because of JENKINS-51038
      projectName: launcher.getFullProjectName(),
      filter: "testing_support/build_configs/${JOB_BASE_NAME}.env",
      fingerprintArtifacts: true,
      target: "config/",
      flatten: true,
      selector: specific("${launcher.getNumber()}") // Must be String...
    )
    launcher_config = readJSON text:
      sh (script: ". config/${JOB_BASE_NAME}.env && echo \${JSON_Config}", returnStdout: true)
    // Let's figure out EVERYTHING we need now... working around JENKINS-51038 etc.
    // What is our platform? Parse JOB_NAME, e.g. Test_Results/AV_2985_Jenkins_2/modelsim/devices__matchstiq_z1_rx
    def final job_name_parsed = JOB_NAME.split('/')
    pf = job_name_parsed[job_name_parsed.length-2] // 2nd from last
    if (DEBUG) echo "Our platform is "+pf
    // Branch should be 3rd from last (but we should use upstream_git_branch for proper symbols)
    assert job_name_parsed[job_name_parsed.length-3] == upstream_git_branch.replace('-','_').replace('.','_dot_')
    // What is our test to run?
    // Example: "COMPONENT_TO_TEST": "assets/components/comms_comps/mfsk_mapper.test/"
    // What projects do we need to import?
    projs = [launcher_config.COMPONENT_TO_TEST.split('/')[0], 'core', 'assets'] as Set // First entry of path and always take in core and assets
    if (DEBUG) echo "Our test path is "+launcher_config.COMPONENT_TO_TEST
    if (DEBUG) echo "Our project list is "+projs.join(', ')
    // Upstream Job to copy from
    if (DEBUG) echo "Should copy from ${upstream_job} #${upstream_job_build}. Pulling their metadata."
    copyArtifacts(
      projectName: upstream_job,
      filter: ".jenkins_metadata_job2.json",
      fingerprintArtifacts: true,
      target: "config/",
      flatten: true,
      selector: specific("${upstream_job_build}")
    )
    job2_config = readJSON file: "config/.jenkins_metadata_job2.json"
    // TODO: Make this a function - it's used in a few places now... see launcher_for_testings
    // What is our license restriction?
    // Job 2 ensures that there is SOMETHING set for tool_license (could be "unlimited")
    def lic_tool = job2_config.branch_config.hdl_platforms[pf].tool_license
    if (DEBUG) echo "Need license for ${pf} (looking up tool ${lic_tool})"
    lic_label = ''
    if (!job2_config.branch_config.tools[lic_tool]?.never_license) {
      lic_label += job2_config.branch_config.tools[lic_tool].node_label
      if (DEBUG) echo "Need license for ${pf} (resulting label ${lic_label})"
    }
    comps_to_test = [launcher_config.COMPONENT_TO_TEST]
    if (JOB_BASE_NAME =~ /_all$/) {
      // If we are called as "build_proj__all" we should iterate over launcher_config.tests_list (AV-4529)
      comps_to_test = []
      launcher_config.tests_list.each { test, path ->
        comps_to_test << "${launcher_config.proj}/${path}"
        if (DEBUG) echo "build_all (${launcher_config.proj}) pushing ${path} (${comps_to_test.size()})"
      }
    }
    // Add "our" BSP Project to projs, if applicable (AV-4717)
    if (pf in job2_config.bsp_projs) {
      if (DEBUG) echo "${pf} requires a BSP"
      if (job2_config.bsp_shares.containsKey(pf)) {
        if (DEBUG) echo "${pf} requires a shared BSP (${job2_config.bsp_shares[pf]})"
        projs << "bsp_${job2_config.bsp_shares[pf]}"
      } else {
        projs << "bsp_${pf}"
      }
      if (DEBUG) echo "Our updated project list is "+projs.join(', ')
    } // in bsp_projs
  } // node
} // stage

currentBuild.description += "Using artifacts from ${upstream_job.split('/')[0..-2]} #${upstream_job_build}"

if (DEBUG) echo "Waiting for a node matching '${lic_label}'"
node(lic_label) { // JENKINS-44141 will change this!!!
  stage ("Build Unit Test") {
    dir (WORKSPACE) { deleteDir() } // Nuke workspace
    if (DEBUG) echo "Copying support files and workspaces from ${upstream_job} #${upstream_job_build}"
    /* This next section is a serious "lesser of two evils" - the two choices are:
       1. Copy the .tar.xz from Job2 and then extract it. Since it's XZ, this means lots of CPU.
          With ALL of the component tests doing it at the same time (since launcher didn't stagger.)
       2. Copy the files that launcher already extracted - all 19K (and counting!) of them.

       I went with #1 with CPU limiting. The second had too many problems - up to 3-4 minutes for
       modelsim (because it thinks it is a remote system) as well as permissions like "generate.py"
       not being executable.
    */
    def files_to_copy = ['releng/jenkins_runtime.tar']
    projs.each { files_to_copy.push("**/workspace_${it}_${pf}.*") }
    // Pull the extracted version of the project
    copyArtifacts(
      projectName: upstream_job,
      filter: files_to_copy.join(", "),
      fingerprintArtifacts: true,
      target: "workspace/",
      flatten: true,
      selector: specific("${upstream_job_build}")
    )
    dir ('releng') {
      sh "tar xf ../workspace/jenkins_runtime.tar" // Want it to be in usual place under releng
    }
    // Decide if we are using docker or not
    def docker_mount_options = []
    def docker_image = ''
    def dimage
    if (NODE_LABELS.contains(docker_label)) {
      // Find the tools in /opt/
      docker_mount_paths.each {
        if (0 == sh (script: "[ -d ${it} ]", returnStatus: true))
          docker_mount_options.push("-v ${it}/:${it}/:ro")
      }
      if (DEBUG) echo "Using '${docker_mount_options.join(" ")}' for docker mounts"
      // See if there's already a ready-made image:
      docker_image = docker_rpminstalled_image.findimage(upstream_git_branch, 7, job2_config.params.Upstream_Job)
    }
    // Now (maybe) copy the RPMs from the Job 1 - either docker couldn't find it or no docker
    // TODO: Need to be able to do C6 as well at some
    if (!docker_image) {
      files_to_copy = 'centos7/**'
      if (!NODE_LABELS.contains(docker_label))
        files_to_copy = 'support/ocpi_rpms_extracted_C7.tar'
      copyArtifacts(
        projectName: "Job_1/${upstream_git_branch}",
        filter: files_to_copy,
        fingerprintArtifacts: true,
        target: "rpms/",
        flatten: false,
        selector: specific(job2_config.params.Upstream_Job)
      )
    }
    // Set up env
    if (NODE_LABELS.contains(docker_label)) {
      if (docker_image) {
        if (DEBUG) echo "Docker found and re-using image '${docker_image}'"
        dimage = docker.image(docker_image)
      } else { // Need to request one
        if (DEBUG) echo "Docker found, but we didn't find a usable image. Requesting new one."
        dimage = docker_rpminstalled_image(this, "rpms/centos7/")
      }
    } else { // no docker
      if (DEBUG) echo "Not using docker - extracting RPMs (not installing)"
      dir ('workspace') {
        lock("${NODE_NAME}-ocpi_rpms_extracted") {
          sh "tar xf ../rpms/support/ocpi_rpms_extracted_C*.tar"
        }
        // This needs to be double quoted to fill in the blanks.
        // This is copied from Job 1 and spread out here and below
        def OS=7
        sh """
          set +x
          printf "
              export OCPI_CDK_DIR=`pwd`/opt/opencpi/cdk/
              export OCPI_PREREQUISITES_DIR=`pwd`/opt/opencpi/prerequisites/
              export OCPI_BUILD_SHARED_LIBRARIES=0
              export PATH=`pwd`/opt/opencpi/cdk/centos${OS}/bin:\${PATH}
              " >> jenv.sh
          shopt -s nullglob
          for f in `pwd`/opt/opencpi/cdk/env.d/*.sh; do
            echo "Importing \$f into jenv.sh"
            cat \$f >> jenv.sh
          done
        """
      } // workspace
    } // Docker or not
    // Both with and without docker need these:
    dir ('workspace') {
      // These are for our bash script
      sh """
        echo 'export HDL_PLATFORM=${pf}' >> jenv.sh
      """
      // Separated from above because this is from Job 1 copies...
      sh """set +x
        # echo "export OCPI_LOG_LEVEL=11 V=1 AT='' OCPI_DEBUG_MAKE=1 OCPI_HDL_VERBOSE_OUTPUT=1" >> jenv.sh
        printf "
          export OCPI_XILINX_LICENSE_FILE=${global_config.license_server()}
          export OCPI_ALTERA_LICENSE_FILE=${global_config.license_server()}
          export OCPI_MODELSIM_LICENSE_FILE=${global_config.license_server()}
          set -o pipefail
          echo jenv.sh imported
          set -x
        " >> jenv.sh
      """
      // Extract our workspaces
      dir ('built_area') {
        projs.each {
          echo "Extracting tarball for ${it}"
          lock("${NODE_NAME}-workspace-${it}") {
            sh "tar xf ../workspace_${it}_${pf}*"
          }
        }
      }
      // Remove previous build logs and bitz files
      (log_files + bitz_files).each { sh "find . -name '${it}' -delete" }
      // And FINALLY do the actual build!
      // TODO: Move the licensing logic/loop out of bash script?
      try {
        comps_to_test.eachWithIndex { path, idx ->
          echo "=== Building component test ${idx+1} of ${comps_to_test.size()}: ${path} ==="
          if (dimage) {
            dir ("..") { // Jenkins exports to docker from call site
              if (DEBUG) echo "Launching docker: '${docker_mount_options.join(" ")} --name ${BUILD_TAG}'"
              dimage.inside("${docker_mount_options.join(" ")} --name ${BUILD_TAG}") {
                sh """#!/bin/bash -xel
                  cd workspace/built_area
                  export TEST_PATH=${path}
                  ../../releng/jenkins/runtime/bash/build_test.sh
                  """.trim()
              } // inside() container
            }
          } else { // non-docker version
            sh "cd built_area && export TEST_PATH=${path} && ../../releng/jenkins/runtime/bash/build_test.sh"
          }
        }
      } catch (err) {
        echo "Something did not work: '${err}'. Capturing logs."
        error "Something did not work: '${err}'. Captured logs."
      } finally { // Always get the logs
        log_files.each {
          echo "Compressing obnoxiously large files matching '${it}' found in built_area"
          dir ("built_area") {
            sh "find . -name '${it}' -size +10M | xargs -rn1 xz -1v || :"
          }
          dir ("logs") {
            sh """(cd ../built_area && find -name '${it}' -o -name '${it}.xz') | xargs -r -IXXX bash -c 'mkdir -p \$(dirname XXX) && cp -l ../built_area/XXX \$(dirname XXX)' || :"""
          }
        }
        archiveArtifacts artifacts: "logs/**", allowEmptyArchive: true, fingerprint: false, onlyIfSuccessful: false
      }
      // Collect results (archiveArtifacts "**/*.bitz,**/*.tar.gz, **/*.bit.gz" crashed Jenkins - circular dependencies with the registry?)
      comps_to_test.eachWithIndex { test_path, idx ->
        echo "=== Packaging component test ${idx+1} of ${comps_to_test.size()}: ${test_path} ==="
        // Any other cleanup needed for each test:
        sh "find built_area -name kernel-headers -type d | xargs --no-run-if-empty rm -rf || :" // Nuke linux headers
        dir ("results") {
          /* bitz_files.each { // Is this even needed? Would they EVER be outside of the .test directory?
            sh """(cd ../built_area && find -name '${it}') | xargs -r -IXXX bash -c 'mkdir -p \$(dirname XXX) && cp ../built_area/XXX \$(dirname XXX)' || :"""
          } */
          // Copy the test directory (placing it in expected location)
          // Then tar it all up (because JENKINS-13128 / JENKINS-14269 - permissions on script files)
          // Using -0 vs -1 saved about 1/4 the time; took almost 5 minutes for cic_dec otherwise.
          // Can't mix and match "only copy valid symlinks" - https://stackoverflow.com/a/27871147/836748
          sh """mkdir -p ${test_path}
            find ../built_area/${test_path} -name '*.pdf' -xtype l -delete || echo "No broken PDF symlinks"
            cp -LR ../built_area/${test_path} ${test_path}/../
            XZ_OPT='-0 -v -T4' tar Jcf ../${test_path.split('/')[-1]}.tar .
          """
          deleteDir()
        } // results dir
      } // each test

      // Save off all our configuration information
      writeFile file: '.test_metadata.json', text: groovy.json.JsonOutput.prettyPrint(groovy.json.JsonOutput.toJson([
        'comps_to_test': comps_to_test,
        'upstream_job': upstream_job,
        'upstream_job_build': upstream_job_build,
        'upstream_git_branch': upstream_git_branch,
        'use_valgrind': use_valgrind,
        'launcher_config': launcher_config,
        'job2_config': job2_config,
        'projs': projs,
      ]))
      archiveArtifacts artifacts: ".test_metadata.json, *.tar", fingerprint: true, onlyIfSuccessful: true
    } // workspace dir
  } // stage
} // node

stage("Deploy to NFS") {
  node("master") {
    dir ('deploy') {
      deleteDir()
      unarchive mapping: ["*.test.tar" : '.']
      // TODO: Add some metadata file or something to tell where it came from?
      sh """
        set +x
        mkdir -p ${global_config.nfs_export()}/test_assemblies/${upstream_git_branch}/${pf}
        cp -v *.test.tar ${global_config.nfs_export()}/test_assemblies/${upstream_git_branch}/${pf}/
      """
    }
  } // node
} // stage

// Launch the run job (should these be combined?)
// Right now, this job cannot be manually launched, but the test one can be re-run. Is there any end-user utility?

/// Temporary guard only sims:
if (pf.find(/sim$/)) {
if (JOB_BASE_NAME =~ /_all$/) {
  launcher_config.tests_list.each { test, path ->
    echo "Launching Run Job: ${test}"
    launch_job(env.JOB_NAME.replace("/build_${launcher_config.proj}__all", "/run_${launcher_config.proj}__${test}"))
  } // each test
} else { // individual test
  launch_job(env.JOB_NAME.replace('/build_', '/run_'))
}
} // sim-only

// Launch follow-on run job (not caring if it worked or not)
// This is here because if you do something like launch xsim and then right after launch
// modelsim, the modelsim job will DISABLE the downstream run jobs. This way, it will be
// tagged unstable and the run test can at least be re-launched.
void launch_job(job_in) {
  try {
    build job: job_in, quietPeriod: 0, wait: false
  } catch (err) {
    echo "[IGNORED ERROR] Could not launch job: ${err}"
    currentBuild.result = 'UNSTABLE'
  }
} // launch_job
