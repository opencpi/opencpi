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

-- This time service is the "inner" block used to create the time server worker.
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi, util, bsv; use ocpi.all, ocpi.types.all, util.util.all, bsv.bsv.SyncFIFO;
library platform; use platform.platform_pkg.all;
entity time_service is
  generic (
    -- ML605=200e6, ALST4=100e6, Zynq=50e6  
    g_TIMECLK_FREQ     : integer := 200e6);
  port (
    CLK                 : in  std_logic;
    RST                 : in  std_logic;
    timeCLK             : in  std_logic;
    timeRST             : in  std_logic;
    ppsIn               : in  std_logic;  -- ASYNC
    -- Property interface
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
    -- Outputs in tiock clock domain
    ppsOut              : out std_logic;
    time_service        : out time_service_t  -- time service clock domain
    );
end entity time_service;

architecture rtl of time_service is
  -----------------------------------------------------------------------------
  -- Platform:  TC Freq   TC-10%        TC+0.1%         TC-0.1%         FracInc
  --            (MHz)     (MHz)         (MHz)           (MHz)           (MHz)
  --
  -- Simulation 100        90           100.1           99.9            2621541
  -- ML605      200        180          200.2           199.8           1407374
  -- ALST4      100        90           100.1           99.9            2621541
  -- Zynq       50         4            50.05           49.95           5243082
  
  -----------------------------------------------------------------------------
  -- Used to generate ppsDrive (a possible source/mode for ppsOut)
  -- (defines 90% of a second counter @ TimeCLK rate)
  -- ML605 200M-(200M*10%) = 180M = x"ABA_9500"
  -- Simulation & ALST4 100M-(100M*10%) = 90M = x"55D_4A80"
  -- Zynq-based (currently fmax limited to 50MHz) 50-(50M*10%) = 45M =x"2AE_A540"
  constant c_refPerCountTH : std_logic_vector(27 downto 0)
    := std_logic_vector(to_unsigned(g_TIMECLK_FREQ-(g_TIMECLK_FREQ)/10, 28));
  -----------------------------------------------------------------------------
  -- Upper Threshold of Window for detecting External PPS pulse:
  -- (Counter @ TimeCLK after one second + 0.1%)
  -- ML605 200M+0.1% = 200200000 = x"BEE_CF40"
  -- Simulation & ALST4 100M+0.1% = 100100000 = x"5F7_67A0"
  -- Zynq-based (currently fmax limited to 50MHz) 50M+0.1% = 50050000 = x"2FB_B3D0"
  constant c_upperThresOfWindow : std_logic_vector(27 downto 0)
    := std_logic_vector(to_unsigned(g_TIMECLK_FREQ + (g_TIMECLK_FREQ)/1000, 28));
  -----------------------------------------------------------------------------
  -- Lower Threshold of Window for detecting External PPS pulse:
  -- (Counter @ TimeCLK after one second - 0.1%)
  -- ML605 200M-0.1% = 199800000 = x"BE8_B4C0"
  -- Simulation & ALST4 100M-0.1% = 99900000 = x"5F4_5A60"
  -- Zynq implementations (currently fmax limited to 50MHz) 50M-0.1% = 49950000= x"2FA_2D30"
  constant c_lowerThresOfWindow : std_logic_vector(27 downto 0)
    := std_logic_vector(to_unsigned(g_TIMECLK_FREQ - (g_TIMECLK_FREQ)/1000, 28));
  -----------------------------------------------------------------------------
  -- Fraction Increment Value:
  -- (fracPrecision {units=ns} * 2^(fracInc Vector Length)) / Time Clock Rate
  --
  -- Fraction Precision: Ideal=0.25ns, Actual 1 sec/2^32=23.283964ns
  --
  -- Time Clock Rate:   Platforms
  -- 200MHz:            ML605
  -- 100MHz:            Simulations & ALST4
  -- 50MHz:             Zynq-based (currently fmax limited to 50MHz)
  --
  -- TODO:
  -- The BSV code used the ideal 0.25ns for calculations,
  -- but should the acutal 23.283964ns be used instead?
  --
  -- (0.25 * 2^50)/(Time Clock Rate)
  -- (281474976710656)/(Time Clock Rate MHz)
  -- (281474977)/((Time Clock Rate MHz)/100M)
  -- (0.25 * 2^50)/(200MHz)= 1407374 = x15798E
  -- (0.25 * 2^50)/(100MHz)= 2814749 = x2AF31D
  -- (0.25 * 2^50)/(50MHz) = 5629499 = x55E63B
  --
  -- (0.23283964 * 2^50)/(Time Clock Rate)
  -- (262154128985270)/(Time Clock Rate MHz)
  -- (262154129)/((Time Clock Rate MHz)/100M)
  -- (0.23283964 * 2^50)/(200MHz) = 1310770 = x14_0032
  -- (0.23283964 * 2^50)/(100MHz) = 2621541 = x28_0064
  -- (0.23283964 * 2^50)/(50MHz) = 5243082 = x50_00C8
  --
  constant c_fracInc : std_logic_vector(49 downto 0)
    := std_logic_vector(to_unsigned(281474977/(g_TIMECLK_FREQ/1000000), 50));
  -------------------------------------------------------------------------------

  signal s_doClear   : std_logic;
  signal s_timeIn    : std_logic_vector(63 downto 0);
  signal s_statusOut : std_logic_vector(31 downto 0);

  signal s_disableServo_dD_OUT      : std_logic;
  signal s_disableServo_dD_OUT_slv0 : std_logic_vector(0 downto 0);
  signal s_disableServo_sD_IN_slv0  : std_logic_vector(0 downto 0);
  signal s_disableServo_sRDY        : std_logic;

  signal s_nowInCC_dD_OUT : std_logic_vector(63 downto 0);
  signal s_nowInCC_sD_IN  : std_logic_vector(63 downto 0);
  signal s_nowInCC_sRDY   : std_logic;

  signal s_nowInCC_sReg1  : std_logic_vector(63 downto 0);
  signal s_nowInCC_dReg1  : std_logic_vector(63 downto 0);

  signal s_nowTC : std_logic_vector(63 downto 0);

  signal s_ppsDisablePPS_dD_OUT      : std_logic;
  signal s_ppsDisablePPS_dD_OUT_slv0 : std_logic_vector(0 downto 0);
  signal s_ppsDisablePPS_sD_IN_slv0  : std_logic_vector(0 downto 0);
  signal s_ppsDisablePPS_sRDY        : std_logic;

  signal s_ppsLostCC_dD_OUT      : std_logic;
  signal s_ppsLostCC_dD_OUT_slv0 : std_logic_vector(0 downto 0);
  signal s_ppsLostCC_sD_IN_slv0  : std_logic_vector(0 downto 0);
  signal s_ppsLostCC_sRDY        : std_logic;

  signal s_ppsOKCC_dD_OUT      : std_logic;
  signal s_ppsOKCC_dD_OUT_slv0 : std_logic_vector(0 downto 0);
  signal s_ppsOKCC_sD_IN_slv0  : std_logic_vector(0 downto 0);
  signal s_ppsOKCC_sRDY        : std_logic;

  signal s_ppsOutMode_dD_OUT : std_logic_vector(1 downto 0);
  signal s_ppsOutMode_sD_IN  : std_logic_vector(1 downto 0);
  signal s_ppsOutMode_sRDY   : std_logic;

  signal s_refPerPPS_dD_OUT : std_logic_vector(27 downto 0);
  signal s_refPerPPS_sD_IN  : std_logic_vector(27 downto 0);
  signal s_refPerPPS_sEN    : std_logic;
  signal s_refPerPPS_sRDY   : std_logic;

  signal s_rollingPPSIn_dD_OUT : std_logic_vector(7 downto 0);
  signal s_rollingPPSIn_sD_IN  : std_logic_vector(7 downto 0);
  signal s_rollingPPSIn_sRDY   : std_logic;

  signal s_setRefF_dD_OUT   : std_logic_vector(63 downto 0);
  signal s_setRefF_sENQ     : std_logic;
  signal s_setRefF_dDEQ     : std_logic;
  signal s_setRefF_dEMPTY_N : std_logic;

  signal s_gpsInSticky    : std_logic;
  signal s_ppsInSticky    : std_logic;
  signal s_ppsLostSticky  : std_logic;
  signal s_rplTimeControl : std_logic_vector(4 downto 0);
  signal s_timeSetSticky  : std_logic;

  signal s_ppsExtSync_d1 : std_logic;
  signal s_ppsExtSync_d2 : std_logic;
  signal s_ppsExtSyncD   : std_logic;
  signal s_xo2           : std_logic;

  signal s_refFromRise                    : std_logic_vector(27 downto 0);
  signal s_refFromRise_lowerThresOfWindow : std_logic;
  signal s_refFromRise_upperThresOfWindow : std_logic;
  signal s_ppsIn_detectedInWindow         : std_logic;
  signal s_ppsIn_detectedOutWindow        : std_logic;

  signal s_refFreeCount : std_logic_vector(27 downto 0);
  signal s_refFreeSamp  : std_logic_vector(27 downto 0);
  signal s_refFreeSpan  : std_logic_vector(27 downto 0);

  signal s_refSecCount_EN : std_logic;
  signal s_refSecCount    : std_logic_vector(31 downto 0);
  signal s_refPerCount    : std_logic_vector(31 downto 0);
  signal s_ppsDrive       : std_logic;

  signal x_281474976710656_minus_delSecond_50b : std_logic_vector(49 downto 0);
  signal x_281474976710656_minus_delSecond_22b : std_logic_vector(21 downto 0);
  signal s_tmp                                 : std_logic_vector(49 downto 0);
  signal s_delSec                              : std_logic_vector(1 downto 0);
  signal s_delSecond                           : std_logic_vector(49 downto 0);
  signal s_lastSecond                          : std_logic_vector(49 downto 0);
  signal s_fracInc                             : std_logic_vector(49 downto 0);
  signal s_jamFrac_EN                          : std_logic;
  signal s_jamFrac_Val                         : std_logic_vector(49 downto 0);
  signal s_fracSeconds                         : std_logic_vector(49 downto 0);

  signal s_ppsEdgeCount : std_logic_vector(7 downto 0);
  signal s_ppsLost      : std_logic;
  signal s_ppsOK        : std_logic;
  signal RST_N          : std_logic;
  signal timeRST_N      : std_logic;
