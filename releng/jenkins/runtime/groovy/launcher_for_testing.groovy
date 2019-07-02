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

/* NOTE: This is in "Job DSL" format and NOT "pipeline" / "workflow" like other Jenkins groovy files!
https://jenkinsci.github.io/job-dsl-plugin/
*/

import groovy.json.JsonOutput
import groovy.json.JsonSlurper
import java.lang.ProcessBuilder
// import groovy.transform.Field

/* Incoming parameters (when run manually):
GIT_SELECTED_BRANCH
GIT_SELECTED_BRANCH_stripped
JOB_BUILD_NUMBER
HDL_PLATFORMS
COMPONENTS
UPSTREAM_JOB_NAME
COPYARTIFACT_BUILD_NUMBER_JOB2 (should be the same as JOB_BUILD_NUMBER)
*/

/* Things done by the job before this script is run:
 - Set parameters
 - Checked out repo into current directory
 - Copied from Job2 as requested into ${WORKSPACE}/built_area
*/

// Utility functions:
// The "meat" - run a command under bash with optional working directory and list of env variables to export
// Returns two elements with STDOUT, STDERR.
String[] run_with_err(final String cmd, final String cwd = binding.variables.WORKSPACE, final java.util.ArrayList exports = [], final Map exports_map = [:]) {
  def outputStream = new StringBuffer()
  def outputStream_err = new StringBuffer()
  println "Running in '${cwd}' ''' ${cmd} '''\n"
  def pb = new ProcessBuilder(['bash', '-c', cmd]);
  def env = pb.environment();
  exports.each {
    env.put(it, binding.variables[it]);
  }
  exports_map.each { k,v ->
    env.put(k, v)
  }
  def proc = pb.directory(new File(cwd)).start()
  proc.waitForProcessOutput(outputStream, outputStream_err)
  return [outputStream.toString(), outputStream_err.toString(), proc.exitValue()]
}
// Simpler version that drops STDERR and only returns STDOUT as a string. Can fail on STDERR being non-empty.
String run(final String cmd, final String cwd = binding.variables.WORKSPACE, final java.util.ArrayList exports = [], final Map exports_map = [:], final Boolean fail_on_err = false) {
  def res = run_with_err(cmd, cwd, exports, exports_map)
  if (fail_on_err && ('0' != res[2]))
    throw new javaposse.jobdsl.dsl.DslException("Error running '${cmd}': STDOUT='${res[0]}' STDERR='${res[1]}' EXIT=${res[2]}")
  /* println "Command completed successfully"
  println "STDOUT='${res[0]}'"
  println "STDERR='${res[1]}'" */
  return res[0]
}

// More Groovy-like - wrappers take a map for optional named parameters: cmd cwd env fail
// However, "env" can only list variables coming INTO this job - not defined in closures!
// env_map is written directly for that use case...
def run_with_err(final params) {
  if (params?.fail)
    throw new javaposse.jobdsl.dsl.DslException("Called run_with_err with fail parameter - invalid usage!")
  return run_with_err(params.cmd, params.cwd ?: binding.variables.WORKSPACE, params.env ?: [], params.env_map ?: [:])
}
def run(final params) {
  return run(params.cmd, params.cwd ?: binding.variables.WORKSPACE, params.env ?: [], params.env_map ?: [:], params.fail ?: false)
}

// Exports a JSON file as a bash "declare -x" script that can then be re-read in another job (to work around JENKINS-51038)
// Named parameters:
//   Required: file input
//   Optional: cwd
void export_JSON(final params) {
  if (!params.input || !params.file)
    throw new javaposse.jobdsl.dsl.DslException("export_JSON missing required parameters - needs at least file and input")
  def cwd = params.cwd ?: binding.variables.WORKSPACE
  run cmd: "export | grep JSON_Config > ${params.file}",
      cwd: cwd,
      env_map: ['JSON_Config': JsonOutput.toJson(params.input)],
      fail: true
}
// End execution helper functions

/*
// Some tests of the above:

println("going to call execute now... (${WORKSPACE})")
def res = run("ls -alF; env", WORKSPACE, ['WORKSPACE'])
println("STDOUT:")
println(res)

res = run("ls -alF", "/")
println("STDOUT:")
println(res)

def res2 = run_with_err("echo error > /proc/self/fd/2")
println("STDOUT:")
println(res2[0])
println("STDERR:")
println(res2[1])

res2 = run_with_err cmd: "echo error > /proc/self/fd/2 ; env", env: ['WORKSPACE']
println("STDOUT:")
println(res2[0])
println("STDERR:")
println(res2[1])
*/

