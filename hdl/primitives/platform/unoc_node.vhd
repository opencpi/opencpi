library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all;

entity unoc_node_rv is
  generic(
    control          : bool_t;
    position         : ulong_t := to_ulong(0)
    );
  port(
    up_in      : in  unoc_master_out_t;
    up_out     : out unoc_master_in_t;
    client_in  : in  unoc_master_in_t;
    client_out : out unoc_master_out_t;
    down_in    : in  unoc_master_in_t;
    down_out   : out unoc_master_out_t
    );
end entity unoc_node_rv;
architecture rtl of unoc_node_rv is
  -- component for verilog
  component mkTLPSM is
    port(
      pfk                 : in std_logic_vector(13 downto 0);
      CLK                 : in std_logic;
      RST_N               : in std_logic;
      -- action method s_request_put
      s_request_put       : in std_logic_vector(152 downto 0);
      EN_s_request_put    : in std_logic ;
      RDY_s_request_put   : out std_logic;

      -- actionvalue method s_response_get
      EN_s_response_get   : in std_logic ;
      s_response_get      : out std_logic_vector(152 downto 0);
      RDY_s_response_get  : out std_logic;

      -- actionvalue method c0_request_get
      EN_c0_request_get   : in std_logic ;
      c0_request_get      : out std_logic_vector(152 downto 0);
      RDY_c0_request_get  : out std_logic;

      -- action method c0_response_put
      c0_response_put     : in  std_logic_vector(152 downto 0);
      EN_c0_response_put  : in  std_logic ;
      RDY_c0_response_put : out std_logic;

      -- actionvalue method c1_request_get
      EN_c1_request_get   : in  std_logic ;
      c1_request_get      : out std_logic_vector(152 downto 0);
      RDY_c1_request_get  : out std_logic;

      -- action method c1_response_put
      c1_response_put     : in  std_logic_vector(152 downto 0);
      EN_c1_response_put  : in  std_logic;
      RDY_c1_response_put : out std_logic
      );
  end component mkTLPSM;
  signal pfk : std_logic_vector(15 downto 0);
  -- local signals for server/uplink request consumer (aka request put)
  signal RDY_s_request_put : std_logic;
  signal EN_s_request_put : std_logic;
  -- local signals for client response consumer (aka response put)
  signal RDY_c0_response_put : std_logic;
  signal EN_c0_response_put : std_logic;
  -- local signals for downlink response comsumer
  signal RDY_c1_response_put : std_logic;
  signal EN_c1_response_put : std_logic;
begin
  -- The outoing take signal is a DEQ to the producer side computed by us
  -- The incoming valid signal is an indication of not empty from the producer side
  EN_s_request_put   <= up_in.valid and RDY_s_request_put;
  up_out.take        <= EN_s_request_put;
  EN_c0_response_put <= client_in.valid and RDY_c0_response_put;
  client_out.take    <= EN_c0_response_put;
  EN_c1_response_put <= down_in.valid and RDY_c1_response_put;
  down_out.take      <= EN_c1_response_put;

  client_out.id      <= up_in.id or std_logic_vector(position(2 downto 0));
  client_out.clk     <= up_in.clk;
  client_out.reset_n <= up_in.reset_n;

  down_out.id        <= up_in.id;
  down_out.clk       <= up_in.clk;
  down_out.reset_n   <= up_in.reset_n;


  -- Fork key - basically a union
  -- Bits 13:12 is discriminator:
  --   0 Simple Bar (3 bit Bar in (2:0))
  --   1 Bus-based (8 bit Bus)
  --   2 Bar64: (3 bit bar (6:4), 1 bit "high address bit"(3), 3 bit function(2:0))
  --   3 RouteSub: (4 bit addr(11:8), 8 bit bus number(7:0))

  pfk <= X"0550" when its(control) else       -- match on bar0
         X"2510" or std_logic_vector(position(2 downto 0));
        -- match on bar1 and DWAaddr [15:13] == position
        -- or completion and func == position
         
sm : mkTLPSM
  port map(
    pfk   => pfk(13 downto 0),
    CLK   => up_in.clk,
    RST_N => up_in.reset_n,

    -- action method s_request_put - consumer
    s_request_put => up_in.data,
    EN_s_request_put => EN_s_request_put,
    RDY_s_request_put => RDY_s_request_put,

    -- actionvalue method s_response_get - producer
    EN_s_response_get => up_in.take,
    s_response_get => up_out.data,
    RDY_s_response_get => up_out.valid,

    -- actionvalue method c0_request_get - producer
    EN_c0_request_get => client_in.take,
    c0_request_get => client_out.data,
    RDY_c0_request_get => client_out.valid,

    -- action method c0_response_put = consumer
    c0_response_put => client_in.data,
    EN_c0_response_put => EN_c0_response_put,
    RDY_c0_response_put => RDY_c0_response_put,

    -- actionvalue method c1_request_get - producer
    EN_c1_request_get => down_in.take,
    c1_request_get => down_out.data,
    RDY_c1_request_get => down_out.valid,

    -- action method c1_response_put - consumer
    c1_response_put => down_in.data,
    EN_c1_response_put => EN_c1_response_put,
    RDY_c1_response_put => RDY_c1_response_put
    );
end architecture rtl;
