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

/* Used to checkout a pre-cached version of a repository and then sync with "real" master.
   Will check out into CURRENT directory, so wrap with dir() call as needed. 2nd (optional)
   argument is whether or not to do the final "checkout" call. If false, returns empty string!

   NOTE: The connection to the git server has improved considerably; however, since the repo
   is so large, and will never get smaller, this still results in considerable speed
   improvements; as much as 30s each time.
*/
import groovy.transform.Field

def call(caller_in, scm_in, do_checkout_in = true) {
  def final cached_checkout_enabled = true
  def final usage_example = """
  Original:
  checkout scm

  Replaced:
  cached_checkout this, scm
  """

  def my_scm = scm_in
  def do_checkout = do_checkout_in
  def caller = caller_in
  def scmVars = ''
  // 'scm' described in https://javadoc.jenkins.io/plugin/git/hudson/plugins/git/GitSCM.html
  // but severely limited in what scripts can see/do, e.g. getRepositories/getRepositoryByName banned
  // See JENKINS-42860 for some stuff I hand-smashed into scriptApproval.xml...
  // However, we might be handed a HashMap that will BECOME an SCM object in the future, but isn't yet,
  //  and we cannot check with "instanceof" in a groovy sandbox environment, so just try
  def scmUrl = ''
  try {
    // Native object call
    scmUrl = my_scm.getUserRemoteConfigs()[0].getUrl()
  } catch (err) {
    // Proto-object (e.g. from checkout_branch)
    assert err.message.contains('No signature of method')
    scmUrl = my_scm["userRemoteConfigs"][0]["url"]
  }
  // Want to get last term and then make the '.git' ⟶ '/.git' along with mapping '.' to '_'
  // e.g. "av.bsp.boardname.git ⟶ av_bsp_boardname/.git" or "opencpi.git" ⟶ 'opencpi/.git'
  def proj_to_copy = scmUrl.split('/')[-1].replace('.','_').replace('_git','/.git')
  echo "cached_checkout: Detected original URL of '${scmUrl}' and going to pull cached from '${proj_to_copy}'"
  // This next line is to DISABLE this function based on cached_checkout_enabled (if asked to checkout, just do it directly below)
  // If we're not supported to do a final checkout, then give the cached version so that SOMETHING comes out.
  if (cached_checkout_enabled || !do_checkout) {
    try {
      git name: 'cache', // name seems to be ignored?
          url: (('master' == caller.env.NODE_NAME)?'':"ssh://${global_config.scm_cached_job_user()}") +
               global_config.scm_cached_job_workspace() +
               proj_to_copy
    } catch (err) {
      // echo "cached_checkout: in error handler: ${err}"
      assert err.message.contains('find any revision to build') // This is OK / normal because no branches there
    }
    echo "cached_checkout: Cached copy now present"
  } else {
    echo "cached_checkout: Disabled; skipping cache checkout"
  }
  if (do_checkout) scmVars = checkout my_scm
  echo "cached_checkout: Done"
  return scmVars
}

return this
