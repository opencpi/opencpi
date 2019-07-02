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


/* Used to send a mattermost message to a user.
   Provides plenty of reasonable defaults.
*/
def usage_example = """
@Library('opencpi-libs')_
mattermost_send(this)
or
mattermost_send(this, [msg: "Weekly Assembly Build of `develop` started:"])
or
mattermost_send(this, [msg: "Weekly Assembly Build of `develop` started:", all: true]) // send to '@all'
or
mattermost_send(this, [msg: "Weekly Assembly Build of `develop` started:", icon: "https://jenkins.io/images/logos/austin/austin.png"])
or
mattermost_send(this, [msg: "Develop is broken again!", all: true, icon: "https://jenkins.io/images/logos/fire/fire.png"])
or
mattermost_send(this, [msg: "Weekly Assembly Build of `develop` started:", attn: "@all"])
"""

void call(caller_in, in_config=[]) {
  def caller = caller_in
  def config = in_config
  final msg = config.msg ?: "${caller.env.JOB_NAME}"
  final icon = config.icon ?: "https://jenkins.io/images/logos/formal/formal.png"
  def recip = config.attn ?: '@all'
  if (!config.all && !config.attn) { // Then try to figure out who launched us
    recip = '' // clear it in case we cannot tell who so it won't spam
    def build_causes = caller.currentBuild.getBuildCauses('hudson.model.Cause$UserIdCause')
    build_causes.each {
      recip += '@'+it.userId+' '
    }
  }

  try {
    mattermostSend (
      icon: icon,
      message: "${msg} (<${caller.env.BUILD_URL}|${caller.env.BUILD_NUMBER}>)",
      text: recip
    )
  } catch (err) {
    print "Error sending mattermost message: ${err}"
  } // catch
} // call

return this
