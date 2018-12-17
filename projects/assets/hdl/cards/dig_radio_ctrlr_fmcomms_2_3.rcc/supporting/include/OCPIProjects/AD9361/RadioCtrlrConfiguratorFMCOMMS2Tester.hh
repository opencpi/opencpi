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

#ifndef _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_TESTER_FMCOMMS2_HH
#define _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_TESTER_FMCOMMS2_HH

#include "RadioCtrlr.hh" // config_value_t
#include "RadioCtrlrConfiguratorTester.hh" // ConfiguratorTester
#include "RadioCtrlrConfiguratorFMCOMMS2.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

class ConfiguratorFMCOMMS2Tester :
    public ConfiguratorTester<ConfiguratorFMCOMMS2<> > {

public    : ConfiguratorFMCOMMS2Tester(ConfiguratorFMCOMMS2<>& UUT);

public    : bool run_tests();

protected : void test_constrain_RX_tuning_freqs_equal_Rx_RFPLL_LO_freq();
protected : void test_constrain_TX_tuning_freqs_equal_Tx_RFPLL_LO_freq();
protected : void test_constrain_tuning_freq_FMCOMMS2_balun_limits(config_key_t data_stream_ID);
protected : void test_constrain_RX_bandwidths_equal_rx_rf_bandwidth();
protected : void test_constrain_TX_bandwidths_equal_tx_rf_bandwidth();
protected : void test_constrain_RX_sampling_rates_equal_RX_SAMPL_FREQ();
protected : void test_constrain_TX_sampling_rates_equal_TX_SAMPL_FREQ();
protected : void test_constrain_RX_SAMPL_FREQ_func_TX_SAMPL_FREQ_and_divider();
protected : void test_constrain_TX_SAMPL_FREQ_func_RX_SAMPL_FREQ_and_divider();
protected : void test_constrain_DAC_Clk_divider();
protected : void test_constrain_RX_gain_abs_range();
protected : void test_constrain_RX_gain_limits();
protected : void test_constrain_RX_gain_mode_abs_range();
protected : void test_constrain_TX_gain_abs_range();
protected : void test_constrain_TX_gain_mode_abs_range();

/*! @param[in] bw_min_MHz  Known valid (valid for all data stream) minimum
 *                         bandwidth. Must be be <= fs_min or test will fail.
 *  @param[in] bw_max_MHz  Known valid (valid for all data stream) bandwidth
 *                         which is somewhere between min and max (could very
 *                         well be equal to both in MHz. Must
 *                         be <= fs_middle or test will fail.
 *  @param[in] bw_max_MHz  Known valid (valid for all data stream) maximum
 *                         bandwidth. Must be <= fs_max or test will fail.
 *  @param[in] fs_min_Msps Known valid (valid for all data stream) minimum
 *                         sampling rate.
 *  @param[in] fs_max_Msps Known valid (valid for all data stream) bandwidth
 *                         which is somewhere between min and max (could very
 *                         well be equal to both).
 *  @param[in] fs_max_Msps Known valid (valid for all data stream) maximum
 *                         sampling rate.
 ****************************************************************************/
protected : void test_constrain_Nyquist_criteria(
    config_value_t bw_min_MHz,
    config_value_t bw_middle_MHz,
    config_value_t bw_max_MHz,
    config_value_t fs_min_Msps,
    config_value_t fs_middle_Msps,
    config_value_t fs_max_Msps);

}; // class ConfiguratorFMCOMMS2Tester

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "RadioCtrlrConfiguratorFMCOMMS2Tester.cc"

#endif // _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_TESTER_FMCOMMS2_HH
