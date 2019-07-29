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

-- A WSI master
library ieee; use ieee.std_logic_1164.all, ieee.numeric_std.all, ieee.math_real.all;
library ocpi; use ocpi.all, ocpi.types.all, ocpi.wsi.all, ocpi.util.all;
entity master is
  generic (precise          : boolean; -- are we precise-only?
           mdata_width      : natural; -- width of mdata path
           mdata_info_width : natural; -- width of mdatainfo path
           burst_width      : natural; -- width of burst length
           n_bytes          : natural; -- number of bytes
           byte_width       : natural; -- byte_width
           opcode_width     : natural; -- bits in reqinfo
           own_clock        : boolean; -- does the port have a clock different than the wci?
           hdl_version      : natural; -- hdl interface version
           early_request    : boolean; -- are datavalid and datalast used?
           insert_eom       : boolean; -- create output messages of length message_length (a port)
           max_bytes        : natural; -- max possible supported message size in bytes
           max_latency      : natural := 255;  -- max supported latency value
           worker_eof       : boolean; -- worker will supply eof, so we shouldn't pass it (v2+)
           fixed_buffer_size: boolean;
           debug            : boolean
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
    MData            : out std_logic_vector(mdata_width-1 downto 0);
    MDataInfo        : out std_logic_vector(mdata_info_width-1 downto 0);
    MDataLast        : out std_logic;
    MDataValid       : out std_logic;
    MReqInfo         : out std_logic_vector(opcode_width-1 downto 0);
    MReqLast         : out std_logic;
    MReset_n         : out std_logic;
    -- Signals connected from the worker's WCI to this interface;
    wci_clk          : in  std_logic;
    wci_reset        : in  Bool_t;
    wci_is_operating : in  Bool_t;
    -- Non-worker internal signals
    first_take       : in  Bool_t;   -- the first datum after is_operating taken, a pulse
    input_eof        : in  Bool_t;   -- an EOF is pending from (the first) input port
    buffer_size      : in  UShort_t; -- the buffer size property
    latency          : out UShort_t; -- the measured latency
    --====== Signals to and from the worker=====
    -- only used if abortable
    abort            : in  Bool_t; -- message is aborted
    -- only used if precise is required
    burst_length     : in  std_logic_vector(burst_width-1 downto 0);
    -- only used if number of opcodes > 1
    opcode           : in  std_logic_vector(opcode_width-1 downto 0);
    eof              : in  Bool_t; -- eof from worker, false if not driven
    give             : in  Bool_t := bfalse;
    data             : in  std_logic_vector(n_bytes * max(1,byte_width)-1 downto 0);
    byte_enable      : in  std_logic_vector(n_bytes-1 downto 0) := (others => '1');
    som              : in  Bool_t := bfalse;
    eom              : in  Bool_t := bfalse;
    valid            : in  Bool_t := bfalse;
    reset            : out Bool_t;  -- this port is being reset from outside/peer
    ready            : out Bool_t); -- data can be given
