library IEEE, ocpi, platform;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ieee.math_real.all, ocpi.types.all,
  ocpi.util.all, platform.platform_pkg.all;
architecture rtl of unoc2cp_rv is
  -- The verilog being wrapped
  component mkTLPSerializer is
    port(
      pciDevice               : in  std_logic_vector(15 downto 0);
      CLK                     : in  std_logic;
      RST_N                   : in  std_logic;
      -- action method server_request_put
      server_request_put      : in  std_logic_vector(unoc_data_width-1 downto 0);
      EN_server_request_put   : in  std_logic ;
      RDY_server_request_put  : out std_logic;

      -- actionvalue method server_response_get
      EN_server_response_get  : in  std_logic ;
      server_response_get     : out std_logic_vector(unoc_data_width-1 downto 0);
      RDY_server_response_get : out std_logic;

      -- actionvalue method client_request_get
      EN_client_request_get   : in  std_logic ;
      client_request_get      : out std_logic_vector(60 downto 0);
      RDY_client_request_get  : out std_logic;

      -- action method client_response_put
      client_response_put     : in  std_logic_vector(39 downto 0);
      EN_client_response_put  : in  std_logic;
      RDY_client_response_put : out std_logic
      );
  end component mkTLPSerializer;
  signal RDY_server_request_put  : std_logic;
  signal EN_server_request_put   : std_logic;
  signal RDY_client_response_put : std_logic;
  signal EN_client_response_put  : std_logic;
  signal server_response_get     : std_logic_vector(unoc_data_width-1 downto 0);
  -- Our request and response bundles
  signal request : std_logic_vector(60 downto 0);
  signal response : std_logic_vector(39 downto 0);
begin
  -- The outoing take signal is a DEQ to the producer side computed by us
  -- The incoming valid signal is an indication of not empty from the producer side
  EN_server_request_put <= client_in.valid and RDY_server_request_put;
  client_out.take       <= EN_server_request_put;
  client_out.data       <= to_unoc(server_response_get);
  EN_client_response_put <= cp_in.valid and RDY_client_response_put;

  -- Using this adapter means using the unoc clk and reset as the control clk and reset
  cp_out.clk          <= client_in.clk;
  cp_out.reset        <= not client_in.reset_n;
  -- request fields to occp
  cp_out.is_read      <= request(60);
  cp_out.address      <= request(59 downto 36);
  cp_out.byte_en      <= request(35 downto 32);
  cp_out.data         <= request(31 downto 0);
  cp_out.take         <= EN_client_response_put;

  response            <= cp_in.tag & cp_in.data;


  tlps : mkTLPSerializer
    port map(
      CLK                    => client_in.clk,
      RST_N                  => client_in.reset_n,
      pciDevice              => client_in.id,
      -- action method server_request_put - we are the consumer of these requests
      server_request_put     => to_slv(client_in.data),
      EN_server_request_put  => EN_server_request_put,
      RDY_server_request_put => RDY_server_request_put,

      -- actionvalue method server_response_get - we are the producer of these responses
      EN_server_response_get  => client_in.take,
      server_response_get     => server_response_get,
      RDY_server_response_get => client_out.valid,

      -- actionvalue method client_request_get - we are the producer of these requests
      EN_client_request_get   => cp_in.take,
      client_request_get      => request,
      RDY_client_request_get  => cp_out.valid,

      -- action method client_response_put - we are the consumer of these responses
      client_response_put     => response,
      EN_client_response_put  => EN_client_response_put,
      RDY_client_response_put => RDY_client_response_put
      
   );
end rtl;
