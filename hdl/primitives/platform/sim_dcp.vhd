-- The sim control plane, which combines the simIO verilog/BSV module,
-- which does I/O from the named pipes, with the simDCP module,
-- which implements the DCP protocol and talks to OCCP

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all;

entity sim_dcp is
  port(
    clk                     : in  std_logic;
    reset                   : in  std_logic;
    cp_in                   : in  occp_out_t;
    cp_out                  : out occp_in_t);
end sim_dcp;

architecture rtl of sim_dcp is
  -- The verilog code we are wrapping
  component mkSimIO is
    port(
      CLK                   : in  std_logic;
      RST_N                 : in  std_logic;
      EN_host_request_get   : in  std_logic;
      host_request_get      : out std_logic_vector(7 downto 0);
      RDY_host_request_get  : out std_logic;
      host_response_put     : in  std_logic_vector(7 downto 0);
      EN_host_response_put  : in  std_logic;
      RDY_host_response_put : out std_logic);
  end component mkSimIO;

  component mkSimDCP is
    port(
      CLK                     : in  std_logic;
      RST_N                   : in  std_logic;

      host_request_put        : in  std_logic_vector(7 downto 0);
      EN_host_request_put     : in  std_logic;
      RDY_host_request_put    : out std_logic;

      EN_host_response_get    : in  std_logic;
      host_response_get       : out std_logic_vector(7 downto 0);
      RDY_host_response_get   : out std_logic;

      EN_client_request_get   : in  std_logic;
      client_request_get      : out std_logic_vector(60 downto 0);
      RDY_client_request_get  : out std_logic;
    
      client_response_put     : in  std_logic_vector(39 downto 0);
      EN_client_response_put  : in  std_logic;
      RDY_client_response_put : out std_logic);
  end component mkSimDCP;
  -- Signals for mkSimIO ports
  signal io_EN_host_request_get      : std_logic;                    -- computed locally
  signal io_host_request_get         : std_logic_vector(7 downto 0); -- local connection
  signal io_RDY_host_request_get     : std_logic;                    -- needed for local expr
  signal io_host_response_put        : std_logic_vector(7 downto 0); -- local connection
  signal io_EN_host_response_put     : std_logic;                    -- computed locally
  signal io_RDY_host_response_put    : std_logic;                    -- needed for local expr
  -- Signals for mkSimDP ports
  signal dcp_RDY_host_request_put    : std_logic; -- needed for local expression
  signal dcp_RDY_host_response_get   : std_logic; -- needed for local expression
  signal dcp_EN_client_response_put  : std_logic; -- computed locally
  signal dcp_RDY_client_response_put : std_logic; -- needed for local expression
  signal RST_N                       : std_logic;
  -- Our request and response bundles
  signal request                     : std_logic_vector(60 downto 0);
  signal response                    : std_logic_vector(39 downto 0);
begin
  RST_N <= not reset;

  cp_out.clk                  <= clk;
  cp_out.reset                <= reset;
  cp_out.take                 <= dcp_EN_client_response_put; -- we take the response
  cp_out.is_read              <= request(60);
  cp_out.address              <= request(59 downto 36);
  cp_out.byte_en              <= request(35 downto 32);
  cp_out.data                 <= request(31 downto 0); -- lsb 8 bits is tag on read
  response                    <= cp_in.tag & cp_in.data;

  io_EN_host_request_get      <= dcp_RDY_host_request_put and io_RDY_host_request_get;
  io_EN_host_response_put     <= dcp_RDY_host_response_get and io_RDY_host_response_put;
  io : mkSimIO
    port map(
      CLK                     => clk,
      RST_N                   => RST_N,

      EN_host_request_get     => io_EN_host_request_get,
      host_request_get        => io_host_request_get,
      RDY_host_request_get    => io_RDY_host_request_get,

      host_response_put       => io_host_response_put,
      EN_host_response_put    => io_EN_host_response_put,
      RDY_host_response_put   => io_RDY_host_response_put
      );

  -- Receive side "and" of producer valid/non-empty and local consumer ready/not-full
  dcp_EN_client_response_put  <= dcp_RDY_client_response_put and cp_in.valid;
  dcp : mkSimDCP
    port map(
      CLK                     => clk,
      RST_N                   => RST_N,
      -- input from mkSimIO
      host_request_put        => io_host_request_get,
      EN_host_request_put     => io_EN_host_request_get,
      RDY_host_request_put    => dcp_RDY_host_request_put,

      -- output to mkSimIO
      EN_host_response_get    => io_EN_host_response_put,
      host_response_get       => io_host_response_put,
      RDY_host_response_get   => dcp_RDY_host_response_get,

      -- output channel to OCCP
      EN_client_request_get   => cp_in.take,
      client_request_get      => request,
      RDY_client_request_get  => cp_out.valid,

      -- input from OCCP
      client_response_put     => response,
      EN_client_response_put  => dcp_EN_client_response_put, -- computed locally
      RDY_client_response_put => dcp_RDY_client_response_put -- needed locally
      );
end architecture rtl;

