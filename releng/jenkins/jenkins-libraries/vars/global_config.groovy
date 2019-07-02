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


// This is used to define any global-level configuration used by groovy code, e.g.
// the IP address of the Jenkins master server

/* I swear at one point I had this working as non-methods where there were just variables with
names like global_config.license_server but then it stopped working...
*/

// Mattermost user name of admin for @attn call-outs
final String jenkins_admin_username() { return 'admin' }

// license server to set OCPI_XILINX_LICENSE_FILE et al
final String license_server() { return '1234@1.2.3.4' }

// Location of local git repo mirrors
final String local_prereq_mirror() { return 'ssh://git@1.2.3.4:/mirror' }

// Location of local copies of prereq tarballs, e.g. patchelf-0.9.tar.gz
final String local_prereq_path() { return '/share/stuff/prerequisites' }

// Top-level URL for Jenkins, e.g. https://192.168.1.1:8086/
final String our_jenkins() { return 'https://1.2.3.4:8080/' }

// NFS top-level export on Jenkins master
final String nfs_export() { return '/export/nfs_export/' }

// ssh login and IP, e.g. jenkins@IP
final String scm_cached_job_user() { return 'jenkins@1.2.3.4' }

// Physical location of sync workspace
final String scm_cached_job_workspace() { return '/var/lib/jenkins_workspaces/git_sync/' }

// Path on master where yum repos will live
final String yum_base() { return '/export/www/yum/' }

// top-level yum repo URL for yum clients
final String yum_repo_url() { return 'http://1.2.3.4/yum/' }

return this