end entity;
architecture rtl of master is
  constant data_width : natural := n_bytes * byte_width;
  constant count_width : natural := width_for_max(max(max_bytes,1)/(max(data_width,8)/8)-1);
  type state_t is (BEFORE_SOM_e, EARLY_SOM_e, AFTER_SOM_e, NEED_EOM_e, NEED_EOF_e, FINISHED_e);
  signal state_r : state_t;
  signal reset_i      : Bool_t; -- internal reset, in ports clock domain
  signal som_i        : Bool_t; -- internal som
  signal ocp_eom      : Bool_t; -- internal eom for ocp for various reasons
  signal early_som    : std_logic; -- som is being given without eom or valid
  signal ready_r      : Bool_t;
  signal eof_zlm      : Bool_t;
  signal eof_now      : Bool_t;
  signal input_eof_r  : Bool_t; -- always give the worker one cycle during eof propagation
  signal my_give      : Bool_t; -- internal version of: give or version>2 and valid
  signal ocp_give     : Bool_t;
  signal opcode_now   : std_logic_vector(opcode'range);
  signal opcode_r     : std_logic_vector(opcode'range);
  signal latency_r    : unsigned(width_for_max(max_latency)-1 downto 0);
  signal first_data_r : Bool_t; -- measuring latency
  signal last_data_r  : Bool_t; -- the last possible word in a message has been given.
  -- counter to check output length or insert eom automatically
  signal data_count_r : unsigned(count_width-1 downto 0);
  signal byte_limit   : unsigned(width_for_max(max_bytes)-1 downto 0);
  signal data_limit   : unsigned(count_width-1 downto 0);
  constant bs_shift   : natural := width_for_max(max(data_width,8)/8) - 1;
begin
  byte_limit <= to_unsigned(max_bytes, byte_limit'length) when fixed_buffer_size
                else resize(ocpi.util.min(to_ushort(max_bytes), buffer_size), byte_limit'length);
  data_limit <= resize((byte_limit srl bs_shift) - 1, data_limit'length);
  -- FIXME WHEN OWN CLOCK
  reset_i <= wci_reset or not SReset_n;
  reset   <= reset_i;
  ready   <= ready_r;
  latency <= resize(latency_r,latency'length);
  -- FIXME WHEN OWN CLOCK
  -- internal conveniences
  -- Version 2 allows valid only, and allows valid as well as give to lead ready, like AXI
  my_give    <= give when hdl_version < 2 else ready_r and (give or valid);
  som_i      <= to_bool(som or (state_r = before_som_e));
  opcode_now <= opcode when its(som_i) else opcode_r;
  ocp_eom    <= to_bool(state_r = NEED_EOF_E or state_r = NEED_EOM_E or
                        (data_count_r = data_limit and insert_eom) or
                        (eof and state_r = BEFORE_SOM_E) or
                        (eom and not its(eof_zlm)));
  -- A condition where we are not passing through to OCP, an early SOM without valid
  early_som  <= som and not valid and not eom;
  -- A condition where we are not passing through to OCP, an EOF ZLM
  eof_zlm    <= to_bool(hdl_version <= 1 and eom and not its(valid) and
                        opcode_now = slv0(opcode_now'length) and
                        (som or state_r = EARLY_SOM_e));
  -- The condition when we know we must do an eof, not necessarily sync'd to give
  eof_now    <= to_bool((input_eof_r and hdl_version > 1 and not worker_eof) or eof);
  -- The condition when we can issue an OCP command
  ocp_give   <= to_bool(state_r /= FINISHED_e and
                        ((my_give and not its(early_som) and not its(eof_zlm)) or
                         state_r = NEED_EOM_e or state_r = NEED_EOF_e));
  -- Outputs to OCP
  MReset_n   <= not wci_reset;
  MCmd       <= ocpi.ocp.MCmd_WRITE when ready_r and ocp_give else ocpi.ocp.MCmd_IDLE;
  MDataLast  <= ocp_eom;
  MReqLast   <= ocp_eom;
  MDataValid <= ocp_give;
  MByteEn    <= (others => '0') when state_r = NEED_EOM_e or state_r = NEED_EOF_e or not its(valid) else
                (others => valid) when n_bytes = 1 else
                byte_enable;
  MReqInfo   <= opcode_now;
  -- If there are parts of bytes in data_info_width, split them properly for OCP
  gen0: if mdata_info_width > 1 generate
    gen1: for i in 0 to n_bytes-1 generate
      MData(i*8+7 downto i*8) <= data(i*byte_width+7 downto i*byte_width);
      MDataInfo(i*(byte_width-8) + (byte_width-8)-1 downto i*(byte_width-8)) <=
        data(i*byte_width + byte_width-1 downto i*byte_width + byte_width - (byte_width - 8));
    end generate gen1;
  end generate gen0;
  -- If there are no partial bytes in datainfo, the worker's data is just MData.
  gen2: if mdata_info_width <= 1 and data_width > 0 generate
    MData <= data;
  end generate gen2;
  MDataInfo(MDataInfo'left) <= to_bool(abort or state_r = NEED_EOF_e or
                                       (hdl_version > 1 and state_r = BEFORE_SOM_E and eof));
  process(Clk) is
    -- procedure to deal with the worker giving data
    procedure do_data is
    begin
      state_r <= AFTER_SOM_e;
      if its(last_data_r) then
        report "Output message, " & integer'image(to_integer(data_count_r)) &
          ", has exceeded the maximum size";
        report "  Data width " & integer'image(data_width) &
          ", implementation max bytes " &  integer'image(max_bytes) &
          ", configured max bytes " &  integer'image(to_integer(buffer_size))
          severity failure;
      elsif its(ocp_eom) then
        data_count_r <= (others => '0');
        last_data_r <= bfalse;
        state_r <= BEFORE_SOM_e;
      elsif data_count_r = data_limit then
        last_data_r <= btrue;
      else
        data_count_r <= data_count_r + 1;
      end if;
    end procedure do_data;
    -- procedure when you are doing the first thing in a message, maybe after an early SOM
    procedure do_som is
    begin
      if its(valid) then
        do_data; -- can go to AFTER_SOM or BEFORE_SOM
      elsif its(eof_zlm) then
        state_r <= NEED_EOF_e;
      else -- must be zlm that is not converted to EOF
        state_r <= BEFORE_SOM_e;
      end if;
    end procedure;
  begin
    if rising_edge(Clk) then
      if its(reset_i) then
        if (its(wci_reset) or state_r /= FINISHED_e) then
          state_r      <= BEFORE_SOM_e;
        end if;
        ready_r      <= bfalse;
        opcode_r     <= (others => '0'); -- perhaps unnecessary, but supresses a warning
        latency_r    <= (others => '0');
        first_data_r <= bfalse;
        last_data_r  <= bfalse;
        data_count_r <= (others => '0');
        input_eof_r  <= bfalse;
      else
        ready_r <= (wci_is_operating or eof_now) and not SThreadBusy(0); -- for next cycle.  OCP pipelining
        if ready_r and its(first_take) and not its(my_give) then -- start latency measurement
          first_data_r <= btrue;
        end if;
        if its(ready_r) then
          input_eof_r <= input_eof;
          if my_give and not its(som or valid or eom or abort or eof) then
            report "Illegal message metadata: no SOM or VALID or EOM" severity failure;
          end if;
          if its(my_give) then
            first_data_r <= bfalse;
            if som and (state_r = EARLY_SOM_e or state_r = AFTER_SOM_e) then
              report "Illegal SOM after SOM with no EOM" severity failure;
            end if;
            case state_r is
              when BEFORE_SOM_e => -- between messages
                opcode_r <= opcode;
                if its(early_som) then
                  state_r <= EARLY_SOM_e;
                elsif hdl_version > 1 and eof then -- allow v2+ to "give" eof
                  state_r <= FINISHED_e;
                else
                  do_som;
                end if;
              when EARLY_SOM_e => -- after an early SOM
                do_som;
              when AFTER_SOM_e =>
                if its(valid) then
                  do_data;
                else
                  data_count_r <= (others => '0');
                  last_data_r <= bfalse;
                  state_r <= BEFORE_SOM_e; -- trailing eom
                end if;
              when NEED_EOF_e => -- ignoring give here
                state_r <= FINISHED_e;
              when NEED_EOM_e => -- ignoring give here
                state_r <= NEED_EOF_e;
              when FINISHED_e =>
                null;
            end case;
          else -- not giving
            case state_r is
              when BEFORE_SOM_e => -- between messages
                if its(eof_now) then
                  state_r <= NEED_EOF_e;
                end if;
              when AFTER_SOM_E =>
                if its(eof_now) then
                  if insert_eom then
                    state_r <= NEED_EOM_e; -- we need to close off the last message
                  else
                    state_r <= NEED_EOF_e;
                  end if;
                end if;
              when NEED_EOM_e =>
                if its(ready_r) then
                  state_r <= NEED_EOF_e;
                end if;
              when NEED_EOF_e =>
                if its(ready_r) then
                  state_r <= FINISHED_e;
                end if;
              when others =>
                null;
            end case;
            -- we are measuring latency so count while not giving
            if its(first_data_r) then -- if not giving, count latency
              if latency_r /= max_latency then
                if latency_r = max_latency-1 then
                  report "measured latency exceeds max latency parameter";
                end if;
                latency_r <= latency_r + 1;
              else
              end if;
            end if;
          end if; -- not giving
        end if; -- not ready_r
      end if; -- not reset
    end if; -- rising edge
  end process;

  -- For precise bursts we capture the length at som
  -- and then decrement on non-first data-valid.
  -- assumption is eom will be coincident with valid.
  l1: if precise generate
    signal my_burst_length : std_logic_vector(MBurstLength'range);
  begin
    burst: process(Clk) is
    begin
      if rising_edge(clk) and my_give and ready_r and som_i then
        my_burst_length <= burst_length;
      end if;
    end process;
    MBurstLength <= burst_length when its(som_i) else my_burst_length;
  end generate;
  l2: if not precise generate
  begin
    MBurstLength <=
      std_logic_vector(to_unsigned(1, MBurstLength'length)) when its(ocp_eom)
      else std_logic_vector(to_unsigned(2, MBurstLength'length));
  end generate;
end architecture rtl;