// This where the jobs will be created:
def final ROOT="Test_Results"
def final BRANCH_FOLDER="${GIT_SELECTED_BRANCH_stripped}".replace('-','_').replace('.','_dot_')
def final DEBUG=true
def InstalledBSPs = [] as Set

if (DEBUG) println "All variables known to seed script: "+(binding.variables.keySet()).sort().join(', ')

def final BUILT_AREA = WORKSPACE + "/testing_support/built_area/"

// Job DSL cannot use "load" to bring in import_config...
def final job2_config = new JsonSlurper().parseText(readFileFromWorkspace("${BUILT_AREA}/.jenkins_metadata_job2.json"))

// if (DEBUG) println JsonOutput.prettyPrint(JsonOutput.toJson(job2_config))

def hdl_platforms = job2_config.hdl_platforms // do we care about targets? I don't think so...
// Decide platform(s)
if (HDL_PLATFORMS) { // User specified - need to verify built in requested upstream
  println "User specified platforms: '${HDL_PLATFORMS}'"
  hdl_platforms = HDL_PLATFORMS.tokenize(", ")
  hdl_platforms.each { in_pf ->
    if (!job2_config.hdl_platforms.contains(in_pf))
      throw new javaposse.jobdsl.dsl.DslException("${in_pf} was not built in build ${JOB_BUILD_NUMBER} of ${GIT_SELECTED_BRANCH_stripped}")
  }
}

if (DEBUG) println "hdl_platforms final: ${hdl_platforms.join(', ')}"

// Decide component(s) to test
def projs = ['core', 'assets', 'assets_ts'] // This is the order they will be processed
// Remap the projects to shared BSPs if possible
def bsp_list = [] as Set
job2_config.bsp_projs.each { vpfn ->
/* Overkill   if (proj.find(/^bsp_/)) {
    def final matcher = proj =~ /bsp_(.*)/
    if (!matcher.matches()) error "Strange Regex/Glob Mismatch Error - '${proj}'"
    def final vpfn = matcher[0][1] // 0 is first match, within that 0 would hold whole match, 1 is first capture */
    if (job2_config.bsp_shares.containsKey(vpfn)) {
      def final pf_bsp = job2_config.bsp_shares[vpfn]
      if (DEBUG) println "Found shared BSP for ${vpfn}: ${pf_bsp}"
      bsp_list << "bsp_${pf_bsp}"
    } else { // not shared
      if (DEBUG) println "No shared BSP for ${vpfn}: saving as-is"
      bsp_list << vpfn
    }
/*
  } else { // not BSP
    println "Something weird happened - ${proj} should be a BSP but doesn't start with 'bsp_'?"
    bsp_list << proj
  } */
} // proj   /* vpfn */
projs.addAll(bsp_list)
if (DEBUG) println "final projs list: ${projs.join(", ")}"

// We need a temporary copy of the CDK with a few things
// NOTE: I HATE THIS CODE. There doesn't seem to be a way to copy an artifact within a DSL Seed Job!
// Job 2 could do this for us, but the manual launcher cannot.
run cmd: "rm -rf cdk_extracted && mkdir -p cdk_extracted && cd cdk_extracted && curl -ksgL -o cdk.tar ${JENKINS_URL}/job/Job_1/job/${GIT_SELECTED_BRANCH_stripped}/${job2_config.params.Upstream_Job}/artifact/support/ocpi_rpms_extracted_C7.tar", fail: true
run cmd: "tar xf cdk.tar ./opt/opencpi/cdk/", cwd: WORKSPACE+'/cdk_extracted', fail: true

def tests_list = [:] // empty map
def tests_pfs = [:]
// Build local registry
run cmd:
   """set -e
   rm -rf ${WORKSPACE}/cdk_extracted/registry
   mkdir ${WORKSPACE}/cdk_extracted/registry
   """, cwd: BUILT_AREA, fail: true

