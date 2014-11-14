-- THIS FILE WAS ORIGINALLY GENERATED ON Sat Dec 28 17:15:49 2013 EST
-- BASED ON THE FILE: zed.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: zed

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library platform; use platform.platform_pkg.all;
library zynq; use zynq.zynq_pkg.all;
library unisim; use unisim.vcomponents.all;
library bsv;
architecture rtl of zed_worker is
  signal ps_axi_gp_in  : m_axi_gp_in_t;        -- s2m
  signal ps_axi_gp_out : m_axi_gp_out_t;       -- m2s
  signal ps_axi_hp_in  : s_axi_hp_in_array_t(0 to C_S_AXI_HP_COUNT-1);  -- m2s
  signal ps_axi_hp_out : s_axi_hp_out_array_t(0 to C_S_AXI_HP_COUNT-1); -- s2m
  signal my_cp_out     : occp_in_t;
  signal fclk          : std_logic_vector(3 downto 0);
  signal clk           : std_logic;
  signal raw_rst_n     : std_logic; -- FCLKRESET_Ns need synchronization
  signal rst_n         : std_logic; -- the synchronized negative reset
  signal reset         : std_logic; -- our positive reset
  signal count         : unsigned(25 downto 0);
  signal seen_burst    : std_logic;
  signal unoc_count_in    : unsigned(3 downto 0);
  signal unoc_header_in   : ulonglong_array_t(0 to 15);
  signal unoc_header_in1  : ulonglong_array_t(0 to 15);
  signal unoc_count_out    : unsigned(3 downto 0);
  signal unoc_header_out   : ulonglong_array_t(0 to 15);
  signal unoc_header_out1  : ulonglong_array_t(0 to 15);
  signal unoc_out_status   : ulong_array_t(0 to 15);
  signal axi_wdcount    : unsigned(3 downto 0);
  signal axi_rdcount    : unsigned(3 downto 0);
  signal axi_wacount    : unsigned(3 downto 0);
  signal axi_racount    : unsigned(3 downto 0);
  signal axi_rdata     : ulonglong_array_t(0 to 15);
  signal axi_raddr     : ulonglong_array_t(0 to 15);
  signal axi_wdata     : ulonglong_array_t(0 to 15);
  signal axi_waddr     : ulonglong_array_t(0 to 15);
  signal my_zynq_out   : platform.platform_pkg.unoc_master_out_t;
  signal dbg_state     : ulonglong_t;
  signal dbg_state1    : ulonglong_t;
  signal dbg_state2    : ulonglong_t;
