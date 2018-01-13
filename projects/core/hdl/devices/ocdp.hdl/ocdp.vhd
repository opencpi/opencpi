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

-- THIS FILE WAS ORIGINALLY GENERATED ON Mon Oct 28 15:38:21 2013 EDT
-- BASED ON THE FILE: ocdp.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ocdp

-- Note THIS IS THE OUTER skeleton, since the 'outer' attribute was set.
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
library platform; use platform.platform_pkg.all;
architecture rtl of ocdp_rv is
  -- This is the verilog being wrapped
  component mkOCDP4B is
    generic (
      hasPush                 : std_logic_vector(0 downto 0) := "1";
      hasPull                 : std_logic_vector(0 downto 0) := "1";
      hasDebugLogic           : std_logic_vector(0 downto 0) := "1"
      );
    port(
      pciDevice               : in  std_logic_vector(15 downto 0);
      CLK                     : in  std_logic;
      RST_N                   : in  std_logic;  
      wci_s_MCmd              : in  std_logic_vector(2 downto 0);
      wci_s_MAddrSpace        : in  std_logic;
      wci_s_MByteEn           : in  std_logic_vector(3 downto 0);
      wci_s_MAddr             : in  std_logic_vector(31 downto 0);
      wci_s_MData             : in  std_logic_vector(31 downto 0);
      wci_s_SResp             : out std_logic_vector(1 downto 0);
      wci_s_SData             : out std_logic_vector(31 downto 0);
      wci_s_SThreadBusy       : out std_logic;
      wci_s_SFlag             : out std_logic_vector(2 downto 0);
      wci_s_MFlag             : in  std_logic_vector(1 downto 0);
      wti_s_req               : in  std_logic_vector(66 downto 0);
      wti_s_SThreadBusy       : out std_logic;
      wti_s_SReset_n          : out std_logic;
      wmiS0_MCmd              : in  std_logic_vector(2 downto 0);
      wmiS0_MReqLast          : in  std_logic;
      wmiS0_MReqInfo          : in  std_logic;
      wmiS0_MAddrSpace        : in  std_logic;
      wmiS0_MAddr             : in  std_logic_vector(13 downto 0);
      wmiS0_MBurstLength      : in  std_logic_vector(11 downto 0);
      wmiS0_MDataValid        : in  std_logic;
      wmiS0_MDataLast         : in  std_logic;
      wmiS0_MData             : in  std_logic_vector(31 downto 0);
      wmiS0_MDataByteEn       : in  std_logic_vector(3 downto 0);
      wmiS0_SResp             : out std_logic_vector(1 downto 0);
      wmiS0_SData             : out std_logic_vector(31 downto 0);
      wmiS0_SThreadBusy       : out std_logic;
      wmiS0_SDataThreadBusy   : out std_logic;
      wmiS0_SRespLast         : out std_logic;
      wmiS0_SFlag             : out std_logic_vector(31 downto 0);
      wmiS0_arg_mFlag         : in  std_logic_vector(31 downto 0);
      wmiS0_SReset_n          : out std_logic;
      wmiS0_MReset_n          : in  std_logic;
      server_request_put      : in  std_logic_vector(152 downto 0);
      EN_server_request_put   : in  std_logic;
      RDY_server_request_put  : out std_logic;
      EN_server_response_get  : in  std_logic;
      server_response_get     : out std_logic_vector(152 downto 0);
      RDY_server_response_get : out std_logic
      );
  end component mkOCDP4B;
  -- Our locally computed enqueue signal for input data
  signal EN_server_request_put  : std_logic;
  -- Our locally used input-FIFO-not-full signal for input data
  signal RDY_server_request_put : std_logic;
  -- To construct the BSV WTI request
  signal wti_s_req              : std_logic_vector(66 downto 0);
  signal wci_s_MAddr            : std_logic_vector(31 downto 0);
  signal wmiS0_MAddr            : std_logic_vector(31 downto 0);
  signal my_reset_n             : std_logic;
  signal server_response_get    : std_logic_vector(152 downto 0);
begin
  -- We tell mkOCCP to enqueue when it is ready (not full) and there is input data
  EN_server_request_put <= RDY_server_request_put and client_in.valid;
  -- We tell the producer side to dequeue when we are enqueueing
  client_out.take       <= EN_server_request_put;
  wti_s_req             <= wti_in.MCmd & wti_in.MData;
  wci_s_MAddr           <= X"000000" & ctl_in.MAddr;
  wti_out.clk           <= ctl_in.clk;
  -- we are reset if either specific WCI reset or the global (uNoC) reset asserted (low)
  my_reset_n            <= ctl_in.MReset_n and client_in.reset_n;
  client_out.data       <= to_unoc(server_response_get);
dp : mkOCDP4B
  generic map(
      hasPush       => slv(includePush),
      hasPull       => slv(includePull),
      hasDebugLogic => slv(ocpi_debug)
      )
  port map(
    pciDevice               => client_in.id,
    CLK                     => ctl_in.CLK,      -- ctl_in.clk and client_in.clk are the same clk
    RST_N                   => my_reset_n,
    wci_s_MCmd              => ctl_in.MCmd,
    wci_s_MAddrSpace        => ctl_in.MAddrSpace(0),
    wci_s_MByteEn           => ctl_in.MByteEn,
    wci_s_MAddr             => wci_s_MAddr,
    wci_s_MData             => ctl_in.MData,
    wci_s_SResp             => ctl_out.SResp,
    wci_s_SData             => ctl_out.SData,
    wci_s_SThreadBusy       => ctl_out.SThreadBusy(0),
    wci_s_SFlag             => ctl_out.SFlag,
    wci_s_MFlag             => ctl_in.MFlag,
    wti_s_req               => wti_s_req,
    wti_s_SThreadBusy       => wti_out.SThreadBusy(0),
    wti_s_SReset_n          => wti_out.SReset_n,
    wmiS0_MCmd              => data_in.MCmd,
    wmiS0_MReqLast          => data_in.MReqLast,
    wmiS0_MReqInfo          => data_in.MReqInfo(0),
    wmiS0_MAddrSpace        => data_in.MAddrSpace(0),
    wmiS0_MAddr             => data_in.MAddr,
    wmiS0_MBurstLength      => data_in.MBurstLength,
    wmiS0_MDataValid        => data_in.MDataValid,
    wmiS0_MDataLast         => data_in.MDataLast,
    wmiS0_MData             => data_in.MData,
    wmiS0_MDataByteEn       => data_in.MDataByteEn,
    wmiS0_SResp             => data_out.SResp,
    wmiS0_SData             => data_out.SData,
    wmiS0_SThreadBusy       => data_out.SThreadBusy(0),
    wmiS0_SDataThreadBusy   => data_out.SDataThreadBusy(0),
    wmiS0_SRespLast         => data_out.SRespLast,
    wmiS0_SFlag             => data_out.SFlag,
    wmiS0_arg_mFlag         => data_in.MFlag,
    wmiS0_SReset_n          => data_out.SReset_n,
    wmiS0_MReset_n          => data_in.MReset_n,
    server_request_put      => to_slv(client_in.data),
    EN_server_request_put   => EN_server_request_put,
    RDY_server_request_put  => RDY_server_request_put,
    EN_server_response_get  => client_in.take,
    server_response_get     => server_response_get,
    RDY_server_response_get => client_out.valid
  );
end rtl;
