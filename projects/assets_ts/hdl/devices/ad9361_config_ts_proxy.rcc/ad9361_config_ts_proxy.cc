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

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Mar 27 13:25:45 2019 EDT
 * BASED ON THE FILE: ad9361_config_ts_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the ad9361_config_ts_proxy worker in C++
 */

#include <sstream> // needed for std::ostringstream
#include <unistd.h> // usleep()
#include "ad9361_config_ts_proxy-worker.hh"
#include "AD9361ConfigProxy.hh" // AD9361ConfigProxy class
#include "OcpiOsDebugApi.hh" // OCPI_LOG_DEBUG

extern "C" {
#include "config.h" // ALTERA_PLATFORM
#include "ad9361_api.h"
#include "parameters.h"
int32_t spi_init(OCPI::RCC::RCCUserSlave* _slave);
}

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Ad9361_config_ts_proxyWorkerTypes;

#define CHECK_IF_POINTER_IS_NULL(p)        checkIfPointerIsNull(p, #p)

// LIBAD9361_API_INIT                 used for libad9361 calls to ad9361_init()
// LIBAD9361_API_1PARAM(pfun, a1)     used for libad9361 calls with 1 parameter (no parameter printing)
// LIBAD9361_API_1PARAMP(pfun, a1)    used for libad9361 calls with 1 parameter, with parameter printing
// LIBAD9361_API_1PARAMV(pfun, a1)    used for libad9361 calls with 1 parameter and return type of void
// LIBAD9361_API_2PARAM(pfun, a1, a2) used for libad9361 calls with 2 parameters (no parameter printing)
// LIBAD9361_API_CHAN_GET(pfun, a1)   used for libad9361 multichannel get calls
// LIBAD9361_API_CHAN_GETN(pfun, a1)  used for libad9361 multichannel get calls where the second channel should be disabled when ad9361_phy->pdata->rx2tx2 == 0
// LIBAD9361_API_CHAN_SET(pfun, a1)   used for libad9361 multichannel set calls
#define LIBAD9361_API_INIT(pfun,a1)        libad9361_API_init(pfun, a1, #pfun)
#define LIBAD9361_API_1PARAM(pfun, a1)     libad9361_API_1param(pfun, a1, #pfun)
#define LIBAD9361_API_1PARAMP(pfun, a1)    libad9361_API_1paramp(pfun, a1, #pfun)
#define LIBAD9361_API_1PARAMV(pfun, a1)    libad9361_API_1paramv(pfun, a1, #pfun)
#define LIBAD9361_API_2PARAM(pfun, a1, a2) libad9361_API_2param(pfun, a1, a2, #pfun)
#define LIBAD9361_API_CHAN_GET(pfun, a1)   libad9361_API_chan_get(pfun, a1, #pfun)
#define LIBAD9361_API_CHAN_GETN(pfun, a1)  libad9361_API_chan_get(pfun, a1, #pfun, true)
#define LIBAD9361_API_CHAN_SET(pfun, a1)   libad9361_API_chan_set(pfun, a1, #pfun)

#define ENUM_BBPLL_DIVIDER(val, prop)       case val: m_properties.prop = BBPLL_DIVIDER_##val; break;
#define ENUM_TX_BBF_TUNE_DIVIDER(val, prop) case val: m_properties.prop = TX_BBF_TUNE_DIVIDER_##val; break;
#define ENUM_RX_BBF_TUNE_DIVIDE( val, prop) case val: m_properties.prop = RX_BBF_TUNE_DIVIDE_##val; break;

#define D7_BITMASK 0x80
#define D6_BITMASK 0x40
#define D5_BITMASK 0x20
#define D4_BITMASK 0x10
#define D3_BITMASK 0x08
#define D2_BITMASK 0x04
#define D1_BITMASK 0x02
#define D0_BITMASK 0x01

class Ad9361_config_ts_proxyWorker : public Ad9361_config_ts_proxyWorkerBase {
  RunCondition m_aRunCondition;
  OCPIProjects::AD9361ConfigProxy<Slave1> m_ad9361_config_proxy; /// @todo / FIXME - Eventually all OpenCPI AD9361 functionality, No-OS or otherwise, will be moved inside the AD9361ConfigProxy class

public:
  Ad9361_config_ts_proxyWorker() : m_aRunCondition(RCC_NO_PORTS),
    m_ad9361_config_proxy(ad9361_phy, slave, 40e6) /// @todo / FIXME - Eventually all OpenCPI AD9361 functionality, No-OS or otherwise, will be moved inside the AD9361ConfigProxy class, when that happens, the passing ad9361_phy will no longer be necessary
    {

    //Run function should never be called
    setRunCondition(&m_aRunCondition);

    ad9361_phy = 0;
  }
  ~Ad9361_config_ts_proxyWorker() {
    ad9361_free(ad9361_phy); // this function added in ad9361.patch
  }
private:
  // note that RX_FAST_LOCK_CONFIG_WORD_NUM is in fact applicable to both
  // RX and TX faslock configs
  typedef struct fastlock_profile_s {
    uint32_t id;
    uint8_t values[RX_FAST_LOCK_CONFIG_WORD_NUM];
  } fastlock_profile_t;

  std::vector<fastlock_profile_t>::iterator find_worker_fastlock_profile(
      uint32_t id, std::vector<fastlock_profile_t>& vec)
  {
    for (std::vector<fastlock_profile_t>::iterator it = vec.begin();
         it != vec.end(); ++it)
    {
      if(it->id == id) return it;
    }
    return vec.end();
  }

  std::vector<fastlock_profile_t> m_rx_fastlock_profiles;
  std::vector<fastlock_profile_t> m_tx_fastlock_profiles;
  struct ad9361_rf_phy *ad9361_phy;
  AD9361_InitParam m_init_param;

  template<typename T> RCCResult
  checkIfPointerIsNull(T* p, const char* pChar) {
    if(p == ((T*)0))
    {
      std::ostringstream err;
      std::string pStr(pChar);
      err << pStr << " pointer was null";
      return setError(err.str().c_str());
    }
    return RCC_OK;
  }

  void libad9361_API_print_idk(std::string functionStr) {
    // typical use results in leading '&' on functionStr, erase for prettiness
    std::string functionStdStr(functionStr);
    if(functionStdStr[0] == '&') functionStdStr.erase(functionStdStr.begin());

    // we don't know how to parameters for this function, so just printing ...
    log(OCPI_LOG_DEBUG, "No-OS API call: %s(...)\n", functionStdStr.c_str());
  }

  template<typename T> void
  libad9361_API_print(std::string functionStr, T param,
                      uint8_t chan = 255) {
    // typical use results in leading '&' on functionStr, erase for prettiness
    std::string functionStdStr(functionStr);
    if(functionStdStr[0] == '&') functionStdStr.erase(functionStdStr.begin());
    std::ostringstream paramStr;
    paramStr << (long long) param;
    if(chan != 255)
    {
      log(OCPI_LOG_DEBUG, "No-OS API call: %s(ad9361_phy, %i, %s)\n",
          functionStdStr.c_str(), chan, paramStr.str().c_str());
    }
    else
    {
      log(OCPI_LOG_DEBUG, "No-OS API call: %s(ad9361_phy, %s)\n",
          functionStdStr.c_str(), paramStr.str().c_str());
    }
  }

  void enforce_ensm_config() {

    slave.set_Half_Duplex_Mode(not ad9361_phy->pdata->fdd);

    uint8_t ensm_config_1 = slave.get_ensm_config_1();
    //log(OCPI_LOG_INFO, "ensm_config_1=%u", ensm_config_1);
    slave.set_ENSM_Pin_Control((ensm_config_1 & 0x10) == 0x10);
    slave.set_Level_Mode((ensm_config_1 & 0x08) == 0x08);

    uint8_t ensm_config_2 = slave.get_ensm_config_2();
    //log(OCPI_LOG_INFO, "ensm_config_2=%u", ensm_config_2);
    slave.set_FDD_External_Control_Enable((ensm_config_2 & 0x80) == 0x80);
  }

  /*! @brief Function that should be used to make the ad9361_init() API call.
   *  @param[in]   function    libad9361 API function pointer
   *  @param[in]   param       parameter for ad9361 API function
   *  @param[in]   functionStr Stringified function name
   ****************************************************************************/
  RCCResult
  libad9361_API_init(int32_t function(struct ad9361_rf_phy**, AD9361_InitParam*),
      AD9361_InitParam* param, const char* functionStr) {
    libad9361_API_print_idk(functionStr);
    
    const bool    LVDS          = slave.get_LVDS();
    const bool    single_port   = slave.get_single_port();
    const bool    half_duplex   = slave.get_half_duplex();
    const uint8_t drs           = (uint8_t)slave.get_data_rate_config();
    const bool modeIsCMOS       = (LVDS          == false);
    const bool modeIsFullDuplex = (half_duplex   == false);
    const bool modeIsDualPort   = (single_port   == false);
    const bool modeIsDDR        = (drs           == 1);

    param->lvds_rx_onchip_termination_enable = modeIsCMOS ? 0 : 1;

    // FDD is NOT synonymous with full duplex port mode! - e.g., LVDS (which is
    // full duplex) supports TDD
    //param->frequency_division_duplex_mode_enable = ;

    // -----SPI Register 0x010-Parallel Port Configuration 1-----///
    // D7-PP Tx Swap IQ
    // passed in via param variable

    // D6-PP Rx Swap IQ
    // passed in via param variable

    // D5-TX Channel Swap
    // passed in via param variable

    // D4-RX Channel Swap
    // passed in via param variable

    //const uint8_t rx_ch_swap    = (uint8_t)slave.get_channels_are_swapped();
    //const bool dontCareIfChansSwapped = (rx_ch_swap >= 2);
    //if(!dontCareIfChansSwapped)
    //{
    //  const bool channelsAreSwapped = (rx_ch_swap == 1);
    //  param->rx_channel_swap_enable = channelsAreSwapped ? 1 : 0;
    //}

    // D3-RX Frame Pulse Mode
    const uint8_t rx_fr_usage   = (uint8_t)slave.get_rx_frame_usage();
    const bool rxFrameUsageIsToggle = (rx_fr_usage == 1);
    param->rx_frame_pulse_mode_enable = rxFrameUsageIsToggle ? 1 : 0;

    // D2-2R2T Timing
    // passed in via param variable

    // D1-Invert Data Bus
    const uint8_t data_bus_idx_inv = (uint8_t)slave.get_data_bus_index_direction();
    const bool dataBusIdxDirectionIsInverted = (data_bus_idx_inv == 1);
    param->invert_data_bus_enable = dataBusIdxDirectionIsInverted ? 1 : 0;

    // D0-Invert DATA_CLK
    const uint8_t data_clk_inv = (uint8_t)slave.get_data_clk_is_inverted();
    const bool dataClkIsInverted = (data_clk_inv == 1);
    param->invert_data_clk_enable = dataClkIsInverted ? 1 : 0;

    // -----SPI Register 0x011-Parallel Port Configuration 2-----///
    //D7-FDD Alt Word Order
#define FDD_ALT_WORD_ORDER_ENABLE 0 ///@TODO / FIXME read this value from FPGA, I think we want 0 for now ??
    param->fdd_alt_word_order_enable = FDD_ALT_WORD_ORDER_ENABLE;

    // D1:D0-Delay Rx Data[1:0]
    // passed in via param variable

    //D2-Invert Rx Frame
    const uint8_t rx_frame_inv = (uint8_t)slave.get_rx_frame_is_inverted();
    const bool rxFrameIsInverted = (rx_frame_inv == 1);
    param->invert_rx_frame_enable = rxFrameIsInverted ? 1 : 0;
    
    // -----SPI Register 0x012-Parallel Port Configuration 3-----///
    // D7-FDD RX Rate=2*Tx Rate
    // I think we want to leave the initialization-time value default and rely
    // on runtime libad9361 API to set this
    
    // D6-Swap Ports
    const bool swap_ports = slave.get_swap_ports();
    param->swap_ports_enable = modeIsCMOS ? (swap_ports ? 1 : 0) : 0;
    
    // D4-LVDS Mode
    param->lvds_mode_enable = modeIsCMOS ? 0 : 1;
    if(param->lvds_mode_enable == 1)
    {
      param->half_duplex_mode_enable = 0; // D3-Half-Duplex Mode
      param->single_data_rate_enable = 0; // D5-Single Data Rate
      param->single_port_mode_enable = 0; // D2-Single Port Mode
    }
    else
    {
      param->half_duplex_mode_enable = modeIsFullDuplex ? 0 : 1; // D3-Half-Duplex Mode
      param->single_data_rate_enable = modeIsDDR        ? 0 : 1; // D5-Single Data Rate
      param->single_port_mode_enable = modeIsDualPort   ? 0 : 1; // D2-Single Port Mode
    }
    
    // D1-Full Port
    if((param->half_duplex_mode_enable == 0) &&
       (param->single_port_mode_enable == 0))
    {
      param->full_port_enable = 1;
    }
    else
    {
      param->full_port_enable = 0;
    }
    if(!modeIsCMOS) param->full_port_enable = 0; // not sure why this is needed, but it's how ADI's code works...
 
    // D0-Full Duplex Swap Bit
    // not sure how to use this, leaving initialization-time value default for now
    
    // adc1 or dac1 indicates second chan (index starts at 0)
    const bool qadc1_is_present = slave.get_qadc1_is_present();
    const bool qdac1_is_present = slave.get_qdac1_is_present();
#define _2R1Tconfig ((    qadc1_is_present) and (not qdac1_is_present))
#define _1R2Tconfig ((not qadc1_is_present) and (    qdac1_is_present))
#define _2R2Tconfig ((    qadc1_is_present) and (    qdac1_is_present))
    // quote from ADI's UG-570 Rev. A's
    // SINGLE PORT HALF DUPLEX MODE (CMOS),
    // SINGLE PORT FULL DUPLEX MODE (CMOS),
    // DUAL PORT HALF DUPLEX MODE (CMOS),
    // DUAL PORT FULL DUPLEX MODE (CMOS), and
    // DUAL PORT FULL DUPLEX MODE (LVDS) paragraphs:
    // "For a system with a 2R1T or 1R2T configuration, the clock
    // frequencies, sample periods, and data capture timing are the
    // same as if configured for a 2R2T system."
    {
      // I *think* this is step 1 of 2 on how to use No-OS to implement 1R2T
      // I *think* this is step 1 of 2 on how to use No-OS to implement 2R1T
      uint8_t _2r2tmen = (_2R1Tconfig or _1R2Tconfig or _2R2Tconfig);
      param->two_rx_two_tx_mode_enable = _2r2tmen ? 1 : 0;
    }

    // assign param->gpio_resetb to the arbitrarily defined GPIO_RESET_PIN so
    // that the platform driver knows to drive the force_reset property of the
    // sub-device
    param->gpio_resetb = GPIO_RESET_PIN;

    log(OCPI_LOG_DEBUG, "param->pp_tx_swap_enable = %i", (int)param->pp_tx_swap_enable);
    log(OCPI_LOG_DEBUG, "param->pp_rx_swap_enable = %i", (int)param->pp_rx_swap_enable);
    log(OCPI_LOG_DEBUG, "param->tx_channel_swap_enable = %i", (int)param->tx_channel_swap_enable);
    log(OCPI_LOG_DEBUG, "param->rx_channel_swap_enable = %i", (int)param->rx_channel_swap_enable);
    log(OCPI_LOG_DEBUG, "param->rx_frame_pulse_mode_enable = %i", (int)param->rx_frame_pulse_mode_enable);
    log(OCPI_LOG_DEBUG, "param->two_t_two_r_timing_enable = %i", (int)param->two_t_two_r_timing_enable);
    log(OCPI_LOG_DEBUG, "param->invert_data_bus_enable = %i", (int)param->invert_data_bus_enable);
    log(OCPI_LOG_DEBUG, "param->invert_data_clk_enable = %i", (int)param->invert_data_clk_enable);
    log(OCPI_LOG_DEBUG, "param->fdd_alt_word_order_enable = %i", (int)param->fdd_alt_word_order_enable);
    log(OCPI_LOG_DEBUG, "param->invert_rx_frame_enable = %i", (int)param->invert_rx_frame_enable);
    log(OCPI_LOG_DEBUG, "param->fdd_rx_rate_2tx_enable = %i", (int)param->fdd_rx_rate_2tx_enable);
    log(OCPI_LOG_DEBUG, "param->swap_ports_enable = %i", (int)param->swap_ports_enable);
    log(OCPI_LOG_DEBUG, "param->single_data_rate_enable = %i", (int)param->single_data_rate_enable);
    log(OCPI_LOG_DEBUG, "param->lvds_mode_enable = %i", (int)param->lvds_mode_enable);
    log(OCPI_LOG_DEBUG, "param->half_duplex_mode_enable = %i", (int)param->half_duplex_mode_enable);
    log(OCPI_LOG_DEBUG, "param->single_port_mode_enable = %i", (int)param->single_port_mode_enable);
    log(OCPI_LOG_DEBUG, "param->full_port_enable = %i", (int)param->full_port_enable);
    log(OCPI_LOG_DEBUG, "param->full_duplex_swap_bits_enable = %i", (int)param->full_duplex_swap_bits_enable);
    log(OCPI_LOG_DEBUG, "param->delay_rx_data = %i", (int)param->delay_rx_data);
    log(OCPI_LOG_DEBUG, "param->rx_data_clock_delay = %i", (int)param->rx_data_clock_delay);
    log(OCPI_LOG_DEBUG, "param->rx_data_delay = %i", (int)param->rx_data_delay);
    log(OCPI_LOG_DEBUG, "param->tx_fb_clock_delay= %i", (int)param->tx_fb_clock_delay);
    log(OCPI_LOG_DEBUG, "param->tx_data_delay = %i", (int)param->tx_data_delay);

    slave.set_force_two_r_two_t_timing(param->two_t_two_r_timing_enable);

    // note that it's okay for ad9361_phy to be a null pointer prior
    // to calling function()
    const int32_t res = function(&ad9361_phy, param);
    RCCResult ret = CHECK_IF_POINTER_IS_NULL(ad9361_phy);
    if(ret != RCC_OK) return ret;
    if(res != 0)
    {
      std::ostringstream err;
      // typical use results in leading '&' on functionStr, erase for prettiness
      std::string functionStdStr(functionStr);
      if(functionStdStr[0] == '&') functionStdStr.erase(functionStdStr.begin());
      err << functionStdStr << "() returned: " << res;
      return setError(err.str().c_str());
    }

    enforce_ensm_config();

    if(_1R2Tconfig)
    {
      // I *think* this is step 2 of 2 on how to use No-OS to implement 1R2T
      ad9361_en_dis_rx(ad9361_phy, RX_2, RX_DISABLE);
    }
    if(_2R1Tconfig)
    {
      // I *think* this is step 2 of 2 on how to use No-OS to implement 2R1T
      ad9361_en_dis_tx(ad9361_phy, TX_2, TX_DISABLE);
    }

    set_FPGA_channel_config(); // because channel config potentially changed

    return RCC_OK;
  }
  
