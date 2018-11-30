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

#ifndef _OCPI_PROJECTS_DIG_RADIO_CONFIGURATOR_FMCOMMS2_TUNE_RESAMP_HH
#define _OCPI_PROJECTS_DIG_RADIO_CONFIGURATOR_FMCOMMS2_TUNE_RESAMP_HH

#include "RadioCtrlrConfiguratorFMCOMMS2.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

/*! @brief Configurator for an FMCOMMS2
 *         whose AD9361 RX data stream signal paths
 *         each contain a Tuner and DownSampler (TDS),
 *         and whose AD9361 TX data stream signal paths
 *         each contain an UpSampler and Tuner (UST).
 *         A configurator is a software-only representation of hardware
 *         capabilities.
 *         This class is mainly intended to be used as a member of a
 *         DigRadioCtrlr class.
 *  @todo / FIXME - move all tune/filt/resamp constraints to a class of its own
 *
 * @verbatim
                 OpenCPI Assembly
           -------------------------------------------------------+
                                cic_dec.hdl     complex_mixer.hdl |   FMCOMMS2
                               (downsampler)    (tuner)           |  +--------+
                                  +----+         +----+           |  |        |
           data stream 0 <--------|    |<--------|    |<----------|<-|----RX1A|<
                                  +----+         +----+           |  |    SMA |
                                                                  |  |        |
                                cic_dec.hdl     complex_mixer.hdl |  |        |
                               (downsampler)    (tuner)           |  |        |
                                  +----+         +----+           |  |        |
           data stream 1 <--------|    |<--------|    |<----------|<-|----RX2A|<
                                  +----+         +----+           |  |    SMA |
                                                                  |  |        |
                                cic_int.hdl     complex_mixer.hdl |  |        |
                               (upsampler)      (tuner)           |  |        |
                                  +----+         +----+           |  |        |
           data stream 2 -------->|    |-------->|    |---------->|->|----TX1A|>
                                  +----+         +----+           |  |    SMA |
                                                                  |  |        |
                                cic_int.hdl     complex_mixer.hdl |  |        |
                               (upsampler)      (tuner)           |  |        |
                                  +----+         +----+           |  |        |
           data stream 3 -------->|    |-------->|    |---------->|->|----TX2A|>
                                  +----+         +----+           |  |    SMA |
                                                                  |  +--------+
           -------------------------------------------------------+
   @endverbatim
 ******************************************************************************/
