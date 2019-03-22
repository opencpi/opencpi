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

#ifndef _AD9361_CONFIG_PROXY_HH
#define _AD9361_CONFIG_PROXY_HH

#include <cstdint> // uint8_t, uint32_t
#include <sstream> // std::ostringstream
#include <string> // std::string
#include "LogForwarder.hh" // LogForwarder class

extern "C" {
#include "ad9361_api.h" // RX1, RX2, TX1, TX2 macros
}

namespace OCPIProjects {

/*! @brief std::string is expected to be thrown (w/ strong guarantee of
 *         exception safety) for any and all register reads which correspond
 *         to invalid values.
 *  @todo / FIXME - Eventually all OpenCPI AD9361 functionality, No-OS or
 *                  otherwise, will be moved inside of this class.
 ******************************************************************************/
template<class ad9361_config_slave_t>
class AD9361ConfigProxy : public LogForwarder<OCPI_log_func_args_t> {

protected : using LogForwarder<OCPI_log_func_args_t>::log_debug;
protected : using LogForwarder<OCPI_log_func_args_t>::set_forwarding_callback_log_info;
protected : using LogForwarder<OCPI_log_func_args_t>::set_forwarding_callback_log_debug;
protected : using LogForwarder<OCPI_log_func_args_t>::set_forwarding_callback_log_warn;
protected : using LogForwarder<OCPI_log_func_args_t>::set_forwarding_callback_log_error;
protected : using LogForwarder<OCPI_log_func_args_t>::set_arg0_log_info;
protected : using LogForwarder<OCPI_log_func_args_t>::set_arg0_log_debug;
protected : using LogForwarder<OCPI_log_func_args_t>::set_arg0_log_warn;
protected : using LogForwarder<OCPI_log_func_args_t>::set_arg0_log_error;

/*! @brief Subset of AD9361 register set which is used internally for caching
 *         purposes. Only registers necessary for internal calculation are
 *         included, and more registers may be added in the future. Each member
 *         corresponds to an ad9361_config.hdl property (which also corresponds
 *         to a single 8-bit AD9361 register).
 ******************************************************************************/
protected : struct reg_cache_t {
              uint8_t m_bbpll_ref_clock_scaler;
              uint8_t m_bbpll_integer_bb_freq_word;
              uint8_t m_bbpll_fract_bb_freq_word_1;
              uint8_t m_bbpll_fract_bb_freq_word_2;
              uint8_t m_bbpll_fract_bb_freq_word_3;
              uint8_t m_clock_bbpll;
              uint8_t m_general_rx_enable_filter_ctrl;
            } m_reg_cache;

/*  @brief Reference to object passed around to/from No-OS method calls.
 *  @todo / FIXME - Eventually all OpenCPI AD9361 functionality, No-OS or
 *                  otherwise, will be moved inside of this class. When that
 *                  happens, this will be a pointer and not a reference to a
 *                  a pointer.
 ******************************************************************************/
protected : struct ad9361_rf_phy*& m_ad9361_rf_phy;
/// @brief Reference to OpenCPI RCC slave object for ad9361_config.hdl.
protected : ad9361_config_slave_t&  m_ad9361_config_slave;
protected : double                  m_fref_Hz;
//@{
/*! @brief Bitmasks which correspond to the register bit idenfiers D7 - D0 in
 *         AD9361_Register_Map_Reference_Manual_UG-671.pdf
 ******************************************************************************/
protected : const int m_BITMASK_D7;
protected : const int m_BITMASK_D6;
protected : const int m_BITMASK_D5;
protected : const int m_BITMASK_D4;
protected : const int m_BITMASK_D3;
protected : const int m_BITMASK_D2;
protected : const int m_BITMASK_D1;
protected : const int m_BITMASK_D0;
//@}

private   : const char* m_no_os_msg;

/*! @param[in] slave   Reference to OpenCPI RCC slave object for
 *                     ad9361_config.hdl.
 *  @param[in] fref_Hz Clock rate of the AD9361 reference clock connected.
 *  @todo / FIXME - Eventually all OpenCPI AD9361 functionality, No-OS or
 *                  otherwise, will be moved inside of this class. When that
 *                  happens, this method will be deprecated!!!
 ******************************************************************************/
public    : AD9361ConfigProxy(struct ad9361_rf_phy*& phy,
                              ad9361_config_slave_t& slave, double fref_Hz);

/*! @param[in] slave   Reference to OpenCPI RCC slave object for
 *                     ad9361_config.hdl.
 *  @param[in] fref_Hz Clock rate of the AD9361 reference clock connected.
 ******************************************************************************/
public    : AD9361ConfigProxy(ad9361_config_slave_t& slave, double fref_Hz);

//@{
/*! @brief One of the methods which exposes a high-level methods of the No-OS
 *         software library. For more info see
 *         https://wiki.analog.com/resources/eval/user-guides/ad-fmcomms2-ebz/software/no-os-functions
 ******************************************************************************/
public    : int32_t ad9361_get_rx_fir_config(uint8_t rx_ch,
                                             AD9361_RXFIRConfig *fir_cfg);
public    : int32_t ad9361_get_rx_fir_en_dis(uint8_t *en_dis);
//@}

//@{
/*! @brief One of the methods which exposes high-level AD9361 functionality not
 *         included in the No-OS software library.
 ******************************************************************************/

/*! @brief See AD9361_Reference_Manual_UG-570.pdf equation (14).
 *  @param[in] rx_ch AD9361 RX RF port indicator. Must be set to one of the
 *                   No-OS RX1 or RX2 macro values (throws std::string exception
 *                   otherwise).
 ******************************************************************************/
public    : double  get_digital_rx_block_delay_sec(uint8_t rx_ch);
//@}

//@{
/*! @brief One of the methods which exposes low-level AD9361 functionality not
 *         included in the No-OS software library.
 ******************************************************************************/

/*! @param[in] rx_ch AD9361 RX RF port indicator. Must be set to one of the
 *                   No-OS RX1 or RX2 macro values (throws std::string exception
 *                   otherwise).
 ******************************************************************************/
public    : uint8_t  get_rx_fir_num_taps(uint8_t rx_ch);
/*! @param[in] rx_ch AD9361 RX RF port indicator. Must be set to one of the
 *                   No-OS RX1 or RX2 macro values (throws std::string exception
 *                   otherwise).
 ******************************************************************************/
public    : uint8_t  get_rx_fir_filter_order(uint8_t rx_ch);
public    : double   get_bbpll_input_fref_Hz() const;
public    : uint32_t get_bbpll_n_fractional();
public    : float    get_bbpll_ref_scaler();
public    : uint8_t  get_bbpll_n_integer();
public    : uint8_t  get_bbpll_divider();
public    : bool     get_rhb3_enable();
public    : uint8_t  get_rhb3_decimation_factor();
public    : uint8_t  get_rhb3_num_taps();
public    : uint8_t  get_rhb3_filter_order();
public    : bool     get_rhb2_enable();
public    : uint8_t  get_rhb2_decimation_factor();
public    : uint8_t  get_rhb2_num_taps();
public    : uint8_t  get_rhb2_filter_order();
public    : bool     get_rhb1_enable();
public    : uint8_t  get_rhb1_decimation_factor();
public    : uint8_t  get_rhb1_num_taps();
public    : uint8_t  get_rhb1_filter_order();
public    : double   get_bbpll_freq_Hz();
public    : double   get_adc_freq_Hz();
public    : double   get_r2_freq_Hz();
public    : double   get_r1_freq_Hz();
public    : double   get_clkrf_freq_Hz();
/*! @param[in] rx_ch AD9361 RX RF port indicator. Must be set to one of the
 *                   No-OS RX1 or RX2 macro values (throws std::string exception
 *                   otherwise).
 *  @return Indicates whether the specified AD9361 RF port is actively sending
 *          DAC samples or being digitized via an ADC.
 ******************************************************************************/
public    : bool     get_rx_ch_is_enabled(uint8_t rx_ch);
//@}

//@{ /// @brief One of the methods for internal use only.
protected : void     register_OpenCPI_logging_API();
/** Allows use of use_reg_cache=true in certain (not necessarily all) internal
 *  API methods. */
protected : void     cache_clkrf_freq_regs();

protected : uint32_t _get_bbpll_n_fractional(        bool use_reg_cache = false);
protected : float    _get_bbpll_ref_scaler(          bool use_reg_cache = false);
protected : uint8_t  _get_bbpll_n_integer(           bool use_reg_cache = false);
protected : uint8_t  _get_bbpll_divider(             bool use_reg_cache = false);
protected : bool     _get_rhb3_enable(               bool use_reg_cache = false);
protected : uint8_t  _get_rhb3_decimation_factor(    bool use_reg_cache = false);
protected : uint8_t  _get_rhb3_num_taps(             bool use_reg_cache = false);
protected : uint8_t  _get_rhb3_filter_order(         bool use_reg_cache = false);
protected : bool     _get_rhb2_enable(               bool use_reg_cache = false);
protected : uint8_t  _get_rhb2_decimation_factor(    bool use_reg_cache = false);
protected : bool     _get_rhb1_enable(               bool use_reg_cache = false);
protected : uint8_t  _get_rhb1_decimation_factor(    bool use_reg_cache = false);
protected : double   _get_bbpll_freq_Hz(             bool use_reg_cache = false);
protected : double   _get_adc_freq_Hz(               bool use_reg_cache = false);
protected : double   _get_r2_freq_Hz(                bool use_reg_cache = false);
protected : double   _get_r1_freq_Hz(                bool use_reg_cache = false);
protected : double   _get_clkrf_freq_Hz(             bool use_reg_cache = false);
/*! @param[in] rx_ch A std::string exception will be thrown if this is not one
 *                   of the RX1 or RX2 macros (from No-OS header), each of which
 *                   correspond to an AD9361 RF port.
 ******************************************************************************/
protected : double   _get_digital_rx_block_delay_sec(uint8_t rx_ch,
                                                     bool use_reg_cache = false);

protected : void throw_if_ad9361_rf_phy_is_zero();
/*! @brief Necessary because the following No-OS software library methods do not
 *         produce a failure for invalid rx_ch or tx_ch values:
 *         ad9361_get_rx_fir_config
 ******************************************************************************/
protected : void throw_if_invalid_rx_ch(uint8_t rx_ch);

//@}

}; // class AD9361ConfigProxy

} // namespace OCPIProjects

#include "AD9361ConfigProxy.cc"

#endif // _AD9361_CONFIG_PROXY_HH
