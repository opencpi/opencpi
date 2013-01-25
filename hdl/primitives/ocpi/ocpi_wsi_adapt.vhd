-- WSI width reducer for imprecise, initially no byte enables, imprecise, no data info,
-- all opcodes, no abort, no early-request
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wsi.all; use ocpi.util.all;

entity wsi_reduce is
  generic (in_width, out_width : natural);
  port (
    ctl_Clk               : in  std_logic;
    in_MBurstLength       : in  std_logic_vector(1 downto 0);
    in_MCmd               : in  std_logic_vector(2 downto 0);
    in_MData              : in  std_logic_vector(in_width-1 downto 0);
    in_MReqInfo           : in  std_logic_vector(8-1 downto 0);
    in_MReqLast           : in  std_logic;
    in_MReset_n           : in  std_logic;
    in_SReset_n           : out std_logic;
    in_SThreadBusy        : out std_logic_vector(0 downto 0);
    out_SReset_n          : in  std_logic;
    out_SThreadBusy       : in  std_logic_vector(0 downto 0);
    out_MBurstLength      : out std_logic_vector(1 downto 0);
    out_MCmd              : out std_logic_vector(2 downto 0);
    out_MData             : out std_logic_vector(out_width-1 downto 0);
    out_MReqInfo          : out std_logic_vector(8-1 downto 0);
    out_MReqLast          : out std_logic;
    out_MReset_n          : out std_logic 
  );
end entity wsi_reduce;
architecture rtl of wsi_reduce is
  signal in_reset : bool_t;
  signal in_ready : bool_t;
  signal in_som : bool_t;
  signal in_eom : bool_t;
  signal in_valid : bool_t;
  signal in_data : std_logic_vector(in_width-1 downto 0);
  signal in_my_valid : std_logic; -- my synthesis of this
  signal in_take : bool_t;
  signal out_reset : bool_t;
  signal out_ready : bool_t;
  signal out_som : bool_t;
  signal out_eom : bool_t;
  signal out_valid : bool_t;
  signal out_give : bool_t;
  signal out_data : std_logic_vector(in_width-1 downto 0);
  signal out_opcode : std_logic_vector(8-1 downto 0);
  signal my_reset : bool_t;
begin
  -- workarounds for ancient VHDL
  in_my_valid <=  to_bool(in_MCmd = ocp.MCmd_WRITE);
  my_reset    <= in_MReset_n or out_SReset_n;
  in_port : component ocpi.wsi.slave
    generic map(precise         => false,
                data_width      => in_width,
                data_info_width => 1,
                burst_width     => 2,
                n_bytes         => 1,
                opcode_width    => 8,
                own_clock       => false)
    port map   (Clk              => ctl_Clk,
                MBurstLength     => in_MBurstLength,
                MByteEn          => (others => '1'),
                MCmd             => in_MCmd,
                MData            => in_MData,
                MDataInfo        => (others => '0'),
                MDataLast        => in_MReqLast,
                MDataValid       => in_my_valid,
                MReqInfo         => in_MReqInfo,
                MReset_n         => in_MReset_n,
                SReset_n         => in_SReset_n,
                SThreadBusy      => in_SThreadBusy,
                wci_clk          => ctl_Clk,
                wci_reset        => in_MReset_n,
                wci_is_operating => btrue,
                -- are outputs to the "worker"
                reset            => in_reset,
                ready            => in_ready,
                som              => in_som,
                eom              => in_eom,
                valid            => in_valid,
                data             => in_data,
                abort            => open,
                byte_enable      => open,
                burst_length     => open,
                opcode           => out_opcode,
                take             => in_take);
  --
  -- The WSI interface helper component instance for port "out"
  out_port : component ocpi.wsi.master
    generic map(precise         => false,
                data_width      => out_width,
                data_info_width => 1,
                burst_width     => 2,
                n_bytes         => 1,
                opcode_width    => 8,
                own_clock       => false)
    port map   (Clk              => ctl_Clk,
                MBurstLength     => out_MBurstLength,
                MByteEn          => open,
                MCmd             => out_MCmd,
                MData            => out_MData,
                MDataInfo        => open,
                MDataLast        => open,
                MDataValid       => open,
                MReqInfo         => out_MReqInfo,
                MReqLast         => out_MReqLast,
                MReset_n         => out_MReset_n,
                SReset_n         => out_SReset_n,
                SThreadBusy      => out_SThreadBusy,
                wci_clk          => ctl_Clk,
                wci_reset        => my_reset,
                wci_is_operating => '1',
                reset            => open,
                ready            => out_ready,
                som              => out_som,
                eom              => out_eom,
                valid            => out_valid,
                data             => out_data,
                abort            => '0',
                byte_enable      => open,
                burst_length     => (others => '0'),
                opcode           => out_opcode,
                give             => out_give);

  -- now the inplementation
  out_ready <= in_ready;
  out_som <= in_som;
  out_eom <= in_eom;
  out_valid <= in_valid;
  out_data <= in_data;
  out_give <= in_ready and out_ready;
  in_take <= in_ready and out_ready;
end architecture rtl;

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wsi.all; use ocpi.util.all;

entity wsi_32_x_16 is
  port (
    ctl_Clk               : in  std_logic;
    in_MBurstLength       : in  std_logic_vector(1 downto 0);
    in_MCmd               : in  std_logic_vector(2 downto 0);
    in_MData              : in  std_logic_vector(32-1 downto 0);
    in_MReqInfo           : in  std_logic_vector(8-1 downto 0);
    in_MReqLast           : in  std_logic;
    in_MReset_n           : in  std_logic;
    in_SReset_n           : out std_logic;
    in_SThreadBusy        : out std_logic_vector(0 downto 0);
    out_SReset_n          : in  std_logic;
    out_SThreadBusy       : in  std_logic_vector(0 downto 0);
    out_MBurstLength      : out std_logic_vector(1 downto 0);
    out_MCmd              : out std_logic_vector(2 downto 0);
    out_MData             : out std_logic_vector(16-1 downto 0);
    out_MReqInfo          : out std_logic_vector(8-1 downto 0);
    out_MReqLast          : out std_logic;
    out_MReset_n          : out std_logic 
  );
end entity wsi_32_x_16;
architecture rtl of wsi_32_x_16 is
begin
  wsi32x16 : entity wsi_reduce
    generic map(in_width => 32, out_width => 16)
    port map(
      ctl_Clk => ctl_Clk,
      in_MBurstLength => in_MBurstLength,
      in_MCmd => in_MCmd,
      in_MData => in_MData,
      in_MReqInfo => in_MReqInfo,
      in_MReqLast => in_MReqLast,
      in_MReset_n => in_MReset_n,
      in_SReset_n => in_SReset_n,
      in_SThreadBusy => in_SThreadBusy,
      out_SReset_n => out_SReset_n,
      out_SThreadBusy => out_SThreadBusy,
      out_MBurstLength => out_MBurstLength,
      out_MCmd => out_MCmd,
      out_MData => out_MData,
      out_MReqInfo => out_MReqInfo,
      out_MReqLast => out_MReqLast,
      out_MReset_n => out_MReset_n
      );
end architecture rtl;