projs.each { proj ->
  if (DEBUG) println "Processing proj ${proj}"
  def pfs = [] as Set
  hdl_platforms.each { pf ->
    if (DEBUG) println "Untarring ${pf}::${proj}"
/*DELME
    def extracted = false
    if (proj.find(/^bsp_/)) { // BSP - see if it is shared or not
      def final matcher = proj =~ /bsp_(.*)/
      if (!matcher.matches()) error "Strange Regex/Glob Mismatch Error - '${proj}'"
      def final vpfn = matcher[0][1] // 0 is first match, within that 0 would hold whole match, 1 is first capture
      if (job2_config.bsp_shares.containsKey(vpfn)) {
        def final pf_bsp = job2_config.bsp_shares[vpfn]
        if (!(pf_bsp in InstalledBSPs)) {
          if (DEBUG) println "Untarring ${pf}::${proj} - found shared BSP for ${vpfn}: ${pf_bsp}"
          run cmd: "tar -xC ../.. -f workspace_bsp_${pf_bsp}_${pf}.tar.xz", cwd: BUILT_AREA+"/workspaces/platforms/", fail: true
          // if (DEBUG) println "Untarred ${pf_bsp}::${proj} for ${pf}; moving"
          // run cmd: "mv bsp_${pf_bsp} ${proj}", cwd: BUILT_AREA, fail: true
          InstalledBSPs << pf_bsp
        } else {
          if (DEBUG) println "(Skipping; ${pf_bsp} has already been installed for ${pf}.)"
        }
        extracted = true
      } // not shared
    } // not BSP
    if (!extracted) {
    */
    try {
      run cmd: "tar -xC ../.. -f workspace_${proj}_${pf}.tar.xz && rm workspace_${proj}_${pf}.tar.xz", cwd: BUILT_AREA+"/workspaces/platforms/", fail: true
      pfs << pf
    } catch (err) {
      if (!(proj in bsp_list)) throw err
      println "Untarring ${proj} for platform ${pf} failed, but it is a BSP so (hopefully) this is acceptable."
    }
    /*DELME } */
  } // each pf in hdl_platforms
  if (DEBUG) println "Untarring complete for ${proj}. Platforms found: ${pfs}"

  if (!pfs) {
    if (DEBUG) println "No platforms found! Aborting ${proj} closure."
    return
  }
  // Jenkins artifact tool mangles symlinks so nuke imports and exports of the project
  run cmd: "rm -rf imports exports", cwd: BUILT_AREA+proj, fail: true
  tests_list[proj] = [:]
  tests_pfs[proj] = pfs
/*
  // For testing of duplicate finding code, inserts a top-level device to conflict with platform-specific one:
    if (DEBUG && proj == "assets") {
      tests_list[proj]["devices__matchstiq_z1_rx"] = "hdl/platforms/matchstiq_z1/devices/matchstiq_z1_rx.test/"
    }
*/
  // The "register project" output text won't be valid JSON and can be ignored; if there's an error, it will be dumped from "run" failing
  def final proj_metadata = new JsonSlurper().parseText(run([cmd:
     """set -e
     export OCPI_PROJECT_REGISTRY_DIR=${WORKSPACE}/cdk_extracted/registry
     # Need to put a 'core' in place to know about centos platforms in opencpi-setup.sh .
     if [ ! -d ${WORKSPACE}/cdk_extracted/opt/opencpi/projects ]; then
       mkdir ${WORKSPACE}/cdk_extracted/opt/opencpi/{projects,prerequisites}
       ln -s ${BUILT_AREA}/core ${WORKSPACE}/cdk_extracted/opt/opencpi/projects/core
     fi
     source ${WORKSPACE}/cdk_extracted/opt/opencpi/cdk/opencpi-setup.sh -s
     if ! grep -sqF /built_area/${proj} <(ocpidev show projects --table); then
       ocpidev register project 1>&2
     fi
     ocpidev show tests --json --local-scope
     """, cwd: BUILT_AREA+proj, fail: true]))

  if (DEBUG) println "proj_metadata: " + JsonOutput.prettyPrint(JsonOutput.toJson(proj_metadata))

  proj_metadata.'project'.'libraries'.each { lib_name, lib_val ->
    if (DEBUG) println "Parsing library ${lib_name}"
    lib_val?.'tests'.each { test_name, test_val ->
      def rel_path = test_val.replace(proj_metadata.project.directory+'/', '') // remove leading paths
      def line = rel_path - ~/\.test\/?$/ // remove suffix
      line = line.replace('components/', '') // ignore "components"...
      line = line.replace('hdl/', '') // ...and "hdl"
      line = line.replace('/', '__') // expand all slashes to double unders
      def orig_line = line // save this off in case of conflicts
      line = line.replaceAll('^platforms__.*?__', '') // remove platforms/XXX/ prefix ( https://stackoverflow.com/a/4316725/836748 )
      if (!tests_list[proj].containsKey(line)) {
        tests_list[proj][line] = rel_path
      } else {
        if (DEBUG) println "${proj}::${line} has name collision - using ${proj}::${orig_line}"
        tests_list[proj][orig_line] = rel_path
      }
    } // test_name / test_val
  } // lib_name / lib_val
} // each proj