begin
  g0: if its(ocpi_debug) generate
    -- If we don't assign the outputs, the "debug overhead" will disappear
    props_out.axi_waddr <= axi_waddr;
    props_out.axi_raddr <= axi_raddr;
    props_out.axi_wdata <= axi_wdata;
    props_out.axi_rdata <= axi_rdata;
    props_out.axi_racount <= resize(axi_racount,32);
    props_out.axi_wacount <= resize(axi_wacount,32);
    props_out.axi_rdcount <= resize(axi_rdcount,32);
    props_out.axi_wdcount <= resize(axi_wdcount,32);
    props_out.unoc_count_in <= resize(unoc_count_in,32);
    props_out.unoc_headers_in <= unoc_header_in;
    props_out.unoc_headers_in1 <= unoc_header_in1;
    props_out.unoc_count_out <= resize(unoc_count_out,32);
    props_out.unoc_headers_out <= unoc_header_out;
    props_out.unoc_headers_out1 <= unoc_header_out1;
  end generate g0;
  cp_out <= my_cp_out;
  clkbuf   : BUFG   port map(I => fclk(3),
                             O => clk);
  -- The FCLKRESET signals from the PS are documented as asynchronous with the
  -- associated FCLK for whatever reason.  Here we make a synchronized reset from it.
  sr : bsv.bsv.SyncResetA
    generic map(RSTDELAY => 17)

    port map(IN_RST  => raw_rst_n,
             CLK     => clk,
             OUT_RST => rst_n);
  reset <= not rst_n;
  -- Instantiate the processor system (i.e. the interface to it).
  ps : zynq_ps
    port map(
      -- Signals from the PS used in the PL
      ps_out.FCLK           => fclk,
      ps_out.FCLKRESET_N    => raw_rst_n,
      m_axi_gp_in           => ps_axi_gp_in,
      m_axi_gp_out          => ps_axi_gp_out,
      s_axi_hp_in           => ps_axi_hp_in,
      s_axi_hp_out          => ps_axi_hp_out
      );
  -- Adapt the axi master from the PS to be a CP Master
  cp : axi2cp
    port map(
      clk     => clk,
      reset   => reset,
      axi_in  => ps_axi_gp_out,
      axi_out => ps_axi_gp_in,
      cp_in   => cp_in,
      cp_out  => my_cp_out
      );
  zynq_out <= my_zynq_out;
  props_out.debug_state <= dbg_state;
  props_out.debug_state1 <= dbg_state1;
  dp0 : unoc2axi
    generic map(
      ocpi_debug => true
      )
    port map(
      clk       => clk,
      reset     => reset,
      unoc_in   => zynq_in,
      unoc_out  => my_zynq_out,
      axi_in    => ps_axi_hp_out(0),
      axi_out   => ps_axi_hp_in(0),
      axi_error => props_out.axi_error,
      dbg_state => dbg_state,
      dbg_state1 => dbg_state1,
      dbg_state2 => dbg_state2
      );
  dp1 : axinull
    port map(
      clk       => clk,
      reset     => reset,
      axi_in    => ps_axi_hp_out(1),
      axi_out   => ps_axi_hp_in(1)
      );
  dp2 : axinull
    port map(
      clk       => clk,
      reset     => reset,
      axi_in    => ps_axi_hp_out(2),
      axi_out   => ps_axi_hp_in(2)
      );
  dp3 : axinull
    port map(
      clk       => clk,
      reset     => reset,
      axi_in    => ps_axi_hp_out(3),
      axi_out   => ps_axi_hp_in(3)
      );

  term_unoc : unoc_terminator
    port    map(up_in      => zynq_slave_in,
                up_out     => zynq_slave_out,
                drop_count => props_out.unocDropCount);
  
  -- This piece of generic infrastructure in is instantiated here because
  -- it localizes all these signals here in the platform worker, and thus
  -- the platform worker simply produces clock, reset, and time, all in the
  -- clock domain of the timekeepping clock.
  ts : time_server
    port map(
      CLK                 => clk,
      RST_N               => rst_n,
      timeCLK             => clk,
      timeRST_N           => rst_n,

      timeControl         => props_in.timeControl,
      timeControl_written => props_in.timeControl_written,
      timeStatus          => props_out.timeStatus,
      timeNowIn           => props_in.timeNow,
      timeNow_written     => props_in.timeNow_written,
      timeNowOut          => props_out.timeNow,
      timeDeltaIn         => props_in.timeDelta,
      timeDelta_written   => props_in.timeDelta_written,
      timeDeltaOut        => props_out.timeDelta,
      ticksPerSecond      => props_out.ticksPerSecond,
      
      -- PPS interface
      ppsIn               => '0',
      ppsOut              => open,

      -- Time service output
      time_service        => time_out
      );
  -- Output/readable properties
  props_out.platform        <= to_string("zed", props_out.platform'length-1);
  props_out.dna             <= (others => '0');
  props_out.nSwitches       <= (others => '0');
  props_out.switches        <= (others => '0');
  props_out.memories_length <= to_ulong(1);
  props_out.memories        <= (others => to_ulong(0));
  props_out.nLEDs           <= to_ulong(0); --led'length);
  props_out.UUID            <= metadata_in.UUID;
  props_out.romData         <= metadata_in.romData;
  -- props_out.pciId          <= ushort_t(unsigned(pci_id));
  -- Settable properties - drive the leds that are not driven by hardware from the property
  -- led(6 downto 1)           <= std_logic_vector(props_in.leds(6 downto 1));
  -- led(led'left downto 8)    <= (others => '0');
  -- Drive metadata interface
  metadata_out.clk          <= clk;
  metadata_out.romAddr      <= props_in.romAddr;
  metadata_out.romEn        <= props_in.romData_read;
  led(0) <= count(count'left);
  led(1) <= ps_axi_gp_out.ARVALID;
  led(2) <= seen_burst; -- my_cp_out.is_read;
  led(3) <= cp_in.take;

  led(4) <= cp_in.valid;
  led(5) <= ps_axi_gp_in.ARREADY;
  led(6) <= ps_axi_gp_in.RVALID;
  led(7) <= ps_axi_gp_out.RREADY;
  work : process(clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        count <= (others => '0');
        unoc_count_in <= (others => '0');
        unoc_count_out <= (others => '0');
        axi_raddr  <= (others => (others => '0'));
        axi_waddr  <= (others => (others => '0'));
        axi_rdata  <= (others => (others => '0'));
        axi_wdata  <= (others => (others => '0'));
        axi_wdcount <= (others => '0');
        axi_rdcount <= (others => '0');
        axi_wacount <= (others => '0');
        axi_racount <= (others => '0');
        seen_burst <= '0';
      else
        if its(my_zynq_out.valid) and zynq_in.take and
          unoc_count_out /= 15 then
          unoc_header_out(to_integer(unoc_count_out)) <=
            to_ulonglong(my_zynq_out.data.payload(0) & my_zynq_out.data.payload(1));
          unoc_header_out1(to_integer(unoc_count_out)) <=
            to_ulonglong(my_zynq_out.data.payload(2) & my_zynq_out.data.payload(3));
          unoc_out_status(to_integer(unoc_count_out)) <=
            dbg_state2(31 downto 0);
          unoc_count_out <= unoc_count_out + 1;
        end if;
        if its(zynq_in.valid) and my_zynq_out.take and
          unoc_count_in /= 15 then
          unoc_header_in(to_integer(unoc_count_in)) <=
            to_ulonglong(zynq_in.data.payload(0) & zynq_in.data.payload(1));
          unoc_header_in1(to_integer(unoc_count_in)) <=
            to_ulonglong(zynq_in.data.payload(2) & zynq_in.data.payload(3));
          unoc_count_in <= unoc_count_in + 1;
        end if;
        if its(ps_axi_hp_out(0).R.VALID) and ps_axi_hp_in(0).R.READY and axi_rdcount /= 15 then
          axi_rdata(to_integer(axi_rdcount)) <=
            to_ulonglong(ps_axi_hp_out(0).R.DATA(63 downto 0)); -- &
--                         "00010010001101000101011001110000");
          axi_rdcount <= axi_rdcount + 1;
        end if;
        if its(ps_axi_hp_in(0).AR.VALID and ps_axi_hp_out(0).AR.READY) and axi_racount /= 15 then
          axi_raddr(to_integer(axi_racount)) <=
            to_ulonglong(std_logic_vector(dbg_state1(60 downto 56)) & -- 5
                         std_logic_vector(dbg_state(26 downto 4)) & -- 23
                         ps_axi_hp_in(0).AR.LEN & -- 4
                         ps_axi_hp_in(0).AR.ADDR); -- 32
          axi_racount <= axi_racount + 1;
        end if;
        if its(ps_axi_hp_in(0).W.VALID) and ps_axi_hp_out(0).W.READY and axi_wdcount /= 15 then
          axi_wdata(to_integer(axi_wdcount)) <=
            to_ulonglong(
              "000000" & std_logic_vector(count) &
--                         std_logic_vector(dbg_state1(51 downto 32)) &
--                         "000" & ps_axi_hp_in(0).WLAST & -- 1
--                         ps_axi_hp_in(0).WSTRB & -- 8
              ps_axi_hp_in(0).W.DATA(31 downto 0)); -- 32
--                         "00010010001101000101011001110000");
          axi_wdcount <= axi_wdcount + 1;
        end if;
        if its(ps_axi_hp_in(0).AW.VALID and ps_axi_hp_out(0).AW.READY) and axi_wacount /= 15 then
          axi_waddr(to_integer(axi_wacount)) <=
            to_ulonglong(
--              std_logic_vector(dbg_state(27 downto 4)) & -- 24
--                                          "0" & ps_axi_hp_in(0).AWSIZE & -- 4
--                                          ps_axi_hp_in(0).AWLEN & -- 4
              "000000" & std_logic_vector(count) &
              ps_axi_hp_in(0).AW.ADDR); -- 32
          axi_wacount <= axi_wacount + 1;
        end if;
        count <= count + 1;
        if ps_axi_gp_out.ARVALID = '1' and ps_axi_gp_out.ARLEN = "0001" then
          seen_burst <= '1';
        end if;
      end if;
    end if;
  end process;
end rtl;
