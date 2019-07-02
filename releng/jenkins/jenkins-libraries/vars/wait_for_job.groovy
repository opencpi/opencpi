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


/* Used to wait for another job to complete by polling it.
   NOTE: It will wait a MINIMUM of 2 * polling delay.
*/
def usage_example = """
@Library('opencpi-libs')_
wait_for_job(currentBuild.upstreamBuilds[0])
def polling_delay_in_sec = 30
wait_for_job(currentBuild.upstreamBuilds[0], polling_delay_in_sec)
"""

def call(watch_job_in, delay_in = 5) {
  def watch_job = watch_job_in
  def final delay = delay_in
  def watch_job_runtimes = [-2, -1]
  while (watch_job_runtimes[-1] != watch_job_runtimes[-2]) {
    sleep time: delay, unit: 'SECONDS'
    watch_job_runtimes << watch_job.getDuration()
    echo "Waiting for upstream job (${watch_job.getFullDisplayName()}) to complete."
  }
}

return this
