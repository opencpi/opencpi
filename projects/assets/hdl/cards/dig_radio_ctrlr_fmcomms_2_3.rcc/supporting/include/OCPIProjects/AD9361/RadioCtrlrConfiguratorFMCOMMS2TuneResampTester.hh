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

#ifndef _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_FMCOMMS2_TUNE_RESAMP_TESTER_HH
#define _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_FMCOMMS2_TUNE_RESAMP_TESTER_HH

#include "RadioCtrlrConfiguratorTester.hh"             // ConfiguratorTester
#include "RadioCtrlrConfiguratorFMCOMMS2TuneResamp.hh" // ConfiguratorFMCOMMS2TuneResamp

namespace OCPIProjects {

namespace RadioCtrlr {

class ConfiguratorFMCOMMS2TuneResampTester :
    public ConfiguratorTester<ConfiguratorFMCOMMS2TuneResamp<> > {

public    : ConfiguratorFMCOMMS2TuneResampTester(
                ConfiguratorFMCOMMS2TuneResamp<>& UUT);

public    : bool run_tests();

protected : void test_constrain_RX_gain_mode_abs_range();
protected : void test_constrain_TX_gain_mode_abs_range();
protected : void test_constrain_RX_gain_abs_range();
protected : void test_constrain_TX_gain_abs_range();
protected : void test_constrain_tuning_freq_equals_RFPLL_LO_freq_plus_complex_mixer_NCO_freq(
                data_stream_ID_t data_stream_ID, config_key_t RFPLL_LO_freq);
protected : void test_constrain_data_stream_tuning_freq_by_RFPLL_LO_freq_plus_data_stream_complex_mixer_NCO_freq(
                data_stream_ID_t data_stream_ID,
                config_key_t     RFPLL_LO_freq,
                config_value_t   expected_min_NCO_freq,
                config_value_t   expected_max_NCO_freq);
protected : void test_constrain_RFPLL_LO_freq_by_data_stream_tuning_freq_minus_data_stream_complex_mixer_NCO_freq(
                data_stream_ID_t data_stream_ID,
                config_key_t     RFPLL_LO_freq,
                config_value_t   expected_min_NCO_freq,
                config_value_t   expected_max_NCO_freq);
protected : void test_constrain_data_stream_complex_mixer_NCO_freq_by_RFPLL_LO_freq_minus_data_stream_tuning_freq(
                data_stream_ID_t data_stream_ID,
                config_key_t     RFPLL_LO_freq,
                config_value_t   expected_min_NCO_freq,
                config_value_t   expected_max_NCO_freq);
protected : void test_constrain_RX_gain_limits();
protected : void test_constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_dec(
                data_stream_ID_t data_stream_ID,
                config_key_t     frontend_bandwidth);
protected : void test_constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_int(
                data_stream_ID_t data_stream_ID,
                config_key_t     frontend_bandwidth);
protected : void test_constrain_RX_SAMPL_FREQ_to_TX_SAMPL_FREQ_multiplied_by_DAC_Clk_divider();
protected : void test_constrain_TX_SAMPL_FREQ_to_RX_SAMPL_FREQ_divided_by_DAC_Clk_divider();
protected : void test_constrain_DAC_Clk_divider_to_RX_SAMPL_FREQ_divided_by_TX_SAMPL_FREQ();
protected : void test_constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_dec(
                data_stream_ID_t data_stream_ID,
                config_key_t frontend_samp_rate);
protected : void test_constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_int(
                data_stream_ID_t data_stream_ID,
                config_key_t frontend_samp_rate);
protected : void test_constrain_FE_samp_rate_to_func_of_DS_complex_mixer_freq(
                data_stream_ID_t data_stream_ID,
                config_key_t     frontend_samp_rate,
                config_value_t   expected_min_NCO_freq,
                config_value_t   expected_max_NCO_freq,
                config_value_t   expected_min_sampling_rate_Msps,
                config_value_t   expected_max_sampling_rate_Msps);
protected : void test_constrain_DS_complex_mixer_freq_to_func_of_FE_samp_rate(
                data_stream_ID_t data_stream_ID,
                config_key_t     frontend_samp_rate,
                config_value_t   expected_min_NCO_freq,
                config_value_t   expected_max_NCO_freq,
                config_value_t   expected_min_sampling_rate_Msps,
                config_value_t   expected_max_sampling_rate_Msps);
protected : void test_constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(
                data_stream_ID_t data_stream_ID,
                config_key_t     frontend_samp_rate,
                config_value_t   expected_min_NCO_freq,
                config_value_t   expected_max_NCO_freq,
                config_value_t   expected_min_sampling_rate_Msps,
                config_value_t   expected_max_sampling_rate_Msps);
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

}; // class ConfiguratorFMCOMMS2TuneResampTester

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "RadioCtrlrConfiguratorFMCOMMS2TuneResampTester.cc"

#endif // _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_FMCOMMS2_TUNE_RESAMP_TESTER_HH
