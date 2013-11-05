-- per-property decoder - purely combinatorial
-- result is write_enable, offset-in-array, and aligned data output
-- output data has full 31 bits when sub32 arrays are being written
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;

entity property_decoder is
  generic (
        property     : property_t;   -- property type
        decode_width : natural);     -- decoder width in bits
  port (
        reset        : in  bool_t;                            -- active-low WCI worker reset
        offset_in    : in  unsigned(decode_width-1 downto 0); -- offset in Bytes
        nbytes_1     : in  byte_offset_t;                     -- how many valid bytes
        is_read      : in  bool_t;
        is_write     : in  bool_t;
        data_in      : in  std_logic_vector(31 downto 0);     -- WCI master data
        write_enable : out bool_t;                            -- active-high write pulse
        read_enable  : out bool_t;                            -- active-high read pulse
        offset_out   : out unsigned(decode_width-1 downto 0); -- 
        index_out    : out unsigned(decode_width-1 downto 0); --
        data_out     : out std_logic_vector(31 downto 0)); --
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
  -- This is a function rather than a signal to avoid simulation issues
  -- when the combinatorial offset calculation is fed as an array reference
  function my_decode(reset : bool_t; offset : decode_t) return bool_t is
  begin
    return to_bool(not its(reset) and offset >= property.offset and
                   offset <= property.offset(decode_t'range) + property.bytes_1);
  end my_decode;
  begin
  my_offset    <= offset_in - property.offset(decode_width-1 downto 0)
                  when its(my_decode(reset, offset_in))
                  else (others => '0');
  byte_offset  <= offset_in(1 downto 0);
  write_enable <= to_bool(is_write and property.writable and my_decode(reset, offset_in));
  read_enable  <= to_bool(is_read and property.readable and my_decode(reset, offset_in));
  offset_out   <= my_offset;
  -- never on strings so no non-power of 2 math needed.
  index_out    <= my_offset/element_bytes(property);
  l32: if property.data_width >= 32 generate
   data_out <= data_in;
  end generate;
  l16: if property.data_width = 16 generate
   data_out(31 downto 16) <= data_in(31 downto 16);
   data_out(15 downto 0) <= data_in(31 downto 16) when nbytes_1 = 1 and byte_offset = 2 else
                            data_in(15 downto 0);
  end generate;
  l8: if property.data_width = 8 generate
   data_out(31 downto 16) <= data_in(31 downto 16);
   data_out(15 downto 8)  <= data_in(31 downto 24) when byte_offset = 2 else data_in(15 downto 8);
   data_out(7 downto 0)   <= data_in(31 downto 24) when byte_offset = 3 else
                             data_in(23 downto 16) when byte_offset = 2 else
                             data_in(15 downto  8) when byte_offset = 1 else
                             data_in( 7 downto  0);
  end generate;
end rtl;
