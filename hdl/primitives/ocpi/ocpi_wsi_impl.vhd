-- These WSI master and slave modules have all possible signals that may be configured
-- It is expected that the generated code on both sides will be deal with the
-- external optionality (on the outside in plumbing to OCP, and on the inside, when plumbing
-- to the worker.

-- The only place the lack of signal optionality carries a cost is in the input/slave side
-- FIFO.  The "optional" signals are at least one bit wide.  The signals that could be zero
-- bits are "data", "datainfo", "burstlength", reqinfo", and "byteen", so in the worst case
-- the FIFO is 5 bits wider than it would be with custom generated code.
--------------------------------------------
-- A WSI slave can deal with either imprecise or precise, but it needs to know
-- whether it needs to convey precise to the worker.
-- so there are really just two modes: insist/assume precise, or deal with everything
-- All potential input ports are present here,
-- and widths of unneeded fields are 1
-- We assume MDataValid/MDataLast.
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wsi.all; use ocpi.util.all;

entity slave is
  generic (precise         : boolean; -- are we precise-only?
           data_width      : natural; -- width of data path
           data_info_width : natural; -- width of data info path
           burst_width     : natural; -- burst width
           n_bytes         : natural; -- number of bytes
           opcode_width    : natural; -- bits in reqinfo
           own_clock       : boolean; -- does the port have a clock different thanthe wci?
           early_request   : boolean  -- are datavalid and datalast used? 
           );
  port (
    -- Exterior OCP input/master signals
    --- this is the same as wci_clock unless metadata says it isn't
    Clk              : in  std_logic;
    --- only used if burst are precise
    MBurstLength     : in  std_logic_vector(burst_width - 1 downto 0);
    --- only used if bytesize < data width or zlm 
    MByteEn          : in  std_logic_vector(n_bytes - 1 downto 0);
    MCmd             : in  ocpi.ocp.MCmd_t;
    MData            : in  std_logic_vector(data_width-1 downto 0);
    --- only used for aborts or bytesize not 8 and less than datawidth
    MDataInfo        : in  std_logic_vector(data_info_width-1 downto 0);
    --- only used if the "early_request" option is selected.
    MDataLast        : in  std_logic;
    MDataValid       : in  std_logic;
    --- only used if number of opcodes > 1
    MReqInfo         : in  std_logic_vector(opcode_width-1 downto 0);
    MReqLast         : in  std_logic;
    MReset_n         : in  std_logic;
    -- Exterior OCP output/slave signals
    SReset_n         : out std_logic;
    SThreadBusy      : out std_logic_vector(0 downto 0);
    -- Signals connected from the worker's WCI to this module;
    wci_clk          : in  std_logic;
    wci_reset        : in  Bool_t;
    wci_is_operating : in  Bool_t;
    -- Interior signals used by worker logic
    take             : in  Bool_t; -- the worker is taking data
    reset            : out Bool_t; -- this port is being reset from outside/peer
    ready            : out Bool_t; -- data can be taken
    som, eom, valid  : out Bool_t;
    data             : out std_logic_vector(data_width-1 downto 0);
    -- only used if abortable
    abort            : out Bool_t; -- message is aborted
    -- only used if bytes are required (zlm or byte size < data width)
    byte_enable      : out std_logic_vector(n_bytes-1 downto 0);
    -- only used if precise is required
    burst_length     : out std_logic_vector(burst_width-1 downto 0);
    -- only used if number of opcodes > 1
    opcode           : out std_logic_vector(opcode_width-1 downto 0)
    );
end entity;

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wsi.all; use ocpi.util.all;
architecture rtl of slave is
  signal reset_i : Bool_t; -- internal version of output to worker
  signal reset_n   : Bool_t; -- internal assert-low reset (to avoid silly isim warning).

  constant fifo_width : natural :=
    data_width + 3 + data_info_width + n_bytes + burst_width + opcode_width;

  -- The bundle of signals that goes through the FIFO.  Packing and unpacking records isn't worth it.
  constant data_bits   : natural := fifo_width - 1;
  constant valid_bit   : natural := data_bits - data_width;
  constant som_bit     : natural := valid_bit - 1;
  constant eom_bit     : natural := som_bit - 1;
  constant abort_bit   : natural := eom_bit - 1;
  constant info_bits   : natural := eom_bit - 1;
  constant enable_bits : natural := info_bits - data_info_width;
  constant burst_bits  : natural := enable_bits - n_bytes;
  constant opcode_bits : natural := burst_bits - burst_width;
  function pack(data : std_logic_vector(data_width - 1 downto 0);
                valid, som, eom : std_logic;
                info : std_logic_vector(data_info_width - 1 downto 0);
                enable : std_logic_vector(n_bytes - 1 downto 0);
                burst : std_logic_vector(burst_width - 1 downto 0);
                opcode : std_logic_vector(opcode_width - 1 downto 0))
    return std_logic_vector is
  begin
    return data & valid & som & eom & info & enable & burst & opcode;
  end pack;

  -- output signals from the fifo
  signal fifo_full_n : std_logic;
  -- input signals to the fifo (packed via the "pack" function).
  signal fifo_valid, fifo_som, fifo_eom : bool_t;

  -- actual state of the interface - 2 FFs
  signal in_message : bool_t; -- State bit for in message or not
  signal last_was_eom : bool_t;   -- state bit for previous data word
  signal fifo_in, fifo_out : std_logic_vector(fifo_width - 1 downto 0);
  signal fifo_enq : std_logic;
  signal fifo_ready : bool_t;
begin
  -- Combi resets:
  --   We get wci reset and wsi peer reset (from master)
  --   We produce wsi peer reset (from slave)
  -- Worker sees reset if wci is doing it or we're not started yet or peer is reset
  reset_i <= wci_reset or not MReset_n; -- FIXME WHEN own_clock
  reset_n <= not reset_i;
  reset <= reset_i; -- in wci clock domain for now
  -- Pear sees reset if wci is doing it or we're not started
  SReset_n <= not wci_reset; -- FIXME WHEN OWN CLOCK

  -- pack fifo input
  fifo_in <= pack(MData,
                  fifo_valid,
                  fifo_som,
                  fifo_eom,
                  MDataInfo,
                  MByteEn,
                  MBurstLength,
                  MReqInfo);

  -- unpack fifo output
  data         <= fifo_out(data_bits downto valid_bit+1);
  valid        <= fifo_out(valid_bit) and fifo_ready;
  som          <= fifo_out(som_bit) and fifo_ready;
  eom          <= fifo_out(eom_bit) and fifo_ready;
  abort        <= fifo_out(abort_bit) and fifo_ready;
  byte_enable  <= fifo_out(enable_bits downto burst_bits+1);
  burst_length <= fifo_out(burst_bits downto opcode_bits+1);
  opcode       <= fifo_out(opcode_bits downto 0);
  ready        <= fifo_ready;
  -- fifo inputs
  fifo_valid <= to_bool(((early_request and to_bool(MDataValid)) or
                         ((not early_request and MCmd = ocpi.ocp.MCmd_WRITE))) and
                        MByteEn /= (MByteEn'range => '0'));
  fifo_som <= to_bool(MCmd = ocpi.ocp.MCmd_WRITE and
                      (not its(in_message) or last_was_eom));
  fifo_eom <= to_bool((early_request and to_bool(MDataLast)) or (not early_request and to_bool(MReqLast)));
  fifo_enq <= fifo_valid or fifo_som or fifo_eom;

  -- Instantiate and connect the FIFO
  fifo : FIFO2
    generic map(width                       => fifo_width)
    port    map(clk                         => Clk,
                rst                         => reset_n,
                d_in                        => fifo_in,
                enq                         => fifo_enq,
                full_n                      => fifo_full_n,
                d_out                       => fifo_out,
                deq                         => take,
                empty_n                     => fifo_ready,
                clr                         => '0');
  -- FIXME WHEN OWN CLOCK
  SThreadBusy(0) <= reset_i or not fifo_full_n or not wci_is_operating;

  -- Manage two little bits of state - are we inside a message and was the
  -- previous clock a last-word-in-a-message
  reg: process(Clk) is
  begin
    -- in_message may be on on every cycle with back-to-back messages
    if rising_edge(clk) then
      if its(reset_i) then
        in_message <= bfalse;
        last_was_eom <= bfalse;
      else
        if MCmd = ocpi.ocp.MCmd_WRITE then -- FIXME: pipelined input
          in_message <= btrue;
        elsif its(last_was_eom) then
          in_message <= bfalse;
        end if;
        last_was_eom <= fifo_eom; -- perhaps MDataLast is enough.
      end if;
    end if;
  end process;
end architecture rtl;


-- A WSI master
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wsi.all; use ocpi.util.all;
entity master is
  generic (precise         : boolean; -- are we precise-only?
           data_width      : natural; -- width of data path
           data_info_width : natural; -- width of data path
           burst_width     : natural; -- width of burst length
           n_bytes         : natural; -- number of bytes
           opcode_width    : natural; -- bits in reqinfo
           own_clock       : boolean; -- does the port have a clock different thanthe wci?
           early_request   : boolean  -- are datavalid and datalast used? 
           );
  port (
    -- Exterior OCP input/slave signals
    Clk              : in  std_logic; -- MIGHT BE THE SAME AS wci_clk
    SReset_n         : in  std_logic;
    SThreadBusy      : in  std_logic_vector(0 downto 0);
    -- Exterior OCP output/master signals
    MBurstLength     : out std_logic_vector(burst_width - 1 downto 0);
    MByteEn          : out std_logic_vector(n_bytes - 1 downto 0);
    MCmd             : out ocpi.ocp.MCmd_t;
    MData            : out std_logic_vector(data_width-1 downto 0);
    MDataInfo        : out std_logic_vector(data_info_width-1 downto 0);
    MDataLast        : out std_logic;
    MDataValid       : out std_logic;
    MReqInfo         : out std_logic_vector(opcode_width-1 downto 0);
    MReqLast         : out std_logic;
    MReset_n         : out std_logic;
    -- Signals connected from the worker's WCI to this interface;
    wci_clk          : in  std_logic;
    wci_reset        : in  Bool_t;
    wci_is_operating : in  Bool_t;
    -- Interior signals used by worker logic
    reset            : out Bool_t; -- this port is being reset from outside/peer
    ready            : out Bool_t; -- data can be given
    -- only used if abortable
    abort            : in  Bool_t; -- message is aborted
    -- only used if precise is required
    burst_length     : in  std_logic_vector(burst_width-1 downto 0);
    -- only used if number of opcodes > 1
    opcode           : in  std_logic_vector(opcode_width-1 downto 0);
    give             : in  Bool_t;
    data             : in  std_logic_vector(data_width-1 downto 0);
    byte_enable      : in  std_logic_vector(n_bytes-1 downto 0) := (others => '1');
    som, eom, valid  : in  Bool_t);
end entity;
architecture rtl of master is
  signal reset_i   : Bool_t; -- internal reset, in ports clock domain
  signal early_som : Bool_t; -- som has been given without eom or valid
  signal last_eom  : Bool_t; -- previous give was eom
  signal opcode_i  : std_logic_vector(opcode'range);
begin
  -- FIXME WHEN OWN CLOCK
  reset_i <= wci_reset or not SReset_n;
  reset <= reset_i;
  -- FIXME WHEN OWN CLOCK
  MReset_n <= not wci_reset;
  MCmd <= ocpi.ocp.MCmd_WRITE when give and not its(early_som) else ocpi.ocp.MCmd_IDLE;
  MData <= data;
  MDataLast <= give and eom;
  MReqLast <= give and eom;
  MDataValid <= give and (eom or valid or abort);
  MDataInfo(MDataInfo'left) <= abort;
  MByteEn <= byte_enable when its(valid) else (others => '0');
  MReqInfo <= opcode when last_eom or som else opcode_i;
  -- We need to manage the state at the start of the message
  process(Clk) is
  begin
    if rising_edge(Clk) then
      if its(reset_i) then
        ready <= bfalse;
        last_eom <= bfalse;
        early_som <= bfalse;
        opcode_i <= (others => '0'); -- perhaps unnecessary, but supresses a warning
      else
        ready <= wci_is_operating and not SThreadBusy(0);
      end if;
      if its(give) then
        if som or last_eom then
          if not its(valid) and not its(eom) then
            early_som <= btrue;
          else
            early_som <= bfalse; 
          end if;
          opcode_i <= opcode;
        end if;   
        last_eom <= eom;
      end if;
    end if;
  end process;

  -- For precise bursts we capture the length at som
  -- and then decrement on non-first data-valid.
  -- assumption is eom will be coincident with valid.
  l1: if precise generate 
    signal my_burst_length : std_logic_vector(MBurstLength'range);
  begin
    burst: process(Clk) is
    begin
      if rising_edge(clk) and give and som then
        my_burst_length <= burst_length;
      end if;
    end process;
    MBurstLength <= burst_length when its(som) else my_burst_length;
  end generate;
  l2: if not precise generate
  begin
    MBurstLength <=
      std_logic_vector(to_unsigned(1, MBurstLength'length)) when its(eom)
      else std_logic_vector(to_unsigned(2, MBurstLength'length));
  end generate;
end architecture rtl;
