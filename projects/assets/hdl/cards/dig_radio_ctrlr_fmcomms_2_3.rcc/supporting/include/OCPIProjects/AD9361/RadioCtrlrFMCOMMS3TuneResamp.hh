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

#ifndef _OCPI_PROJECTS_RADIO_CTRLR_RADIO_CTRLR_FMCOMMS3_TUNE_RESAMP_HH
#define _OCPI_PROJECTS_RADIO_CTRLR_RADIO_CTRLR_FMCOMMS3_TUNE_RESAMP_HH

#include "RadioCtrlrNoOSTuneResamp.hh"
#include "RadioCtrlrConfiguratorFMCOMMS3TuneResamp.hh"
#include "OcpiApi.hh" // OCPI::API::

extern "C" {
#include "ad9361.h" // struct ad9361_rf_phy
}

namespace OCPIProjects {

namespace RadioCtrlr {

namespace OA = OCPI::API;

/// @brief controls FMCOMMS3 via an OpenCPI device proxy's slave
template<class slave_t>
class RadioCtrlrFMCOMMS3TuneResamp :
    public RadioCtrlrNoOSTuneResamp<ConfiguratorFMCOMMS3TuneResamp<OCPI_log_func_args_t>, slave_t> {

typedef ConfiguratorFMCOMMS3TuneResamp<OCPI_log_func_args_t> configurator_t;

/*! @brief No-OS only used internally (this class instanced somewhere that
 *         doesn't need to use No-OS).
 *  @param[in] descriptor    Logs prints will be prepended with this (recommend
 *                           setting to name of OpenCPI worker which uses this
 *                           class).
 *  @param[in] slave         Reference to OpenCPI RCC slave object.
 *  @param[in] app           Reference to OpenCPI application object.
 ******************************************************************************/
public    : RadioCtrlrFMCOMMS3TuneResamp(const char* descriptor,
                slave_t& slave, OA::Application& app);

/*! @brief No-OS is available for use externally. ad9361_rf_phy must be freed
 *         (using ad9361_free) outside of this class!!
 *  @param[in] descriptor    Logs prints will be prepended with this (recommend
 *                           setting to name of OpenCPI worker which uses this
 *                           class).
 *  @param[in] slave         Reference to OpenCPI RCC slave object.
 *  @param[in] app           Reference to OpenCPI application object.
 *  @param[in] ad9361_rf_phy Reference to pointer to No_OS AD9361 phy struct.
 ******************************************************************************/
public    : RadioCtrlrFMCOMMS3TuneResamp(const char* descriptor,
                slave_t& slave, OA::Application& app,
                struct ad9361_rf_phy*& ad9361_rf_phy);

}; // class RadioCtrlrFMCOMMS3TuneResamp

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "RadioCtrlrFMCOMMS3TuneResamp.cc"

#endif // _OCPI_PROJECTS_RADIO_CTRLR_RADIO_CTRLR_FMCOMMS3_TUNE_RESAMP_HH
