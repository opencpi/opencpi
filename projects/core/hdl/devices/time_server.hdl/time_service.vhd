-- This file is protected by Copyright. Please refer to the COPYRIGHT file
-- distributed with this source distribution.
--
-- This file is part of OpenCPI <http://www.opencpi.org>
--
-- OpenCPI is free software: you can redistribute it and/or modify it under the
-- terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option) any
-- later version.
--
-- OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
-- A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
-- This module implement the Hardware Time Service (HTS) of the OpenCPI infrastructure.
-- The HTS module is instanced once per chip to serve as the chip-local source
-- of time. The "time" (sec and fractional) of the HTS may be set by
-- software (coarse) or disciplined to a hardware GPS (fine).
--
-- For each worker that requires "time", a Time Client is used to synchronize
-- from the HTS clock domain to the target's clock domain.
-- 
-- Ideally, the HTS is clocked off the fastest and most stable (MHz +/- PPM) clock
-- available to the system, such that, the most accurate "time keeping" may be
-- achieved and sustained during the absence of the GPS PPS signal.
--
--
-- TODO:
--
-- All CDC modules should be closely examined for proper usage.
--
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all; use IEEE.math_real.all;
library ocpi, util, bsv; use ocpi.all, ocpi.types.all, util.util.all, bsv.bsv.SyncFIFO;
library platform; use platform.platform_pkg.all;
entity time_service is
  generic (
    g_TIMECLK_FREQ      : integer := 200e6);
  port (
    CLK                 : in  std_logic;
    RST                 : in  std_logic;
    timeCLK             : in  std_logic;
    timeRST             : in  std_logic;
    ppsIn               : in  std_logic;  -- ASYNC
    -- Property interface (Control Clock Domain)
    timeControl         : in  ulong_t;
    timeControl_written : in  bool_t;
    timeStatus          : out ulong_t;
    timeNowIn           : in  ulonglong_t;
    timeNow_written     : in  bool_t;
    timeNowOut          : out ulonglong_t;
    timeDeltaIn         : in  ulonglong_t;
    timeDelta_written   : in  bool_t;
    timeDeltaOut        : out ulonglong_t;
    ticksPerSecond      : out ulong_t;
    -- Outputs (Time (timeCLK) Clock Domain)
    ppsOut              : out std_logic;
    time_service        : out time_service_t  -- time service clock domain
    );
end entity time_service;

