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

#include <cmath> // pow()
#include "OcpiOsDebugApi.hh" // OCPI_LOG_..., OCPI::OS::logPrintV()
#include "AD9361ConfigProxy.hh" // AD9361ConfigProxy class, AD9361 class

extern "C" {
#include "ad9361_api.h" // AD9361_RXFIRConfig type, ad9361_get_*() methods
#include "ad9361.h" // BBPLL_MODULUS, RX1, RX2 macros
}

namespace OCPIProjects {

uint8_t get_filter_order(uint8_t num_taps) {
  if(num_taps == 0) {
    throw std::string("requested filter order for filter with zero taps");
  }
  return num_taps-1;
}

template<class ad9361_config_slave_t>
AD9361ConfigProxy<ad9361_config_slave_t>::AD9361ConfigProxy(
    struct ad9361_rf_phy*& phy, ad9361_config_slave_t& slave, double fref_Hz) :
    m_ad9361_rf_phy(phy), m_ad9361_config_slave(slave), m_fref_Hz(fref_Hz),
    m_BITMASK_D7(0x80),
    m_BITMASK_D6(0x40),
    m_BITMASK_D5(0x20),
    m_BITMASK_D4(0x10),
    m_BITMASK_D3(0x08),
    m_BITMASK_D2(0x04),
    m_BITMASK_D1(0x02),
    m_BITMASK_D0(0x01),
    m_no_os_msg("No-OS API call: ") {

  register_OpenCPI_logging_API();
}

template<class ad9361_config_slave_t>
AD9361ConfigProxy<ad9361_config_slave_t>::AD9361ConfigProxy(
    ad9361_config_slave_t& slave, double fref_Hz) :
    m_ad9361_config_slave(slave), m_fref_Hz(fref_Hz),
    m_BITMASK_D7(0x80),
    m_BITMASK_D6(0x40),
    m_BITMASK_D5(0x20),
    m_BITMASK_D4(0x10),
    m_BITMASK_D3(0x08),
    m_BITMASK_D2(0x04),
    m_BITMASK_D1(0x02),
    m_BITMASK_D0(0x01),
    m_no_os_msg("No-OS API call: ") {

  register_OpenCPI_logging_API();
}

template<class ad9361_config_slave_t>
int32_t AD9361ConfigProxy<ad9361_config_slave_t>::ad9361_get_rx_fir_config(
    uint8_t rx_ch, AD9361_RXFIRConfig *fir_cfg) {

  throw_if_invalid_rx_ch(rx_ch);

  int rxchi = rx_ch;
  log_debug("%sad9361_get_rx_fir_config(phy, %i, fir_cfg)", m_no_os_msg, rxchi);
  return ::ad9361_get_rx_fir_config(m_ad9361_rf_phy, rx_ch, fir_cfg);
}

template<class ad9361_config_slave_t>
int32_t AD9361ConfigProxy<ad9361_config_slave_t>::ad9361_get_rx_fir_en_dis(
    uint8_t *en_dis) {

  log_debug("%sad9361_get_rx_fir_en_dis(phy, en_dis)", m_no_os_msg);
  return ::ad9361_get_rx_fir_en_dis(m_ad9361_rf_phy, en_dis);
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::get_digital_rx_block_delay_sec(
    uint8_t rx_ch) {

  return _get_digital_rx_block_delay_sec(rx_ch);
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rx_fir_num_taps(
    uint8_t rx_ch) {

  AD9361_RXFIRConfig cfg;

  if(not get_rx_ch_is_enabled(rx_ch)) {
    std::ostringstream oss;
    oss << "requested AD9361 rx_fir_num_taps when rx_ch " << rx_ch;
    oss << " is not enabled";
    throw oss.str();
  }

  log_debug("%sad9361_get_rx_fir_config(phy, %i, fir_cfg)", m_no_os_msg, rx_ch);

  int32_t ret = this->ad9361_get_rx_fir_config(rx_ch, &cfg);
  if(ret != 0) {
    std::ostringstream oss;
    oss << "ad9361_get_rx_fir_config returned non-zero value: " << ret;
    throw oss.str();
  }

  return cfg.rx_coef_size;
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rx_fir_filter_order(
    uint8_t rx_ch) {

  return get_filter_order(get_rx_fir_num_taps(rx_ch));
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::get_bbpll_input_fref_Hz() const {

  return m_fref_Hz;
}

template<class ad9361_config_slave_t>
uint32_t AD9361ConfigProxy<ad9361_config_slave_t>::get_bbpll_n_fractional() {

  return _get_bbpll_n_fractional();
}

template<class ad9361_config_slave_t>
float AD9361ConfigProxy<ad9361_config_slave_t>::get_bbpll_ref_scaler() {
  
  return _get_bbpll_ref_scaler();
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_bbpll_n_integer() {

  return _get_bbpll_n_integer();
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_bbpll_divider() {

  return _get_bbpll_divider();
}

template<class ad9361_config_slave_t>
bool AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb3_enable() {

  return _get_rhb3_enable();
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb3_decimation_factor() {

  return _get_rhb3_decimation_factor();
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb3_num_taps() {

  return _get_rhb3_num_taps();
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb3_filter_order() {

  return _get_rhb3_filter_order();
}

template<class ad9361_config_slave_t>
bool AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb2_enable() {

  return _get_rhb2_enable();
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb2_decimation_factor() {

  return _get_rhb2_decimation_factor();
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb2_num_taps() {

  // [-9, 0, 73, 128, 73, 0, -9]
  return 7;
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb2_filter_order() {

  return get_filter_order(get_rhb2_num_taps());
}

template<class ad9361_config_slave_t>
bool AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb1_enable() {

  return _get_rhb1_enable();
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb1_decimation_factor() {

  return _get_rhb1_decimation_factor();
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb1_num_taps() {

  // [-8, 0, 42, 0, -147, 0, 619, 1013, 619, 0, -147, 0, 42, 0, -8]
  return 15;
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::get_rhb1_filter_order() {

  return get_filter_order(get_rhb1_num_taps());
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::get_bbpll_freq_Hz() {

  return _get_bbpll_freq_Hz();
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::get_adc_freq_Hz() {

  return _get_adc_freq_Hz();
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::get_r2_freq_Hz() {

  return _get_r2_freq_Hz();
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::get_r1_freq_Hz() {

  return _get_r1_freq_Hz();
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::get_clkrf_freq_Hz() {

  return _get_clkrf_freq_Hz();
}

template<class ad9361_config_slave_t>
bool AD9361ConfigProxy<ad9361_config_slave_t>::get_rx_ch_is_enabled(uint8_t rx_ch) {

  throw_if_ad9361_rf_phy_is_zero();
  throw_if_invalid_rx_ch(rx_ch);

  bool ret = false;
  if(m_ad9361_rf_phy->pdata->rx2tx2) {
    ret = true;
  }
  else {
    if(rx_ch == RX1) {
      ret = m_ad9361_rf_phy->pdata->rx1tx1_mode_use_rx_num == RX_1;
    }
    else { // rx_ch == RX2
      ret = m_ad9361_rf_phy->pdata->rx1tx1_mode_use_rx_num == RX_2;
    }
  }

  return ret;
}

template<class ad9361_config_slave_t>
void AD9361ConfigProxy<ad9361_config_slave_t>::register_OpenCPI_logging_API() {
  // register OpenCPI logging API with the configurator
  set_forwarding_callback_log_info(OCPI::OS::logPrintV);
  set_arg0_log_info(OCPI_LOG_INFO);
  set_forwarding_callback_log_debug(OCPI::OS::logPrintV);
  set_arg0_log_debug(OCPI_LOG_DEBUG);
  set_forwarding_callback_log_warn(OCPI::OS::logPrintV);
  set_arg0_log_warn(OCPI_LOG_INFO); // not sure if warn->info is best...
  set_forwarding_callback_log_error(OCPI::OS::logPrintV);
  set_arg0_log_error(OCPI_LOG_BAD);
}

template<class ad9361_config_slave_t>
void AD9361ConfigProxy<ad9361_config_slave_t>::cache_clkrf_freq_regs() {

  auto& rc = m_reg_cache;
  auto& sl = m_ad9361_config_slave;
  rc.m_bbpll_ref_clock_scaler        = sl.get_bbpll_ref_clock_scaler();
  rc.m_bbpll_integer_bb_freq_word    = sl.get_bbpll_integer_bb_freq_word();
  rc.m_bbpll_fract_bb_freq_word_1    = sl.get_bbpll_fract_bb_freq_word_1();
  rc.m_bbpll_fract_bb_freq_word_2    = sl.get_bbpll_fract_bb_freq_word_2();
  rc.m_bbpll_fract_bb_freq_word_3    = sl.get_bbpll_fract_bb_freq_word_3();
  rc.m_clock_bbpll                   = sl.get_clock_bbpll();
  rc.m_general_rx_enable_filter_ctrl = sl.get_general_rx_enable_filter_ctrl();
}

//@{ // begins doxygen member group
/*! @param[in] use_reg_cache Read from register cache instead of issuing SPI
 *                           accesses. Set to true runs risk of reading from
 *                           outdated cache. This risk is mitigated by knowing
 *                           what you're doing.
 ******************************************************************************/

template<class ad9361_config_slave_t>
uint32_t AD9361ConfigProxy<ad9361_config_slave_t>::_get_bbpll_n_fractional(
    bool use_reg_cache) {

  uint8_t reg;
  uint32_t tmp = 0;

  // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 19

  // Fractional BB Frequency Word[20:16]
  if(use_reg_cache) {
    reg = m_reg_cache.m_bbpll_fract_bb_freq_word_1;
  }
  else {
    reg = m_ad9361_config_slave.get_bbpll_fract_bb_freq_word_1();
  }
  uint8_t Fractional_BB_Frequency_Word_20_16 = reg & 0x1f;
  tmp |= ((uint32_t)Fractional_BB_Frequency_Word_20_16) << 16;

  // Fractional BB Frequency Word[15:8]
  if(use_reg_cache) {
    reg = m_reg_cache.m_bbpll_fract_bb_freq_word_2;
  }
  else {
    reg = m_ad9361_config_slave.get_bbpll_fract_bb_freq_word_2();
  }
  uint8_t Fractional_BB_Frequency_Word_15_8 = reg;
  tmp |= ((uint32_t)Fractional_BB_Frequency_Word_15_8) << 8;

  // Fractional BB Frequency Word[7:0]
  if(use_reg_cache) {
    reg = m_reg_cache.m_bbpll_fract_bb_freq_word_3;
  }
  else {
    reg = m_ad9361_config_slave.get_bbpll_fract_bb_freq_word_3();
  }
  uint8_t Fractional_BB_Frequency_Word_7_0 = reg;
  tmp |= ((uint32_t)Fractional_BB_Frequency_Word_7_0);

  return tmp;
}

template<class ad9361_config_slave_t>
float AD9361ConfigProxy<ad9361_config_slave_t>::_get_bbpll_ref_scaler(
    bool use_reg_cache) {

  float ret;

  // from AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 21:
  // SPI Register 0x045-Ref Clock Scaler
  // [D1:D0]-Ref Clock Scaler[1:0]
  // The reference clock frequency is scaled before it enters the
  // BBPLL. 00: x1; 01: x1/2; 10: x1/4; 11: x2.

  uint8_t reg;
  if(use_reg_cache) {
    reg = m_reg_cache.m_bbpll_ref_clock_scaler;
  }
  else {
    reg = m_ad9361_config_slave.get_bbpll_ref_clock_scaler();
  }
 
  uint8_t Ref_Clock_Scaler_1_0 = reg & 0x03;
  switch(Ref_Clock_Scaler_1_0) {
    case 0x00:
      ret = 1.0F;
      break;
    case 0x01:
      ret = 0.5F;
      break;
    case 0x02:
      ret = 0.25F;
      break;
    default: // 0x03
      ret = 2.0F;
      break;
  }
  return ret;
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::_get_bbpll_n_integer(
    bool use_reg_cache) {

  uint8_t reg;
  if(use_reg_cache) {
    reg = m_reg_cache.m_bbpll_integer_bb_freq_word;
  }
  else {
    reg = m_ad9361_config_slave.get_bbpll_integer_bb_freq_word();
  }
  
  return reg;
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::_get_bbpll_divider(
    bool use_reg_cache) {

  // from AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 7:
  // SPI Register 0x00A-BBPLL
  // [D2:D0]-BBPLL Divider[2:0]
  // BBPLL Divider[2:0] is valid from 1 through 6.

  uint8_t reg;
  if(use_reg_cache) {
    reg = m_reg_cache.m_clock_bbpll;
  }
  else {
    reg = m_ad9361_config_slave.get_clock_bbpll();
  }

  uint8_t BBPLL_Divider_2_0 = (reg & 0x07);
  if((BBPLL_Divider_2_0 == 0) or (BBPLL_Divider_2_0 >= 7)) {
    std::ostringstream oss;
    oss << "could not calculate AD9361 BBPLL Divider due to invalid value ";
    oss << "of " << reg << "for register 0x00A";
    throw oss.str();
  }

  return BBPLL_Divider_2_0;
}

template<class ad9361_config_slave_t>
bool AD9361ConfigProxy<ad9361_config_slave_t>::_get_rhb3_enable(
    bool use_reg_cache) {

  return _get_rhb3_decimation_factor(use_reg_cache) != 1;
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::_get_rhb3_decimation_factor(
    bool use_reg_cache) {

  uint8_t ret;

  // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 4
  uint8_t reg;
  if(use_reg_cache) {
    reg = m_reg_cache.m_general_rx_enable_filter_ctrl;
  }
  else {
    reg = m_ad9361_config_slave.get_general_rx_enable_filter_ctrl();
  }

  uint8_t RHB3_Enable_and_Decimation_1_0;
  {
    // operands to binary operators (e.g. >>) always undergo integral promotion
    int tmp = (reg & (m_BITMASK_D5 | m_BITMASK_D4)) >> 4;
    RHB3_Enable_and_Decimation_1_0 = (uint8_t)(tmp & 0xff);
  }
  bool is_valid = false;
  // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 5 Table 4.
  switch(RHB3_Enable_and_Decimation_1_0) {
    case 0x00:
      ret = 1;
      is_valid = true;
      break;
    case 0x01:
      ret = 2;
      is_valid = true;
      break;
    case 0x02:
      ret = 3;
      is_valid = true;
      break;
    default:
      is_valid = false;
      break;
  }

  if(not is_valid) {
    std::ostringstream oss;
    oss << "could not calculate AD9361 RHB3 decimation factor due to ";
    oss << "invalid value of " << reg << "for register 0x003";
    throw oss.str();
  }
  
  return ret;
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::_get_rhb3_num_taps(
    bool use_reg_cache) {

  uint8_t ret;
  uint8_t val = _get_rhb3_decimation_factor(use_reg_cache);

  switch(val) {
    case 1:
      ret = 0;
      break;
    case 2:
      // [1, 4, 6, 4, 1]
      ret = 5;
      break;
    default: // 3
      // [55, 83, 0, -393, -580, 0, 1914, 4041, 5120, 4041,
      //  1914, 0, -580, -393, 0, 83, 55]
      ret = 17;
      break;
  }

  return ret;
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::_get_rhb3_filter_order(
    bool use_reg_cache) {

  return get_filter_order(_get_rhb3_num_taps(use_reg_cache));
}

template<class ad9361_config_slave_t>
bool AD9361ConfigProxy<ad9361_config_slave_t>::_get_rhb2_enable(
    bool use_reg_cache) {

  uint8_t reg;
  if(use_reg_cache) {
    reg = m_reg_cache.m_general_rx_enable_filter_ctrl;
  }
  else {
    reg = m_ad9361_config_slave.get_general_rx_enable_filter_ctrl();
  }

  return ((reg & m_BITMASK_D3) == m_BITMASK_D3);
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::_get_rhb2_decimation_factor(
    bool use_reg_cache) {

  return _get_rhb2_enable(use_reg_cache) ? 2 : 1;
}

template<class ad9361_config_slave_t>
bool AD9361ConfigProxy<ad9361_config_slave_t>::_get_rhb1_enable(
    bool use_reg_cache) {

  uint8_t reg;
  if(use_reg_cache) {
    reg = m_reg_cache.m_general_rx_enable_filter_ctrl;
  }
  else {
    reg = m_ad9361_config_slave.get_general_rx_enable_filter_ctrl();
  }

  return ((reg & m_BITMASK_D2) == m_BITMASK_D2);
}

template<class ad9361_config_slave_t>
uint8_t AD9361ConfigProxy<ad9361_config_slave_t>::_get_rhb1_decimation_factor(
    bool use_reg_cache) {

  return _get_rhb1_enable(use_reg_cache) ? 2 : 1;
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::_get_bbpll_freq_Hz(
    bool use_reg_cache) {

  // see "BBPLL_freq formula" for explanation of the following;
  double frac, tmp;
  frac  = (double)_get_bbpll_n_fractional(use_reg_cache);
  frac /= (double)BBPLL_MODULUS;
  tmp   = (double)get_bbpll_input_fref_Hz();
  tmp  *= (double)_get_bbpll_ref_scaler(use_reg_cache);
  tmp  *= (((double)_get_bbpll_n_integer(use_reg_cache))+frac);
  //log_debug("bbpll_freq_Hz=%0.15f", tmp);

  return tmp;
}

/// @brief See AD9361_Register_Map_Reference_Manual_UG-671.pdf eq (2)
template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::_get_adc_freq_Hz(
    bool use_reg_cache) {

  double divisor = pow(2.,(double)_get_bbpll_divider(use_reg_cache));

  return _get_bbpll_freq_Hz(use_reg_cache) / divisor;
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::_get_r2_freq_Hz(bool use_reg_cache) {

  bool use = use_reg_cache;
  return _get_adc_freq_Hz(use) / _get_rhb3_decimation_factor(use);
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::_get_r1_freq_Hz(bool use_reg_cache) {

  bool use = use_reg_cache;
  return _get_r2_freq_Hz(use) / _get_rhb2_decimation_factor(use);
}

template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::_get_clkrf_freq_Hz(
    bool use_reg_cache) {

  bool use = use_reg_cache;
  return _get_r1_freq_Hz(use) / _get_rhb1_decimation_factor(use);
}

/*! @brief See AD9361_Reference_Manual_UG-570.pdf equation (14).
 ******************************************************************************/
template<class ad9361_config_slave_t>
double AD9361ConfigProxy<ad9361_config_slave_t>::
_get_digital_rx_block_delay_sec(uint8_t rx_ch, bool use_reg_cache) {

  double delta_t = 0.;

  // either 1) use_reg_cache=true, which is why get_*(true) is called below
  //     or 2) use_reg_cache=false, in which case we still perform some
  //           optimization (use of get_*(true) below performs optimization, but
  //           a partial cache refresh is still required for dependent
  //           registers)
  if(not use_reg_cache) {
    cache_clkrf_freq_regs();
  }

  // Assuming AD9361_Reference_Manual_UG-570.pdf example in equation (13) is
  // INCORRECT - "filter order (number of taps)" should instead
  // say "filter order (number of taps minus one)", which is the DSP standard.
  // Assuming AD9361_Reference_Manual_UG-570.pdf example in equation (14) is
  // INCORRECT - "64 x 1 / 30.72M" should instead say "63.5 x 1 / 30.72M"
  // because the filter order divided by 2 of a 128 tap filter is 63.5.
  typedef double dd;
  const bool bb = true;
  if(_get_rhb3_enable(bb)) {
    delta_t += ((dd)_get_rhb3_filter_order(bb)) / 2. / _get_adc_freq_Hz(bb);
  }
  if(_get_rhb2_enable(use_reg_cache)) {
    delta_t += ((dd)get_rhb2_filter_order()) / 2. / _get_r2_freq_Hz(bb);
  }
  if(_get_rhb1_enable(use_reg_cache)) {
    delta_t += ((dd)get_rhb1_filter_order()) / 2. / _get_r1_freq_Hz(bb);
  }

  uint8_t en_dis_obj;
  int32_t ret = ad9361_get_rx_fir_en_dis(&en_dis_obj);
  if(ret != 0) {
    std::ostringstream oss;
    oss << "ad9361_get_rx_fir_en_dis returned non-zero value: " << ret;
    throw oss.str();
  }
  if(en_dis_obj == 1) {
    double tmp;
    tmp = ((dd)get_rx_fir_filter_order(rx_ch)) / 2. / _get_clkrf_freq_Hz(bb);
    delta_t += tmp;
  }

  return delta_t;
}

//@} // ends doxygen member group

template<class ad9361_config_slave_t>
void AD9361ConfigProxy<ad9361_config_slave_t>::throw_if_ad9361_rf_phy_is_zero() {

  if(m_ad9361_rf_phy == 0) {
    throw std::string("ad9361_rf_phy pointer was 0");
  }
}

template<class ad9361_config_slave_t>
void AD9361ConfigProxy<ad9361_config_slave_t>::throw_if_invalid_rx_ch(
    uint8_t rx_ch) {

  if((rx_ch != RX1) and (rx_ch != RX2)) {
    std::ostringstream oss;
    oss << "Bad rx_ch value of " << rx_ch;
    throw oss.str();
  }
}

} // namespace OCPIProjects
