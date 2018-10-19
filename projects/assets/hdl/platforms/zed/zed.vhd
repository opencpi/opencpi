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

-- THIS FILE WAS ORIGINALLY GENERATED ON Sat Dec 28 17:15:49 2013 EST
-- BASED ON THE FILE: zed.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: zed

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library platform; use platform.platform_pkg.all;
library zynq; use zynq.zynq_pkg.all;
library axi; use axi.axi_pkg.all;
library unisim; use unisim.vcomponents.all;
library bsv;
library sdp; use sdp.sdp.all, sdp.sdp_axi.all;
architecture rtl of zed_worker is
  constant whichGP : natural := to_integer(unsigned(from_bool(useGP1)));
  signal ps_m_axi_gp_in  : m_axi_gp_in_array_t(0 to C_M_AXI_GP_COUNT-1);  -- s2m
  signal ps_m_axi_gp_out : m_axi_gp_out_array_t(0 to C_M_AXI_GP_COUNT-1); -- m2s
  signal ps_s_axi_gp_in  : s_axi_gp_in_array_t(0 to C_S_AXI_GP_COUNT-1);  -- m2s
  signal ps_s_axi_gp_out : s_axi_gp_out_array_t(0 to C_S_AXI_GP_COUNT-1); -- s2m
  signal ps_axi_hp_in  : s_axi_hp_in_array_t(0 to C_S_AXI_HP_COUNT-1);    -- m2s
  signal ps_axi_hp_out : s_axi_hp_out_array_t(0 to C_S_AXI_HP_COUNT-1);   -- s2m
  signal fclk             : std_logic_vector(3 downto 0);
  signal clk              : std_logic;
  signal raw_rst_n        : std_logic; -- FCLKRESET_Ns need synchronization
  signal rst_n            : std_logic; -- the synchronized negative reset
  signal reset            : std_logic; -- our positive reset
  signal count            : unsigned(25 downto 0);
  signal my_zynq_out      : zynq_out_array_t;
  signal my_zynq_out_data : zynq_out_data_array_t;
  signal dbg_state        : ulonglong_array_t(0 to 3);
  signal dbg_state1       : ulonglong_array_t(0 to 3);
  signal dbg_state2       : ulonglong_array_t(0 to 3);
  signal dbg_state_r      : ulonglong_array_t(0 to 3);
  signal dbg_state1_r     : ulonglong_array_t(0 to 3);
  signal dbg_state2_r     : ulonglong_array_t(0 to 3);
  -- debug state
  constant ntrace  : natural := to_integer(maxtrace);
  signal zynq_in_seen     : std_logic;
  signal zynq_out_seen    : std_logic;
  signal sdp_starting_in  : bool_t;
  signal sdp_starting_out : bool_t;
  signal sdp_header_count_in : unsigned(5 downto 0);
  signal sdp_count_in     : unsigned(5 downto 0);
  signal sdp_header_in    : ulonglong_array_t(0 to ntrace-1);
  signal sdp_data_in      : ulong_array_t(0 to ntrace-1);
  signal sdp_header_count_out : unsigned(5 downto 0);
  signal sdp_count_out     : unsigned(5 downto 0);
  signal sdp_header_out   : ulonglong_array_t(0 to ntrace-1);
  signal sdp_data_out     : ulonglong_array_t(0 to ntrace-1);
  signal sdp_out_status   : ulong_array_t(0 to ntrace-1);
  signal axi_cdcount      : unsigned(5 downto 0);
  signal axi_cacount      : unsigned(5 downto 0);
  signal axi_wdcount      : unsigned(5 downto 0);
  signal axi_rdcount      : unsigned(5 downto 0);
  signal axi_wacount      : unsigned(5 downto 0);
  signal axi_racount      : unsigned(5 downto 0);
  signal axi_cdata        : ulonglong_array_t(0 to ntrace-1);
  signal axi_caddr        : ulonglong_array_t(0 to ntrace-1);
  signal axi_rdata        : ulonglong_array_t(0 to ntrace-1);
  signal axi_raddr        : ulonglong_array_t(0 to ntrace-1);
  signal axi_wdata        : ulonglong_array_t(0 to ntrace-1);
  signal axi_waddr        : ulonglong_array_t(0 to ntrace-1);
  signal sdp_seen_r       : bool_t;
  function fyv(b : std_logic) return std_logic_vector is
  variable v : std_logic_vector(0 downto 0);
  begin
    v(0) := b;
    return v;
  end fyv;
