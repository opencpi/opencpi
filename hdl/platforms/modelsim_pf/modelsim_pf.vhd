-- THIS FILE WAS ORIGINALLY GENERATED ON Thu Sep 26 15:32:12 2013 EDT
-- BASED ON THE FILE: modelsim_pf.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: modelsim_pf

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library platform; use platform.platform_pkg.all;
library modelsim_pf;
architecture rtl of modelsim_pf_worker is
  signal   ctl_clk                   : std_logic;          -- clock we produce and use for the control plane
  signal   ctl_rst_n                 : std_logic := '0';  -- reset associated with control plane clock
  signal   ctl_rst_count             : unsigned(7 downto 0) := (others => '0'); -- count clocks for reset
  constant clk_half_period           : time := 5 ns;       -- 100 mhz
  constant ctl_reset_clocks          : natural := 17;
  -- Signals for mkSimIO ports
  signal io_host_request_get         : std_logic_vector(7 downto 0); -- local connection io->dcp
  signal io_RDY_host_request_get     : std_logic;                    -- needed for local expression
  signal io_EN_host_request_get      : std_logic;                    -- computed locally
  signal io_host_response_put        : std_logic_vector(7 downto 0); -- local connection dcp->io
  signal io_RDY_host_response_put    : std_logic;                    -- needed for local expression
  signal io_EN_host_response_put     : std_logic;                    -- computed locally
  -- Signals for mkSimDP ports
  signal dcp_RDY_host_request_put    : std_logic;                    -- needed for local expression
  signal dcp_EN_host_request_put     : std_logic;                    -- computed locally
  signal dcp_RDY_host_response_get   : std_logic;                    -- needed for local expression
  signal dcp_EN_host_response_get    : std_logic;                    -- computed locally
  signal dcp_RDY_client_response_put : std_logic;                    -- needed for local expression
  signal dcp_EN_client_response_put  : std_logic;                    -- computed locally
begin

  -- generate a clock

  clock : process
  begin
    ctl_clk <= '0';
    wait for clk_half_period;
    ctl_clk <= '1';
    wait for clk_half_period;
  end process;

  -- generate a reset for some number of clocks

  work : process(ctl_clk)
  begin
    if rising_edge(ctl_clk) then
      if ctl_rst_n = '0' then
        if ctl_rst_count < ctl_reset_clocks then
          ctl_rst_count <= ctl_rst_count + 1;
        else
          ctl_rst_n <= '1';
        end if;
      end if;   
    end if;
  end process;
  -- This piece of generic infrastructure in is instantiated here because
  -- it localizes all these signals here in the platform worker, and thus
  -- the platform worker simply produces clock, reset, and time, all in the
  -- clock domain of the timekeepping clock.
  ts : time_server
    port map(
      CLK                     => ctl_clk,
      RST_N                   => ctl_rst_n,
      timeCLK                 => ctl_clk,
      timeRST_N               => ctl_rst_n,

      timeControl             => props_in.timeControl,
      timeControl_written     => props_in.timeControl_written,
      timeStatus              => props_out.timeStatus,
      timeNowIn               => props_in.timeNow,
      timeNow_written         => props_in.timeNow_written,
      timeNowOut              => props_out.timeNow,
      timeDeltaIn             => props_in.timeDelta,
      timeDelta_written       => props_in.timeDelta_written,
      timeDeltaOut            => props_out.timeDelta,
      ticksPerSecond          => props_out.ticksPerSecond,
      
      -- PPS interface
      ppsIn                   => '0', -- we could actually generate this
      ppsOut                  => open,

      -- Time service output
      time_service            => time_out
      );

  io_EN_host_request_get      <= dcp_RDY_host_request_put and io_RDY_host_request_get;
  io_EN_host_response_put     <= dcp_RDY_host_response_get and io_RDY_host_response_put;
  io : platform.platform_pkg.mkSimIO
    port map(
      CLK                     => ctl_clk,
      RST_N                   => ctl_rst_n,
      host_response_put       => io_host_response_put,
      EN_host_request_get     => io_EN_host_request_get,
      EN_host_response_put    => io_EN_host_response_put,
      host_request_get        => io_host_request_get,
      RDY_host_request_get    => io_RDY_host_request_get,
      RDY_host_response_put   => io_RDY_host_response_put
      );

  dcp_EN_host_request_put     <= dcp_RDY_host_request_put and io_RDY_host_request_get;
  dcp_EN_host_response_get    <= dcp_RDY_host_response_get and io_RDY_host_response_put;
  dcp_EN_client_response_put  <= dcp_RDY_client_response_put and cp_in.valid;
  cp_out.take                 <= dcp_EN_client_response_put; -- we as consumer telling the producer to enqueue
  cp_out.clk                  <= ctl_clk;
  cp_out.reset_n              <= ctl_rst_n;
  dcp : platform.platform_pkg.mkSimDCP
    port map(
      CLK                     => ctl_clk,
      RST_N                   => ctl_rst_n,
      -- input from mkSimIO
      host_request_put        => io_host_request_get,
      RDY_host_request_put    => dcp_RDY_host_request_put,
      EN_host_request_put     => dcp_EN_host_request_put,

      -- output to mkSimIO
      host_response_get       => io_host_response_put,
      RDY_host_response_get   => dcp_RDY_host_response_get,
      EN_host_response_get    => dcp_EN_host_response_get,

      -- output to OCCP
      client_request_get      => cp_out.data,
      RDY_client_request_get  => cp_out.valid,
      EN_client_request_get   => cp_in.take,

      -- input from OCCP
      client_response_put     => cp_in.data,
      RDY_client_response_put => dcp_RDY_client_response_put, -- needed locally
      EN_client_response_put  => dcp_EN_client_response_put   -- computed locally
  );
    
  props_out.platform          <= to_string("modelsim_pf", props_out.platform'length);
  props_out.dna               <= (others => '0');
  props_out.nSwitches         <= (others => '0');
  props_out.switches          <= (others => '0');
  props_out.memories_length   <= to_ulong(1);
  props_out.memories          <= (others => to_ulong(0));
  props_out.nLEDs             <= (others => '0');
  props_out.UUID              <= metadata_in.UUID;
  props_out.romData           <= metadata_in.romData;
  metadata_out.clk            <= ctl_clk;
  metadata_out.romAddr        <= props_in.romAddr;
  metadata_out.romEn          <= props_in.romData_read;
end rtl;
