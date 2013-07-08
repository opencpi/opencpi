
-- A WSI master
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wsi.all; use ocpi.util.all;
entity master is
  generic (precise         : boolean; -- are we precise-only?
           data_width      : natural; -- width of data path
           data_info_width : natural; -- width of data path
           burst_width     : natural; -- width of burst length
           n_bytes         : natural; -- number of bytes
           byte_width      : natural; -- byte_width
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
    data             : in  std_logic_vector(byte_width*n_bytes-1 downto 0);
    byte_enable      : in  std_logic_vector(n_bytes-1 downto 0) := (others => '1');
    som, eom, valid  : in  Bool_t);
end entity;
architecture rtl of master is
  signal reset_i   : Bool_t; -- internal reset, in ports clock domain
  signal ready_i   : Bool_t;
  signal early_som : Bool_t; -- som has been given without eom or valid
  signal last_eom  : Bool_t; -- previous give was eom
  signal opcode_i  : std_logic_vector(opcode'range);
begin
  -- FIXME WHEN OWN CLOCK
  reset_i <= wci_reset or not SReset_n;
  reset <= reset_i;
  ready <= ready_i;
  -- FIXME WHEN OWN CLOCK
  MReset_n <= not wci_reset;
  MCmd <= ocpi.ocp.MCmd_WRITE when its(give) and ready_i and not its(early_som) else ocpi.ocp.MCmd_IDLE;
  -- If there are parts of bytes in data_info_width, split them properly for OCP
  gen0: if data_info_width > 1 generate
    gen1: for i in 0 to n_bytes-1 generate
      MData(i*8+7 downto i*8) <= data(i*byte_width+7 downto i*byte_width);
      MDataInfo(i*(byte_width-8) + (byte_width-8)-1 downto i*(byte_width-8)) <=
        data(i*byte_width + byte_width-1 downto i*byte_width + byte_width - (byte_width - 8));
    end generate gen1;
  end generate gen0;
  -- If there are no partial bytes in datainfo, the worker's data is just MData.
  gen2: if data_info_width <= 1 generate
    MData <= data;
  end generate gen2;
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
        ready_i <= bfalse;
        last_eom <= bfalse;
        early_som <= bfalse;
        opcode_i <= (others => '0'); -- perhaps unnecessary, but supresses a warning
      else
        ready_i <= wci_is_operating and not SThreadBusy(0);
      end if;
      if give and ready_i then -- prevent the worker from giving when not ready
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
