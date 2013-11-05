library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all;

entity unoc_cp_adapter is
  port(
    -- I'm a unoc client, connecting to the NOC
    client_in  : in  unoc_master_out_t;
    client_out : out unoc_master_in_t;
    -- I'm connecting to occp
    occp_in    : in  occp_out_t;
    occp_out   : out occp_in_t
    );
end entity unoc_cp_adapter;

architecture rtl of unoc_cp_adapter is
  -- The verilog being wrapped
  component mkTLPSerializer is
    port(
      pciDevice               : in  std_logic_vector(15 downto 0);
      CLK                     : in  std_logic;
      RST_N                   : in  std_logic;
      -- action method server_request_put
      server_request_put      : in  std_logic_vector(unoc_width-1 downto 0);
      EN_server_request_put   : in  std_logic ;
      RDY_server_request_put  : out std_logic;

      -- actionvalue method server_response_get
      EN_server_response_get  : in  std_logic ;
      server_response_get     : out std_logic_vector(unoc_width-1 downto 0);
      RDY_server_response_get : out std_logic;

      -- actionvalue method client_request_get
      EN_client_request_get   : in  std_logic ;
      client_request_get      : out std_logic_vector(occp_request_width-1 downto 0);
      RDY_client_request_get  : out std_logic;

      -- action method client_response_put
      client_response_put     : in  std_logic_vector(occp_response_width-1 downto 0);
      EN_client_response_put  : in  std_logic;
      RDY_client_response_put : out std_logic
      );
  end component mkTLPSerializer;
  signal RDY_server_request_put  : std_logic;
  signal EN_server_request_put   : std_logic;
  signal RDY_client_response_put : std_logic;
  signal EN_client_response_put  : std_logic;
begin
  -- The outoing take signal is a DEQ to the producer side computed by us
  -- The incoming valid signal is an indication of not empty from the producer side
  EN_server_request_put <= client_in.valid and RDY_server_request_put;
  client_out.take       <= EN_server_request_put;

  EN_client_response_put <= occp_in.valid and RDY_client_response_put;
  occp_out.take         <= EN_client_response_put;

  -- Using this adapter means using the unoc clk and reset as the control clk and reset
  occp_out.clk          <= client_in.clk;
  occp_out.reset_n      <= client_in.reset_n;

  tlps : mkTLPSerializer
    port map(
      CLK                    => client_in.clk,
      RST_N                  => client_in.reset_n,
      pciDevice              => client_in.id,
      -- action method server_request_put - we are the consumer of these requests
      server_request_put     => client_in.data,
      EN_server_request_put  => EN_server_request_put,
      RDY_server_request_put => RDY_server_request_put,

      -- actionvalue method server_response_get - we are the producer of these responses
      EN_server_response_get  => client_in.take,
      server_response_get     => client_out.data,
      RDY_server_response_get => client_out.valid,

      -- actionvalue method client_request_get - we are the producer of these requests
      EN_client_request_get   => occp_in.take,
      client_request_get      => occp_out.data,
      RDY_client_request_get  => occp_out.valid,

      -- action method client_response_put - we are the consumer of these responses
      client_response_put     => occp_in.data,
      EN_client_response_put  => EN_client_response_put,
      RDY_client_response_put => RDY_client_response_put
      
   );
end architecture rtl;
