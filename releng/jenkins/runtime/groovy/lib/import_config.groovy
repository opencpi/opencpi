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

// import groovy.json.JsonOutput

def usage_example = """
checkout scm
def import_config = load("releng/jenkins/runtime/groovy/lib/import_config.groovy")
def branch_config = import_config()
def bsp_repos = []
branch_config.hdl_platforms.each { pf, dat -> if (dat.bsp_repo) bsp_repos.push([pf,dat.bsp_repo]) }
"""

// Helper function to push "true" into a key if it doesn't exist
void default_enable(config, final in_key) {
  // We cannot use the Elvis operator because "false" looks like a null.
  if (!config.containsKey(in_key)) config[in_key] = true
}

// This assumes that you already have a checkout and you run this from the base of the checkout
def call() {
  echo "import_config: Importing configuration information"
  def branch_config = readJSON file: "releng/jenkins/runtime/config.json"
  // Walk thru all platforms and inject enabled/hdl_enabled flags.
  branch_config.hdl_platforms.each { pfn, dat ->
    default_enable(branch_config.hdl_platforms[pfn], "enabled")
    default_enable(branch_config.hdl_platforms[pfn], "hdl_enabled")
  }
  branch_config.rcc_platforms.each { pfn, dat ->
    default_enable(branch_config.rcc_platforms[pfn], "enabled")
  }

  // For SOME REASON (???) Jenkins won't allow map.remove() to be called...
  // Filters out any tag that starts with _ for comments
  def branch_config_clean = [:]
  branch_config.each{ k, v -> if (! k.startsWith("_")) branch_config_clean[k] = v }
  // echo JsonOutput.toJson(branch_config)
  return branch_config_clean
}

return this
