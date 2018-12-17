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

#ifndef _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_FMCOMMS2_HH
#define _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_FMCOMMS2_HH

#include "RadioCtrlrConfiguratorAD9361.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

/// @brief Configurator for an FMCOMMS2, which has an AD9361.
template<class log_callback_t = int (*)(const char*, va_list)>
class ConfiguratorFMCOMMS2 : public ConfiguratorAD9361<log_callback_t> {

protected : using ConfiguratorAD9361<log_callback_t>::m_data_stream_RX1;
protected : using ConfiguratorAD9361<log_callback_t>::m_data_stream_RX2;
protected : using ConfiguratorAD9361<log_callback_t>::m_data_stream_TX1;
protected : using ConfiguratorAD9361<log_callback_t>::m_data_stream_TX2;
protected : using ConfiguratorAD9361<log_callback_t>::get_config;

public    : ConfiguratorFMCOMMS2();

/*! @brief Constrain config by FMCOMMS 2 balun limits and
 *         minimum data stream sampling rate.
 *         The FMCOMMS2 PCB has balun whose bandwidth limits the RF range to
 *         [ 2400 to 2500 ] MHz, which effectively limits the
 *         Rx_RFPLL_freq_MHz/Tx_RFPLL_freq_MHz operational range to
 *         [ 2400-(data stream complex sampling_rate_Msps/2) to
 *           2500+(data stream complex sampling_rate_Msps/2) ] MHz for all
 *         data streams.
 ******************************************************************************/
protected : void constrain_by_FMCOMMS2_balun_limits_and_min_DS_samp_rate(
                data_stream_ID_t data_stream_ID, config_key_t key);
/*! @return Boolean indicator of whether config represented by data_stream_ID
 *          and config_key had its possible ranges changed.
 ******************************************************************************/
protected : bool constrain_data_stream_samp_rate_by_f_config(
                data_stream_ID_t data_stream_ID, config_key_t config_key,
                bool t_min_f_max);

protected : bool limit_Y_to_ge_X_times_A_plus_B(
                LockRConstrConfig& cfg_Y, const LockRConstrConfig& cfg_X,
                config_value_t A, config_value_t B) const;
protected : bool limit_Y_to_le_X_times_A_plus_B(
                LockRConstrConfig& cfg_Y, const LockRConstrConfig& cfg_X,
                config_value_t A, config_value_t B) const;
protected : bool constrain_Y_ge_X_times_A_plus_B(
                LockRConstrConfig& cfg_Y, LockRConstrConfig& cfg_X,
                config_value_t A, config_value_t B) const;
protected : bool constrain_Y_le_X_times_A_plus_B(
                LockRConstrConfig& cfg_Y, LockRConstrConfig& cfg_X,
                config_value_t A, config_value_t B) const;

protected : void constrain_config_f_data_stream_samp_rate(
                config_key_t config_key, const std::string& ds,
                bool t_greaterequal_f_lessequal);

protected : void constrain_data_stream_samp_rate_le_2500_minus_2400(
                const std::string& ds);

/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & Rx\_RFPLL\_LO\_freq >= sampling\_rate\_Msps_{data\_stream\_0})/2
 *          + 2400
 * @f}
 ******************************************************************************/
protected : void constrain_Rx_RFPLL_LO_freq_ge_f_data_stream_0_samp_rate();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & Rx\_RFPLL\_LO\_freq <= -sampling\_rate\_Msps_{data\_stream\_0})/2
 *          + 2500
 * @f}
 ******************************************************************************/
protected : void constrain_Rx_RFPLL_LO_freq_le_f_data_stream_0_samp_rate();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & Rx\_RFPLL\_LO\_freq >= sampling\_rate\_Msps_{data\_stream\_1})/2
 *          + 2400
 * @f}
 ******************************************************************************/
protected : void constrain_Rx_RFPLL_LO_freq_ge_f_data_stream_1_samp_rate();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & Rx\_RFPLL\_LO\_freq <= -sampling\_rate\_Msps_{data\_stream\_1})/2
 *          + 2500
 * @f}
 ******************************************************************************/
protected : void constrain_Rx_RFPLL_LO_freq_le_f_data_stream_1_samp_rate();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & Tx\_RFPLL\_LO\_freq >= sampling\_rate\_Msps_{data\_stream\_2})/2
 *          + 2400
 * @f}
 ******************************************************************************/