begin
  timebase_out.clk   <= clk;
  timebase_out.reset <= reset;
  timebase_out.ppsIn <= '0';

  g0: if its(ocpi_debug) generate
    -- If we don't assign the outputs, the "debug overhead" will disappear
    props_out.axi_caddr <= axi_caddr;
    props_out.axi_waddr <= axi_waddr;
    props_out.axi_raddr <= axi_raddr;
    props_out.axi_cdata <= axi_cdata;
    props_out.axi_wdata <= axi_wdata;
    props_out.axi_rdata <= axi_rdata;
    props_out.axi_racount <= resize(axi_racount,32);
    props_out.axi_wacount <= resize(axi_wacount,32);
    props_out.axi_rdcount <= resize(axi_rdcount,32);
    props_out.axi_wdcount <= resize(axi_wdcount,32);
    props_out.sdp_header_count_in <= resize(sdp_header_count_in,32);
    props_out.sdp_count_in <= resize(sdp_count_in,32);
    props_out.sdp_headers_in <= sdp_header_in;
    props_out.sdp_data_in <= sdp_data_in;
    props_out.sdp_header_count_out <= resize(sdp_header_count_out,32);
    props_out.sdp_count_out <= resize(sdp_count_out,32);
    props_out.sdp_headers_out <= sdp_header_out;
    props_out.sdp_data_out <= sdp_data_out;
  end generate g0;

  clkbuf   : BUFG   port map(I => fclk(0),
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
      ps_in.debug           => (31 => useGP1,
                                others => '0'),
      ps_out.FCLK           => fclk,
      ps_out.FCLKRESET_N    => raw_rst_n,
      m_axi_gp_in           => ps_m_axi_gp_in,
      m_axi_gp_out          => ps_m_axi_gp_out,
      s_axi_gp_in           => ps_s_axi_gp_in,
      s_axi_gp_out          => ps_s_axi_gp_out,
      s_axi_hp_in           => ps_axi_hp_in,
      s_axi_hp_out          => ps_axi_hp_out
      );
  -- Adapt the axi master from the PS to be a CP Master
  cp : axi2cp
    port map(
      clk     => clk,
      reset   => reset,
      axi_in  => ps_m_axi_gp_out(whichGP),
      axi_out => ps_m_axi_gp_in(whichGP),
      cp_in   => cp_in,
      cp_out  => cp_out
      );
  zynq_out               <= my_zynq_out;
  zynq_out_data          <= my_zynq_out_data;
  props_out.sdpDropCount <= zynq_in(0).dropCount;
  props_out.debug_state  <= dbg_state_r;
  props_out.debug_state1 <= dbg_state1_r;
  props_out.debug_state2 <= dbg_state2_r;
  g : for i in 0 to 3 generate
    dp : sdp2axi
      generic map(ocpi_debug => true,
                  sdp_width  => to_integer(sdp_width),
                  axi_width  => ps_axi_hp_in(0).W.DATA'length/dword_size)
      port map(   clk          => clk,
                  reset        => reset,
                  sdp_in       => zynq_in(i),
                  sdp_in_data  => zynq_in_data(i),
                  sdp_out      => my_zynq_out(i),
                  sdp_out_data => my_zynq_out_data(i),
                  axi_in       => ps_axi_hp_out(i),
                  axi_out      => ps_axi_hp_in(i),
                  axi_error    => props_out.axi_error(i),
                  dbg_state    => dbg_state(i),
                  dbg_state1   => dbg_state1(i),
                  dbg_state2   => dbg_state2(i));
  end generate;
-- Output/readable properties
  props_out.dna             <= (others => '0');
  props_out.nSwitches       <= (others => '0');
  props_out.switches        <= (others => '0');
  props_out.memories_length <= to_ulong(1);
  props_out.memories        <= (others => to_ulong(0));
  props_out.nLEDs           <= to_ulong(0); --led'length);
  props_out.UUID            <= metadata_in.UUID;
  props_out.romData         <= metadata_in.romData;
  --props_out.slotCardIsPresent_length <= nSlots; -- TODO / FIXME comment back in once volatile sequence properties are fixed in codegen (which SHOULD result in this property being changed from an array to a sequence)
  props_out.slotCardIsPresent <= (0 => not fmc_prsnt, -- fmc_prsnt is active low, this coincides with index 0 of slotName property
                                  others => '0');
--  props_out.slotCardIsPresent(1 to 63) <= (others => '0'); -- TODO / FIXME remove this line once volatile sequence properties are fixed in codegen (which SHOULD result in this property being changed from an array to a sequence)
  -- Settable properties - drive the leds that are not driven by hardware from the property
  -- led(6 downto 1)           <= std_logic_vector(props_in.leds(6 downto 1));
  -- led(led'left downto 8)    <= (others => '0');
  -- Drive metadata interface
  metadata_out.clk          <= clk;
  metadata_out.romAddr      <= props_in.romAddr;
  metadata_out.romEn        <= props_in.romData_read;
  led(0) <= count(count'left);
  led(1) <= ps_m_axi_gp_out(whichGP).ARVALID;
  led(2) <= '0';
  led(3) <= cp_in.take;
  led(4) <= cp_in.valid;
  led(5) <= ps_m_axi_gp_in(whichGP).ARREADY;
  led(6) <= ps_m_axi_gp_in(whichGP).RVALID;
  led(7) <= ps_m_axi_gp_out(whichGP).RREADY;
  work : process(clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        count <= (others => '0');
        sdp_count_in <= (others => '0');
        sdp_count_out <= (others => '0');
        sdp_header_count_in <= (others => '0');
        sdp_header_count_out <= (others => '0');
        sdp_header_in  <= (others => (others => '0'));
        sdp_header_out <= (others => (others => '0'));
        axi_raddr  <= (others => (others => '0'));
        axi_waddr  <= (others => (others => '0'));
        axi_rdata  <= (others => (others => '0'));
        axi_wdata  <= (others => (others => '0'));
        axi_wdcount <= (others => '0');
        axi_rdcount <= (others => '0');
        axi_wacount <= (others => '0');
        axi_racount <= (others => '0');
        axi_cacount <= (others => '0');
        axi_cdcount <= (others => '0');
        zynq_in_seen <= '0';
        zynq_out_seen <= '0';
        sdp_starting_in <= btrue;
        sdp_starting_out <= btrue;
        sdp_seen_r <= bfalse;
      else
        if its(props_in.sdp_count_in_written) then
          sdp_count_in <= (others => '0');
          sdp_count_out <= (others => '0');
          sdp_header_count_in <= (others => '0');
          sdp_header_count_out <= (others => '0');
          axi_wdcount <= (others => '0');
          axi_rdcount <= (others => '0');
          axi_wacount <= (others => '0');
          axi_racount <= (others => '0');
          axi_cacount <= (others => '0');
          axi_cdcount <= (others => '0');
          sdp_seen_r <= bfalse;
        end if;
        dbg_state_r <= dbg_state;
        dbg_state1_r <= dbg_state1;
        dbg_state2_r <= dbg_state2;
        if its(my_zynq_out(0).sdp.valid) then
          zynq_out_seen <= '1';
        end if;
        if its(zynq_in(0).sdp.valid) then
          zynq_in_seen <= '1';
        end if;
        if its(my_zynq_out(0).sdp.valid) and zynq_in(0).sdp.ready then
          sdp_starting_out <= my_zynq_out(0).sdp.eop;
          if its(sdp_starting_out) then
            if sdp_header_count_out /= ntrace-1 then
              sdp_header_out(to_integer(sdp_header_count_out)) <=
                to_ulonglong(std_logic_vector(count(15 downto 0)) &
                             std_logic_vector(header2dws(my_zynq_out(0).sdp.header)(1)(14 downto 0)) & "0" &
                             std_logic_vector(header2dws(my_zynq_out(0).sdp.header)(0))); 
              sdp_header_count_out <= sdp_header_count_out + 1;
            end if;
          end if;
          if sdp_count_out /= ntrace-1 then
            sdp_data_out(to_integer(sdp_count_out)) <=
              to_ulonglong(std_logic_vector(count(15 downto 0)) &
                           std_logic_vector(dbg_state(0)(15 downto 0)) &
                           my_zynq_out_data(0)(0));
            sdp_count_out <= sdp_count_out + 1;
          end if;
        end if;
        if its(zynq_in(0).sdp.valid) and my_zynq_out(0).sdp.ready then
          sdp_starting_in <= zynq_in(0).sdp.eop;
          if its(sdp_starting_in) then
            sdp_seen_r <= btrue;
            if sdp_header_count_in /= ntrace-1 then
              sdp_header_in(to_integer(sdp_header_count_in)) <=
                to_ulonglong(std_logic_vector(count(15 downto 0)) &
                             std_logic_vector(header2dws(zynq_in(0).sdp.header)(1)(10 downto 0)) &
                             std_logic_vector(header2dws(zynq_in(0).sdp.header)(0)(31 downto 25)) & "00" &
                             fyv(zynq_in(0).sdp.eop) & -- 27
                             "00" &
                             std_logic_vector(header2dws(zynq_in(0).sdp.header)(0)(24 downto 0))); 
              sdp_header_count_in <= sdp_header_count_in + 1;
            end if;
          end if;
          if sdp_count_in /= ntrace-1 then
            sdp_data_in(to_integer(sdp_count_in)) <= to_ulong(zynq_in_data(0)(0));
            sdp_count_in <= sdp_count_in + 1;
          end if;
        end if;
        if its(ps_axi_hp_out(0).R.VALID) and ps_axi_hp_in(0).R.READY and axi_rdcount /= ntrace-1 then
          axi_rdata(to_integer(axi_rdcount)) <=
            to_ulonglong(ps_axi_hp_out(0).R.DATA(63 downto 0)); -- &
--                         "00010010001101000101011001110000");
          axi_rdcount <= axi_rdcount + 1;
        end if;
        if its(ps_axi_hp_in(0).AR.VALID and ps_axi_hp_out(0).AR.READY) and axi_racount /= ntrace-1 then
          axi_raddr(to_integer(axi_racount)) <=
            to_ulonglong(--std_logic_vector(count(15 downto 0)) & -- 16
                         std_logic_vector(dbg_state(0)(31 downto 0)) & -- 28
                         ps_axi_hp_in(0).AR.LEN & -- 4
                         ps_axi_hp_in(0).AR.ADDR(27 downto 0)); -- 28
          axi_racount <= axi_racount + 1;
        end if;
        if its(ps_axi_hp_in(0).W.VALID) and ps_axi_hp_out(0).W.READY and axi_wdcount /= ntrace-1 then
          axi_wdata(to_integer(axi_wdcount)) <=
            to_ulonglong(
              std_logic_vector(count(15 downto 0)) & -- 16
              std_logic_vector(dbg_state1(0)(15 downto 0)) &
--               fyv(ps_axi_hp_in(0).W.LAST) & -- 1
--              ps_axi_hp_in(0).W.STRB & -- 8
--              std_logic_vector(dbg_state1(31 downto 0)));
              ps_axi_hp_in(0).W.DATA(63 downto 56) & -- 8
              ps_axi_hp_in(0).W.DATA(39 downto 32) & -- 8
              ps_axi_hp_in(0).W.DATA(31 downto 24) & -- 8
              ps_axi_hp_in(0).W.DATA(7 downto 0)); -- 8
          axi_wdcount <= axi_wdcount + 1;
        end if;
        if its(ps_axi_hp_in(0).AW.VALID and ps_axi_hp_out(0).AW.READY) and axi_wacount /= ntrace-1 then
          axi_waddr(to_integer(axi_wacount)) <=
            to_ulonglong(
--              std_logic_vector(dbg_state(27 downto 4)) & -- 24
--                                          "0" & ps_axi_hp_in(0).AWSIZE & -- 4
--                                          ps_axi_hp_in(0).AWLEN & -- 4
              std_logic_vector(count(15 downto 0)) & -- 16
              "000000000000" &
              ps_axi_hp_in(0).AW.LEN & -- 4
              ps_axi_hp_in(0).AW.ADDR); -- 32
          axi_wacount <= axi_wacount + 1;
        end if;
        if its(ps_m_axi_gp_out(whichGP).WVALID) and ps_m_axi_gp_in(whichGP).WREADY and axi_cdcount /= ntrace-1 and sdp_seen_r then
          axi_cdata(to_integer(axi_cdcount)) <=
            to_ulonglong(
              std_logic_vector(count(15 downto 0)) & -- 16
              std_logic_vector(dbg_state1(0)(15 downto 5)) &
              fyv(ps_m_axi_gp_out(whichGP).WLAST) & -- 1
              ps_m_axi_gp_out(whichGP).WSTRB & -- 8
              ps_m_axi_gp_out(whichGP).WDATA);
--   ps_axi_hp_in(0).W.DATA(63 downto 56) & -- 8
--              ps_axi_hp_in(0).W.DATA(39 downto 32) & -- 8
--              ps_axi_hp_in(0).W.DATA(31 downto 24) & -- 8
--              ps_axi_hp_in(0).W.DATA(7 downto 0)); -- 8
          axi_cdcount <= axi_cdcount + 1;
        end if;
        if its(ps_m_axi_gp_out(whichGP).AWVALID and ps_m_axi_gp_in(whichGP).AWREADY) and axi_cacount /= ntrace-1 and sdp_seen_r then
          axi_caddr(to_integer(axi_cacount)) <=
            to_ulonglong(
--              std_logic_vector(dbg_state(27 downto 4)) & -- 24
--                                          "0" & ps_axi_hp_in(0).AWSIZE & -- 4
--                                          ps_axi_hp_in(0).AWLEN & -- 4
              std_logic_vector(count(15 downto 0)) & -- 16
              "000000000000" &
              ps_m_axi_gp_out(whichGP).AWLEN & -- 4
              ps_m_axi_gp_out(whichGP).AWADDR); -- 32
          axi_cacount <= axi_cacount + 1;
        end if;
        count <= count + 1;
--        if ps_m_axi_gp_out(whichGP).ARVALID = '1' and ps_m_axi_gp_out(whichGP).ARLEN = "0001" then
--          seen_burst <= '1';
--        end if;
      end if;
    end if;
  end process;
end rtl;