begin
  -- For older SyncFIFO modules until they get fixed
  RST_N <= not RST;
  timeRST_N <= not timeRST;
  -----------------------------------------------------------------------------
  -- Outputs assignments
  -----------------------------------------------------------------------------
  -- Control Clock Domain  
  s_statusOut <= (s_ppsLostSticky & s_gpsInSticky & s_ppsInSticky & s_timeSetSticky
                  & s_ppsOKCC_dD_OUT & s_ppsLostCC_dD_OUT & "00" & x"0000" & s_rollingPPSIn_dD_OUT);
  timeStatus <= ulong_t(s_statusOut);

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
  -- Async to Time clk domain: Double register the EXTERNAL PPS sync pulse input
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
        s_xo2           <= not s_xo2;   -- misc: carried over from generated BSV
                                        -- timeCLK/2        
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
      when "00"   => ppsOut <= s_ppsDrive; -- '1' while cnt < 10% of TimeCLK count
      when "01"   => ppsOut <= s_ppsExtSync_d2;  -- ppsIn reg'ed in TimeCLK domain
      when "10"   => ppsOut <= s_xo2;   -- toggle @ TimeCLK/2
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
  -- continue to operate, in conjuction with a valid ppsIn (input pin).
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
  s_refPerPPS_sEN <= '1' when (s_refPerPPS_sRDY = '1'
                               and s_ppsExtSync_d2 = '1'
                               and s_ppsExtSyncD = '0') else '0';
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
  -- Time to Control clk domain: Seconds & Fraction
  -- The exact SAME Sec/Frac value as reported in the Time clock domain
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
  -- Register Second & Fraction counts in the Time clock domain
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
  -- Reference count: # of cycles FROM last valid RISING edge of ppsIn pulse.
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

  -- Flags to mark the 'good' window for the next expected ppsIn pulse
  -- (Time Clock count +/- 0.1%)
  
  -- TODO: LARGE BIT COMPARISONS, CONSIDER REGISTERING THESE SIGNALS
  s_refFromRise_lowerThresOfWindow <= '1' when (s_refFromRise <= c_lowerThresOfWindow) else '0';
  s_refFromRise_upperThresOfWindow <= '1' when (s_refFromRise < c_upperThresOfWindow)  else '0';

  -- External PPS pulse detected INSIDE of expected window
  s_ppsIn_detectedInWindow <= '1' when (s_ppsExtSync_d2 = '1' and s_ppsExtSyncD = '0'
                                        and s_refFromRise_lowerThresOfWindow = '0'
                                        and s_refFromRise_upperThresOfWindow = '1') else '0';

  -- External PPS pulse detected OUTSIDE of expected window
  s_ppsIn_detectedOutWindow <= '1' when ((s_ppsExtSync_d2 = '1' and s_ppsExtSyncD = '0'
                                          and (s_refFromRise_lowerThresOfWindow = '1'
                                               or s_refFromRise_upperThresOfWindow = '0'))
                                         or (s_refFromRise > c_upperThresOfWindow)) else '0';

  -----------------------------------------------------------------------------
  -- A Free Running Counter (FRC), @ the Time (fast) clock domain, is used to
  -- calculate the Span between detected External PPS sync pulses, which are within
  -- the expected window of operation.
  -- NOTE:
  -- - The Span value is not guaranteed to be from consecutive External PPS pulses.
  -- - On host, may need to check Sticky bits, & ensure ppsOK active and
  --   ppsLost inactive before using the Span value.
  -- The Span value will cross-thru a syncReg into the Control clock domain
  -- and made available to host via register 'ticksPerSecond'.
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
  -- refSecCount: # of Seconds output by the time service.
  -- refPerCount: # of cycles from start of PPS pulse (external or internal)
  -- ppsDrive: active for 0.9 of a second
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
        -- if external PPS present AND operating within expected valid range,
        -- OR
        -- working off of internal PPS circuit.
        if (s_setRefF_dEMPTY_N = '1' or s_refSecCount_EN = '1') then
          if (s_setRefF_dEMPTY_N = '1') then
            s_refSecCount <= s_setRefF_dD_OUT(63 downto 32);
          else
            s_refSecCount <= std_logic_vector(signed(s_refSecCount) + 1);
          end if;
        end if;

        -- Count # of clk cycles while PPS is in valid operating range
        -- & upon detection of leading edge of ppsIn
        -- OR working off of internal PPS circuit AND
        -- operating within expected valid range.
        if (s_refSecCount_EN = '1') then
          s_refPerCount <= (others => '0');
        else
          s_refPerCount <= std_logic_vector(signed(s_refPerCount) + 1);
        end if;

        if (s_refPerCount < c_refPerCountTH) then
          s_ppsDrive <= '1';
        else
          s_ppsDrive <= '0';
        end if;
      end if;
    end if;
  end process;

  cmb_refSecCount_EN : process(s_ppsOK, s_ppsExtSync_d2, s_ppsExtSyncD, s_delSec, s_fracSeconds(49 downto 48))
  begin
    if (s_ppsOK = '1') then
      -- if external PPS is present & good, wait for next valid ppsIn pulse (leading edge),
      -- then allow Seconds counter to increment
      if (s_ppsExtSync_d2 = '1' and s_ppsExtSyncD = '0') then
        s_refSecCount_EN <= '1';
      else
        s_refSecCount_EN <= '0';
      end if;
    else
      -- wait for s_fracSeconds to rollover, then allow Seconds counter to increment
      if (s_delSec /= s_fracSeconds(49 downto 48)) then
        s_refSecCount_EN <= '1';
      else
        s_refSecCount_EN <= '0';
      end if;
    end if;
  end process;
    
  -----------------------------------------------------------------------------
  x_281474976710656_minus_delSecond_50b <= std_logic_vector("01" & x"0000_0000_0000"
                                                            - signed(s_delSecond));
  x_281474976710656_minus_delSecond_22b <= x_281474976710656_minus_delSecond_50b(49 downto 28);
  s_tmp <= (49 downto 22 => x_281474976710656_minus_delSecond_22b(21)) & x_281474976710656_minus_delSecond_22b;

  -----------------------------------------------------------------------------
  -- Fraction: calculate or set by host
  -----------------------------------------------------------------------------
  reg_Fraction : process(timeCLK)
  begin
    if(rising_edge(timeCLK)) then
      if (timeRST = '1') then
        s_delSec      <= (others => '0');
        s_lastSecond  <= (others => '0');
        s_delSecond   <= "01" & x"0000_0000_0000";
        s_fracInc     <= c_fracInc;
        s_fracSeconds <= (others => '0');
        s_jamFrac_EN  <= '0';
        s_jamFrac_Val <= (others => '0');
      else
        s_delSec <= s_fracSeconds(49 downto 48);

        if (s_ppsIn_detectedInWindow = '1') then
          s_lastSecond <= s_fracSeconds;
          s_delSecond  <= std_logic_vector(signed(s_fracSeconds) - signed(s_lastSecond));
        end if;

        if (s_ppsIn_detectedInWindow = '1' and s_ppsOK = '1' and s_disableServo_dD_OUT = '0') then
          s_fracInc <= std_logic_vector(signed(s_fracInc) + signed(s_tmp));
        end if;

        if (s_jamFrac_EN = '1') then    -- new frac value from host
          s_fracSeconds <= s_jamFrac_Val;
        else
          s_fracSeconds <= std_logic_vector(signed(s_fracSeconds) + signed(s_fracInc));
        end if;

        -- If PPS is not valid, allow host to "Jam" a new fraction value into the circuit.
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
  -- Misc: A rolling count of PPS pulses detected, and PPS OK & Lost flags
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

        -- If PPS valid but external PPS Sync has not been detected in window
        if (s_ppsOK = '1' and s_ppsIn_detectedOutWindow = '1') then
          s_ppsLost <= '1';
        else
          s_ppsLost <= '0';
        end if;

        if (s_ppsIn_detectedInWindow = '1' or (s_ppsOK = '1' and s_ppsLost = '0')) then
          s_ppsOK <= '1';
        else
          s_ppsOK <= '0';
        end if;
        
      end if;
    end if;
  end process;
  
end rtl;