protected : void constrain_Tx_RFPLL_LO_freq_ge_f_data_stream_2_samp_rate();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & Tx\_RFPLL\_LO\_freq <= -sampling\_rate\_Msps_{data\_stream\_2})/2
 *          + 2500
 * @f}
 ******************************************************************************/
protected : void constrain_Tx_RFPLL_LO_freq_le_f_data_stream_2_samp_rate();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & Tx\_RFPLL\_LO\_freq >= sampling\_rate\_Msps_{data\_stream\_3})/2
 *          + 2400
 * @f}
 ******************************************************************************/
protected : void constrain_Tx_RFPLL_LO_freq_ge_f_data_stream_3_samp_rate();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & Tx\_RFPLL\_LO\_freq <= -sampling\_rate\_Msps_{data\_stream\_3})/2
 *          + 2500
 * @f}
 ******************************************************************************/
protected : void constrain_Tx_RFPLL_LO_freq_le_f_data_stream_3_samp_rate();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & sampling\_rate\_Msps_{data\_stream\_0}) <= 2500 - 2400
 * @f}
 ******************************************************************************/
protected : void constrain_data_stream_0_samp_rate_le_2500_minus_2400();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & sampling\_rate\_Msps_{data\_stream\_1}) <= 2500 - 2400
 * @f}
 ******************************************************************************/
protected : void constrain_data_stream_1_samp_rate_le_2500_minus_2400();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & sampling\_rate\_Msps_{data\_stream\_2}) <= 2500 - 2400
 * @f}
 ******************************************************************************/
protected : void constrain_data_stream_2_samp_rate_le_2500_minus_2400();
/*! @brief This enforces the constraint:
 * @f{eqnarray*}{
 *  s.t. & sampling\_rate\_Msps_{data\_stream\_3}) <= 2500 - 2400
 * @f}
 ******************************************************************************/
protected : void constrain_data_stream_3_samp_rate_le_2500_minus_2400();

/*! @brief The FMCOMMS2 configuration space is a further-constrained version
 *         of the AD9361 configuration space. The purpose of these constraints
 *         is to ensure that the tuning freqs and sampling rates are set such
 *         that the digital data does not include digitized RF spectrum that
 *         is outside of the known FMCOMMS2 balun limits of [2400,2500] MHz.
 * @f}
 * This function enforces the AD9361-specific constraints by calling
 * ConfiguratorAD9361<LC>::impose_constraints_single_pass().
 * In addition to the AD9361-specific constraints, this function applies the
 * following FMCOMMS2-specific constraints:
 * @f{eqnarray*}{
 *  s.t. & Rx\_RFPLL\_LO\_freq >= sampling\_rate\_Msps_{data\_stream\_0})/2
 *          + 2400, \\
 *       & Rx\_RFPLL\_LO\_freq <= -sampling\_rate\_Msps_{data\_stream\_0})/2
 *          + 2500, \\
 *       & Rx\_RFPLL\_LO\_freq >= sampling\_rate\_Msps_{data\_stream\_1})/2
 *          + 2400, \\
 *       & Rx\_RFPLL\_LO\_freq <= -sampling\_rate\_Msps_{data\_stream\_1})/2
 *          + 2500, \\
 *       & Tx\_RFPLL\_LO\_freq >= sampling\_rate\_Msps_{data\_stream\_2})/2
 *          + 2400, \\
 *       & Tx\_RFPLL\_LO\_freq <= -sampling\_rate\_Msps_{data\_stream\_2})/2
 *          + 2500, \\
 *       & Tx\_RFPLL\_LO\_freq >= sampling\_rate\_Msps_{data\_stream\_3})/2
 *          + 2400, \\
 *       & Tx\_RFPLL\_LO\_freq <= -sampling\_rate\_Msps_{data\_stream\_3})/2
 *          + 2500, \\
 *       & sampling\_rate\_Msps_{data\_stream\_0}) <= 2500 - 2400, \\
 *       & sampling\_rate\_Msps_{data\_stream\_1}) <= 2500 - 2400, \\
 *       & sampling\_rate\_Msps_{data\_stream\_2}) <= 2500 - 2400, \\
 *       & sampling\_rate\_Msps_{data\_stream\_3}) <= 2500 - 2400 \\
 * @f}
 ******************************************************************************/
protected : virtual void impose_constraints_single_pass();

}; // class ConfiguratorFMCOMMS2

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "RadioCtrlrConfiguratorFMCOMMS2.cc"

#endif // _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_FMCOMMS2_HH