  RCCResult ad9361_init_check(AD9361_InitParam init_param,
                              const bool force_init = false)
  {
    static bool ad9361_init_called = false;
    if((!ad9361_init_called) || force_init)
    {
      ad9361_init_called = true;

      // ADI forum post recommended setting ENABLE/TXNRX pins high *prior to
      // ad9361_init() call* when
      // frequency_division_duplex_independent_mode_enable is set to 1

      slave.set_ENABLE_force_set(true);
      slave.set_TXNRX_force_set(true);

      // sleep duration chosen to be relatively small in relation to AD9361
      // initialization duration (which, through observation, appears to be
      // roughly 200 ms), but a long enough pulse that AD9361 is likely
      // recognizing it many, many times over
      usleep(1000);

      RCCResult res = LIBAD9361_API_INIT(&ad9361_init, &init_param);

      slave.set_ENABLE_force_set(false);
      slave.set_TXNRX_force_set(false);

      return res;
    }
    return RCC_OK;
  }

  inline RCCResult ad9361_pre_API_call_validation(AD9361_InitParam init_param,
                                                  const bool force_init = false)
  {
    RCCResult ret = ad9361_init_check(init_param, force_init);
    if(ret != RCC_OK) return ret;
    ret  = CHECK_IF_POINTER_IS_NULL(ad9361_phy->pdata);
    if(ret != RCC_OK) return ret;
    ret = CHECK_IF_POINTER_IS_NULL(ad9361_phy);
    return ret;
  }

  /*! @brief Function that should be used to make channel-agnostic libad9361 API
   *         calls with 1 parameter with most common return type of int32_t.
   *         Also performs debug printing of function call and passed
   *         parameters.
   *  @param[in]   function    libad9361 API function pointer
   *  @param[in]   param       second and last parameter for ad9361 API function
   *  @param[in]   functionStr Stringified function name
   ****************************************************************************/
  template<typename T> RCCResult
  libad9361_API_1paramp(int32_t function(struct ad9361_rf_phy*, T),
      T param, const char* functionStr) {
    libad9361_API_print(functionStr, param);
    return libad9361_API_1param(function, param, functionStr, false);
  }

  /*! @brief Function that should be used to make channel-agnostic libad9361 API
   *         calls with 1 parameter with most common return type of int32_t.
   *  @param[in]   function    libad9361 API function pointer
   *  @param[in]   param       second and last parameter for ad9361 API function
   *  @param[in]   functionStr Stringified function name
   *  @param[in]   doPrint     Enables debugging printing of function call and
   *                           passed parameters.
   *                           Default is true.
   ****************************************************************************/
  template<typename T> RCCResult
  libad9361_API_1param(int32_t function(struct ad9361_rf_phy*, T),
      T param, const char* functionStr, bool doPrint = true) {
    if (doPrint) libad9361_API_print_idk(functionStr);

    RCCResult ret = ad9361_pre_API_call_validation(m_init_param);
    if(ret != RCC_OK) return ret;

    const int32_t res = function(ad9361_phy, param);
    if(res != 0)
    {
      std::ostringstream err;
      // typical use results in leading '&' on functionStr, erase for prettiness
      std::string functionStdStr(functionStr);
      if(functionStdStr[0] == '&') functionStdStr.erase(functionStdStr.begin());
      err << functionStdStr << "() returned: " << res;
      return setError(err.str().c_str());
    }
    return RCC_OK;
  }

  /*! @brief Function that should be used to make channel-agnostic libad9361 API
   *         calls with 1 parameter with return type of void. Also performs
   *         debug printing of function call and passed parameters.
   *  @param[in]   function    libad9361 API function pointer
   *  @param[in]   param       second and last parameter for ad9361 API function
   *  @param[in]   functionStr Stringified function name
   ****************************************************************************/
  template<typename T> RCCResult
  libad9361_API_1paramv(void function(struct ad9361_rf_phy*, T),
      T param, const char* functionStr) {
    libad9361_API_print_idk(functionStr);

    RCCResult ret = ad9361_pre_API_call_validation(m_init_param);
    if(ret != RCC_OK) return ret;

    function(ad9361_phy, param);
    return RCC_OK;
  }

  /*! @brief Function that should be used to make channel-agnostic libad9361 API
   *         calls with 2 parameters. Also performs debug printing of function
   *         call and passed parameters.
   *  @param[in]   function    libad9361 API function pointer
   *  @param[in]   param1      second parameter for ad9361 API function
   *  @param[in]   param2      third and last parameter for ad9361 API function
   *  @param[in]   functionStr Stringified function name
   ****************************************************************************/
  template<typename T, typename R> RCCResult
  libad9361_API_2param(int32_t function(struct ad9361_rf_phy*, T, R),
      T param1, R param2,
      const char* functionStr) {
    libad9361_API_print_idk(functionStr);

    RCCResult ret = ad9361_pre_API_call_validation(m_init_param);
    if(ret != RCC_OK) return ret;

    const int32_t res = function(ad9361_phy, param1, param2);
    if(res != 0)
    {
      std::ostringstream err;
      // typical use results in leading '&' on functionStr, erase for prettiness
      std::string functionStdStr(functionStr);
      if(functionStdStr[0] == '&') functionStdStr.erase(functionStdStr.begin());
      err << functionStdStr << "() returned: " << res;
      return setError(err.str().c_str());
    }
    return RCC_OK;
  }

  /*! @brief Function that should be used to make channel-dependent libad9361
   *         API get calls. Also performs debug printing of function call and
   *         passed parameters.
   *  @param[in]   function    libad9361 API function pointer
   *  @param[in]   param       Reference to variable sent as second and last
   *                           parameter for ad9361 API function
   *  @param[in]   functionStr Stringified function name
   *  @param[in]   ch1Disable  While channel 0 is always processing, channel 1
   *                           may be disabled by setting this to true. This
   *                           allows for handling of some libad9361 *_get_*
   *                           calls falling when ch=1 and
   *                           ad9361_phy->pdata->rx2tx2 == 0
   ****************************************************************************/
  template<typename T> RCCResult
  libad9361_API_chan_get(int32_t function(struct ad9361_rf_phy*,
      uint8_t chan, T*), T* param, const char* functionStr,
      bool ch1Disable = false) {
    for(uint8_t chan=0; chan<AD9361_CONFIG_TS_PROXY_RX_NCHANNELS; chan++) {
      libad9361_API_print_idk(functionStr);

      RCCResult ret = ad9361_pre_API_call_validation(m_init_param);
      if(ret != RCC_OK) return ret;

      const int32_t res = function(ad9361_phy, chan, param++);
      if(res != 0) return setError("%s() returned: %i", functionStr, res);
      if(ch1Disable)
      {
        if(ad9361_phy->pdata->rx2tx2 == 0)
        {
          break; // some _get_ calls (intentionally) fail when ch=1 and rx2tx2=0
        }
      }
    }
    return RCC_OK;
  }

  /*! @brief Function that should be used to make channel-dependent libad9361
   *         API set calls. Also performs debug printing of function call and
   *         passed parameters.
   *  @param[in]   function    libad9361 API function pointer
   *  @param[in]   param       Constant Reference to variable sent as second and
   *                           last parameter for ad9361 API function
   *  @param[in]   functionStr Stringified function name
   ****************************************************************************/
  template<typename T> RCCResult
  libad9361_API_chan_set(int32_t function(struct ad9361_rf_phy*,
      uint8_t chan, T), T* param, const char* functionStr) {

    for(uint8_t chan=0; chan<AD9361_CONFIG_TS_PROXY_RX_NCHANNELS; chan++) {
      libad9361_API_print(functionStr, *param, chan);

      RCCResult ret = ad9361_pre_API_call_validation(m_init_param);
      if(ret != RCC_OK) return ret;

      const int32_t res = function(ad9361_phy, chan, *param++);
      if(res != 0)
      {
        std::ostringstream err;
        // typical use results in leading '&' on functionStr, erase for prettiness
        std::string functionStdStr(functionStr);
        if(functionStdStr[0] == '&') functionStdStr.erase(functionStdStr.begin());
        err << functionStdStr << "() returned: " << res;
        return setError(err.str().c_str());
      }
      if(ad9361_phy->pdata->rx2tx2 == 0)
      {
        break; // all _set_ calls fail when ch=1 and rx2tx2=0
      }
    }
    return RCC_OK;
  }

  /*! @brief The AD9361 register set determines which channel mode is used
   *         (1R1T, 1R2T, 2R1T, or 1R2T). This mode eventually determines which
   *         timing diagram the AD9361 is expecting for the TX data path pins.
   *         This function tells the FPGA bitstream which channel mode should be
   *         assumed when generating the TX data path signals.
   ****************************************************************************/
  void set_FPGA_channel_config(void) {
    uint8_t general_tx_enable_filter_ctrl =
        slave.get_general_tx_enable_filter_ctrl();
    bool two_t = TX_CHANNEL_ENABLE(TX_1 | TX_2) ==
        (general_tx_enable_filter_ctrl & 0xc0);
    uint8_t general_rx_enable_filter_ctrl =
        slave.get_general_rx_enable_filter_ctrl();
    bool two_r = RX_CHANNEL_ENABLE(RX_1 | RX_2) ==
        (general_rx_enable_filter_ctrl & 0xc0);
    slave.set_config_is_two_r(two_r);
    slave.set_config_is_two_t(two_t);
  }