if (DEBUG) println "All tests:" + JsonOutput.prettyPrint(JsonOutput.toJson(tests_list))

if (COMPONENTS) { // User-specified - must filter
  def final comps = COMPONENTS.tokenize(", ")
  if (DEBUG) println "Filtered components requested: "+comps
  // We will allow the user to specify the component by:
  // simple name: bias or data_src
  // entire library: misc_comps
  def new_tests_list = [:]
  def seen_comps = [:]
  projs.each { proj ->
    tests_list[proj].each { test, path ->
      test.split('__').each { // '/' is mapped to '__' elsewhere
        if (it in comps) {
          if (DEBUG) println "Match: ${test} (${it})"
          if (!new_tests_list.containsKey(proj))
            new_tests_list[proj] = [:]
          new_tests_list[proj][test] = path
          seen_comps[it] = 1
        } // match
      } // each split test
    } // each test
  } // each proj
  tests_list = new_tests_list
  if (!tests_list)
    throw new javaposse.jobdsl.dsl.DslException("Requested tests ${comps} but resulted in none!")
  // Cannot compare output list size because could be 1:many
  if (seen_comps.size() != comps.size())
      throw new javaposse.jobdsl.dsl.DslException("Requested ${comps.size()} (${comps}) but only matched ${seen_comps.size()} (${seen_comps.keySet()})!")
}

// Make sure the top two folders exist
folder("/${ROOT}") {
  displayName ROOT.replace('_', ' ')
}
folder("/${ROOT}/${BRANCH_FOLDER}") {
  displayName GIT_SELECTED_BRANCH_stripped
}

