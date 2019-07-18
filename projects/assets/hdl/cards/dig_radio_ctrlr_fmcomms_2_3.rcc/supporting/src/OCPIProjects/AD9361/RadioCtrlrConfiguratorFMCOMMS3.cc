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

#include "RadioCtrlrConfiguratorFMCOMMS3.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

/*! @brief data stream 0 -maps-to-> FMCOMMS3 RX1A SMA port,
 *         data stream 1 -maps-to-> FMCOMMS3 RX2A SMA port,
 *         data stream 2 -maps-to-> FMCOMMS3 TX2A SMA port,
 *         data stream 3 -maps-to-> FMCOMMS3 TX2A SMA port.
 *         A configurator is a software-only representation of hardware
 *         capabilities.
 ******************************************************************************/
template<class LC>
ConfiguratorFMCOMMS3<LC>::ConfiguratorFMCOMMS3() :
    ConfiguratorAD9361<LC>("SMA_RX1A","SMA_RX2A","SMA_TX1A","SMA_TX2A") {
}

template<class LC>
void ConfiguratorFMCOMMS3<LC>::impose_constraints_single_pass() {

  // FMCOMMS3 constraints are exactly the same as AD9361 constraints

  ConfiguratorAD9361<LC>::impose_constraints_single_pass();
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
