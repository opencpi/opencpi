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

#ifndef _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_TESTER_HH
#define _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_TESTER_HH

#include <iostream> // std::cout
#include "RadioCtrlr.hh" // data_stream_ID_t, config_value_t

namespace OCPIProjects {

namespace RadioCtrlr {

template<class T>
class ConfiguratorTester {

protected : T& m_UUT;
public    : ConfiguratorTester(T& UUT);
/*! @brief Test data stream-specificconfig lock.
 *  @param[in] ds_ID     Data stream ID.
 *  @param[in] cfg_key   Config key specifier.
 *  @param[in] expect    Boolean indication of whether lock is expected to be
 *                       successful.
 *  @param[in] val       Val to lock to.
 *  @param[in] tol       Tolerance to use for configurator lock request.
 ******************************************************************************/
protected : void test_config_lock(const data_stream_ID_t& ds_ID,
                const config_key_t& cfg_key, config_value_t val, bool expect,
                config_value_t tol = 0);
/*! @brief Test data stream-agnostic config lock.
 *  @param[in] cfg_key   Config key specifier.
 *  @param[in] expect    Boolean indication of whether lock is expected to be
 *                       successful.
 *  @param[in] val       Val to lock to.
 *  @param[in] tol       Tolerance to use for configurator lock request.
 ******************************************************************************/
protected : void test_config_lock(const config_key_t cfg_key,
                config_value_t val, bool expect,
                config_value_t tol = 0);
/*! @param[in] actual    Actual value.
 *  @param[in] expect    Expected value.
 ******************************************************************************/
protected : void expected_value_did_occur(bool actual, bool expect);

}; // class ConfiguratorTester

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "RadioCtrlrConfiguratorTester.cc"

#endif // _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_TESTER_HH