  RCCResult initialize() {
    AD9361_InitParam default_init_param = {
      /* Device selection */
      ID_AD9361,	// dev_sel
      /* Identification number */
      0,		//id_no
      /* Reference Clock */
      40000000UL,	//reference_clk_rate
      /* Base Configuration */
      1,		//two_rx_two_tx_mode_enable *** adi,2rx-2tx-mode-enable
      1,		//one_rx_one_tx_mode_use_rx_num *** adi,1rx-1tx-mode-use-rx-num
      1,		//one_rx_one_tx_mode_use_tx_num *** adi,1rx-1tx-mode-use-tx-num
      1,		//frequency_division_duplex_mode_enable *** adi,frequency-division-duplex-mode-enable
      1,		//frequency_division_duplex_independent_mode_enable *** adi,frequency-division-duplex-independent-mode-enable
      0,		//tdd_use_dual_synth_mode_enable *** adi,tdd-use-dual-synth-mode-enable
      0,		//tdd_skip_vco_cal_enable *** adi,tdd-skip-vco-cal-enable
      0,		//tx_fastlock_delay_ns *** adi,tx-fastlock-delay-ns
      0,		//rx_fastlock_delay_ns *** adi,rx-fastlock-delay-ns
      0,		//rx_fastlock_pincontrol_enable *** adi,rx-fastlock-pincontrol-enable
      0,		//tx_fastlock_pincontrol_enable *** adi,tx-fastlock-pincontrol-enable
      0,		//external_rx_lo_enable *** adi,external-rx-lo-enable
      0,		//external_tx_lo_enable *** adi,external-tx-lo-enable
      5,		//dc_offset_tracking_update_event_mask *** adi,dc-offset-tracking-update-event-mask
      6,		//dc_offset_attenuation_high_range *** adi,dc-offset-attenuation-high-range
      5,		//dc_offset_attenuation_low_range *** adi,dc-offset-attenuation-low-range
      0x28,	//dc_offset_count_high_range *** adi,dc-offset-count-high-range
      0x32,	//dc_offset_count_low_range *** adi,dc-offset-count-low-range
      0,		//split_gain_table_mode_enable *** adi,split-gain-table-mode-enable
      MAX_SYNTH_FREF,	//trx_synthesizer_target_fref_overwrite_hz *** adi,trx-synthesizer-target-fref-overwrite-hz
      0,		// qec_tracking_slow_mode_enable *** adi,qec-tracking-slow-mode-enable
      /* ENSM Control */
      0,		//ensm_enable_pin_pulse_mode_enable *** adi,ensm-enable-pin-pulse-mode-enable
      0,		//ensm_enable_txnrx_control_enable *** adi,ensm-enable-txnrx-control-enable
      /* LO Control */
      2400000000UL,	//rx_synthesizer_frequency_hz *** adi,rx-synthesizer-frequency-hz
      2400000000UL,	//tx_synthesizer_frequency_hz *** adi,tx-synthesizer-frequency-hz
      1,        //tx_lo_powerdown_managed_enable *** adi,tx-lo-powerdown-managed-enable
      /* Rate & BW Control */
      {983040000, 245760000, 122880000, 61440000, 30720000, 30720000},// rx_path_clock_frequencies[6] *** adi,rx-path-clock-frequencies
      {983040000, 122880000, 122880000, 61440000, 30720000, 30720000},// tx_path_clock_frequencies[6] *** adi,tx-path-clock-frequencies
      18000000,//rf_rx_bandwidth_hz *** adi,rf-rx-bandwidth-hz
      18000000,//rf_tx_bandwidth_hz *** adi,rf-tx-bandwidth-hz
      /* RF Port Control */
      0,		//rx_rf_port_input_select *** adi,rx-rf-port-input-select
      0,		//tx_rf_port_input_select *** adi,tx-rf-port-input-select
      /* TX Attenuation Control */
      10000,	//tx_attenuation_mdB *** adi,tx-attenuation-mdB
      0,		//update_tx_gain_in_alert_enable *** adi,update-tx-gain-in-alert-enable
      /* Reference Clock Control */
      0,		//xo_disable_use_ext_refclk_enable *** adi,xo-disable-use-ext-refclk-enable
      {8, 5920},	//dcxo_coarse_and_fine_tune[2] *** adi,dcxo-coarse-and-fine-tune
      CLKOUT_DISABLE,	//clk_output_mode_select *** adi,clk-output-mode-select
      /* Gain Control */
      2,		//gc_rx1_mode *** adi,gc-rx1-mode
      2,		//gc_rx2_mode *** adi,gc-rx2-mode
      58,		//gc_adc_large_overload_thresh *** adi,gc-adc-large-overload-thresh
      4,		//gc_adc_ovr_sample_size *** adi,gc-adc-ovr-sample-size
      47,		//gc_adc_small_overload_thresh *** adi,gc-adc-small-overload-thresh
      8192,	//gc_dec_pow_measurement_duration *** adi,gc-dec-pow-measurement-duration
      0,		//gc_dig_gain_enable *** adi,gc-dig-gain-enable
      800,	//gc_lmt_overload_high_thresh *** adi,gc-lmt-overload-high-thresh
      704,	//gc_lmt_overload_low_thresh *** adi,gc-lmt-overload-low-thresh
      24,		//gc_low_power_thresh *** adi,gc-low-power-thresh
      15,		//gc_max_dig_gain *** adi,gc-max-dig-gain
      /* Gain MGC Control */
      2,		//mgc_dec_gain_step *** adi,mgc-dec-gain-step
      2,		//mgc_inc_gain_step *** adi,mgc-inc-gain-step
      0,		//mgc_rx1_ctrl_inp_enable *** adi,mgc-rx1-ctrl-inp-enable
      0,		//mgc_rx2_ctrl_inp_enable *** adi,mgc-rx2-ctrl-inp-enable
      0,		//mgc_split_table_ctrl_inp_gain_mode *** adi,mgc-split-table-ctrl-inp-gain-mode
      /* Gain AGC Control */
      10,		//agc_adc_large_overload_exceed_counter *** adi,agc-adc-large-overload-exceed-counter
      2,		//agc_adc_large_overload_inc_steps *** adi,agc-adc-large-overload-inc-steps
      0,		//agc_adc_lmt_small_overload_prevent_gain_inc_enable *** adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable
      10,		//agc_adc_small_overload_exceed_counter *** adi,agc-adc-small-overload-exceed-counter
      4,		//agc_dig_gain_step_size *** adi,agc-dig-gain-step-size
      3,		//agc_dig_saturation_exceed_counter *** adi,agc-dig-saturation-exceed-counter
      1000,	// agc_gain_update_interval_us *** adi,agc-gain-update-interval-us
      0,		//agc_immed_gain_change_if_large_adc_overload_enable *** adi,agc-immed-gain-change-if-large-adc-overload-enable
      0,		//agc_immed_gain_change_if_large_lmt_overload_enable *** adi,agc-immed-gain-change-if-large-lmt-overload-enable
      10,		//agc_inner_thresh_high *** adi,agc-inner-thresh-high
      1,		//agc_inner_thresh_high_dec_steps *** adi,agc-inner-thresh-high-dec-steps
      12,		//agc_inner_thresh_low *** adi,agc-inner-thresh-low
      1,		//agc_inner_thresh_low_inc_steps *** adi,agc-inner-thresh-low-inc-steps
      10,		//agc_lmt_overload_large_exceed_counter *** adi,agc-lmt-overload-large-exceed-counter
      2,		//agc_lmt_overload_large_inc_steps *** adi,agc-lmt-overload-large-inc-steps
      10,		//agc_lmt_overload_small_exceed_counter *** adi,agc-lmt-overload-small-exceed-counter
      5,		//agc_outer_thresh_high *** adi,agc-outer-thresh-high
      2,		//agc_outer_thresh_high_dec_steps *** adi,agc-outer-thresh-high-dec-steps
      18,		//agc_outer_thresh_low *** adi,agc-outer-thresh-low
      2,		//agc_outer_thresh_low_inc_steps *** adi,agc-outer-thresh-low-inc-steps
      1,		//agc_attack_delay_extra_margin_us; *** adi,agc-attack-delay-extra-margin-us
      0,		//agc_sync_for_gain_counter_enable *** adi,agc-sync-for-gain-counter-enable
      /* Fast AGC */
      64,		//fagc_dec_pow_measuremnt_duration ***  adi,fagc-dec-pow-measurement-duration
      260,	//fagc_state_wait_time_ns ***  adi,fagc-state-wait-time-ns
      /* Fast AGC - Low Power */
      0,		//fagc_allow_agc_gain_increase ***  adi,fagc-allow-agc-gain-increase-enable
      5,		//fagc_lp_thresh_increment_time ***  adi,fagc-lp-thresh-increment-time
      1,		//fagc_lp_thresh_increment_steps ***  adi,fagc-lp-thresh-increment-steps
      /* Fast AGC - Lock Level (Lock Level is set via slow AGC inner high threshold) */
      1,		//fagc_lock_level_lmt_gain_increase_en ***  adi,fagc-lock-level-lmt-gain-increase-enable
      5,		//fagc_lock_level_gain_increase_upper_limit ***  adi,fagc-lock-level-gain-increase-upper-limit
      /* Fast AGC - Peak Detectors and Final Settling */
      1,		//fagc_lpf_final_settling_steps ***  adi,fagc-lpf-final-settling-steps
      1,		//fagc_lmt_final_settling_steps ***  adi,fagc-lmt-final-settling-steps
      3,		//fagc_final_overrange_count ***  adi,fagc-final-overrange-count
      /* Fast AGC - Final Power Test */
      0,		//fagc_gain_increase_after_gain_lock_en ***  adi,fagc-gain-increase-after-gain-lock-enable
      /* Fast AGC - Unlocking the Gain */
      0,		//fagc_gain_index_type_after_exit_rx_mode ***  adi,fagc-gain-index-type-after-exit-rx-mode
      1,		//fagc_use_last_lock_level_for_set_gain_en ***  adi,fagc-use-last-lock-level-for-set-gain-enable
      1,		//fagc_rst_gla_stronger_sig_thresh_exceeded_en ***  adi,fagc-rst-gla-stronger-sig-thresh-exceeded-enable
      5,		//fagc_optimized_gain_offset ***  adi,fagc-optimized-gain-offset
      10,		//fagc_rst_gla_stronger_sig_thresh_above_ll ***  adi,fagc-rst-gla-stronger-sig-thresh-above-ll
      1,		//fagc_rst_gla_engergy_lost_sig_thresh_exceeded_en ***  adi,fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable
      1,		//fagc_rst_gla_engergy_lost_goto_optim_gain_en ***  adi,fagc-rst-gla-engergy-lost-goto-optim-gain-enable
      10,		//fagc_rst_gla_engergy_lost_sig_thresh_below_ll ***  adi,fagc-rst-gla-engergy-lost-sig-thresh-below-ll
      8,		//fagc_energy_lost_stronger_sig_gain_lock_exit_cnt ***  adi,fagc-energy-lost-stronger-sig-gain-lock-exit-cnt
      1,		//fagc_rst_gla_large_adc_overload_en ***  adi,fagc-rst-gla-large-adc-overload-enable
      1,		//fagc_rst_gla_large_lmt_overload_en ***  adi,fagc-rst-gla-large-lmt-overload-enable
      0,		//fagc_rst_gla_en_agc_pulled_high_en ***  adi,fagc-rst-gla-en-agc-pulled-high-enable
      0,		//fagc_rst_gla_if_en_agc_pulled_high_mode ***  adi,fagc-rst-gla-if-en-agc-pulled-high-mode
      64,		//fagc_power_measurement_duration_in_state5 ***  adi,fagc-power-measurement-duration-in-state5
      /* RSSI Control */
      1,		//rssi_delay *** adi,rssi-delay
      1000,	//rssi_duration *** adi,rssi-duration
      3,		//rssi_restart_mode *** adi,rssi-restart-mode
      0,		//rssi_unit_is_rx_samples_enable *** adi,rssi-unit-is-rx-samples-enable
      1,		//rssi_wait *** adi,rssi-wait
      /* Aux ADC Control */
      256,	//aux_adc_decimation *** adi,aux-adc-decimation
      40000000UL,	//aux_adc_rate *** adi,aux-adc-rate
      /* AuxDAC Control */
      1,		//aux_dac_manual_mode_enable ***  adi,aux-dac-manual-mode-enable
      0,		//aux_dac1_default_value_mV ***  adi,aux-dac1-default-value-mV
      0,		//aux_dac1_active_in_rx_enable ***  adi,aux-dac1-active-in-rx-enable
      0,		//aux_dac1_active_in_tx_enable ***  adi,aux-dac1-active-in-tx-enable
      0,		//aux_dac1_active_in_alert_enable ***  adi,aux-dac1-active-in-alert-enable
      0,		//aux_dac1_rx_delay_us ***  adi,aux-dac1-rx-delay-us
      0,		//aux_dac1_tx_delay_us ***  adi,aux-dac1-tx-delay-us
      0,		//aux_dac2_default_value_mV ***  adi,aux-dac2-default-value-mV
      0,		//aux_dac2_active_in_rx_enable ***  adi,aux-dac2-active-in-rx-enable
      0,		//aux_dac2_active_in_tx_enable ***  adi,aux-dac2-active-in-tx-enable
      0,		//aux_dac2_active_in_alert_enable ***  adi,aux-dac2-active-in-alert-enable
      0,		//aux_dac2_rx_delay_us ***  adi,aux-dac2-rx-delay-us
      0,		//aux_dac2_tx_delay_us ***  adi,aux-dac2-tx-delay-us
      /* Temperature Sensor Control */
      256,	//temp_sense_decimation *** adi,temp-sense-decimation
      1000,	//temp_sense_measurement_interval_ms *** adi,temp-sense-measurement-interval-ms
      (int8_t)0xCE,	//temp_sense_offset_signed *** adi,temp-sense-offset-signed //0xCE,	//temp_sense_offset_signed *** adi,temp-sense-offset-signed
      1,		//temp_sense_periodic_measurement_enable *** adi,temp-sense-periodic-measurement-enable
      /* Control Out Setup */
      0xFF,	//ctrl_outs_enable_mask *** adi,ctrl-outs-enable-mask
      0,		//ctrl_outs_index *** adi,ctrl-outs-index
      /* External LNA Control */
      0,		//elna_settling_delay_ns *** adi,elna-settling-delay-ns
      0,		//elna_gain_mdB *** adi,elna-gain-mdB
      0,		//elna_bypass_loss_mdB *** adi,elna-bypass-loss-mdB
      0,		//elna_rx1_gpo0_control_enable *** adi,elna-rx1-gpo0-control-enable
      0,		//elna_rx2_gpo1_control_enable *** adi,elna-rx2-gpo1-control-enable
      0,		//elna_gaintable_all_index_enable *** adi,elna-gaintable-all-index-enable
      /* Digital Interface Control */
      0,		//digital_interface_tune_skip_mode *** adi,digital-interface-tune-skip-mode
      0,		//digital_interface_tune_fir_disable *** adi,digital-interface-tune-fir-disable
      1,		//pp_tx_swap_enable *** adi,pp-tx-swap-enable
      1,		//pp_rx_swap_enable *** adi,pp-rx-swap-enable
      0,		//tx_channel_swap_enable *** adi,tx-channel-swap-enable
      0,		//rx_channel_swap_enable *** adi,rx-channel-swap-enable
      1,		//rx_frame_pulse_mode_enable *** adi,rx-frame-pulse-mode-enable
      0,		//two_t_two_r_timing_enable *** adi,2t2r-timing-enable
      0,		//invert_data_bus_enable *** adi,invert-data-bus-enable
      0,		//invert_data_clk_enable *** adi,invert-data-clk-enable
      0,		//fdd_alt_word_order_enable *** adi,fdd-alt-word-order-enable
      0,		//invert_rx_frame_enable *** adi,invert-rx-frame-enable
      0,		//fdd_rx_rate_2tx_enable *** adi,fdd-rx-rate-2tx-enable
      0,		//swap_ports_enable *** adi,swap-ports-enable
      0,		//single_data_rate_enable *** adi,single-data-rate-enable
      1,		//lvds_mode_enable *** adi,lvds-mode-enable
      0,		//half_duplex_mode_enable *** adi,half-duplex-mode-enable
      0,		//single_port_mode_enable *** adi,single-port-mode-enable
      0,		//full_port_enable *** adi,full-port-enable
      0,		//full_duplex_swap_bits_enable *** adi,full-duplex-swap-bits-enable
      0,		//delay_rx_data *** adi,delay-rx-data
      0,		//rx_data_clock_delay *** adi,rx-data-clock-delay
      4,		//rx_data_delay *** adi,rx-data-delay
      7,		//tx_fb_clock_delay *** adi,tx-fb-clock-delay
      0,		//tx_data_delay *** adi,tx-data-delay
    #ifdef ALTERA_PLATFORM
      300,	//lvds_bias_mV *** adi,lvds-bias-mV
    #else
      150,	//lvds_bias_mV *** adi,lvds-bias-mV
    #endif
      1,		//lvds_rx_onchip_termination_enable *** adi,lvds-rx-onchip-termination-enable
      0,		//rx1rx2_phase_inversion_en *** adi,rx1-rx2-phase-inversion-enable
      0xFF,	//lvds_invert1_control *** adi,lvds-invert1-control
      0x0F,	//lvds_invert2_control *** adi,lvds-invert2-control
      /* GPO Control */
      0,		//gpo0_inactive_state_high_enable *** adi,gpo0-inactive-state-high-enable
      0,		//gpo1_inactive_state_high_enable *** adi,gpo1-inactive-state-high-enable
      0,		//gpo2_inactive_state_high_enable *** adi,gpo2-inactive-state-high-enable
      0,		//gpo3_inactive_state_high_enable *** adi,gpo3-inactive-state-high-enable
      0,		//gpo0_slave_rx_enable *** adi,gpo0-slave-rx-enable
      0,		//gpo0_slave_tx_enable *** adi,gpo0-slave-tx-enable
      0,		//gpo1_slave_rx_enable *** adi,gpo1-slave-rx-enable
      0,		//gpo1_slave_tx_enable *** adi,gpo1-slave-tx-enable
      0,		//gpo2_slave_rx_enable *** adi,gpo2-slave-rx-enable
      0,		//gpo2_slave_tx_enable *** adi,gpo2-slave-tx-enable
      0,		//gpo3_slave_rx_enable *** adi,gpo3-slave-rx-enable
      0,		//gpo3_slave_tx_enable *** adi,gpo3-slave-tx-enable
      0,		//gpo0_rx_delay_us *** adi,gpo0-rx-delay-us
      0,		//gpo0_tx_delay_us *** adi,gpo0-tx-delay-us
      0,		//gpo1_rx_delay_us *** adi,gpo1-rx-delay-us
      0,		//gpo1_tx_delay_us *** adi,gpo1-tx-delay-us
      0,		//gpo2_rx_delay_us *** adi,gpo2-rx-delay-us
      0,		//gpo2_tx_delay_us *** adi,gpo2-tx-delay-us
      0,		//gpo3_rx_delay_us *** adi,gpo3-rx-delay-us
      0,		//gpo3_tx_delay_us *** adi,gpo3-tx-delay-us
      /* Tx Monitor Control */
      37000,	//low_high_gain_threshold_mdB *** adi,txmon-low-high-thresh
      0,		//low_gain_dB *** adi,txmon-low-gain
      24,		//high_gain_dB *** adi,txmon-high-gain
      0,		//tx_mon_track_en *** adi,txmon-dc-tracking-enable
      0,		//one_shot_mode_en *** adi,txmon-one-shot-mode-enable
      511,	//tx_mon_delay *** adi,txmon-delay
      8192,	//tx_mon_duration *** adi,txmon-duration
      2,		//tx1_mon_front_end_gain *** adi,txmon-1-front-end-gain
      2,		//tx2_mon_front_end_gain *** adi,txmon-2-front-end-gain
      48,		//tx1_mon_lo_cm *** adi,txmon-1-lo-cm
      48,		//tx2_mon_lo_cm *** adi,txmon-2-lo-cm
      /* GPIO definitions */
      -1,		//gpio_resetb *** reset-gpios
      /* MCS Sync */
      -1,		//gpio_sync *** sync-gpios
      -1,		//gpio_cal_sw1 *** cal-sw1-gpios
      -1,		//gpio_cal_sw2 *** cal-sw2-gpios
      /* External LO clocks */
      NULL,	//(*ad9361_rfpll_ext_recalc_rate)()
      NULL,	//(*ad9361_rfpll_ext_round_rate)()
      NULL	//(*ad9361_rfpll_ext_set_rate)()
    };
    m_init_param = default_init_param;

    // nasty cast below included since compiler wouldn't let us cast from
    // Ad9361_config_proxyWorkerTypes::Ad9361_config_proxyWorkerBase::Slave to
    // OCPI::RCC_RCCUserSlave since the former inherits privately from the
    // latter inside this worker's generated header
    spi_init(static_cast<OCPI::RCC::RCCUserSlave*>(static_cast<void *>(&slave)));

    m_rx_fastlock_profiles.clear();
    m_tx_fastlock_profiles.clear();

    return RCC_OK;
  }