architecture rtl of time_service is
  -----------------------------------------------------------------------------
  -- Platform:  TC Freq   TC-10%        TC+0.1%         TC-0.1%         FracInc
  --            (MHz)     (MHz)         (MHz)           (MHz)           (MHz)
  --
  -- Simulators 100        90           100.1           99.9            2621541
  -- ML605      200        180          200.2           199.8           1407374
  -- ALST4      100        90           100.1           99.9            2621541
  -- Zynq-based 100        90           100.1           99.9            2621541
  --
  -----------------------------------------------------------------------------
  -- Defines 90% threshold of a "seconds" counter at the timeCLK rate and is 
  -- used to create the s_ppsDrive, which is a possible source for ppsOut.
  --
  -- ML605:
  --   200M - (200M * 10%) = 180,000,000 = x"ABA_9500"
  -- Simulators and ALST4 and Zynq-based:
  --   100M - (100M * 10%) = 90,000,000  = x"55D_4A80"
  --
  constant c_refPerCountTH : std_logic_vector(27 downto 0)
    := std_logic_vector(to_unsigned(g_TIMECLK_FREQ - (g_TIMECLK_FREQ)/10, 28));
  --
  -----------------------------------------------------------------------------
  -- Upper Threshold of Window for detecting External PPS pulse:
  -- (Counter, in timeCLK domain, after one second + 0.1%) (1000 PPM)
  --
  -- ML605:
  --   200M + 0.1% = 200200000 = x"BEE_CF40"
  -- Simulators and ALST4 and Zynq-based:
  --   100M + 0.1% = 100100000 = x"5F7_67A0"
  --
  constant c_upperThresOfWindow : std_logic_vector(27 downto 0)
    := std_logic_vector(to_unsigned(g_TIMECLK_FREQ + (g_TIMECLK_FREQ)/1000, 28));
  --
  -----------------------------------------------------------------------------
  -- Lower Threshold of Window for detecting External PPS pulse:
  -- (Counter, in timeCLK domain, after one second - 0.1%) (1000 PPM)
  --
  -- ML605:
  --   200M - 0.1% = 199800000 = x"BE8_B4C0"
  -- Simulators and ALST4 and Zynq-based
  --   100M - 0.1% = 99900000 = x"5F4_5A60"
  --
  constant c_lowerThresOfWindow : std_logic_vector(27 downto 0)
    := std_logic_vector(to_unsigned(g_TIMECLK_FREQ - (g_TIMECLK_FREQ)/1000, 28));
  --
  -----------------------------------------------------------------------------
  -- Fraction Increment Value (Default):
  --
  -- Fraction Precision:
  --
  -- 1 sec/2^48 = 3.5527137ps, (2^48 = 281474976710656)
  --
  -- Time Clock Rate:   Platforms
  -- 200MHz:            ML605
  -- 100MHz:            Simulators and ALST4 and Zynq-based
  --
  -- (2^48)/(200MHz)= ceil(1407374.88355) = 1407374 = x15_798E
  -- (2^48)/(100MHz)= ceil(2814749.76711) = 2814749 = x2A_F31D
  --
  constant c_fracInc : std_logic_vector(49 downto 0)
    := std_logic_vector(to_unsigned(281474977/(g_TIMECLK_FREQ/1000000), 50));
  -------------------------------------------------------------------------------
  --
  signal s_doClear                             : std_logic;
  signal s_timeIn                              : std_logic_vector(63 downto 0);
  signal s_statusOut                           : std_logic_vector(31 downto 0);
  --
  signal s_disableServo_dD_OUT                 : std_logic;
  signal s_disableServo_dD_OUT_slv0            : std_logic_vector(0 downto 0);
  signal s_disableServo_sD_IN_slv0             : std_logic_vector(0 downto 0);
  signal s_disableServo_sRDY                   : std_logic;
  --
  signal s_nowInCC_dD_OUT                      : std_logic_vector(63 downto 0);
  signal s_nowInCC_sD_IN                       : std_logic_vector(63 downto 0);
  signal s_nowInCC_sRDY                        : std_logic;
  --
  signal s_nowInCC_sReg1                       : std_logic_vector(63 downto 0);
  signal s_nowInCC_dReg1                       : std_logic_vector(63 downto 0);
  --
  signal s_nowTC                               : std_logic_vector(63 downto 0);
  --
  signal s_ppsDisablePPS_dD_OUT                : std_logic;
  signal s_ppsDisablePPS_dD_OUT_slv0           : std_logic_vector(0 downto 0);
  signal s_ppsDisablePPS_sD_IN_slv0            : std_logic_vector(0 downto 0);
  signal s_ppsDisablePPS_sRDY                  : std_logic;
  --
  signal s_ppsLostCC_dD_OUT                    : std_logic;
  signal s_ppsLostCC_dD_OUT_slv0               : std_logic_vector(0 downto 0);
  signal s_ppsLostCC_sD_IN_slv0                : std_logic_vector(0 downto 0);
  signal s_ppsLostCC_sRDY                      : std_logic;
  --
  signal s_ppsOKCC_dD_OUT                      : std_logic;
  signal s_ppsOKCC_dD_OUT_slv0                 : std_logic_vector(0 downto 0);
  signal s_ppsOKCC_sD_IN_slv0                  : std_logic_vector(0 downto 0);
  signal s_ppsOKCC_sRDY                        : std_logic;
  --
  signal s_ppsOutMode_dD_OUT                   : std_logic_vector(1 downto 0);
  signal s_ppsOutMode_sD_IN                    : std_logic_vector(1 downto 0);
  signal s_ppsOutMode_sRDY                     : std_logic;
  --
  signal s_refPerPPS_dD_OUT                    : std_logic_vector(27 downto 0);
  signal s_refPerPPS_sD_IN                     : std_logic_vector(27 downto 0);
  signal s_refPerPPS_sEN                       : std_logic;
  signal s_refPerPPS_sRDY                      : std_logic;
  --
  signal s_rollingPPSIn_dD_OUT                 : std_logic_vector(7 downto 0);
  signal s_rollingPPSIn_sD_IN                  : std_logic_vector(7 downto 0);
  signal s_rollingPPSIn_sRDY                   : std_logic;
  --
  signal s_setRefF_dD_OUT                      : std_logic_vector(63 downto 0);
  signal s_setRefF_sENQ                        : std_logic;
  signal s_setRefF_dDEQ                        : std_logic;
  signal s_setRefF_dEMPTY_N                    : std_logic;
  --
  signal s_gpsInSticky                         : std_logic;
  signal s_ppsInSticky                         : std_logic;
  signal s_ppsLostSticky                       : std_logic;
  signal s_rplTimeControl                      : std_logic_vector(4 downto 0);
  signal s_timeSetSticky                       : std_logic;
  --
  signal s_ppsExtSync_d1                       : std_logic;
  signal s_ppsExtSync_d2                       : std_logic;
  signal s_ppsExtSyncD                         : std_logic;
  signal s_xo2                                 : std_logic;
  --
  signal s_refFromRise                         : std_logic_vector(27 downto 0);
  signal s_refFromRise_lowerThresOfWindow      : std_logic;
  signal s_refFromRise_upperThresOfWindow      : std_logic;
  signal s_ppsIn_detectedInWindow              : std_logic;
  signal s_ppsIn_detectedOutWindow             : std_logic;
  --
  signal s_refFreeCount                        : std_logic_vector(27 downto 0);
  signal s_refFreeSamp                         : std_logic_vector(27 downto 0);
  signal s_refFreeSpan                         : std_logic_vector(27 downto 0);
  --
  signal s_refSecCount_EN                      : std_logic;
  signal s_refSecCount                         : std_logic_vector(31 downto 0);
  signal s_refPerCount                         : std_logic_vector(31 downto 0);
  signal s_ppsDrive                            : std_logic;
  --
  signal x_281474976710656_minus_delSecond_50b : std_logic_vector(49 downto 0);
  signal s_base2exp48_minus_delSecond_50b        : std_logic_vector(49 downto 0);  
  signal x_281474976710656_minus_delSecond_22b : std_logic_vector(21 downto 0);
  signal s_fracBeta                            : std_logic_vector(49 downto 0);
  signal s_delSec                              : std_logic_vector(1 downto 0);
  signal s_delSecond                           : std_logic_vector(49 downto 0);
  signal s_lastSecond                          : std_logic_vector(49 downto 0);
  signal s_fracInc                             : std_logic_vector(49 downto 0);
  signal s_jamFrac_EN                          : std_logic;
  signal s_jamFrac_Val                         : std_logic_vector(49 downto 0);
  signal s_fracSeconds                         : std_logic_vector(49 downto 0);
  --
  signal s_ppsEdgeCount                        : std_logic_vector(7 downto 0);
  signal s_ppsLost                             : std_logic;
  signal s_ppsOK                               : std_logic;
  signal RST_N                                 : std_logic;
  signal timeRST_N                             : std_logic;