// Set up folders and jobs starting with platforms
run cmd: "mkdir -p testing_support/build_configs"
hdl_platforms.each { pf ->
  if (DEBUG) println "Creating folder /${ROOT}/${BRANCH_FOLDER}/${pf} "
  folder("/${ROOT}/${BRANCH_FOLDER}/${pf}") {
    displayName pf.replace('_dot_', '.').replace('_', ' ')
    def desc = "${pf} platform"
    if (pf =~ /sim$/) desc += " (and host RCC)"
    description(desc)
  }
  // What is our license restriction? (TODO: Copied from Jenkinsfile_build_for_testing)
  // Job 2 ensures that there is SOMETHING set for tool_license (could be "unlimited")
  def lic_tool = job2_config.branch_config.hdl_platforms[pf].tool_license
  if (DEBUG) println "Need license for ${pf} (looking up tool ${lic_tool})"
  if (job2_config.branch_config.tools[lic_tool]?.never_license)
    lic_tool = null // Clear it if "never_license" (unlimited) is set
  def sim_projs = [] as Set
  projs.each { proj ->
    if (!(pf in tests_pfs[proj])) {
      /* if (DEBUG) */ println "Skipping unbuilt project '${proj}' for platform '${pf}'"
      return
    }
    tests_list[proj].each { test_in, path ->
      def test = test_in
      def final skip = proj in sim_projs // Skip if this project already queued for simulator
      def my_job = null
      if (pf =~ /sim$/ || lic_tool) { // Special rules for simulators or licensed - build all at once (AV-4529)
        test = "all"
        if (!skip) { // Not already queued
          if (DEBUG) println "Creating single build job for all:"
          sim_projs << proj
        } // already queued or not
      } // sim pf
      if (!skip) {
        println "Creating job /${ROOT}/${BRANCH_FOLDER}/${pf}/build_${proj}__${test} to run in ${proj}/${path}"
        // Work around JENKINS-51038 - store variables into file for next job to read from.
        // External to this DSL file, everything in "build_configs" is archived as an artifact
        // Note: The file gets re-written for each platform so needs to be platform agnostic
        // Note: We export stuff for both an "all" build (tests_list) and a targeted
        //       build (COMPONENT_TO_TEST)
        export_JSON file: "build_${proj}__${test}.env",
                    input: ['COMPONENT_TO_TEST': "${proj}/${path}",
                            'proj': proj,
                            'tests_list': tests_list[proj] ],
                    cwd: WORKSPACE+"/testing_support/build_configs"
        my_job = pipelineJob("/${ROOT}/${BRANCH_FOLDER}/${pf}/build_${proj}__${test}")
        my_job.with {
          description """${test} on ${pf} (${BRANCH_FOLDER})

This job cannot execute the tests itself because it uses "hdlbuilder" resources, not "hdl" assets.""".replace('_dot_', '.').replace('__', '/').replace('_', ' ')
          displayName "${proj}: build ${test}".replace('_dot_', '.').replace('__', '/').replace('_', ' ')
          // Set up parameters
          // https://issues.jenkins-ci.org/browse/JENKINS-51038
          /*
          environmentVariables {
            envs(
              Upstream_Job_Name: "${JOB_NAME}",
              Upstream_Job_Build: "${BUILD_NUMBER}",
              GIT_BRANCH: "${BRANCH_FOLDER}",
              HDL_PLATFORM: "${pf}",
              COMPONENT_TO_TEST: "${proj}/${path}",
              )
            keepBuildVariables(true)
            keepSystemVariables(true)
            overrideBuildParameters()
          }
          */
          definition {
              cps {
                  // Note: readFileFromWorkspace is SEED JOB's workspace
                  script(readFileFromWorkspace('releng/jenkins/runtime/groovy/Jenkinsfile_build_for_testing.groovy'))
                  sandbox()
              }
          }
          quietPeriod 1 // 1 second
        } // my_job
      } // no skip
      // Set up run job
      if (DEBUG) println "Creating job /${ROOT}/${BRANCH_FOLDER}/${pf}/run_${proj}__${test_in} to run in ${proj}/${path}"
      def my_run_job = pipelineJob("/${ROOT}/${BRANCH_FOLDER}/${pf}/run_${proj}__${test_in}")
      my_run_job.with {
        concurrentBuild(false)
        description """${test_in} on ${pf} (${BRANCH_FOLDER})""".replace('_dot_', '.').replace('__', '/').replace('_', ' ')
        displayName "${proj}: run ${test_in}".replace('_dot_', '.').replace('__', '/').replace('_', ' ')
        definition {
          cps {
            // Note: readFileFromWorkspace is SEED JOB's workspace
            script(readFileFromWorkspace('releng/jenkins/runtime/groovy/Jenkinsfile_run_for_testing.groovy'))
            sandbox()
          }
        }
        logRotator {
          artifactDaysToKeep(30)
          artifactNumToKeep(3)
          numToKeep(20)
        }
        quietPeriod 1 // 1 second
      } // my_run_job
      if (pf =~ /sim$/) {
        if (DEBUG) println "Running under simulator; adding parameter choices."
        my_run_job.with {
          parameters { // AV-5359
            choiceParam('Test File IO', ['Test Native', 'Force HDL File IO', 'Force RCC File IO'], 'Override File I/O in unit tests (currently only can force RCC!)') // AV-5373
          }
        }
      } // simulator
      // Launch build job
      if (my_job)
        queue(my_job) // should be able to be a string too
    } // each test + path
  } // each proj
} // each pf

// Cleanup so the archivers don't get confused by circular symlinks
run cmd: "rm -rf ${projs.collect{"$it/{imports,exports}"}.join(' ')} ${WORKSPACE}/cdk_extracted/registry", cwd:BUILT_AREA

// Dereference any symlinks that point outside of this tree (AV-5365)
// Try to be somewhat "safe" about it
av5364_res = run cmd: '''
echo "Dereferencing any symlinks that point outside of this tree (with whitelist)"
while IFS= read -r -d '' link
do
  finfo=$(file ${link})
  if [[ ${finfo} == *"broken symbolic link"* ]]; then
    echo "Ignoring broken link: ${link}"
  else
    for whitelist in /usr/{include,share}; do
      if [[ ${finfo} == *"symbolic link to \\`${whitelist}"* ]]; then
        echo "Fixing absolute link: ${finfo}"
        mv ${link} ${link}.link
        cp -R -L ${link}.link ${link}
        rm ${link}.link
      fi
    done
  fi
done < <(find . -type l -lname '/*' -print0)
echo "Deleting all other absolute symlinks:"
find . -type l -lname '/*' -not -xtype l
find . -type l -lname '/*' -not -xtype l -delete
''', cwd:BUILT_AREA
println av5364_res
