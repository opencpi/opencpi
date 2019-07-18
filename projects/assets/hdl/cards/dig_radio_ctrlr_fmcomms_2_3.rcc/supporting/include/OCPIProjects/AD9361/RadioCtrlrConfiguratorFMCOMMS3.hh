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

#ifndef _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_FMCOMMS3_HH
#define _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_FMCOMMS3_HH

#include "RadioCtrlrConfiguratorAD9361.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

/// @brief Configurator for an FMCOMMS3, which has an AD9361.
template<class log_callback_t = int (*)(const char*, va_list)>
class ConfiguratorFMCOMMS3 : public ConfiguratorAD9361<log_callback_t> {

public    : ConfiguratorFMCOMMS3();

protected : virtual void impose_constraints_single_pass();

}; // class ConfiguratorFMCOMMS3

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "RadioCtrlrConfiguratorFMCOMMS3.cc"

#endif // _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_FMCOMMS3_HH