begin

  -- For older SyncFIFO modules until they get fixed
  RST_N <= not RST;
  timeRST_N <= not timeRST;

  -----------------------------------------------------------------------------
  -- Outputs assignments
  -----------------------------------------------------------------------------

  -- Control Clock Domain  
  s_statusOut <= (s_ppsLostSticky & s_gpsInSticky & s_ppsInSticky & s_timeSetSticky &
                  s_ppsOKCC_dD_OUT & s_ppsLostCC_dD_OUT & "00" &
                  x"0000" & s_rollingPPSIn_dD_OUT);
  timeStatus  <= ulong_t(s_statusOut);

  ticksPerSecond       <= ulong_t(resize(unsigned(s_refPerPPS_dD_OUT), ulong_t'length));
  timeNowOut           <= ulonglong_t(s_nowInCC_dD_OUT);

  -- Time Clock Domain
  time_service.now     <= ulonglong_t(s_nowTC);
  time_service.clk     <= timeCLK;
  time_service.reset   <= timeRST;

  -----------------------------------------------------------------------------
  -- Input assignments
  -----------------------------------------------------------------------------
  s_timeIn <= std_logic_vector(timeNowIn) when its(timeNow_written) else
                   std_logic_vector(timeDeltaIn) when its(timeDelta_written) else
                   std_logic_vector(to_ulonglong(timeControl));
  -- clear Clock domain status/sticky bits
  s_doClear <= '1' when timeControl_written and s_timeIn(31) = '1' else '0';

  -----------------------------------------------------------------------------
  -- Calculate deltaTime based on 'now' and time adjustment provided by host
  -----------------------------------------------------------------------------
  outReg_deltaTime : process(CLK)
  begin
    if(rising_edge(CLK)) then
      if (timeRST = '1') then
        timeDeltaOut <= (others => '0');
      else
        if (timeDelta_written = '1') then
          timeDeltaOut <= ulonglong_t(std_logic_vector(signed(s_nowInCC_dD_OUT) - signed(s_timeIn)));
        end if;
      end if;
    end if;
  end process;
  
  ------------------------------------------------------------------------------
  -- TODO:
  -- Identify optimization/placement VHDL attributes to assign to register signals
  -- 
  -- Async to Time Clock domain:
  -- Since there is no access to the clock that is generates (synchronized) the PPS,
  -- double registering the External PPS sync pulse input.
  ------------------------------------------------------------------------------
  inRegx2_ppsIn : process(timeCLK)
  begin
    if(rising_edge(timeCLK)) then
      if (timeRST = '1') then
        s_ppsExtSync_d1 <= '0';
        s_ppsExtSync_d2 <= '0';
        s_xo2           <= '0';
      else
        s_ppsExtSync_d1 <= ppsIn;
        s_ppsExtSync_d2 <= s_ppsExtSync_d1;
        s_xo2           <= not s_xo2;   -- misc: Local XO (timeCLK/2)
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Control to Time clk domain: Select the 'mode' of ppsOut
  -----------------------------------------------------------------------------
  s_ppsOutMode_sD_IN <= s_rplTimeControl(1 downto 0);
  syncReg_ppsOutMode : SyncRegister
    generic map (
      width => 2,
      init  => 0)
    port map (
      sCLK   => CLK,
      dCLK   => timeCLK,
      sRST   => RST,
      sD_IN  => s_ppsOutMode_sD_IN,
      sEN    => s_ppsOutMode_sRDY,
      dD_OUT => s_ppsOutMode_dD_OUT,
      sRDY   => s_ppsOutMode_sRDY);

  mux_ppsOut : process(s_ppsOutMode_dD_OUT, s_ppsDrive, s_ppsExtSync_d2, s_xo2)
  begin
    case s_ppsOutMode_dD_OUT is
      when "00"   => ppsOut <= s_ppsDrive;      -- 90% HI, 10% LO (timeCLK count)
      when "01"   => ppsOut <= s_ppsExtSync_d2; -- ppsIn reg'ed in timeCLK domain
      when "10"   => ppsOut <= s_xo2;           -- Local XO (timeCLK/2)
      when others => null;
    end case;
  end process;

  -----------------------------------------------------------------------------
  -- Control to Time clk domain: Control monitoring of External PPS sync pulse
  -----------------------------------------------------------------------------
  s_ppsDisablePPS_sD_IN_slv0 <= (0 downto 0 => s_rplTimeControl(2));
  syncReg_ppsDisablePPS : SyncRegister
    generic map (
      width => 1,
      init  => 0)
    port map (
      sCLK   => CLK,
      dCLK   => timeCLK,
      sRST   => RST,
      sD_IN  => s_ppsDisablePPS_sD_IN_slv0,
      sEN    => s_ppsDisablePPS_sRDY,
      dD_OUT => s_ppsDisablePPS_dD_OUT_slv0,
      sRDY   => s_ppsDisablePPS_sRDY);
  s_ppsDisablePPS_dD_OUT <= s_ppsDisablePPS_dD_OUT_slv0(0);

  -----------------------------------------------------------------------------
  -- Disable synchronization to External PPS sync pulse
  --
  -- TODO:
  -- QUESTION: This register could be driving out a '0' prior to being disabled.
  -- A '0' on this signal is a valid condition that makes most of the time crts
  -- continue to operate, in conjunction with a valid ppsIn (input pin).
  -- Thus far, all of my testing has been without a valid ppsIn, so I'm
  -- curious what will happen with it present, but ppsDisablePPS in the disabled
  -- state and the last registered state is '0'.
  -----------------------------------------------------------------------------
  reg_ppsExtSyncD : process(timeCLK)
  begin
    if(rising_edge(timeCLK)) then
      if (timeRST = '1') then
        s_ppsExtSyncD <= '0';
      else
        if (s_ppsDisablePPS_dD_OUT = '0') then
          s_ppsExtSyncD <= s_ppsExtSync_d2;
        end if;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Time to Control clk domain: ticksPerSecond
  -----------------------------------------------------------------------------
  s_refPerPPS_sD_IN <= s_refFreeSpan;
  s_refPerPPS_sEN <= '1' when (s_refPerPPS_sRDY = '1' and
                               s_ppsExtSync_d2 = '1'  and
                               s_ppsExtSyncD = '0') else '0';
  syncReg_refPerPPS : SyncRegister
    generic map (
      width => 28,
      init  => 0)
    port map (
      sCLK   => timeCLK,
      dCLK   => CLK,
      sRST   => timeRST,
      sD_IN  => s_refPerPPS_sD_IN,
      sEN    => s_refPerPPS_sEN,
      dD_OUT => s_refPerPPS_dD_OUT,
      sRDY   => s_refPerPPS_sRDY);

  -----------------------------------------------------------------------------
  -- Control to Time clk domain: Disable update of 'fracInc' value
  -----------------------------------------------------------------------------  
  s_disableServo_sD_IN_slv0 <= (0 downto 0 => s_rplTimeControl(4));
  syncReg_disableServo : SyncRegister
    generic map (
      width => 1,
      init  => 0)
    port map (
      sCLK   => CLK,
      dCLK   => timeCLK,
      sRST   => RST,
      sD_IN  => s_disableServo_sD_IN_slv0,
      sEN    => s_disableServo_sRDY,
      dD_OUT => s_disableServo_dD_OUT_slv0,
      sRDY   => s_disableServo_sRDY);
  s_disableServo_dD_OUT <= s_disableServo_dD_OUT_slv0(0);

  -----------------------------------------------------------------------------
  -- Time to Control clk domain: Seconds and Fraction
  -- The exact SAME Sec/Frac value as reported in the Time clock domain
  --
  -- TODO: Replace with appropriate CDC (bus-based) module
  -----------------------------------------------------------------------------
  s_nowInCC_sD_IN <= s_refSecCount & s_fracSeconds(47 downto 16);
  s_nowInCC_sRDY  <= '1';
  -- Purely sampling the time clock to cross the clock domain
  reg_nowInCC1 : process(timeCLK)
  begin
    if(rising_edge(timeCLK)) then
      s_nowInCC_sReg1 <= s_nowInCC_sD_IN;
    end if;
  end process;
  reg_nowInCC2 : process(CLK)
  begin
    if(rising_edge(CLK)) then
      s_nowInCC_dReg1  <= s_nowInCC_sReg1;
      s_nowInCC_dD_OUT <= s_nowInCC_dReg1;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Register Second and Fraction counts in the Time clock domain
  -- Same Sec/Frac value reported in the Control clock domain
  -----------------------------------------------------------------------------
  reg_nowTC : process(timeCLK)
  begin
    if(rising_edge(timeCLK)) then
      if (timeRST = '1') then
        s_nowTC <= (others => '0');
      else
        if (s_nowInCC_sRDY = '1') then
          s_nowTC <= s_refSecCount & s_fracSeconds(47 downto 16);
        end if;
      end if;
    end if;
  end process;
  
  -----------------------------------------------------------------------------
  -- Time to Control clk domain: For Status register and setting ppsLostSticky bit
  -----------------------------------------------------------------------------  
  s_ppsLostCC_sD_IN_slv0 <= (0 downto 0 => s_ppsLost);  
  syncReg_ppsLostCC : SyncRegister    
    generic map (
      width => 1,
      init  => 0)
    port map (
      sCLK   => timeCLK,
      dCLK   => CLK,
      sRST   => timeRST,
      sD_IN  => s_ppsLostCC_sD_IN_slv0,
      sEN    => s_ppsLostCC_sRDY,
      dD_OUT => s_ppsLostCC_dD_OUT_slv0,
      sRDY   => s_ppsLostCC_sRDY);
  s_ppsLostCC_dD_OUT <= s_ppsLostCC_dD_OUT_slv0(0);

  -----------------------------------------------------------------------------
  -- Time to Control clk domain: For Status register and setting ppsInSticky bit
  -----------------------------------------------------------------------------
  s_ppsOKCC_sD_IN_slv0 <= (0 downto 0 => s_ppsOK);    
  syncReg_ppsOKCC : SyncRegister    
    generic map (
      width => 1,
      init  => 0)
    port map (
      sCLK   => timeCLK,
      dCLK   => CLK,
      sRST   => timeRST,
      sD_IN  => s_ppsOKCC_sD_IN_slv0,
      sEN    => s_ppsOKCC_sRDY,
      dD_OUT => s_ppsOKCC_dD_OUT_slv0,
      sRDY   => s_ppsOKCC_sRDY);
  s_ppsOKCC_dD_OUT <= s_ppsOKCC_dD_OUT_slv0(0);

  -----------------------------------------------------------------------------
  -- Time to Control clk domain: Count # of ppsIn, via their leading edge.
  -----------------------------------------------------------------------------
  s_rollingPPSIn_sD_IN <= s_ppsEdgeCount;
  syncReg_rollingPPSIn : SyncRegister    
    generic map (
      width => 8,
      init  => 0)
    port map (
      sCLK   => timeCLK,
      dCLK   => CLK,
      sRST   => timeRST,
      sD_IN  => s_rollingPPSIn_sD_IN,
      sEN    => s_rollingPPSIn_sRDY,
      dD_OUT => s_rollingPPSIn_dD_OUT,
      sRDY   => s_rollingPPSIn_sRDY);

  -----------------------------------------------------------------------------
  -- Control to Time clk domain: Clock 's_timeIn' into the Time clk domain
  -----------------------------------------------------------------------------
  s_setRefF_sENQ <= timeNow_written;
  s_setRefF_dDEQ <= s_setRefF_dEMPTY_N;
  syncFifo_setRefF : SyncFIFO
    generic map (
      dataWidth => 64,
      depth  => 2,
      indxWidth => 1)
    port map (
      sCLK   => CLK,
      dCLK   => timeCLK,
      sRST   => RST_N,
      sD_IN  => s_timeIn,
      sENQ   => s_setRefF_sENQ,
      dDEQ   => s_setRefF_dDEQ,
      dD_OUT => s_setRefF_dD_OUT,
      sFULL_N => open,
      dEMPTY_N => s_setRefF_dEMPTY_N);

  -----------------------------------------------------------------------------
  -- Control clock domain: Update registers per host or other events
  -----------------------------------------------------------------------------
  reg_cClkStatusBits : process(CLK)
  begin
    if(rising_edge(CLK)) then
      if (RST = '1') then
        s_gpsInSticky    <= '0';
        s_ppsInSticky    <= '0';
        s_ppsLostSticky  <= '0';
        s_rplTimeControl <= (others => '0');
        s_timeSetSticky  <= '0';
      else
        if (s_doClear = '1') then s_gpsInSticky                                 <= '0'; end if;
        if (s_doClear = '1' or s_ppsOKCC_dD_OUT = '1') then s_ppsInSticky       <= s_ppsOKCC_dD_OUT; end if;
        if (s_doClear = '1' or s_ppsLostCC_dD_OUT = '1') then s_ppsLostSticky   <= s_ppsLostCC_dD_OUT; end if;
        if (s_doClear = '1' or timeControl_written = '1') then s_rplTimeControl <= s_timeIn(4 downto 0); end if;
        if (s_doClear = '1' or timeNow_written = '1') then s_timeSetSticky      <= not s_doClear; end if;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Reference count: # of cycles FROM last valid RISING edge of ppsIn pulse
  -----------------------------------------------------------------------------
  reg_refFromRise : process(timeCLK)
  begin
    if(rising_edge(timeCLK)) then
      if (timeRST = '1') then
        s_refFromRise <= (others => '0');
      else
        -- Count # of clk cycles upon detection of leading edge of ppsIn pulse
        if (s_ppsExtSync_d2 = '1' and s_ppsExtSyncD = '0') then
          s_refFromRise <= (others => '0');
        else
          s_refFromRise <= std_logic_vector(signed(s_refFromRise) + 1);
        end if;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  --
  -- TODO: LARGE BIT COMPARISONS, CONSIDER REGISTERING THESE SIGNALS
  --
  -- Flags to track the leading edge of an External PPS to the next
  -- leading edge. Tolerance is +/- 0.1%, based on Time Clock Domain
  --
  -- Lower Threshold of Detection Window = -0.1%
  s_refFromRise_lowerThresOfWindow <= '1' when (s_refFromRise <= c_lowerThresOfWindow) else '0';
  -- Upper Threshold of Detection Window = +0.1%
  s_refFromRise_upperThresOfWindow <= '1' when (s_refFromRise < c_upperThresOfWindow)  else '0';

  -----------------------------------------------------------------------------
  -- Flags to indicate that an External PPS is detected "In"/"Out" of expected window
  --
  -- External PPS pulse detected INSIDE of expected window
  s_ppsIn_detectedInWindow <= '1' when
                              (s_ppsExtSync_d2 = '1' and s_ppsExtSyncD = '0' and
                               s_refFromRise_lowerThresOfWindow = '0' and
                               s_refFromRise_upperThresOfWindow = '1') else '0';
  --
  -- External PPS pulse detected OUTSIDE of expected window
  s_ppsIn_detectedOutWindow <= '1' when
                               ((s_ppsExtSync_d2 = '1' and s_ppsExtSyncD = '0' and
                                 (s_refFromRise_lowerThresOfWindow = '1' or
                                  s_refFromRise_upperThresOfWindow = '0')) or
                                (s_refFromRise > c_upperThresOfWindow)) else '0';

  -----------------------------------------------------------------------------
  -- s_refFreeSampl - Free Running Counter (FRC), in the Time (fast) clock domain,
  -- that has no run-time reset.
  --
  -- s_refFreeSpan - # of clocks in 1 PPS Span
  --
  -- NOTE:
  -- 1) The Span value is not guaranteed to be from consecutive "External" PPS pulses.
  -- 2) On the host, the Sticky bits should be checked to ensure ppsOK active and
  --    ppsLost inactive before using the Span value.
  --
  -- The "s_refFreeSpan" value passes CDC into the Control clock domain
  -- and be made available to host via register "ticksPerSecond".
  -----------------------------------------------------------------------------
  reg_FreeRunningCounter : process(timeCLK)
  begin
    if(rising_edge(timeCLK)) then
      if (timeRST = '1') then
        s_refFreeCount <= (others => '0');
        s_refFreeSamp  <= (others => '0');
        s_refFreeSpan  <= (others => '0');
      else
        s_refFreeCount <= std_logic_vector(signed(s_refFreeCount) + 1);

        if (s_ppsIn_detectedInWindow = '1') then
          s_refFreeSamp <= s_refFreeCount;
          s_refFreeSpan <= std_logic_vector(signed(s_refFreeCount) - signed(s_refFreeSamp));
        end if;
      end if;
    end if;
  end process;
  
  -----------------------------------------------------------------------------
  -- Seconds:
  -- refSecCount : # of Seconds output by the time service
  -- refPerCount : # of cycles from start of PPS pulse (External or Internal/free-running)
  -- ppsDrive    : Active while count is < 0.9 of a second (90% HI, 10% LO)
  -----------------------------------------------------------------------------
  reg_Second : process(timeCLK)
  begin
    if(rising_edge(timeCLK)) then
      if (timeRST = '1') then
        s_refSecCount <= (others => '0');
        s_refPerCount <= (others => '0');
        s_ppsDrive    <= '0';
      else
        -- Set seconds counter if a value is provided by host,
        -- OR
        -- If External PPS present AND operating within expected valid range,
        -- OR
        -- working off of internal PPS circuit.
        if (s_setRefF_dEMPTY_N = '1' or s_refSecCount_EN = '1') then
          if (s_setRefF_dEMPTY_N = '1') then
            s_refSecCount <= s_setRefF_dD_OUT(63 downto 32);
          else
            s_refSecCount <= std_logic_vector(signed(s_refSecCount) + 1);
          end if;
        end if;

        -- Count # of clk cycles while External PPS is in valid operating range
        -- AND upon detection of leading edge of ppsIn,
        -- OR
        -- Working off of internal PPS circuit AND operating within expected valid range.
        if (s_refSecCount_EN = '1') then
          s_refPerCount <= (others => '0');
        else
          s_refPerCount <= std_logic_vector(signed(s_refPerCount) + 1);
        end if;

        -- Active while count is < 0.9 of a second (90% HI, 10% LO)
        if (s_refPerCount < c_refPerCountTH) then
          s_ppsDrive <= '1';
        else
          s_ppsDrive <= '0';
        end if;
      end if;
    end if;
  end process;

  -- Determine when the Seconds counter may increment:
  -- 1) Priority is given to the presense of a good External PPS.
  --    Wait for the next leading edge ppsIn pulse, then allow Seconds counter to increment.
  -- 2) Secondary is when running off of internal time keeping.
  --    Wait for s_fracSeconds to rollover, then allow Seconds counter to increment
  cmb_refSecCount_EN : process(s_ppsOK, s_ppsExtSync_d2, s_ppsExtSyncD, s_delSec, s_fracSeconds(49 downto 48))
  begin
    if (s_ppsOK = '1') then     -- External PPS is good
      if (s_ppsExtSync_d2 = '1' and s_ppsExtSyncD = '0') then -- lead_edge of External PPS
        s_refSecCount_EN <= '1';
      else
        s_refSecCount_EN <= '0';
      end if;
    else                        -- Free-running counter rolling over
      if (s_delSec /= s_fracSeconds(49 downto 48)) then
        s_refSecCount_EN <= '1';
      else
        s_refSecCount_EN <= '0';
      end if;
    end if;
  end process;
    
  -----------------------------------------------------------------------------
  -- Fractional Seconds with Phase-Locked Loop
  -- 
  -- "Filter" of the PLL
  --
  -- With each PPS, proportionally correct the fractonal increment by the
  -- measured error divided by the number of increments...
  -- Positive beta == reference is SLOW with respect to PPS => must 'speed up' cnt
  -- Negative beta == reference is FAST with respect to PPS => must 'slow down up' cnt
  --
  -- Kp: (gain) 2^28 ~= 200e6, thus our proportional response is slightly over-damped
  -- FixedPoint#(2,48) beta = ((1.0-delSecond)>>28);
  --
  -- Kp: (gain) 2^27 ~= 100e6, thus our proportional response is slightly over-damped
  -- FixedPoint#(2,48) beta = ((1.0-delSecond)>>27);
  --
  -- NOTE: MUST THE GAIN BE PROPORTIONAL TO THE FREQUENCY OF THE timeCLK???
  --
  --x_281474976710656_minus_delSecond_50b <=
  --  std_logic_vector("01" & x"0000_0000_0000" - signed(s_delSecond));

  --x_281474976710656_minus_delSecond_22b <=
  --  x_281474976710656_minus_delSecond_50b(49 downto 28);

  --s_fracBeta <= (49 downto 22 => x_281474976710656_minus_delSecond_22b(21)) &
  --         x_281474976710656_minus_delSecond_22b;

  --s_fracBeta <= (49 downto (s_fracBeta'length - natural(ceil(log2(real(g_TIMECLK_FREQ)))))
  --               => x_281474976710656_minus_delSecond_50b(49)) &
  --              x_281474976710656_minus_delSecond_50b(49 downto natural(ceil(log2(real(g_TIMECLK_FREQ)))));

  s_base2exp48_minus_delSecond_50b <=
    std_logic_vector("01" & x"0000_0000_0000" - signed(s_delSecond));

  s_fracBeta <= (49 downto (s_fracBeta'length - natural(ceil(log2(real(g_TIMECLK_FREQ)))))
                 => s_base2exp48_minus_delSecond_50b(49)) &
                s_base2exp48_minus_delSecond_50b(49 downto natural(ceil(log2(real(g_TIMECLK_FREQ)))));

  -----------------------------------------------------------------------------
  -- "Phase Locked Loop"
  -- Fractional Seconds (free running counter)
  -- Set ("Jam") by host or update the free running fractional second counter
  -- add applying beta adjustments. (2,48 format)
  reg_Fraction : process(timeCLK)
  begin
    if(rising_edge(timeCLK)) then
      if (timeRST = '1') then
        s_delSec      <= (others => '0');
        s_lastSecond  <= (others => '0');
        -- Force compensation to be 'zero' upon startup
        s_delSecond   <= "01" & x"0000_0000_0000";
        s_fracInc     <= c_fracInc;
        s_fracSeconds <= (others => '0');
        s_jamFrac_EN  <= '0';
        s_jamFrac_Val <= (others => '0');
      else
        
        s_delSec <= s_fracSeconds(49 downto 48);

        -- "Phase Detector"
        -- Fractional count delta between 'valid' PPS pulses.
        -- But this delta is NOT guaranteed to be between successive valid pulses.
        -- However, the adjustment feedback value is only applied when PPS is 'OK'.
        if (s_ppsIn_detectedInWindow = '1') then
          s_lastSecond <= s_fracSeconds;
          s_delSecond  <= std_logic_vector(signed(s_fracSeconds) - signed(s_lastSecond));
        end if;

        -- Apply "Filter" value
        -- Apply the proportional 'beta' compensation to the fracInc and
        -- subsequently, to the fracSecond accumulator
        if (s_ppsIn_detectedInWindow = '1' and s_ppsOK = '1' and s_disableServo_dD_OUT = '0') then
          s_fracInc <= std_logic_vector(signed(s_fracInc) + signed(s_fracBeta));
        end if;

        -- "VCO"
        -- Set ('Jam') by Host or update the free running fractional-second counter
        --
        -- NOTE:
        -- 1) This is the process where another device/sub worker which processes
        -- GPS packets to obtain accurate time, could set the 'time' of the HTS
        -- 2) Might the s_delSecond need to be reinitialized too?
        if (s_jamFrac_EN = '1') then    -- New frac value from host
          s_fracSeconds <= s_jamFrac_Val;
        else
          s_fracSeconds <= std_logic_vector(signed(s_fracSeconds) + signed(s_fracInc));
        end if;

        -- If PPS is not valid, (due to loss or PPS tracking is disabled)
        -- allow host to "Jam" a new fraction value into the circuit.
        -- 
        -- NOTE:
        -- An application (ACI) may be required to force the HTS to stop tracking
        -- External PPS, to cause s_ppsOK=0, and then set the s_jamFrac_Val.
        -- (Also, useful then GPS is suspected to be maliciously spoofed)
        if (s_setRefF_dEMPTY_N = '1' and s_ppsOK = '0') then
          s_jamFrac_EN  <= '1';
          s_jamFrac_Val <= ("00" & s_setRefF_dD_OUT(31 downto 0) & x"0000");
        else
          s_jamFrac_EN  <= '0';
          s_jamFrac_Val <= (others => '0');
        end if;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- For External PPS ONLY:
  -- A rolling count of leading edge detections "from the beginning of time".
  -- PPS 'Lost' flag.
  -- PPS 'OK' flag.
  -----------------------------------------------------------------------------
  reg_ppsOKnLost : process(timeCLK)
  begin
    if(rising_edge(timeCLK)) then
      if (timeRST = '1') then
        s_ppsEdgeCount <= (others => '0');
        s_ppsLost      <= '0';
        s_ppsOK        <= '0';
      else
        -- Status Only: count # of ppsIn pulses since last reset (just rollover)
        if (s_ppsExtSync_d2 = '1' and s_ppsExtSyncD = '0') then
          s_ppsEdgeCount <= std_logic_vector(signed(s_ppsEdgeCount) + 1);
        end if;

        -- "Lost" when PPS was valid, but the next leading edge was not
        -- detected within the expected detection window
        if (s_ppsOK = '1' and s_ppsIn_detectedOutWindow = '1') then
          s_ppsLost <= '1';
        else
          s_ppsLost <= '0';
        end if;

        -- "OK" when PPS is detected within the expected detection window, or
        -- was already "OK" and has not been "Lost", yet.
        if (s_ppsIn_detectedInWindow = '1' or (s_ppsOK = '1' and s_ppsLost = '0')) then
          s_ppsOK <= '1';
        else
          s_ppsOK <= '0';
        end if;
        
      end if;
    end if;
  end process;
  
end rtl;
