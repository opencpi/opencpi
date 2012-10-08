library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all;
package body wci is

-- convert byte enables to byte offsets

function decode_access(input : in_t) return access_t is begin
  case input.MCmd is
    when ocp.MCmd_WRITE => if input.MAddrSpace(0) = '1' then return write_e; else return Error_e; end if;
    when ocp.MCmd_READ  => if input.MAddrSpace(0) = '1' then return Read_e; else return Control_e; end if;
    when others => return None_e;
  end case;
end decode_access;


--function "=" (l,r: Property_Io_t) return boolean is begin
--  return Property_io_t'pos(l) = Property_io_t'pos(r);
--end "=";
-- return property access specific to this offset and size and address width
-- the basic decode is redundant across properties, but should be optimized anyawy
--function decode_property (input : in_t; low, high : unsigned) return property_access_t is
--  variable io : property_io_t := config_access(input);
--  variable moffset : unsigned (low'left downto 0)
--    := unsigned(input.MAddr(low'left downto 2) & be2offset(input));
--begin
--  if io /= None_e and moffset >= low and moffset <= high then
--    return property_access_t'(io, moffset - low);
--  end if;
--  return property_access_t'(None_e, property_offset_t'(others => '0'));
--end decode_property;

function get_value(input : in_t; boffset : unsigned; width : natural) return std_logic_vector is
  variable bitoffset : natural := to_integer(boffset & "000");
  variable bitwidth  : natural := width;
begin
  if bitwidth > 32 then bitwidth := 32; end if;
  return input.MData(bitoffset + bitwidth - 1 downto bitoffset);
end get_value;     

function to_control_op(bits : std_logic_vector(2 downto 0)) return control_op_t is
begin
--this fine in VHDL, but not in XST
--return control_op_t'val(to_integer(unsigned(bits)));
  case to_integer(unsigned(bits)) is
    when control_op_t'pos(initialize_e)   => return initialize_e;
    when control_op_t'pos(start_e)        => return start_e;
    when control_op_t'pos(stop_e)         => return stop_e;
    when control_op_t'pos(release_e)      => return release_e;
    when control_op_t'pos(before_query_e) => return before_query_e;
    when control_op_t'pos(after_config_e) => return after_config_e;
    when control_op_t'pos(test_e)         => return test_e;
    when others                           => return no_op_e;
  end case;
end to_control_op;

-- How wide should the data path be from the decoder to the property
function data_out_top (property : property_t) return natural is
begin
  if property.data_width >= 32 or property.nitems > 1 then
    return 31;
  else
    return property.data_width - 1;
  end if;
end data_out_top;

function resize(bits : std_logic_vector; n : natural) return std_logic_vector is begin
  return std_logic_vector(resize(unsigned(bits),n));
end resize;


end wci;

-- per-property decoder - purely combinatorial
-- result is write_enable, offset-in-array, and aligned data output
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;

entity property_decoder is
  generic (
        property     : property_t;   -- property type
        decode_width : natural);     -- decoder width in bits
  port (
        reset_n      : in std_logic;                           -- active-low WCI worker reset
        offset_in    : in unsigned(decode_width-1 downto 0);   -- offset in Bytes
        top          : in natural;                             -- High order bit position of datatype
        access_in    : in access_t;                            -- Enumerated WCI access type
        data_in      : in std_logic_vector(31 downto 0);       -- WCI slave data
        write_enable : out boolean;                            -- active-high write pulse
        read_enable  : out boolean;                            -- active-high read pulse
        offset_out   : out unsigned(decode_width-1 downto 0);  -- 
        index_out    : out unsigned(decode_width-1 downto 0);  --
        data_out     : out std_logic_vector(data_out_top(property) downto 0)); --
end entity property_decoder;

architecture rtl of property_decoder is
  subtype decode_t is unsigned (decode_width-1 downto 0);
  signal my_offset : decode_t;
  signal byte_offset : byte_offset_t;
  function element_bytes(property : property_t) return decode_t is
    variable bytes : natural;
  begin
    if property.string_length /= 0 then
      bytes := ((property.string_length + 4)/4) * 4;
    else
      bytes :=  (property.data_width+7)/8;
    end if;
    return to_unsigned(bytes, decode_t'length);
  end element_bytes;
  impure function my_decode (property : property_t) return boolean is
  begin
    return (Reset_n = '1' and 
            offset_in >= property.offset and
            (my_offset = 0 or
             (property.data_width > 32 and my_offset = 4) or
             (property.nitems > 1 and my_offset > 0 and my_offset <= property.bytes_1)));
  end my_decode;
begin
  byte_offset  <= offset_in(1 downto 0);
  my_offset    <= offset_in - property.offset;
  write_enable <= access_in = write_e and property.writable and my_decode(property);
  read_enable  <= access_in = read_e and property.readable and my_decode(property);
  offset_out   <= (others => '0') when property.nitems <= 1 else my_offset;
  data_out <=
    data_in(data_out_top(property) downto 0)        when property.nitems     <= 1  else
    data_in(31 downto 0)                            when property.data_width >= 32 else
    resize(data_in(top downto 0),  data_out'length) when byte_offset = 0           else
    resize(data_in(15 downto 8),   data_out'length) when byte_offset = 1           else
    resize(data_in(top downto 16), data_out'length) when byte_offset = 2           else
    resize(data_in(31 downto 24),  data_out'length);
  index_out <=
    to_unsigned(0,index_out'length) when property.nitems <= 1 else
    my_offset/element_bytes(property);  -- this won't be used for strings so no
                                        -- non-power of 2 math needed.
end rtl;

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;

entity decoder is
  generic (
      worker                 : worker_t;
      properties             : properties_t);
  port (
      ocp_in                 : in in_t;       
      done                   : in boolean := true;
      resp                   : out ocp.SResp_t;
      write_enables          : out boolean_array_t(properties'range);
      read_enables           : out boolean_array_t(properties'range);
      offsets                : out offset_a_t(properties'range);
      indices                : out offset_a_t(properties'range);
      hi32                   : out boolean;
      nbytes_1               : out byte_offset_t;
      data_outputs           : out data_a_t(properties'range);
      control_op             : out control_op_t;
      state                  : out state_t;
      is_operating           : out boolean;  -- just a convenience for state = operating_e
      abort_control_op       : out boolean;
      is_big_endian          : out boolean   -- for runtime dynamic endian
      );
end entity;

architecture rtl of decoder is
  signal beoffset  : unsigned(1 downto 0);
  signal offset    : unsigned(worker.decode_width-1 downto 0);
  signal my_access : access_t;
  signal control_op_in : control_op_t;
  signal my_write_enables, my_read_enables : boolean_array_t(properties'range);
  signal my_control_op : control_op_t;
  type my_offset_a_t is array(properties'range) of unsigned (worker.decode_width -1 downto 0);
  signal my_offsets : my_offset_a_t;
  signal my_state : state_t;
  -- convert byte enables to low order address bytes
  function be2offset(input: in_t) return byte_offset_t is begin
    case input.MByteEn is
      when b"0010" =>           return b"01";
      when b"0100" | b"1100" => return b"10";
      when b"1000" =>           return b"11";
      when others =>            return b"00";
    end case;
  end be2offset;
  function top_bit(input: in_t) return natural is begin
    case input.MByteEn is
      when b"0001" =>                     return 7;
      when b"0010" | b"0011" =>           return 15;
      when b"0100" =>                     return 23;
      when b"1000" | b"1100" | b"1111" => return 31;
      when others =>                      return 31;                                    
    end case;
  end top_bit;
  function num_bytes_1(input : in_t) return byte_offset_t is begin
    case input.MByteEn is
      when b"0001" | b"0010" | b"0100" | b"1000" => return b"00";
      when b"1100" | b"0011" =>                     return b"01";
      when b"1111" =>                               return b"11";                               
      when others =>                                return b"11";                               
    end case;
  end num_bytes_1;
begin
  -- combinatorial signals used in various places
  state <= my_state;
  nbytes_1 <= num_bytes_1(ocp_in);
  is_big_endian <= ocp_in.MFlag(1) = '1';
  abort_control_op <= ocp_in.MFlag(0) = '1';
  beoffset <= be2offset(ocp_in);
  offset <= unsigned(ocp_in.MAddr(worker.decode_width-1 downto 2)) & beoffset;
  my_access <= decode_access(ocp_in);
  control_op_in <= ocpi.wci.to_control_op(ocp_in.MAddr(4 downto 2));
  hi32 <= ocp_in.MAddr(2) = '1';
  -- generate property instances for each property
  -- they are all combinatorial by design
  gen: for i in 0 to properties'length-1 generate -- properties'left to 0 generate
    prop: entity ocpi.property_decoder
      generic map (properties(i), worker.decode_width)
      port map(ocp_in.MReset_n, offset, top_bit(ocp_in), my_access, ocp_in.MData,
               my_write_enables(i), my_read_enables(i), my_offsets(i),
               indices(i)(worker.decode_width-1 downto 0),
               data_outputs(i)(data_out_top(properties(i)) downto 0));
    offsets(i) <= resize(my_offsets(i),offsets(i)'length); -- resize to 32 bits for VHDL language reasons
  end generate gen;
  
  write_enables <= my_write_enables;
  read_enables <= my_read_enables;
  control_op <= my_control_op;
  -- manage state during control ops and manage the WCI/OCP SResp.
  -- remember that since we have no SCmdAccept signal, any command is only
  -- valid for one clock, but is finished when we assert SResp non-NULL
  reg: process(ocp_in.Clk) is
    -- a mask of allowable operations in the current state
    variable allowed_ops : control_op_mask_t;
    variable next_op : control_op_t;
    -- FIXME check that this synthesizes properly - may have to revert to logic...
    function any_true(bools : boolean_array_t) return boolean is
       variable result: boolean := false;
    begin
      for i in bools'range loop
        if bools(i) then result := true; end if;
      end loop;
      return result;
    end any_true;
  begin
    if rising_edge(ocp_in.Clk) then
      -- default value of the SResp output, which is a register
      resp <= ocp.SResp_NULL;
      if ocp_in.MReset_n = '0' then
        is_operating <= false;
        my_control_op <= NO_OP_e;
        if worker.allowed_ops(control_op_t'pos(initialize_e)) = '1' then
          my_state <= exists_e;
        else
          my_state <= initialized_e;
        end if;
        allowed_ops := next_ops(state_t'pos(my_state));
      elsif my_control_op /= NO_OP_e then
        if done then                   -- FIXME done should also control config i/o
          -- finish the control by setting the state
          is_operating <= false;
          case my_control_op is
            when INITIALIZE_e =>
              my_state <= initialized_e;
            when START_e => null;
              my_state <= operating_e;
              is_operating <= true;
            when STOP_e => null;
              my_state <= suspended_e;
            when RELEASE_e => null;
              my_state <= unusable_e;
            when others => null;                            
          end case;
        end if;
      else
        case my_access is
          when Error_e =>
            resp <= ocp.SResp_ERR;          -- we don't support read yet
          when Read_e =>
            if any_true(my_read_enables) then
              resp <= ocp.SResp_DVA;        -- assume there is no delay for property capture
            else
              resp <= ocp.SResp_ERR;        -- a write that no property accepted...
            end if;
          when Write_e =>
            if any_true(my_write_enables) then
              resp <= ocp.SResp_DVA;        -- assume there is no delay for property capture
            else
              resp <= ocp.SResp_ERR;        -- a write that no property accepted...
            end if;
          when Control_e =>
            if my_control_op /= no_op_e or                   -- prevented by control plane
               worker.allowed_ops(control_op_t'pos(control_op_in)) = '0' or -- prevented by software
               next_ops(state_t'pos(my_state))(control_op_t'pos(control_op_in)) = '0'
            then    -- prevented by software
              -- This would only happen if the control plane or software is broken
              resp <= ocp.SResp_ERR;
            else
              my_control_op <= control_op_in;
            end if;   
          when None_e => null;
        end case;
      end if;
    end if;
  end process;
end rtl;

--
-- A combinatorial readback multiplexer for those properties that have readback
-- 
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;
entity readback is
  generic (properties : properties_t);
  port (
      read_enables : in boolean_array_t(properties'range);
      data_inputs  : in data_a_t(properties'range);
      data_output  : out std_logic_vector(31 downto 0)
      );
end entity readback;

-- This implementation will behave and synthesize correctly; but due to 
-- its clock-less, combinatorial structure, may fail to close timing for
-- higher clock frequencies and/or a large number of properties...
architecture rtl of readback is
  subtype index_t is natural range properties'range;
  function first_true(bools : boolean_array_t(properties'range)) return index_t is
  begin
    for i in 0 to properties'length-1 loop
      if bools(i) then return i; end if;
    end loop;
    return 0;
  end first_true;
begin
  data_output <= data_inputs(first_true(read_enables));  
end rtl;