template<class log_callback_t = int (*)(const char*, va_list)>
class ConfiguratorFMCOMMS2TuneResamp : public ConfiguratorFMCOMMS2<log_callback_t> {

protected:
using ConfiguratorFMCOMMS2<log_callback_t>::m_data_stream_RX1;
using ConfiguratorFMCOMMS2<log_callback_t>::m_data_stream_RX2;
using ConfiguratorFMCOMMS2<log_callback_t>::m_data_stream_TX1;
using ConfiguratorFMCOMMS2<log_callback_t>::m_data_stream_TX2;
using ConfiguratorFMCOMMS2<log_callback_t>::m_data_streams;
using ConfiguratorFMCOMMS2<log_callback_t>::get_config;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_all_XAB_such_that_X_equals_A_plus_B;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_all_XAB_such_that_X_equals_A_divided_by_B;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_Rx_RFPLL_LO_freq_ge_f_data_stream_0_samp_rate;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_Rx_RFPLL_LO_freq_le_f_data_stream_0_samp_rate;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_Rx_RFPLL_LO_freq_ge_f_data_stream_1_samp_rate;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_Rx_RFPLL_LO_freq_le_f_data_stream_1_samp_rate;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_Tx_RFPLL_LO_freq_ge_f_data_stream_2_samp_rate;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_Tx_RFPLL_LO_freq_le_f_data_stream_2_samp_rate;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_Tx_RFPLL_LO_freq_ge_f_data_stream_3_samp_rate;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_Tx_RFPLL_LO_freq_le_f_data_stream_3_samp_rate;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_data_stream_0_samp_rate_le_2500_minus_2400;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_data_stream_1_samp_rate_le_2500_minus_2400;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_data_stream_2_samp_rate_le_2500_minus_2400;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_data_stream_3_samp_rate_le_2500_minus_2400;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_gain_mode_data_stream_0_equals_0_or_1;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_gain_mode_data_stream_1_equals_0_or_1;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_gain_mode_data_stream_2_equals_1;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_gain_mode_data_stream_3_equals_1;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_gain_dB_data_stream_0_equals_func_of_Rx_RFPLL_LO_freq;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_gain_dB_data_stream_1_equals_func_of_Rx_RFPLL_LO_freq;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_gain_dB_data_stream_2_is_in_range_neg_89p75_to_0;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_gain_dB_data_stream_3_is_in_range_neg_89p75_to_0;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_RX_SAMPL_FREQ_MHz_equals_TX_SAMPL_FREQ_MHz_times_DAC_Clk_divider;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_samples_are_complex_data_stream_0_equals_1;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_samples_are_complex_data_stream_1_equals_1;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_samples_are_complex_data_stream_2_equals_1;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_samples_are_complex_data_stream_3_equals_1;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_rx_rf_bandwidth_less_than_or_equal_to_RX_SAMPL_FREQ_MHz;
using ConfiguratorFMCOMMS2<log_callback_t>::constrain_tx_rf_bandwidth_less_than_or_equal_to_TX_SAMPL_FREQ_MHz;

/// @brief Assumes any decimator/interpolation values are possible
public    : ConfiguratorFMCOMMS2TuneResamp();

public    : ConfiguratorFMCOMMS2TuneResamp(
                ConfigValueRanges CIC_dec_abs_ranges,
                ConfigValueRanges CIC_int_abs_ranges);

protected : void add_stream_config_RX_tuning_freq_complex_mixer(
                data_stream_ID_t data_stream);

protected : void add_stream_config_TX_tuning_freq_complex_mixer(
                data_stream_ID_t data_stream);

protected : void add_stream_config_CIC_dec_decimation_factor(
                data_stream_ID_t  data_stream,
                ConfigValueRanges CIC_dec_abs_ranges);

protected : void add_stream_config_CIC_int_interpolation_factor(
                data_stream_ID_t  data_stream,
                ConfigValueRanges CIC_int_abs_ranges);

protected : void constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(
                data_stream_ID_t data_stream,
                config_key_t     samp_rate);

protected : void constrain_FE_samp_rate_to_func_of_DS_complex_mixer_freq(
                data_stream_ID_t data_stream,
                config_key_t     samp_rate);

protected : void constrain_DS_complex_mixer_freq_to_func_of_FE_samp_rate(
                data_stream_ID_t data_stream,
                config_key_t     samp_rate);

protected : void constrain_tuning_freq_equals_Rx_RFPLL_LO_freq_plus_complex_mixer_NCO_freq(
                data_stream_ID_t data_stream);

protected : void constrain_tuning_freq_equals_Tx_RFPLL_LO_freq_plus_complex_mixer_NCO_freq(
                data_stream_ID_t data_stream);

protected : void constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_dec(
                data_stream_ID_t data_stream,
                config_key_t     frontend_bandwidth);

protected : void constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_int(
                data_stream_ID_t data_stream,
                config_key_t     frontend_bandwidth);

protected : void constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_dec(
                data_stream_ID_t data_stream,
                config_key_t     frontend_samp_rate);

protected : void constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_int(
                data_stream_ID_t data_stream,
                config_key_t     frontend_samp_rate);

/*! @brief The FMCOMMS2TuneResamp configuration space is a further-constrained
 *         version of the FMCOMMS2 configuration space.
 ******************************************************************************/
protected: virtual void impose_constraints_single_pass();

}; // class ConfiguratorFMCOMMS2TuneResamp

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "RadioCtrlrConfiguratorFMCOMMS2TuneResamp.cc"

#endif // _OCPI_PROJECTS_DIG_RADIO_CONFIGURATOR_FMCOMMS2_TUNE_RESAMP_HH
