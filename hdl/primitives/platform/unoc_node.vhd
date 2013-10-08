library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all;

entity unoc_node is
  generic(
    -- Be a little proactive - this is more control than we have now
    control          : boolean := false;
    func             : natural := 0;      -- PCI function
    offset           : natural := 0;      -- Offset in bar: now 0 or 32k
    size             : natural := 0;      -- Window size in bar: now 32k
    up_producer_hack : boolean := false   -- hack described below for compatibility
    );
  port(
    CLK, RST_N : in std_logic;
    up_in      : in  unoc_link_t;
    up_out     : out unoc_link_t;
    client_in  : in  unoc_link_t;
    client_out : out unoc_link_t;
    down_in    : in  unoc_link_t;
    down_out   : out unoc_link_t
    );
end entity unoc_node;
architecture rtl of unoc_node is
  -- component for verilog
  component mkTLPSM is
    port(
      pfk                 : in std_logic_vector(13 downto 0);
      CLK, RST_N          : in std_logic;
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
  signal RDY_s_request_put : std_logic;
  signal RDY_c0_response_put : std_logic;
  signal RDY_c1_response_put : std_logic;
  signal EN_s_request_put : std_logic;
begin
  -- We have an option/hack for the up interface in case it has
  -- already computed the ENQ signal directly (i.e. is not
  -- following the "consumer has the and gate" rule)
  g0: if up_producer_hack generate
    -- The HACK:
    -- The outgoing take signal is really "our fifo is not full"
    -- The incoming valid signal is really "enqueue"
    up_out.take <= RDY_s_request_put;
    EN_s_request_put <= up_in.valid;
  end generate g0;
  g1: if not up_producer_hack generate
    -- The NORMAL symmetrical scheme:
    -- The outoing take signal is a DEQ to the other side computed by us
    -- The incoming valid signal is an indication of EMPTY_N from the other side
    EN_s_request_put <= up_in.valid     and RDY_s_request_put;
    up_out.take <= EN_s_request_put;
  end generate g1;
  client_out.take <= client_in.valid and RDY_c0_response_put;
  down_out.take   <= down_in.valid   and RDY_c1_response_put;

  pfk               <= X"0550" when control else
                       X"2510" when func = 0 else
                       X"2519" when func = 1 else
                       X"251a" when func = 2 else (others => '0');
         
sm : mkTLPSM
  port map(
    pfk => pfk(13 downto 0),
    CLK => CLK,
    RST_N => RST_N,

    -- action method s_request_put - consumer
    s_request_put => up_in.data,
    EN_s_request_put => up_in.valid,
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
    EN_c0_response_put => client_in.valid,
    RDY_c0_response_put => RDY_c0_response_put,

    -- actionvalue method c1_request_get - producer
    EN_c1_request_get => down_in.take,
    c1_request_get => down_out.data,
    RDY_c1_request_get => down_out.valid,

    -- action method c1_response_put - consumer
    c1_response_put => down_in.data,
    EN_c1_response_put => down_in.valid,
    RDY_c1_response_put => RDY_c1_response_put
    );
end architecture rtl;