  // notification that ad9361_init property has been written
  RCCResult ad9361_init_written() {
    AD9361_InitParam init_param = m_init_param;
    init_param.reference_clk_rate =
        m_properties.ad9361_init.reference_clk_rate;
    init_param.one_rx_one_tx_mode_use_rx_num =
        m_properties.ad9361_init.one_rx_one_tx_mode_use_rx_num;
    init_param.one_rx_one_tx_mode_use_tx_num =
        m_properties.ad9361_init.one_rx_one_tx_mode_use_tx_num;
    init_param.frequency_division_duplex_mode_enable =
        m_properties.ad9361_init.frequency_division_duplex_mode_enable;
    init_param.xo_disable_use_ext_refclk_enable =
        m_properties.ad9361_init.xo_disable_use_ext_refclk_enable;
    init_param.two_t_two_r_timing_enable =
        m_properties.ad9361_init.two_t_two_r_timing_enable;
    init_param.pp_tx_swap_enable =
        m_properties.ad9361_init.pp_tx_swap_enable;
    init_param.pp_rx_swap_enable =
        m_properties.ad9361_init.pp_rx_swap_enable;
    init_param.tx_channel_swap_enable =
        m_properties.ad9361_init.tx_channel_swap_enable;
    init_param.rx_channel_swap_enable =
        m_properties.ad9361_init.rx_channel_swap_enable;
    init_param.delay_rx_data =
        m_properties.ad9361_init.delay_rx_data;
    init_param.rx_data_clock_delay=
        m_properties.ad9361_init.rx_data_clock_delay;
    init_param.rx_data_delay =
        m_properties.ad9361_init.rx_data_delay;
    init_param.tx_fb_clock_delay =
        m_properties.ad9361_init.tx_fb_clock_delay;
    init_param.tx_data_delay =
        m_properties.ad9361_init.tx_data_delay;
    return ad9361_pre_API_call_validation(init_param, true);
  }
  // notification that ad9361_rf_phy property will be read
  RCCResult ad9361_rf_phy_read() {
    RCCResult res = CHECK_IF_POINTER_IS_NULL(ad9361_phy);
    if(res != RCC_OK) return res;
    res           = CHECK_IF_POINTER_IS_NULL(ad9361_phy->pdata);
    if(res != RCC_OK) return res;
    m_properties.ad9361_rf_phy.clk_refin.rate    = ad9361_phy->clk_refin->rate;
    m_properties.ad9361_rf_phy.pdata.rx2tx2      = ad9361_phy->pdata->rx2tx2;
    m_properties.ad9361_rf_phy.pdata.fdd         = ad9361_phy->pdata->fdd;
    m_properties.ad9361_rf_phy.pdata.use_extclk  = ad9361_phy->pdata->use_extclk;
    m_properties.ad9361_rf_phy.pdata.dcxo_coarse = ad9361_phy->pdata->dcxo_coarse;
    m_properties.ad9361_rf_phy.pdata.dcxo_fine   = ad9361_phy->pdata->dcxo_fine;
    m_properties.ad9361_rf_phy.pdata.rx1tx1_mode_use_rx_num = ad9361_phy->pdata->rx1tx1_mode_use_rx_num;
    m_properties.ad9361_rf_phy.pdata.rx1tx1_mode_use_tx_num = ad9361_phy->pdata->rx1tx1_mode_use_tx_num;
    return RCC_OK;
  }
  // notification that en_state_machine_mode property has been written
  RCCResult en_state_machine_mode_written() {
    RCCResult ret = LIBAD9361_API_1PARAMP(&ad9361_set_en_state_machine_mode,
                                 m_properties.en_state_machine_mode);
    enforce_ensm_config();
    return ret;
  }
  // notification that en_state_machine_mode property will be read
  RCCResult en_state_machine_mode_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_en_state_machine_mode,
                                &(m_properties.en_state_machine_mode));
  }
  // notification that rx_rf_gain property has been written
  RCCResult rx_rf_gain_written() {
    return LIBAD9361_API_CHAN_SET(&ad9361_set_rx_rf_gain,
                                  &(m_properties.rx_rf_gain[0]));
  }
  // notification that rx_rf_gain property will be read
  RCCResult rx_rf_gain_read() {
    return LIBAD9361_API_CHAN_GETN(&ad9361_get_rx_rf_gain,
                                   &(m_properties.rx_rf_gain[0]));
  }
  // notification that rx_rf_bandwidth property has been written
  RCCResult rx_rf_bandwidth_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_rx_rf_bandwidth,
                                 m_properties.rx_rf_bandwidth);
  }
  // notification that rx_rf_bandwidth property will be read
  RCCResult rx_rf_bandwidth_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_rx_rf_bandwidth,
                                &(m_properties.rx_rf_bandwidth));
  }
  // notification that rx_sampling_freq property has been written
  RCCResult rx_sampling_freq_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_rx_sampling_freq,
                                 m_properties.rx_sampling_freq);
    return RCC_OK;
  }
  // notification that rx_sampling_freq property will be read
  RCCResult rx_sampling_freq_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_rx_sampling_freq,
                                &(m_properties.rx_sampling_freq));
  }
  // notification that rx_lo_freq property has been written
  RCCResult rx_lo_freq_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_rx_lo_freq,
                                 m_properties.rx_lo_freq);
  }
  // notification that rx_lo_freq property will be read
  RCCResult rx_lo_freq_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_rx_lo_freq,
                                &(m_properties.rx_lo_freq));
  }
  // notification that rx_lo_int_ext property has been written
  RCCResult rx_lo_int_ext_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_rx_lo_int_ext,
                                 m_properties.rx_lo_int_ext);
  }
  // notification that rx_rssi property will be read
  RCCResult rx_rssi_read() {
    rf_rssi rssi[AD9361_CONFIG_TS_PROXY_RX_NCHANNELS];
    RCCResult res = LIBAD9361_API_CHAN_GETN(&ad9361_get_rx_rssi, &(rssi[0]));
    if(res != RCC_OK) return res;
    // explicitly copying struct members so as to avoid potential
    // memory corruption due to the fact that OpenCPI sometimes adds
    // additional "padding" members
    for(uint8_t chan=0; chan<AD9361_CONFIG_TS_PROXY_RX_NCHANNELS; chan++) {
      m_properties.rx_rssi[chan].ant        = rssi[chan].ant;
      m_properties.rx_rssi[chan].symbol     = rssi[chan].symbol;
      m_properties.rx_rssi[chan].preamble   = rssi[chan].preamble;
      m_properties.rx_rssi[chan].multiplier = rssi[chan].multiplier;
      m_properties.rx_rssi[chan].duration   = rssi[chan].duration;
    }
    return RCC_OK;
  }
  // notification that rx_gain_control_mode property has been written
  RCCResult rx_gain_control_mode_written() {
    return LIBAD9361_API_CHAN_SET(&ad9361_set_rx_gain_control_mode,
        &(m_properties.rx_gain_control_mode[0]));
  }
  // notification that rx_gain_control_mode property will be read
  RCCResult rx_gain_control_mode_read() {
    return LIBAD9361_API_CHAN_GET(&ad9361_get_rx_gain_control_mode,
        &(m_properties.rx_gain_control_mode[0]));
  }
  // notification that rx_fir_config_write property has been written
  RCCResult rx_fir_config_write_written() {
    // explicitly copying struct members so as to avoid potential
    // memory corruption due to the fact that OpenCPI sometimes adds
    // additional "padding" members
    AD9361_RXFIRConfig config;
    config.rx = m_properties.rx_fir_config_write.rx;
    config.rx_gain = m_properties.rx_fir_config_write.rx_gain;
    config.rx_dec = m_properties.rx_fir_config_write.rx_dec;
    for(uint8_t idx=0; idx<128; idx++) {
      config.rx_coef[idx] = m_properties.rx_fir_config_write.rx_coef[idx];
    }
    config.rx_coef_size = m_properties.rx_fir_config_write.rx_coef_size;
    // rx_path_clocks is a member of the struct but ad9361_set_rx_fir_config
    // ignores this value, commented it out to avoid the scenario where end user
    // thinks they are setting this value when they really aren't
    //for(uint8_t idx=0; idx<6; idx++) {
    //  config.rx_path_clks[idx] =
    //    m_properties.rx_fir_config_write.rx_path_clks[idx];
    //}
    // rx_bandwidth is a member of the struct but ad9361_set_rx_fir_config_
    // ignores this value, commented it out to avoid the scenario where end user
    // thinks they are setting this value when they really aren't
    //config.rx_bandwidth = m_properties.rx_fir_config_write.rx_bandwidth;

    return LIBAD9361_API_1PARAM(&ad9361_set_rx_fir_config, config);
  }
  // notification that rx_fir_config property will be read
  RCCResult rx_fir_config_read_read() {
    AD9361_RXFIRConfig cfg[AD9361_CONFIG_TS_PROXY_RX_NCHANNELS];

    // explicitly copying struct members so as to avoid potential
    // memory corruption due to the fact that OpenCPI sometimes adds
    // additional "padding" members
    for(uint8_t ch=0; ch<AD9361_CONFIG_TS_PROXY_RX_NCHANNELS; ch++) {

      int32_t res;
      res = m_ad9361_config_proxy.ad9361_get_rx_fir_config(ch, &(cfg[ch]));
      if(res != 0) {
        std::ostringstream oss;
        oss << "ad9361_get_rx_fir_en_dis() returned non-zero value: " << res;
        return setError(oss.str().c_str());
      }

      // rx is a member of the struct but the information conveyed is redundant
      //m_properties.rx_fir_config_read[ch].rx = config[ch].rx;
      
      m_properties.rx_fir_config_read[ch].rx_gain = cfg[ch].rx_gain;
      m_properties.rx_fir_config_read[ch].rx_dec = cfg[ch].rx_dec;
      for(uint8_t idx=0; idx<128; idx++) {
        m_properties.rx_fir_config_read[ch].rx_coef[idx] =
            cfg[ch].rx_coef[idx];
      }
      m_properties.rx_fir_config_read[ch].rx_coef_size =
          cfg[ch].rx_coef_size;

      // rx_path_clocks is a member of the struct but ad9361_set_rx_fir_config
      // ignores this value, commented it out to avoid the scenario where end
      // user thinks they are reading a meaningful value
      //for(uint8_t idx=0; idx<6; idx++) {
      //  m_properties.rx_fir_config_read[ch].rx_path_clks[idx] =
      //      cfg[ch].rx_path_clks[idx];
      //}

      // rx_bandwidth is a member of the struct but ad9361_set_rx_fir_config
      // ignores this value, commented it out to avoid the scenario where end
      // user thinks they are reading a meaningful value
      //m_properties.rx_fir_config_read[ch].rx_bandwidth =
      //    cfg[ch].rx_bandwidth;
    }
    return RCC_OK;
  }
  // notification that rx_fir_en_dis property has been written
  RCCResult rx_fir_en_dis_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_rx_fir_en_dis,
                                 m_properties.rx_fir_en_dis);
  }
  // notification that rx_fir_en_dis property will be read
  RCCResult rx_fir_en_dis_read() {
    uint8_t* en_dis = &m_properties.rx_fir_en_dis;
    int32_t res = m_ad9361_config_proxy.ad9361_get_rx_fir_en_dis(en_dis);
    if(res != 0) {
      std::ostringstream oss;
      oss << "ad9361_get_rx_fir_en_dis() returned non-zero value: " << res;
      return setError(oss.str().c_str());
    }
    return RCC_OK;
  }
  // notification that rx_rfdc_track_en_dis property has been written
  RCCResult rx_rfdc_track_en_dis_written() {
    return LIBAD9361_API_1PARAM(&ad9361_set_rx_rfdc_track_en_dis,
                                m_properties.rx_rfdc_track_en_dis);
  }
  // notification that rx_rfdc_track_en_dis property will be read
  RCCResult rx_rfdc_track_en_dis_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_rx_rfdc_track_en_dis,
                                &(m_properties.rx_rfdc_track_en_dis));
  }
  // notification that rx_bbdc_track_en_dis property has been written
  RCCResult rx_bbdc_track_en_dis_written() {
    return LIBAD9361_API_1PARAM(&ad9361_set_rx_bbdc_track_en_dis,
                                m_properties.rx_bbdc_track_en_dis);
  }
  // notification that rx_bbdc_track_en_dis property will be read
  RCCResult rx_bbdc_track_en_dis_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_rx_bbdc_track_en_dis,
                                &(m_properties.rx_bbdc_track_en_dis));
  }
  // notification that rx_quad_track_en_dis property has been written
  RCCResult rx_quad_track_en_dis_written() {
    return LIBAD9361_API_1PARAM(&ad9361_set_rx_quad_track_en_dis,
                                m_properties.rx_quad_track_en_dis);
  }
  // notification that rx_quad_track_en_dis property will be read
  RCCResult rx_quad_track_en_dis_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_rx_quad_track_en_dis,
                                &(m_properties.rx_quad_track_en_dis));
  }
  // notification that rx_rf_port_input property has been written
  RCCResult rx_rf_port_input_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_rx_rf_port_input,
                                 m_properties.rx_rf_port_input);
  }
  // notification that rx_rf_port_input property will be read
  RCCResult rx_rf_port_input_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_rx_rf_port_input,
                                &(m_properties.rx_rf_port_input));
  }
  // notification that rx_fastlock_store property has been written
  RCCResult rx_fastlock_store_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_rx_fastlock_store,
                                 m_properties.rx_fastlock_store);
  }
  // notification that rx_fastlock_recall property has been written
  RCCResult rx_fastlock_recall_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_rx_fastlock_recall,
                                 m_properties.rx_fastlock_recall);
  }
  // notification that rx_fastlock_load_property will be written
  RCCResult rx_fastlock_load_written() {
    fastlock_profile_t profile_to_load;
    std::vector<fastlock_profile_t>::iterator it =
        find_worker_fastlock_profile(
            m_properties.rx_fastlock_load.worker_profile_id,
            m_rx_fastlock_profiles);
    if(it == m_rx_fastlock_profiles.end())
    {
      return setError("worker_profile_id not found");
    }
    return LIBAD9361_API_2PARAM(&ad9361_rx_fastlock_load,
        m_properties.rx_fastlock_load.ad9361_profile_id,
        profile_to_load.values);
  }
  // notification that rx_fastlock_save property will be written
  RCCResult rx_fastlock_save_written() {
    fastlock_profile_t new_rx_fastlock_profile;
    new_rx_fastlock_profile.id = m_properties.rx_fastlock_save.worker_profile_id;
    RCCResult res = LIBAD9361_API_2PARAM(&ad9361_rx_fastlock_save,
        m_properties.rx_fastlock_save.ad9361_profile_id,
        new_rx_fastlock_profile.values);
    if(res != RCC_OK) return res;
    try
    {
      m_rx_fastlock_profiles.push_back(new_rx_fastlock_profile);
    }
    catch(std::bad_alloc&)
    {
      return setError("Memory allocation failure upon saving AD9361 RX fastlock profile in worker memory");
    }
    return RCC_OK;
  }
  // notification that rx_lo_powerdown property has been written
  /*
  RCCResult rx_lo_powerdown_written() {
    return LIBAD9361_API_1PARAM(&ad9361_set_rx_lo_powerdown,
                                m_properties.rx_lo_powerdown);
  }
  */
  // notification that rx_lo_powerdown property will be read
  /*
  RCCResult rx_lo_powerdown_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_rx_lo_powerdown,
                                &(m_properties.rx_lo_powerdown));
  }
  */
  // notification that tx_attenuation property has been written
  RCCResult tx_attenuation_written() {
    return LIBAD9361_API_CHAN_SET(&ad9361_set_tx_attenuation,
        &(m_properties.tx_attenuation[0]));
  }
  // notification that tx_attenuation property will be read
  RCCResult tx_attenuation_read() {
    return LIBAD9361_API_CHAN_GETN(&ad9361_get_tx_attenuation,
        &(m_properties.tx_attenuation[0]));
  }
  // notification that tx_rf_bandwidth property has been written
  RCCResult tx_rf_bandwidth_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_tx_rf_bandwidth,
                                 m_properties.tx_rf_bandwidth);
  }
  // notification that tx_rf_bandwidth property will be read
  RCCResult tx_rf_bandwidth_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_tx_rf_bandwidth,
                                &(m_properties.tx_rf_bandwidth));
  }
  // notification that tx_sampling_freq property has been written
  RCCResult tx_sampling_freq_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_tx_sampling_freq,
                                 m_properties.tx_sampling_freq);
  }
  // notification that tx_sampling_freq property will be read
  RCCResult tx_sampling_freq_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_tx_sampling_freq,
                                &(m_properties.tx_sampling_freq));
  }
  // notification that tx_lo_freq property has been written
  RCCResult tx_lo_freq_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_tx_lo_freq,
                                 m_properties.tx_lo_freq);
  }
  // notification that tx_lo_freq property will be read
  RCCResult tx_lo_freq_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_tx_lo_freq,
                                &(m_properties.tx_lo_freq));
  }
  // notification that tx_lo_int_ext property has been written
  RCCResult tx_lo_int_ext_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_tx_lo_int_ext,
                                 m_properties.tx_lo_int_ext);
  }
  // notification that tx_fir_config_write property has been written
  RCCResult tx_fir_config_write_written() {
    // explicitly copying struct members so as to avoid potential
    // memory corruption due to the fact that OpenCPI sometimes adds
    // additional "padding" members
    AD9361_TXFIRConfig config;
    config.tx = m_properties.tx_fir_config_write.tx;
    config.tx_gain = m_properties.tx_fir_config_write.tx_gain;
    config.tx_int = m_properties.tx_fir_config_write.tx_int;
    for(uint8_t idx=0; idx<128; idx++) {
      config.tx_coef[idx] = m_properties.tx_fir_config_write.tx_coef[idx];
    }
    config.tx_coef_size = m_properties.tx_fir_config_write.tx_coef_size;
    // tx_path_clocks is a member of the struct but ad9361_set_tx_fir_config
    // ignores this value, commented it out to avoid the scenario where end user
    // thinks they are setting this value when they really aren't
    //for(uint8_t idx=0; idx<6; idx++) {
    //  config.tx_path_clks[idx] =
    //      m_properties.tx_fir_config_write.tx_path_clks[idx];
    //}
    // tx_bandwidth is a member of the struct but ad9361_set_tx_fir_config_
    // ignores this value, commented it out to avoid the scenario where end user
    // thinks they are setting this value when they really aren't
    //config.tx_bandwidth = m_properties.tx_fir_config_write.tx_bandwidth;

    return LIBAD9361_API_1PARAM(&ad9361_set_tx_fir_config, config);
  }
  // notification that tx_fir_config_read property will be read
  RCCResult tx_fir_config_read_read() {
    AD9361_TXFIRConfig config[AD9361_CONFIG_TS_PROXY_RX_NCHANNELS];
    RCCResult res = LIBAD9361_API_CHAN_GET(&ad9361_get_tx_fir_config, &(config[0]));
    if(res != RCC_OK) return res;
    // explicitly copying struct members so as to avoid potential
    // memory corruption due to the fact that OpenCPI sometimes adds
    // additional "padding" members
    for(uint8_t chan=0; chan<AD9361_CONFIG_TS_PROXY_TX_NCHANNELS; chan++) {
      // tx is a member of the struct but the information conveyed is redundant
      //m_properties.tx_fir_config_read[chan].tx = config.tx;
      //
      m_properties.tx_fir_config_read[chan].tx_gain = config[chan].tx_gain;
      m_properties.tx_fir_config_read[chan].tx_int = config[chan].tx_int;
      for(uint8_t idx=0; idx<128; idx++) {
        m_properties.tx_fir_config_read[chan].tx_coef[idx] =
            config[chan].tx_coef[idx];
      }
      m_properties.tx_fir_config_read[chan].tx_coef_size =
          config[chan].tx_coef_size;

      // tx_path_clocks is a member of the struct but ad9361_set_tx_fir_config
      // ignores this value, commented it out to avoid the scenario where end
      // user thinks they are setting this value when they really aren't
      //for(uint8_t idx=0; idx<6; idx++) {
      //  m_properties.tx_fir_config_read[chan].tx_path_clks[idx] =
      //      config[chan].tx_path_clks[idx];
      //}

      // tx_bandwidth is a member of the struct but ad9361_set_tx_fir_config
      // ignores this value, commented it out to avoid the scenario where end
      // user thinks they are setting this value when they really aren't
      //m_properties.tx_fir_config_read[chan].tx_bandwidth =
      //    config[chan].tx_bandwidth;
    }
    return RCC_OK;
  }
  // notification that tx_fir_en_dis property has been written
  RCCResult tx_fir_en_dis_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_tx_fir_en_dis,
                                 m_properties.tx_fir_en_dis);
  }
  // notification that tx_fir_en_dis property will be read
  RCCResult tx_fir_en_dis_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_tx_fir_en_dis,
                                &(m_properties.tx_fir_en_dis));
  }
  // notification that tx_rssi property will be read
  RCCResult tx_rssi_read() {
    return LIBAD9361_API_CHAN_GET(&ad9361_get_tx_rssi,
                                  &(m_properties.tx_rssi[0]));
  }
  // notification that tx_rf_port_output property has been written
  RCCResult tx_rf_port_output_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_tx_rf_port_output,
                                 m_properties.tx_rf_port_output);
  }
  // notification that tx_rf_port_output property will be read
  RCCResult tx_rf_port_output_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_tx_rf_port_output,
                                &(m_properties.tx_rf_port_output));
  }
  // notification that tx_auto_cal_en_dis property has been written
  RCCResult tx_auto_cal_en_dis_written() {
    return LIBAD9361_API_1PARAM(&ad9361_set_tx_auto_cal_en_dis,
                                m_properties.tx_auto_cal_en_dis);
  }
  // notification that tx_auto_cal_en_dis property will be read
  RCCResult tx_auto_cal_en_dis_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_tx_auto_cal_en_dis,
                                &(m_properties.tx_auto_cal_en_dis));
  }
  // notification that tx_fastlock_store property has been written
  RCCResult tx_fastlock_store_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_tx_fastlock_store,
                                 m_properties.tx_fastlock_store);
  }
  // notification that tx_fastlock_recall property has been written
  RCCResult tx_fastlock_recall_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_tx_fastlock_recall,
                                 m_properties.tx_fastlock_recall);
  }
  // notification that tx_fastlock_load_property will be written
  RCCResult tx_fastlock_load_written() {
    fastlock_profile_t profile_to_load;
    std::vector<fastlock_profile_t>::iterator it =
        find_worker_fastlock_profile(
            m_properties.tx_fastlock_save.worker_profile_id,
            m_rx_fastlock_profiles);
    if(it == m_tx_fastlock_profiles.end())
    {
      return setError("worker_profile_id not found");
    }
    return LIBAD9361_API_2PARAM(&ad9361_tx_fastlock_load,
        m_properties.tx_fastlock_load.ad9361_profile_id,
        profile_to_load.values);
  }
  // notification that tx_fastlock_save property will be written
  RCCResult tx_fastlock_save_written() {
    fastlock_profile_t new_tx_fastlock_profile;
    new_tx_fastlock_profile.id = m_properties.tx_fastlock_save.worker_profile_id;
    RCCResult res = LIBAD9361_API_2PARAM(&ad9361_tx_fastlock_save,
        m_properties.tx_fastlock_save.ad9361_profile_id,
        new_tx_fastlock_profile.values);
    if(res != RCC_OK) return res;
    try
    {
      m_tx_fastlock_profiles.push_back(new_tx_fastlock_profile);
    }
    catch(std::bad_alloc&)
    {
      return setError("Memory allocation failure upon saving AD9361 TX fastlock profile in worker memory");
    }
    return RCC_OK;
  }
  // notification that tx_lo_powerdown property has been written
  /*
  RCCResult tx_lo_powerdown_written() {
    return LIBAD9361_API_1PARAM(&ad9361_set_tx_lo_powerdown,
                                m_properties.tx_lo_powerdown);
  }
  */
  // notification that tx_lo_powerdown property will be read
  /*
  RCCResult tx_lo_powerdown_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_tx_lo_powerdown,
                                &(m_properties.tx_lo_powerdown));
  }
  */
  // notification that trx_path_clks property has been written
  RCCResult trx_path_clks_written() {
    return LIBAD9361_API_2PARAM(&ad9361_set_trx_path_clks,
                                m_properties.trx_path_clks.rx_path_clks,
                                m_properties.trx_path_clks.tx_path_clks);
  }
  // notification that trx_path_clks property will be read
  RCCResult trx_path_clks_read() {
    return LIBAD9361_API_2PARAM(&ad9361_get_trx_path_clks,
                                &(m_properties.trx_path_clks.rx_path_clks[0]),
                                &(m_properties.trx_path_clks.tx_path_clks[0]));
  }
  // notification that trx_lo_powerdown property has been written
  /*
  RCCResult trx_lo_powerdown_written() {
    return LIBAD9361_API_2PARAM(&ad9361_set_trx_lo_powerdown,
                                m_properties.trx_lo_powerdown.pd_rx,
                                m_properties.trx_lo_powerdown.pd_tx);
  }
  */
  // notification that no_ch_mode property has been written
  RCCResult no_ch_mode_written() {
    RCCResult res;

    // if prop value is MODE_2x2, fail if there aren't 2 qadc and 2 qdac workers
    // in the FPGA bitstream (ad9361_config.hdl has this info in its properties)
    const bool qadc1_is_present = slave.get_qadc1_is_present();
    const bool qdac1_is_present = slave.get_qdac1_is_present();
    if((!qadc1_is_present) && (!qdac1_is_present) &
       (m_properties.no_ch_mode == MODE_2x2))
    {
      const bool qadc0_is_present = slave.get_qadc0_is_present();
      int num_rx = qadc0_is_present ? 1 : 0;
      num_rx += qadc1_is_present ? 1 : 0;
      const bool qdac0_is_present = slave.get_qdac0_is_present();
      int num_tx = qdac0_is_present ? 1 : 0;
      num_tx += qdac1_is_present ? 1 : 0;
      std::ostringstream ostr;
      ostr << "no_ch_mode attempted to set to MODE_2x2 but loaded FPGA " <<
              "bitstream only contains device worker support for " <<
              num_rx << " RX and " <<
              num_tx << " TX channels";
      return setError(ostr.str().c_str());
    }
    
    // No-OS's set_no_ch_mode() re-initializes the AD9361!!!!
    uint32_t tx_sampling_freq;
    res = LIBAD9361_API_1PARAM(&ad9361_get_tx_sampling_freq,
                               &tx_sampling_freq);
    if(res != RCC_OK) return res;

    res = LIBAD9361_API_1PARAMP(&ad9361_set_no_ch_mode,
                                m_properties.no_ch_mode);
    if(res != RCC_OK) return res;

    set_FPGA_channel_config(); // because channel config potentially changed

    // No-OS's set_no_ch_mode() re-initializes the AD9361!!!!
    RCCResult r = LIBAD9361_API_1PARAMP(&ad9361_set_tx_sampling_freq,
                                 tx_sampling_freq);

    enforce_ensm_config();
    return r;
  }
  // notification that trx_fir_en_dis property has been written
  RCCResult trx_fir_en_dis_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_trx_fir_en_dis,
                                 m_properties.trx_fir_en_dis);
  }
  // notification that trx_rate_gov property has been written
  RCCResult trx_rate_gov_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_set_trx_rate_gov,
                                 m_properties.trx_rate_gov);
  }
  // notification that trx_rate_gov property will be read
  RCCResult trx_rate_gov_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_trx_rate_gov,
                                &(m_properties.trx_rate_gov));
  }
  // notification that do_calib property has been written
  RCCResult do_calib_written() {
    return LIBAD9361_API_2PARAM(&ad9361_do_calib,
                                m_properties.do_calib.cal,
                                m_properties.do_calib.arg);
  }
  // notification that trx_load_enable_fir property has been written
  RCCResult trx_load_enable_fir_written() {
    // explicitly copying struct members so as to avoid potential
    // memory corruption due to the fact that OpenCPI sometimes adds
    // additional "padding" members
    AD9361_RXFIRConfig rx_config;
    rx_config.rx = m_properties.trx_load_enable_fir.rx;
    rx_config.rx_gain = m_properties.trx_load_enable_fir.rx_gain;
    rx_config.rx_dec = m_properties.trx_load_enable_fir.rx_dec;
    for(uint8_t idx=0; idx<128; idx++) {
      rx_config.rx_coef[idx] = m_properties.trx_load_enable_fir.rx_coef[idx];
    }
    rx_config.rx_coef_size = m_properties.trx_load_enable_fir.rx_coef_size;
    for(uint8_t idx=0; idx<6; idx++) {
      rx_config.rx_path_clks[idx] =
        m_properties.trx_load_enable_fir.rx_path_clks[idx];
    }
    rx_config.rx_bandwidth = m_properties.trx_load_enable_fir.rx_bandwidth;

    // explicitly copying struct members so as to avoid potential
    // memory corruption due to the fact that OpenCPI sometimes adds
    // additional "padding" members
    AD9361_TXFIRConfig tx_config;
    tx_config.tx = m_properties.trx_load_enable_fir.tx;
    tx_config.tx_gain = m_properties.trx_load_enable_fir.tx_gain;
    tx_config.tx_int = m_properties.trx_load_enable_fir.tx_int;
    for(uint8_t idx=0; idx<128; idx++) {
      tx_config.tx_coef[idx] = m_properties.trx_load_enable_fir.tx_coef[idx];
    }
    tx_config.tx_coef_size = m_properties.trx_load_enable_fir.tx_coef_size;
    for(uint8_t idx=0; idx<6; idx++) {
      tx_config.tx_path_clks[idx] =
          m_properties.trx_load_enable_fir.tx_path_clks[idx];
    }
    tx_config.tx_bandwidth = m_properties.trx_load_enable_fir.tx_bandwidth;

    return LIBAD9361_API_2PARAM(&ad9361_trx_load_enable_fir,
                                rx_config, tx_config);
  }
  // notification that do_dcxo_tune_coarse property has been written
  RCCResult do_dcxo_tune_coarse_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_do_dcxo_tune_coarse,
                                 m_properties.do_dcxo_tune_coarse);
  }
  // notification that do_dcxo_tune_fine property has been written
  RCCResult do_dcxo_tune_fine_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_do_dcxo_tune_fine,
                                 m_properties.do_dcxo_tune_fine);
  }
  // notification that temperature property will be read
  RCCResult temperature_read() {
    return LIBAD9361_API_1PARAM(&ad9361_get_temperature,
                                &(m_properties.temperature));
  }
  // notification that bist_loopback property has been written
  RCCResult bist_loopback_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_bist_loopback,
                                 m_properties.bist_loopback);
  }
  // notification that bist_loopback property will be read
  RCCResult bist_loopback_read() {
    return LIBAD9361_API_1PARAMV(&ad9361_get_bist_loopback,
                                 &(m_properties.bist_loopback));
  }
  // notification that bist_prbs property has been written
  RCCResult bist_prbs_written() {
    return LIBAD9361_API_1PARAMP(&ad9361_bist_prbs,
                                 (ad9361_bist_mode)m_properties.bist_prbs);
  }
  // notification that bist_prbs property will be read
  RCCResult bist_prbs_read() {
    ad9361_bist_mode bist_prbs;

    RCCResult res = LIBAD9361_API_1PARAMV(&ad9361_get_bist_prbs, &bist_prbs);
    if(res != RCC_OK) return res;
    if(bist_prbs > (int)(255))
    {
      std::ostringstream ostr;
      ostr << "ad9361_get_bist_prbs set at unexpected large value which cannot"
           << "be accounted for";
      return setError(ostr.str().c_str());
    }
    m_properties.bist_prbs = (char)(bist_prbs & 0xff);
    return RCC_OK;
  }
  // notification that bist_tone property has been written
  RCCResult bist_tone_written() {
    RCCResult ret = CHECK_IF_POINTER_IS_NULL(ad9361_phy);
    if(ret != RCC_OK) return ret;
    ret           = CHECK_IF_POINTER_IS_NULL(ad9361_phy->pdata);
    if(ret != RCC_OK) return ret;
    // we don't know how to parameters for this function, so just printing ...
    log(OCPI_LOG_DEBUG, "No-OS API call: ad9361_bist_tone(...)\n");
    const int32_t res = ad9361_bist_tone(ad9361_phy,
                                  (ad9361_bist_mode)m_properties.bist_tone.mode,
                                  m_properties.bist_tone.freq_Hz,
                                  m_properties.bist_tone.level_dB,
                                  m_properties.bist_tone.mask);
    if(res != 0)
    {
      std::ostringstream err;
      err << "ad9361_bist_tone() returned: " << res;
      return setError(err.str().c_str());
    }
    return RCC_OK;
  }
  // notification that bist_tone property will be read
  RCCResult bist_tone_read() {
    RCCResult res = CHECK_IF_POINTER_IS_NULL(ad9361_phy);
    if(res != RCC_OK) return res;
    res           = CHECK_IF_POINTER_IS_NULL(ad9361_phy->pdata);
    if(res != RCC_OK) return res;
    ad9361_bist_mode mode;
    // ad9361_get_bist_tone() returns void
    ad9361_get_bist_tone(ad9361_phy,
                         &mode,
                         &m_properties.bist_tone.freq_Hz,
                         &m_properties.bist_tone.level_dB,
                         &m_properties.bist_tone.mask);
    if(mode > (int)(255))
    {
      std::ostringstream ostr;
      ostr << "ad9361_get_bist_tone set at unexpected large value which cannot"
           << "be accounted for";
      return setError(ostr.str().c_str());
    }
    m_properties.bist_tone.mode = (char)(mode & 0xff);
    return RCC_OK;
  }
  // notification that DATA_CLK_Delay property will be read
  RCCResult DATA_CLK_Delay_read() {
    uint8_t Rx_Clock_and_Data_Delay = slave.get_general_rx_clock_data_delay();
    uint8_t DATA_CLK_Delay_3_0 = (Rx_Clock_and_Data_Delay & (D7_BITMASK | D6_BITMASK | D5_BITMASK | D4_BITMASK)) >> 4;
    m_properties.DATA_CLK_Delay = DATA_CLK_Delay_3_0;
    return RCC_OK;
  }
  // notification that Rx_Data_Delay property will be read
  RCCResult Rx_Data_Delay_read() {
    uint8_t Rx_Clock_and_Data_Delay = slave.get_general_rx_clock_data_delay();
    uint8_t Rx_Data_Delay_3_0 = (Rx_Clock_and_Data_Delay & (D3_BITMASK | D2_BITMASK | D1_BITMASK | D0_BITMASK));
    m_properties.Rx_Data_Delay = Rx_Data_Delay_3_0;
    return RCC_OK;
  }
  // notification that FB_CLK_Delay property will be read
  RCCResult FB_CLK_Delay_read() {
    uint8_t Tx_Clock_and_Data_Delay = slave.get_general_tx_clock_data_delay();
    uint8_t FB_CLK_Delay_3_0 = (Tx_Clock_and_Data_Delay & (D7_BITMASK | D6_BITMASK | D5_BITMASK | D4_BITMASK)) >> 4;
    m_properties.FB_CLK_Delay = FB_CLK_Delay_3_0;
    return RCC_OK;
  }
  // notification that Tx_Data_Delay property will be read
  RCCResult Tx_Data_Delay_read() {
    uint8_t Tx_Clock_and_Data_Delay = slave.get_general_tx_clock_data_delay();
    uint8_t Tx_Data_Delay_3_0 = (Tx_Clock_and_Data_Delay & (D3_BITMASK | D2_BITMASK | D1_BITMASK | D0_BITMASK));
    m_properties.Tx_Data_Delay = Tx_Data_Delay_3_0;
    return RCC_OK;
  }
  // notification that THB3_Enable_and_Interp property will be read
  RCCResult THB3_Enable_and_Interp_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 4
    uint8_t reg = slave.get_general_tx_enable_filter_ctrl();
    uint8_t THB3_Enable_and_Interp_1_0 = (reg & (D5_BITMASK | D4_BITMASK)) >> 4;
    enum THB3_Enable_and_Interp& p = m_properties.THB3_Enable_and_Interp;
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 5 Table 2.
    switch(THB3_Enable_and_Interp_1_0)
    {
      case        0x00:
        p = THB3_ENABLE_AND_INTERP_INTERPOLATE_BY_1_NO_FILTERING;
        break;
      case        0x01:
        p = THB3_ENABLE_AND_INTERP_INTERPOLATE_BY_2_HALF_BAND_FILTER;
        break;
      case        0x02:
        p = THB3_ENABLE_AND_INTERP_INTERPOLATE_BY_3_AND_FILTER;
        break;
      default: // 0x03
        p = THB3_ENABLE_AND_INTERP_INVALID;
        break;
    }
    return RCC_OK;
  }
  // notification that THB2_Enable property will be read
  RCCResult THB2_Enable_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 4
    uint8_t reg = slave.get_general_tx_enable_filter_ctrl();
    uint8_t THB2_Enable = (reg & D3_BITMASK) >> 3;
    m_properties.THB2_Enable = (THB2_Enable == 1) ? true : false;
    return RCC_OK;
  }
  // notification that THB1_Enable property will be read
  RCCResult THB1_Enable_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 4
    uint8_t reg = slave.get_general_tx_enable_filter_ctrl();
    uint8_t THB1_Enable = (reg & D2_BITMASK) >> 2;
    m_properties.THB1_Enable = (THB1_Enable == 1) ? true : false;
    return RCC_OK;
  }
  // notification that RHB3_Enable_and_Decimation property will be read
  RCCResult RHB3_Enable_and_Decimation_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 4
    uint8_t reg = slave.get_general_rx_enable_filter_ctrl();
    uint8_t RHB3_Enable_and_Decimation_1_0 = (reg & (D5_BITMASK | D4_BITMASK)) >> 4;
    enum RHB3_Enable_and_Decimation& p = m_properties.RHB3_Enable_and_Decimation;
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 5 Table 4.
    switch(RHB3_Enable_and_Decimation_1_0)
    {
      case        0x00:
        p = RHB3_ENABLE_AND_DECIMATION_DECIMATE_BY_1_NO_FILTERING;
        break;
      case        0x01:
        p = RHB3_ENABLE_AND_DECIMATION_DECIMATE_BY_2_HALF_BAND_FILTER;
        break;
      case        0x02:
        p = RHB3_ENABLE_AND_DECIMATION_DECIMATE_BY_3_AND_FILTER;
        break;
      default: // 0x03
        p = RHB3_ENABLE_AND_DECIMATION_INVALID;
        break;
    }
    return RCC_OK;
  }
  // notification that RHB2_Enable property will be read
  RCCResult RHB2_Enable_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 5
    uint8_t reg = slave.get_general_rx_enable_filter_ctrl();
    uint8_t RHB2_Enable = (reg & D3_BITMASK) >> 3;
    m_properties.RHB2_Enable = (RHB2_Enable == 1) ? true : false;
    return RCC_OK;
  }
  // notification that RHB1_Enable property will be read
  RCCResult RHB1_Enable_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 4
    uint8_t reg = slave.get_general_rx_enable_filter_ctrl();
    uint8_t RHB1_Enable = (reg & D2_BITMASK) >> 2;
    m_properties.RHB1_Enable = (RHB1_Enable == 1) ? true : false;
    return RCC_OK;
  }
  // notification that DAC_Clk_div2 property will be read
  RCCResult DAC_Clk_div2_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 7
    uint8_t BBPLL = slave.get_clock_bbpll();
    uint8_t DAC_Clk_div2 = (BBPLL & 0x08) >> 3;
    m_properties.DAC_Clk_div2 = (DAC_Clk_div2 == 1) ? true : false;
    return RCC_OK;
  }
  // notification that BBPLL_Divider property will be read
  RCCResult BBPLL_Divider_read() {
    // from AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 7:
    // SPI Register 0x00A-BBPLL
    // [D2:D0]-BBPLL Divider[2:0]
    // BBPLL Divider[2:0] is valid from 1 through 6.
    uint8_t val_reg_read = slave.get_clock_bbpll();
    uint8_t BBPLL_Divider_2_0 = (val_reg_read & 0x07);
    if((BBPLL_Divider_2_0 == 0) or
       (BBPLL_Divider_2_0 == 7))
    {
      m_properties.BBPLL_Divider = BBPLL_DIVIDER_INVALID;
    }
    else
    {
      switch(BBPLL_Divider_2_0)
      {
        ENUM_BBPLL_DIVIDER(1, BBPLL_Divider);
        ENUM_BBPLL_DIVIDER(2, BBPLL_Divider);
        ENUM_BBPLL_DIVIDER(3, BBPLL_Divider);
        ENUM_BBPLL_DIVIDER(4, BBPLL_Divider);
        ENUM_BBPLL_DIVIDER(5, BBPLL_Divider);
        default: // 6
          m_properties.BBPLL_Divider = BBPLL_DIVIDER_6; break;
      }
    }

    return RCC_OK;
  }
  // notification that Fractional_BB_Frequency_Word property will be read
  RCCResult Fractional_BB_Frequency_Word_read() {
    uint8_t val_reg_read;
    uint32_t tmp = 0;

    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 19

    // Fractional BB Frequency Word[20:16]
    {
    val_reg_read = slave.get_bbpll_fract_bb_freq_word_1();
    uint8_t Fractional_BB_Frequency_Word_20_16 = (val_reg_read & 0x1f);
    tmp |= ((uint32_t)Fractional_BB_Frequency_Word_20_16) << 16;
    }

    // Fractional BB Frequency Word[15:8]
    {
    val_reg_read = slave.get_bbpll_fract_bb_freq_word_2();
    uint8_t Fractional_BB_Frequency_Word_15_8 = val_reg_read;
    tmp |= ((uint32_t)Fractional_BB_Frequency_Word_15_8) << 8;
    }

    // Fractional BB Frequency Word[7:0]
    {
    val_reg_read = slave.get_bbpll_fract_bb_freq_word_3();
    uint8_t Fractional_BB_Frequency_Word_7_0 = val_reg_read;
    tmp |= ((uint32_t)Fractional_BB_Frequency_Word_7_0);
    }

    m_properties.Fractional_BB_Frequency_Word = tmp;
    return RCC_OK;
  }
  // notification that Integer_BB_Frequency_Word property will be read
  RCCResult Integer_BB_Frequency_Word_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 19
    uint8_t val_reg_read = slave.get_bbpll_integer_bb_freq_word();
    m_properties.Integer_BB_Frequency_Word = (uint16_t) val_reg_read;
    return RCC_OK;
  }
  // notification that BBPLL_Ref_Clock_Scaler property will be read
  RCCResult BBPLL_Ref_Clock_Scaler_read() {
    // from AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 21:
    // SPI Register 0x045-Ref Clock Scaler
    // [D1:D0]-Ref Clock Scaler[1:0]
    // The reference clock frequency is scaled before it enters the
    // BBPLL. 00: x1; 01: x1/2; 10: x1/4; 11: x2.
    uint8_t val_reg_read = slave.get_bbpll_ref_clock_scaler();
    uint8_t Ref_Clock_Scaler_1_0 = (val_reg_read & 0x03);
    switch(Ref_Clock_Scaler_1_0)
    {
      case        0x00: m_properties.BBPLL_Ref_Clock_Scaler = 1.0F;  break;
      case        0x01: m_properties.BBPLL_Ref_Clock_Scaler = 0.5F;  break;
      case        0x02: m_properties.BBPLL_Ref_Clock_Scaler = 0.25F; break;
      default: // 0x03
                        m_properties.BBPLL_Ref_Clock_Scaler = 2.0F;  break;
    }
    return RCC_OK;
  }
  // notification that Tx_BBF_Tune_Divider property will be read
  RCCResult Tx_BBF_Tune_Divider_read() {

    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 34

    uint16_t Tx_BBF_Tune_Divider_8_0;
    {
      uint16_t tmp = 0;

      // Tx BBF Tune Divider[7:0]
      {
      uint8_t Tx_BBF_Tune_Divider_7_0 = slave.get_tx_bbf_tune_divider();
      tmp |= ((uint16_t)Tx_BBF_Tune_Divider_7_0);
      }

      // Tx BBF Tune Divider[8]
      {
      uint8_t val_reg_read = slave.get_tx_bbf_tune_mode();
      uint8_t Tx_BBF_Tune_Divider_8  = (val_reg_read & D0_BITMASK);
      tmp |= (((uint16_t)Tx_BBF_Tune_Divider_8) << 8);
      }

      Tx_BBF_Tune_Divider_8_0 = tmp;
    }

    if(Tx_BBF_Tune_Divider_8_0 == 0)
    {
      m_properties.Tx_BBF_Tune_Divider = TX_BBF_TUNE_DIVIDER_INVALID;
    }
    else
    {
      switch(Tx_BBF_Tune_Divider_8_0)
      {
        ENUM_TX_BBF_TUNE_DIVIDER(1,   Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(2,   Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(3,   Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(4,   Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(5,   Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(6,   Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(7,   Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(8,   Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(9,   Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(10,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(11,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(12,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(13,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(14,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(15,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(16,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(17,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(18,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(19,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(20,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(21,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(22,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(23,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(24,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(25,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(26,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(27,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(28,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(29,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(30,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(31,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(32,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(33,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(34,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(35,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(36,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(37,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(38,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(39,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(40,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(41,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(42,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(43,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(44,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(45,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(46,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(47,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(48,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(49,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(50,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(51,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(52,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(53,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(54,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(55,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(56,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(57,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(58,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(59,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(60,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(61,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(62,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(63,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(64,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(65,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(66,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(67,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(68,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(69,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(70,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(71,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(72,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(73,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(74,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(75,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(76,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(77,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(78,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(79,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(80,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(81,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(82,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(83,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(84,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(85,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(86,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(87,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(88,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(89,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(90,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(91,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(92,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(93,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(94,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(95,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(96,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(97,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(98,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(99,  Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(100, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(101, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(102, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(103, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(104, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(105, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(106, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(107, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(108, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(109, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(110, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(111, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(112, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(113, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(114, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(115, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(116, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(117, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(118, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(119, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(120, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(121, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(122, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(123, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(124, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(125, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(126, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(127, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(128, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(129, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(130, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(131, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(132, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(133, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(134, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(135, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(136, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(137, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(138, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(139, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(140, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(141, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(142, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(143, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(144, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(145, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(146, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(147, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(148, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(149, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(150, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(151, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(152, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(153, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(154, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(155, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(156, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(157, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(158, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(159, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(160, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(161, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(162, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(163, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(164, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(165, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(166, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(167, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(168, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(169, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(170, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(171, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(172, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(173, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(174, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(175, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(176, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(177, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(178, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(179, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(180, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(181, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(182, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(183, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(184, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(185, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(186, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(187, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(188, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(189, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(190, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(191, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(192, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(193, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(194, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(195, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(196, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(197, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(198, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(199, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(200, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(201, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(202, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(203, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(204, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(205, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(206, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(207, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(208, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(209, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(210, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(211, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(212, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(213, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(214, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(215, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(216, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(217, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(218, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(219, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(220, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(221, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(222, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(223, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(224, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(225, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(226, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(227, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(228, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(229, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(230, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(231, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(232, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(233, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(234, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(235, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(236, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(237, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(238, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(239, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(240, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(241, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(242, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(243, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(244, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(245, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(246, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(247, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(248, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(249, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(250, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(251, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(252, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(253, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(254, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(255, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(256, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(257, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(258, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(259, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(260, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(261, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(262, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(263, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(264, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(265, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(266, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(267, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(268, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(269, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(270, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(271, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(272, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(273, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(274, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(275, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(276, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(277, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(278, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(279, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(280, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(281, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(282, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(283, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(284, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(285, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(286, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(287, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(288, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(289, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(290, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(291, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(292, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(293, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(294, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(295, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(296, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(297, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(298, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(299, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(300, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(301, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(302, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(303, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(304, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(305, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(306, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(307, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(308, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(309, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(310, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(311, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(312, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(313, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(314, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(315, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(316, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(317, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(318, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(319, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(320, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(321, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(322, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(323, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(324, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(325, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(326, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(327, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(328, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(329, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(330, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(331, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(332, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(333, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(334, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(335, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(336, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(337, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(338, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(339, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(340, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(341, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(342, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(343, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(344, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(345, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(346, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(347, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(348, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(349, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(350, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(351, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(352, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(353, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(354, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(355, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(356, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(357, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(358, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(359, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(360, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(361, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(362, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(363, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(364, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(365, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(366, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(367, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(368, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(369, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(370, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(371, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(372, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(373, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(374, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(375, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(376, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(377, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(378, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(379, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(380, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(381, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(382, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(383, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(384, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(385, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(386, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(387, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(388, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(389, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(390, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(391, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(392, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(393, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(394, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(395, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(396, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(397, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(398, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(399, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(400, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(401, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(402, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(403, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(404, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(405, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(406, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(407, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(408, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(409, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(410, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(411, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(412, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(413, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(414, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(415, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(416, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(417, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(418, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(419, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(420, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(421, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(422, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(423, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(424, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(425, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(426, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(427, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(428, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(429, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(430, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(431, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(432, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(433, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(434, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(435, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(436, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(437, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(438, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(439, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(440, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(441, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(442, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(443, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(444, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(445, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(446, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(447, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(448, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(449, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(450, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(451, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(452, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(453, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(454, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(455, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(456, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(457, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(458, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(459, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(460, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(461, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(462, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(463, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(464, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(465, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(466, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(467, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(468, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(469, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(470, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(471, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(472, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(473, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(474, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(475, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(476, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(477, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(478, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(479, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(480, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(481, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(482, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(483, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(484, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(485, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(486, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(487, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(488, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(489, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(490, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(491, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(492, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(493, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(494, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(495, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(496, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(497, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(498, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(499, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(500, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(501, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(502, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(503, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(504, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(505, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(506, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(507, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(508, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(509, Tx_BBF_Tune_Divider);
        ENUM_TX_BBF_TUNE_DIVIDER(510, Tx_BBF_Tune_Divider);
        default: // 511
          m_properties.Tx_BBF_Tune_Divider = TX_BBF_TUNE_DIVIDER_511; break;
      }
    }
    return RCC_OK;
  }
  // notification that Tx_Secondary_Filter_Resistor property will be read
  RCCResult Tx_Secondary_Filter_Resistor_read() {
    // from AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 33:
    // SPI Register 0x0D1-Resistor[3:0]
    //------------------------------------------------------
    // Resistor[3:0]  | Post-Filter Stage Resistance (ohms)
    //------------------------------------------------------
    // 0001           | 800
    // 0011           | 400
    // 0100           | 200
    // 1100           | 100
    //------------------------------------------------------
    uint8_t Resistor_3_0;
    {
    uint8_t val_reg_read = slave.get_tx_secondf_resistor();
    Resistor_3_0 = (val_reg_read & 0x0f);
    }
    switch(Resistor_3_0)
    {
      case 0x01: m_properties.Tx_Secondary_Filter_Resistor = TX_SECONDARY_FILTER_RESISTOR_800;     break;
      case 0x03: m_properties.Tx_Secondary_Filter_Resistor = TX_SECONDARY_FILTER_RESISTOR_400;     break;
      case 0x04: m_properties.Tx_Secondary_Filter_Resistor = TX_SECONDARY_FILTER_RESISTOR_200;     break;
      case 0x0c: m_properties.Tx_Secondary_Filter_Resistor = TX_SECONDARY_FILTER_RESISTOR_100;     break;
      default:   m_properties.Tx_Secondary_Filter_Resistor = TX_SECONDARY_FILTER_RESISTOR_INVALID; break;
    }
    return RCC_OK;
  }
  // notification that Tx_Secondary_Filter_Capacitor property will be read
  RCCResult Tx_Secondary_Filter_Capacitor_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 33
    uint8_t Capacitor_5_0;
    {
    uint8_t val_reg_read = slave.get_tx_secondf_capacitor();
    Capacitor_5_0 = (val_reg_read & 0x3f);
    }
    m_properties.Tx_Secondary_Filter_Capacitor = Capacitor_5_0;
    return RCC_OK;
  }
  // notification that Rx_BBF_Tune_Divide property will be read
  RCCResult Rx_BBF_Tune_Divide_read() {

    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 59

    uint16_t Rx_BBF_Tune_Divide_8_0;
    {
      uint16_t tmp = 0;

      // Rx BBF Tune Divide[7:0]
      {
      uint8_t Rx_BBF_Tune_Divide_7_0 = slave.get_rx_bbf_tune_config_divide();
      tmp |= ((uint16_t)Rx_BBF_Tune_Divide_7_0);
      }

      // Rx BBF Tune Divide[8]
      {
      uint8_t val_reg_read = slave.get_rx_bbf_tune_config_config();
      uint8_t Rx_BBF_Tune_Divide_8  = (val_reg_read & D0_BITMASK);
      tmp |= (((uint16_t)Rx_BBF_Tune_Divide_8) << 8);
      }

      Rx_BBF_Tune_Divide_8_0 = tmp;
    }

    if(Rx_BBF_Tune_Divide_8_0 == 0)
    {
      m_properties.Rx_BBF_Tune_Divide = RX_BBF_TUNE_DIVIDE_INVALID;
    }
    else
    {
      switch(Rx_BBF_Tune_Divide_8_0)
      {
        ENUM_RX_BBF_TUNE_DIVIDE(1,   Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(2,   Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(3,   Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(4,   Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(5,   Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(6,   Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(7,   Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(8,   Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(9,   Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(10,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(11,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(12,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(13,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(14,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(15,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(16,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(17,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(18,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(19,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(20,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(21,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(22,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(23,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(24,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(25,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(26,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(27,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(28,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(29,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(30,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(31,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(32,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(33,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(34,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(35,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(36,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(37,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(38,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(39,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(40,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(41,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(42,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(43,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(44,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(45,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(46,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(47,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(48,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(49,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(50,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(51,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(52,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(53,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(54,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(55,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(56,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(57,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(58,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(59,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(60,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(61,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(62,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(63,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(64,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(65,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(66,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(67,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(68,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(69,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(70,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(71,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(72,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(73,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(74,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(75,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(76,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(77,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(78,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(79,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(80,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(81,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(82,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(83,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(84,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(85,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(86,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(87,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(88,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(89,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(90,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(91,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(92,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(93,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(94,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(95,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(96,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(97,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(98,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(99,  Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(100, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(101, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(102, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(103, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(104, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(105, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(106, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(107, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(108, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(109, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(110, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(111, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(112, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(113, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(114, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(115, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(116, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(117, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(118, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(119, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(120, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(121, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(122, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(123, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(124, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(125, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(126, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(127, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(128, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(129, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(130, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(131, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(132, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(133, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(134, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(135, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(136, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(137, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(138, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(139, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(140, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(141, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(142, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(143, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(144, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(145, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(146, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(147, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(148, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(149, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(150, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(151, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(152, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(153, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(154, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(155, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(156, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(157, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(158, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(159, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(160, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(161, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(162, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(163, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(164, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(165, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(166, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(167, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(168, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(169, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(170, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(171, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(172, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(173, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(174, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(175, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(176, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(177, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(178, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(179, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(180, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(181, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(182, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(183, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(184, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(185, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(186, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(187, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(188, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(189, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(190, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(191, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(192, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(193, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(194, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(195, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(196, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(197, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(198, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(199, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(200, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(201, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(202, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(203, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(204, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(205, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(206, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(207, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(208, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(209, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(210, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(211, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(212, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(213, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(214, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(215, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(216, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(217, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(218, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(219, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(220, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(221, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(222, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(223, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(224, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(225, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(226, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(227, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(228, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(229, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(230, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(231, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(232, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(233, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(234, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(235, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(236, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(237, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(238, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(239, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(240, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(241, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(242, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(243, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(244, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(245, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(246, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(247, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(248, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(249, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(250, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(251, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(252, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(253, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(254, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(255, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(256, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(257, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(258, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(259, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(260, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(261, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(262, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(263, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(264, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(265, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(266, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(267, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(268, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(269, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(270, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(271, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(272, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(273, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(274, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(275, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(276, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(277, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(278, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(279, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(280, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(281, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(282, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(283, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(284, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(285, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(286, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(287, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(288, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(289, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(290, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(291, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(292, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(293, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(294, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(295, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(296, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(297, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(298, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(299, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(300, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(301, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(302, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(303, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(304, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(305, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(306, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(307, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(308, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(309, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(310, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(311, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(312, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(313, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(314, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(315, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(316, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(317, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(318, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(319, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(320, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(321, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(322, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(323, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(324, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(325, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(326, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(327, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(328, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(329, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(330, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(331, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(332, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(333, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(334, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(335, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(336, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(337, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(338, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(339, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(340, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(341, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(342, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(343, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(344, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(345, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(346, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(347, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(348, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(349, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(350, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(351, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(352, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(353, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(354, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(355, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(356, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(357, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(358, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(359, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(360, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(361, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(362, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(363, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(364, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(365, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(366, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(367, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(368, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(369, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(370, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(371, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(372, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(373, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(374, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(375, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(376, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(377, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(378, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(379, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(380, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(381, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(382, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(383, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(384, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(385, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(386, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(387, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(388, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(389, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(390, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(391, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(392, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(393, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(394, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(395, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(396, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(397, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(398, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(399, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(400, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(401, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(402, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(403, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(404, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(405, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(406, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(407, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(408, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(409, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(410, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(411, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(412, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(413, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(414, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(415, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(416, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(417, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(418, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(419, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(420, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(421, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(422, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(423, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(424, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(425, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(426, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(427, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(428, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(429, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(430, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(431, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(432, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(433, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(434, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(435, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(436, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(437, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(438, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(439, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(440, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(441, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(442, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(443, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(444, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(445, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(446, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(447, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(448, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(449, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(450, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(451, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(452, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(453, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(454, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(455, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(456, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(457, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(458, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(459, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(460, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(461, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(462, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(463, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(464, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(465, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(466, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(467, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(468, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(469, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(470, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(471, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(472, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(473, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(474, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(475, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(476, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(477, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(478, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(479, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(480, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(481, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(482, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(483, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(484, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(485, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(486, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(487, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(488, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(489, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(490, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(491, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(492, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(493, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(494, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(495, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(496, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(497, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(498, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(499, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(500, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(501, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(502, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(503, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(504, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(505, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(506, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(507, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(508, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(509, Rx_BBF_Tune_Divide);
        ENUM_RX_BBF_TUNE_DIVIDE(510, Rx_BBF_Tune_Divide);
        default: // 511
          m_properties.Rx_BBF_Tune_Divide = RX_BBF_TUNE_DIVIDE_511; break;
      }
    }
    return RCC_OK;
  }
  // notification that bb_pll_is_locked property will be read
  RCCResult bb_pll_is_locked_read() {
    m_properties.bb_pll_is_locked = (slave.get_overflow_ch_1() & BBPLL_LOCK) == BBPLL_LOCK;
    return RCC_OK;
  }
  // notification that rx_pll_is_locked property will be read
  RCCResult rx_pll_is_locked_read() {
    const uint8_t rx_synth_lock_detect_config = LOCK_DETECT_MODE(slave.get_rx_synth_lock_detect_config());
    bool RX_PLL_lock_detect_enabled = false;
    if(rx_synth_lock_detect_config == 0x01) RX_PLL_lock_detect_enabled = true;
    if(rx_synth_lock_detect_config == 0x02) RX_PLL_lock_detect_enabled = true;
    if(!RX_PLL_lock_detect_enabled)
    {
      m_properties.rx_pll_is_locked = RX_PLL_IS_LOCKED_UNKNOWN;
      return RCC_OK;
    }
    const uint8_t rx_synth_cp_ovrg_vco_lock = slave.get_rx_synth_cp_ovrg_vco_lock();
    if((rx_synth_cp_ovrg_vco_lock & VCO_LOCK) == VCO_LOCK)
    {
      m_properties.rx_pll_is_locked = RX_PLL_IS_LOCKED_TRUE;
    }
    else
    {
      m_properties.rx_pll_is_locked = RX_PLL_IS_LOCKED_FALSE;
    }
    return RCC_OK;
  }
  // notification that rx_fastlock_delete property has been written
  RCCResult rx_fastlock_delete_written() {
    std::vector<fastlock_profile_t>::iterator it =
        find_worker_fastlock_profile(m_properties.rx_fastlock_delete,
                                     m_tx_fastlock_profiles);
    if(it == m_rx_fastlock_profiles.end())
    {
      return setError("worker_profile_id not found");
    }
    m_rx_fastlock_profiles.erase(it);
    return RCC_OK;
  }
  // notification that tx_pll_is_locked property will be read
  RCCResult tx_pll_is_locked_read() {
    const uint8_t tx_synth_lock_detect_config = LOCK_DETECT_MODE(slave.get_tx_synth_lock_detect_config());
    bool TX_PLL_lock_detect_enabled = false;
    if(tx_synth_lock_detect_config == 0x01) TX_PLL_lock_detect_enabled = true;
    if(tx_synth_lock_detect_config == 0x02) TX_PLL_lock_detect_enabled = true;
    if(!TX_PLL_lock_detect_enabled)
    {
      m_properties.tx_pll_is_locked = TX_PLL_IS_LOCKED_UNKNOWN;
      return RCC_OK;
    }
    const uint8_t tx_synth_cp_overrange_vco_lock = slave.get_tx_synth_cp_overrange_vco_lock();
    if((tx_synth_cp_overrange_vco_lock & VCO_LOCK) == VCO_LOCK)
    {
      m_properties.tx_pll_is_locked = TX_PLL_IS_LOCKED_TRUE;
    }
    else
    {
      m_properties.tx_pll_is_locked = TX_PLL_IS_LOCKED_FALSE;
    }
    return RCC_OK;
  }
  // notification that tx_fastlock_delete property has been written
  RCCResult tx_fastlock_delete_written() {
    std::vector<fastlock_profile_t>::iterator it =
        find_worker_fastlock_profile(m_properties.tx_fastlock_delete,
                                     m_tx_fastlock_profiles);
    if(it == m_tx_fastlock_profiles.end())
    {
      return setError("worker_profile_id not found");
    }
    m_tx_fastlock_profiles.erase(it);
    return RCC_OK;
  }
  // notification that rx_vco_divider property will be read
  RCCResult rx_vco_divider_read() {
    uint8_t divider = (slave.get_general_rfpll_dividers() & 0x0f);
    switch(divider)
    {
      case 0: m_properties.rx_vco_divider = RX_VCO_DIVIDER_2; break;
      case 1: m_properties.rx_vco_divider = RX_VCO_DIVIDER_4; break;
      case 2: m_properties.rx_vco_divider = RX_VCO_DIVIDER_8; break;
      case 3: m_properties.rx_vco_divider = RX_VCO_DIVIDER_16; break;
      case 4: m_properties.rx_vco_divider = RX_VCO_DIVIDER_32; break;
      case 5: m_properties.rx_vco_divider = RX_VCO_DIVIDER_64; break;
      case 6: m_properties.rx_vco_divider = RX_VCO_DIVIDER_128; break;
      case 7: m_properties.rx_vco_divider = RX_VCO_DIVIDER_EXTERNAL_2; break;
      default: m_properties.rx_vco_divider = RX_VCO_DIVIDER_INVALID; break;
    }
    return RCC_OK;
  }
  // notification that rx_vco_n_integer property will be read
  RCCResult rx_vco_n_integer_read() {
    uint16_t integer = (uint16_t)slave.get_rx_synth_integer_byte_0();
    integer |= ((uint16_t)((slave.get_rx_synth_integer_byte_1() & 0x07)) << 8);
    m_properties.rx_vco_n_integer = integer;
    return RCC_OK;
  }
  // notification that rx_vco_n_fractional property will be read
  RCCResult rx_vco_n_fractional_read() {
    uint32_t frac = (uint32_t)slave.get_rx_synth_fract_byte_0();
    frac |= ((uint32_t)(slave.get_rx_synth_fract_byte_1()) << 8);
    frac |= ((uint32_t)((slave.get_rx_synth_fract_byte_2()) & 0x7f) << 16);
    m_properties.rx_vco_n_fractional = frac;
    return RCC_OK;
  }
  // notification that Rx_Ref_Divider property will be read
  RCCResult Rx_Ref_Divider_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 68
    uint8_t Ref_Divide_Config_1 = slave.get_ref_divide_config_1();
    uint8_t Ref_Divide_Config_2 = slave.get_ref_divide_config_2();
    uint8_t Rx_Ref_Divider_1 = (uint8_t)(Ref_Divide_Config_1 & D0_BITMASK);
    uint8_t Rx_Ref_Divider_0 = (uint8_t)(Ref_Divide_Config_2 & D7_BITMASK) >> 7;
    uint8_t Rx_Ref_Divider_1_0 = (Rx_Ref_Divider_1 << 1) | Rx_Ref_Divider_0;
    switch(Rx_Ref_Divider_1_0)
    {
      case        0x00: m_properties.Rx_Ref_Divider = 1.0F;  break;
      case        0x01: m_properties.Rx_Ref_Divider = 0.5F;  break;
      case        0x02: m_properties.Rx_Ref_Divider = 0.25F; break;
      default: // 0x03
                        m_properties.Rx_Ref_Divider = 2.0F;  break;
    }
    return RCC_OK;
  }
  // notification that tx_vco_divider property will be read
  RCCResult tx_vco_divider_read() {
    uint8_t divider = (slave.get_general_rfpll_dividers() & 0xf0) >> 4;
    switch(divider)
    {
      case 0: m_properties.tx_vco_divider = TX_VCO_DIVIDER_2; break;
      case 1: m_properties.tx_vco_divider = TX_VCO_DIVIDER_4; break;
      case 2: m_properties.tx_vco_divider = TX_VCO_DIVIDER_8; break;
      case 3: m_properties.tx_vco_divider = TX_VCO_DIVIDER_16; break;
      case 4: m_properties.tx_vco_divider = TX_VCO_DIVIDER_32; break;
      case 5: m_properties.tx_vco_divider = TX_VCO_DIVIDER_64; break;
      case 6: m_properties.tx_vco_divider = TX_VCO_DIVIDER_128; break;
      case 7: m_properties.tx_vco_divider = TX_VCO_DIVIDER_EXTERNAL_2; break;
      default: m_properties.tx_vco_divider = TX_VCO_DIVIDER_INVALID; break;
    }
    return RCC_OK;
  }
  // notification that tx_vco_n_integer property will be read
  RCCResult tx_vco_n_integer_read() {
    uint16_t integer = (uint16_t)slave.get_tx_synth_integer_byte_0();
    integer |= ((uint16_t)((slave.get_tx_synth_integer_byte_1() & 0x07)) << 8);
    m_properties.tx_vco_n_integer = integer;
    return RCC_OK;
  }
  // notification that tx_vco_n_fractional property will be read
  RCCResult tx_vco_n_fractional_read() {
    uint32_t frac = (uint32_t)slave.get_tx_synth_fract_byte_0();
    frac |= ((uint32_t)(slave.get_tx_synth_fract_byte_1()) << 8);
    frac |= ((uint32_t)((slave.get_tx_synth_fract_byte_2()) & 0x7f) << 16);
    m_properties.tx_vco_n_fractional = frac;
    return RCC_OK;
  }
  // notification that Tx_Ref_Divider property will be read
  RCCResult Tx_Ref_Divider_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 68
    uint8_t Ref_Divide_Config_2 = slave.get_ref_divide_config_2();
    uint8_t Tx_Ref_Divider_1_0 = (uint8_t)(Ref_Divide_Config_2 & 0x0c) >> 2;
    switch(Tx_Ref_Divider_1_0)
    {
      case        0x00: m_properties.Tx_Ref_Divider = 1.0F;  break;
      case        0x01: m_properties.Tx_Ref_Divider = 0.5F;  break;
      case        0x02: m_properties.Tx_Ref_Divider = 0.25F; break;
      default: // 0x03
                        m_properties.Tx_Ref_Divider = 2.0F;  break;
    }
    return RCC_OK;
  }
  // notification that Tx_Channel_Swap property will be read
  RCCResult Tx_Channel_Swap_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 9
    uint8_t Parallel_Port_Configuration_1 = slave.get_parallel_port_conf_1();
    uint8_t Tx_Channel_Swap = (Parallel_Port_Configuration_1 & D5_BITMASK) >> 5;
    m_properties.Tx_Channel_Swap = (Tx_Channel_Swap == 1);
    return RCC_OK;
  }
  // notification that Rx_Channel_Swap property will be read
  RCCResult Rx_Channel_Swap_read() {
    // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 9
    uint8_t Parallel_Port_Configuration_1 = slave.get_parallel_port_conf_1();
    uint8_t Rx_Channel_Swap = (Parallel_Port_Configuration_1 & D4_BITMASK) >> 4;
    m_properties.Rx_Channel_Swap = (Rx_Channel_Swap == 1);
    return RCC_OK;
  }
  // notification that LVDS property will be read
  RCCResult LVDS_read() {
    m_properties.LVDS = slave.get_LVDS();
    return RCC_OK;
  }
  // notification that single_port property will be read
  RCCResult single_port_read() {
    m_properties.single_port = slave.get_single_port();
    return RCC_OK;
  }
  // notification that swap_ports property will be read
  RCCResult swap_ports_read() {
    m_properties.swap_ports = slave.get_swap_ports();
    return RCC_OK;
  }
  // notification that half_duplex property will be read
  RCCResult half_duplex_read() {
    m_properties.half_duplex = slave.get_half_duplex();
    return RCC_OK;
  }
  // notification that data_rate_config property will be read
  RCCResult data_rate_config_read() {
    const uint8_t drs           = (uint8_t)slave.get_data_rate_config();
    const bool modeIsDDR        = (drs           == 1);
    m_properties.data_rate_config = modeIsDDR ? DATA_RATE_CONFIG_DDR : DATA_RATE_CONFIG_SDR;
    return RCC_OK;
  }
  // notification that DATA_CLK_P_rate_Hz property will be read
  RCCResult DATA_CLK_P_rate_Hz_read() {

    /* @todo / FIXME - replace this with AD9361ConfigProxy class's
     *                 get_clkrf_freq_Hz for maximum precision*/
    uint32_t rx_sampling_freq;
    RCCResult res = LIBAD9361_API_1PARAM(&ad9361_get_rx_sampling_freq,
                                         &rx_sampling_freq);

    /* @todo / FIXME - move this functionality into get_data_clk_p_rate_Hz()
     *                 method in AD9361ConfigProxy class */
    if(res != RCC_OK) { return res; }
    res = CHECK_IF_POINTER_IS_NULL(ad9361_phy);
    if(res != RCC_OK) { return res; }
    res = CHECK_IF_POINTER_IS_NULL(ad9361_phy->pdata);
    if(res != RCC_OK) { return res; }
    const uint8_t parallel_port_conf_1 = (uint8_t)slave.get_parallel_port_conf_1();
    const uint8_t two_t_two_r_timing_enable = ((parallel_port_conf_1 & 0x04) >> 2);
    double DATA_CLK_P_rate_Hz = (two_t_two_r_timing_enable == 1) ?
       2.*rx_sampling_freq : rx_sampling_freq;
    const bool LVDS = slave.get_LVDS();
    DATA_CLK_P_rate_Hz *= LVDS ? 2. : 1.;
    m_properties.DATA_CLK_P_rate_Hz = DATA_CLK_P_rate_Hz;
    return RCC_OK;
  }
  // notification that BIST_Mask_Channel_2_Q_data property has been written
  RCCResult BIST_Mask_Channel_2_Q_data_written() {
    uint8_t reg = slave.get_test_bist_and_data_port_test_config();
    if(m_properties.BIST_Mask_Channel_2_Q_data) {
      reg |= D5_BITMASK;
    }
    else {
      reg &= ~D5_BITMASK;
    }
    slave.set_test_bist_and_data_port_test_config(reg);
    return RCC_OK;
  }
  // notification that BIST_Mask_Channel_2_Q_data property will be read
  RCCResult BIST_Mask_Channel_2_Q_data_read() {
    uint8_t reg = slave.get_test_bist_and_data_port_test_config();
    m_properties.BIST_Mask_Channel_2_Q_data = (reg & D5_BITMASK) == D5_BITMASK;
    return RCC_OK;
  }
  // notification that BIST_Mask_Channel_2_I_data property has been written
  RCCResult BIST_Mask_Channel_2_I_data_written() {
    uint8_t reg = slave.get_test_bist_and_data_port_test_config();
    if(m_properties.BIST_Mask_Channel_2_I_data) {
      reg |= D4_BITMASK;
    }
    else {
      reg &= ~D4_BITMASK;
    }
    slave.set_test_bist_and_data_port_test_config(reg);
    return RCC_OK;
  }
  // notification that BIST_Mask_Channel_2_I_data property will be read
  RCCResult BIST_Mask_Channel_2_I_data_read() {
    uint8_t reg = slave.get_test_bist_and_data_port_test_config();
    m_properties.BIST_Mask_Channel_2_I_data = (reg & D4_BITMASK) == D4_BITMASK;
    return RCC_OK;
  }
  // notification that BIST_Mask_Channel_1_Q_data property has been written
  RCCResult BIST_Mask_Channel_1_Q_data_written() {
    uint8_t reg = slave.get_test_bist_and_data_port_test_config();
    if(m_properties.BIST_Mask_Channel_1_Q_data) {
      reg |= D3_BITMASK;
    }
    else {
      reg &= ~D3_BITMASK;
    }
    slave.set_test_bist_and_data_port_test_config(reg);
    return RCC_OK;
  }
  // notification that BIST_Mask_Channel_1_Q_data property will be read
  RCCResult BIST_Mask_Channel_1_Q_data_read() {
    uint8_t reg = slave.get_test_bist_and_data_port_test_config();
    m_properties.BIST_Mask_Channel_1_Q_data = (reg & D3_BITMASK) == D3_BITMASK;
    return RCC_OK;
  }
  // notification that BIST_Mask_Channel_1_I_data property has been written
  RCCResult BIST_Mask_Channel_1_I_data_written() {
    uint8_t reg = slave.get_test_bist_and_data_port_test_config();
    if(m_properties.BIST_Mask_Channel_1_I_data) {
      reg |= D2_BITMASK;
    }
    else {
      reg &= ~D2_BITMASK;
    }
    slave.set_test_bist_and_data_port_test_config(reg);
    return RCC_OK;
  }
  // notification that BIST_Mask_Channel_1_I_data property will be read
  RCCResult BIST_Mask_Channel_1_I_data_read() {
    uint8_t reg = slave.get_test_bist_and_data_port_test_config();
    m_properties.BIST_Mask_Channel_1_I_data = (reg & D2_BITMASK) == D2_BITMASK;
    return RCC_OK;
  }
  // notification that digital_rx_block_delay_sec property will be read
  RCCResult digital_rx_block_delay_sec_read() {

    m_properties.digital_rx_block_delay_sec.resize(0);

    std::vector<uint8_t> rx_chs;
    rx_chs.push_back(RX1);
    rx_chs.push_back(RX2);
    for(auto it = rx_chs.begin(); it != rx_chs.end(); ++it) {
      const size_t len = m_properties.digital_rx_block_delay_sec.length;
      double delay;
      try {
        delay = m_ad9361_config_proxy.get_digital_rx_block_delay_sec(*it);
      }
      catch(std::string& err) {
        continue;
      }
      m_properties.digital_rx_block_delay_sec.resize(len+1);
      Rf_port port;
      if(*it == RX1) {
        port = DIGITAL_RX_BLOCK_DELAY_SEC_RF_PORT_RX1;
      }
      else { // *it == RX2
        port = DIGITAL_RX_BLOCK_DELAY_SEC_RF_PORT_RX2;
      }
      m_properties.digital_rx_block_delay_sec.data[len].rf_port = port;
      m_properties.digital_rx_block_delay_sec.data[len].delay_sec = delay;
    }

    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    return RCC_DONE;
  }
};

AD9361_CONFIG_TS_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
AD9361_CONFIG_TS_PROXY_END_INFO
