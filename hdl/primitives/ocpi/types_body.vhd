-- Various re-usable functions relating to property data type support.
--library ocpi;
--use ocpi.wci.all;
library ieee;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use std.TextIO.all;
library fixed_float; use fixed_float.all;

package body types is
function bit2vec(b : std_logic; n : natural) return std_logic_vector is
  variable r : std_logic_vector(n-1 downto 0);
begin
  r := (others => '0');
  r(0) := b;
  return r;
end bit2vec;                                                                          
-- THESE ARE DEFINITIONS WHEN Bool_t is std_logic
function its(b : bool_t) return boolean is begin return b = '1'; end;
--this is not here - use bool_t(..), since it can be ambiguous in many places.
function To_bool(b : std_logic) return Bool_t is begin return b; end to_bool;
function To_bool(b : std_logic_vector) return Bool_t is begin return b(0); end to_bool;
function To_bool(b : boolean) return Bool_t is begin if b then return '1'; else return '0'; end if; end;
function from_bool(b : bool_t) return std_logic_vector is begin
if b = '1' then
  return std_logic_vector'(b"1");
else
  return std_logic_vector'(b"0");
end if;
return std_logic_vector'("U");
end from_bool;                                                            
function slv(a: bool_array_t) return std_logic_vector is
  variable v: std_logic_vector(a'length  - 1 downto 0);
begin
  for i in 0 to a'right loop
    v(a'length - i - 1) := a(i);
  end loop;
  return v;
end slv;
function to_slv(a: bool_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_bool_array(a: std_logic_vector) return bool_array_t is
  variable v: bool_array_t(0 to a'length-1);
begin
  for i in 0 to v'right loop
    v(i) := a(a'length-i-1);
  end loop;
  return v;
end to_bool_array;  
function "and"  ( l : bool_t; r : bool_t ) return boolean is begin return its(l) and its(r); end;
function "nand" ( l : bool_t; r : bool_t ) return boolean is begin return its(l) nand its(r); end;
function "or"   ( l : bool_t; r : bool_t ) return boolean is begin return its(l) or its(r); end;
function "nor"  ( l : bool_t; r : bool_t ) return boolean is begin return its(l) nor its(r); end;
function "xor"  ( l : bool_t; r : bool_t ) return boolean is begin return its(l) xor its(r); end;
function "xnor" ( l : bool_t; r : bool_t ) return boolean is begin return its(l) xnor its(r); end;

function "and"  ( l : bool_t; r : boolean ) return boolean is begin return its(l) and r; end;
function "nand" ( l : bool_t; r : boolean ) return boolean is begin return its(l) nand r; end;
function "or"   ( l : bool_t; r : boolean ) return boolean is begin return its(l) or r; end;
function "nor"  ( l : bool_t; r : boolean ) return boolean is begin return its(l) nor r; end;
function "xor"  ( l : bool_t; r : boolean ) return boolean is begin return its(l) xor r; end;
function "xnor" ( l : bool_t; r : boolean ) return boolean is begin return its(l) xnor r; end;

function "and"  ( l : boolean; r : bool_t ) return boolean is begin return l and its(r); end;
function "nand" ( l : boolean; r : bool_t ) return boolean is begin return l nand its(r); end;
function "or"   ( l : boolean; r : bool_t ) return boolean is begin return l or its(r); end;
function "nor"  ( l : boolean; r : bool_t ) return boolean is begin return l nor its(r); end;
function "xor"  ( l : boolean; r : bool_t ) return boolean is begin return l xor its(r); end;
function "xnor" ( l : boolean; r : bool_t ) return boolean is begin return l xnor its(r); end;

--function "or"   ( l : bool_t; r : boolean ) return bool_t is begin return to_bool(to_boolean(l) or r); end;

function "not"  ( l : bool_t             ) return boolean is begin return not its(l); end;

-- THESE ARE DEFINITIONS WHEN Bool_t is BOOLEAN
--function its(b : bool_t) return boolean is begin return b; end;
--function To_bool(b : std_logic) return Bool_t is begin return b = '1'; end to_bool;
--function To_bool(b : std_logic_vector) return Bool_t is begin return b(0) = '1'; end to_bool;
--function To_bool(b : boolean) return Bool_t is begin return b; end;
--function from_bool(b : bool_t) return std_logic_vector is begin
--  if b then return std_logic_vector'(b"1"); else return std_logic_vector'(b"0"); end if;
--end from_bool;                                                            
--function from_bool(b : bool_t) return std_logic is begin
--if b then return '1'; else return '0'; end if;
--end from_bool;                                                            

-- THESE ARE Bool_t related definitions independent of whether bool_t is boolean or std_logic
function btrue return bool_t is begin return to_bool(true); end;
function bfalse return bool_t is begin return to_bool(false); end;
function To_boolean(b : bool_t) return boolean is begin return its(b); end to_boolean;
function from_bool_array(ba : bool_array_t;
                         offset, nbytes_1, byte_offset : unsigned;
                         is_big_endian : boolean) return dword_t is
  variable my_offset : unsigned(offset'range) := offset;
  variable my_byte_offset : unsigned(byte_offset'range) := byte_offset;
  variable my_nbytes_1 : unsigned(nbytes_1'range) := nbytes_1;
  variable last_index  : unsigned(offset'range);
  variable last_offset : unsigned(byte_offset'range);
  variable v : std_logic_vector(0 to 3) := (others => '0');
begin
  if my_offset >= ba'right then
    my_offset := (others => '0');
  end if;
  if my_nbytes_1 >= ba'right then
    my_nbytes_1 := (others => '0');
  end if;
  if my_nbytes_1 > (3 - byte_offset)  then
    my_nbytes_1 := (others => '0');
  end if;
--  report "from_bool1" &
--    " offset " & integer'image(to_integer(offset)) &
--    " nbytes_1 " & integer'image(to_integer(nbytes_1)) &
--    " byte_offset " & integer'image(to_integer(byte_offset));
  last_index  := resize(my_offset + my_nbytes_1, offset'length);
--  report "from_bool2" &
--    " last_index " & integer'image(to_integer(last_index));
  if last_index > ba'right then
    last_index := (others => '0');
  end if;
  last_offset := resize(my_byte_offset + (last_index - my_offset), last_offset'length);
--  report "from_bool3" &
--    " last_index " & integer'image(to_integer(last_index)) &
--    " last_offset " & integer'image(to_integer(last_offset));
  for i in 0 to 3 loop
--    report "from_bool4 " & integer'image(i);
    if i >= my_byte_offset and i <= last_offset then
      v(i) := ba(to_integer(my_offset + (i - my_byte_offset)));
    end if;
  end loop;
  return (0 => v(0),
          8 => v(1),
          16 => v(2),
          24 => v(3),
          others => '0');
--  return result;
end from_bool_array;

-- char

function To_character (c : Char_t) return character is
begin
  return character'val(to_integer(unsigned(c)));
end to_character;
function To_char (c: Character) return char_t is
begin
  return to_signed(character'pos(c),char_t'length);
end to_char;
function To_char (c: integer) return char_t is
begin
 return to_signed(c,char_t'length);
end to_char;
function from_char (c: char_t) return std_logic_vector is begin
  return std_logic_vector(c);
end from_char;                                                            
function slv(a: char_array_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 8) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 8 - 1 downto (a'length - i) * 8 - 8) := from_char(a(i));
  end loop;
  return v;
end slv;
function to_slv(a: char_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_char_array(a: std_logic_vector) return char_array_t is
  variable v: char_array_t(0 to a'length/8-1);
begin
  for i in 0 to v'right loop
    v(i) := char_t(a(a'length-i*8-1 downto a'length-(i+1)*8));
  end loop;
  return v;
end to_char_array;  

-- short

function To_short (c: integer) return short_t is
begin
 return to_signed(c,short_t'length);
end to_short;
function from_short (c: short_t) return std_logic_vector is begin
  return std_logic_vector(c);
end from_short;                                                            
function slv(a: short_array_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 16) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 16 - 1 downto (a'length - i) * 16 - 16) := from_short(a(i));
  end loop;
  return v;
end slv;
function to_slv(a: short_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_short_array(a: std_logic_vector) return short_array_t is
  variable v: short_array_t(0 to a'length/16-1);
begin
  for i in 0 to v'right loop
    v(i) := short_t(a(a'length-i*16-1 downto a'length-(i+1)*16));
  end loop;
  return v;
end to_short_array;  

-- long

function To_long (c: integer) return long_t is
begin
 return to_signed(c,long_t'length);
end to_long;
function from_long (c: long_t) return std_logic_vector is begin
  return std_logic_vector(c);
end from_long;                                                            
function slv(a: long_array_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 32) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 32 - 1 downto (a'length - i) * 32 - 32) := from_long(a(i));
  end loop;
  return v;
end slv;
function to_slv(a: long_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_long_array(a: std_logic_vector) return long_array_t is
  variable v: long_array_t(0 to a'length/32-1);
begin
  for i in 0 to v'right loop
    v(i) := long_t(a(a'length-i*32-1 downto a'length-(i+1)*32));
  end loop;
  return v;
end to_long_array;  

-- uchar

function To_uchar (c: natural) return uchar_t is
begin
 return to_unsigned(c,uchar_t'length);
end to_uchar;
function To_uchar (c: std_logic_vector(uchar_t'range)) return uchar_t is
begin
 return unsigned(c);
end to_uchar;
function from_uchar (c: uchar_t) return std_logic_vector is begin
  return std_logic_vector(c);
end from_uchar;                                                            
function slv(a: uchar_array_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 8) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 8 - 1 downto (a'length - i) * 8 - 8) := from_uchar(a(i));
  end loop;
  return v;
end slv;
function to_slv(a: uchar_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_uchar_array(a: std_logic_vector) return uchar_array_t is
  variable v: uchar_array_t(0 to a'length/8-1);
begin
  for i in 0 to v'right loop
    v(i) := uchar_t(a(a'length-i*8-1 downto a'length-(i+1)*8));
  end loop;
  return v;
end to_uchar_array;  

-- ushort

function To_ushort (c: natural) return ushort_t is
begin
 return to_unsigned(c,ushort_t'length);
end to_ushort;
function from_ushort (c: ushort_t) return std_logic_vector is begin
  return std_logic_vector(c);
end from_ushort;                                                            
function slv(a: ushort_array_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 16) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 16 - 1 downto (a'length - i) * 16 - 16) := from_ushort(a(i));
  end loop;
  return v;
end slv;
function to_slv(a: ushort_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_ushort_array(a: std_logic_vector) return ushort_array_t is
  variable v: ushort_array_t(0 to a'length/16-1);
begin
  for i in 0 to v'right loop
    v(i) := ushort_t(a(a'length-i*16-1 downto a'length-(i+1)*16));
  end loop;
  return v;
end to_ushort_array;  

-- ulong

function To_ulong (c: natural) return ulong_t is
begin
 return to_unsigned(c,ulong_t'length);
end to_ulong;
function To_ulong (c: std_logic_vector(31 downto 0)) return ulong_t is
begin
  return ulong_t(c);
end To_ulong;
function from_ulong (c: ulong_t) return std_logic_vector is begin
  return std_logic_vector(c);
end from_ulong;                                                            
function slv(a: ulong_array_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 32) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 32 - 1 downto (a'length - i) * 32 - 32) := from_ulong(a(i));
  end loop;
  return v;
end slv;
function to_slv(a: ulong_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_ulong_array(a: std_logic_vector) return ulong_array_t is
  variable v: ulong_array_t(0 to a'length/32-1);
begin
  for i in 0 to v'right loop
    v(i) := ulong_t(a(a'length-i*32-1 downto a'length-(i+1)*32));
  end loop;
  return v;
end to_ulong_array;  

-- longlong

function To_longlong (c: integer) return longlong_t is
begin
 return to_signed(c, longlong_t'length);
end to_longlong;
function to_longlong (c: long_t) return longlong_t is
begin
  return resize(c, longlong_t'length);
end to_longlong;
function to_longlong (c: std_logic_vector(longlong_t'range)) return longlong_t is
begin
  return longlong_t(c);
end to_longlong;
function from_longlong (c: longlong_t) return std_logic_vector is begin
  return std_logic_vector(c);
end from_longlong;                                                            
function slv(a: longlong_array_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 64) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 64 - 1 downto (a'length - i) * 64 - 64) := from_longlong(a(i));
  end loop;
  return v;
end slv;
function to_slv(a: longlong_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_longlong_array(a: std_logic_vector) return longlong_array_t is
  variable v: longlong_array_t(0 to a'length/64-1);
begin
  for i in 0 to v'right loop
    v(i) := longlong_t(a(a'length-i*64-1 downto a'length-(i+1)*64));
  end loop;
  return v;
end to_longlong_array;  

-- ulonglong

function To_ulonglong (c: natural) return ulonglong_t is
begin
 return to_unsigned(c, ulonglong_t'length);
end to_ulonglong;
function to_ulonglong (c: ulong_t) return ulonglong_t is
begin
  return resize(c, ulonglong_t'length);
end to_ulonglong;
function to_ulonglong (c: std_logic_vector(ulonglong_t'range)) return ulonglong_t is
begin
  return ulonglong_t(c);
end to_ulonglong;
function from_ulonglong (c: ulonglong_t) return std_logic_vector is begin
  return std_logic_vector(c);
end from_ulonglong;                                                            
function slv(a: ulonglong_array_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 64) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 64 - 1 downto (a'length - i) * 64 - 64) := from_ulonglong(a(i));
  end loop;
  return v;
end slv;
function to_slv(a: ulonglong_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_ulonglong_array(a: std_logic_vector) return ulonglong_array_t is
  variable v: ulonglong_array_t(0 to a'length/64-1);
begin
  for i in 0 to v'right loop
    v(i) := ulonglong_t(a(a'length-i*64-1 downto a'length-(i+1)*64));
  end loop;
  return v;
end to_ulonglong_array;  

--function To_char (c: std_logic_vector) return char_t is
--begin
--return char_t(c(7 downto 0));
--end to_char;

function to_string_dw(inword : dword_t) return wordstring_t is
begin
  return (char_t(inword( 7 downto  0)),
          char_t(inword(15 downto  8)),
          char_t(inword(23 downto 16)),
          char_t(inword(31 downto 24)));
end to_string_dw;

-- Note the length argument is the "stringlength" which does not include the null
function to_string(instring : string; length : natural) return string_t is
  variable s : string_t(0 to length);
begin
  if instring'length /= 0 then
    for i in instring'left to instring'right loop
      s(i - instring'left) := to_char(instring(i));
    end loop;
  end if;
  if instring'length < s'length then
    for i in instring'length to s'length-1 loop
      s(i) := to_char(0);
    end loop;
  end if;
  return s;
end to_string;
function to_string(v: std_logic_vector) return string_t is
  variable s : string_t(0 to v'length/8-1);
begin
  for i in 0 to s'right loop
    s(i) := char_t(v(v'left - i * 8 downto v'left - i * 8 - 7));
  end loop;
  return s;
end to_string;

function from_string(s : string_t; offset : unsigned; is_big_endian : bool_t) return dword_t is
begin 
  return from_string(s, to_integer(offset), its(is_big_endian));
end from_string;

function from_string(s : string_t; off : natural; is_big_endian : boolean) return dword_t is
  variable w : dword_t;
begin 
  w := (others => '0');
  if is_big_endian then
    w(31 downto 24) := std_logic_vector(s(off));
    if off+1 <= s'right then
      w(23 downto 16) := std_logic_vector(s(off+1));
      if off+2 <= s'right then
        w(15 downto 8) := std_logic_vector(s(off+2));
        if off+3 < s'right then
          w(7 downto 0) := std_logic_vector(s(off+3));
        end if;
      end if;
    end if;
  else
    w(7 downto  0) := std_logic_vector(s(off));
    if off+1 <= s'right then
      w(15 downto  8) := std_logic_vector(s(off+1));
      if off+2 <= s'right then
        w(23 downto 16) := std_logic_vector(s(off+2));
        if off+3 < s'right then
          w(31 downto 24) := std_logic_vector(s(off+3));
        end if;
      end if;
    end if;
  end if;
  return w;
end from_string;

function from_string(s : string_t) return std_logic_vector is
  variable v: std_logic_vector(s'length * 8 - 1 downto 0);
begin
  for i in 0 to s'right loop
    v((s'length - i) * 8 - 1 downto (s'length - i)*8 - 8) := from_char(s(i));
  end loop;
  return v;
end from_string;

function from_string(s : string_t) return string is
  variable v: string(1 to s'length-1);
begin
--  report "from_string " & integer'image(s'length);
  for i in 0 to s'right-1 loop
--    report "from_string1 " & integer'image(i) & " " & integer'image(to_integer(s(i)));
    v(i+1) := character'val(to_integer(unsigned(from_char(s(i)))));
  end loop;
  return v;
end from_string;

function slv(a : string_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 8) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 8 - 1 downto (a'length - i) * 8 - 8) := from_char(a(i));
  end loop;
  return v;
end slv;
function slv(a : string_array_t) return std_logic_vector is
  variable v: std_logic_vector(a'length(1) * a'length(2) * 8 - 1 downto 0);
begin
  for i in 0 to a'right(1) loop
   for j in 0 to a'right(2) loop
     v(a'length(1) * a'length(2)*8 - 1 - (i * a'length(2) + j) * 8 downto
       a'length(1) * a'length(2)*8 - 8 - (i * a'length(2) + j) * 8)
       := from_char(a(i,j));
   end loop;
  end loop;
  return v;
end slv;
function to_slv(a: string_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
-- length arg is strlen (not including null)
function to_string_array(a: std_logic_vector; length : natural) return string_array_t is
  variable v: string_array_t(0 to a'length/((length+1)*8) - 1, 0 to length);
begin
  for i in 0 to v'right(1) loop
   for j in 0 to v'right(2) loop
     v(i,j) := char_t(a(a'length-1 - (i*(length+1) + j)*8 downto
                         a'length-8 - (i*(length+1) + j)*8));
   end loop;
  end loop;
  return v;
end to_string_array;  
    
-- Conversions based on the ieee_proposed fixed/float libraries

function to_float(r: real) return float_t is
begin
  return float_pkg.to_slv(float_pkg.to_float(r, 8, 23));
end to_float;
function from_float(f: float_t) return real is
begin
  return float_pkg.to_real(float_pkg.to_float(f, 8, 23));
end from_float;
function from_float (f: float_t) return std_logic_vector is begin
  return std_logic_vector(f);
end from_float;                                                            
function from_float (f: float_t) return integer is begin
  return float_pkg.to_integer(float_pkg.to_float(f, 8, 23));
end from_float;                                                            
function slv(a: float_array_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 32) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 32 - 1 downto (a'length - i) * 32 - 32) := from_float(a(i));
  end loop;
  return v;
end slv;
function to_slv(a: float_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_float_array(a: std_logic_vector) return float_array_t is
  variable v: float_array_t(0 to a'length/32-1);
begin
  for i in 0 to v'right loop
    v(i) := float_t(a(a'length-i*32-1 downto a'length-(i+1)*32));
  end loop;
  return v;
end to_float_array;  

-- double

function to_double(r: real) return double_t is
begin
  return float_pkg.to_slv(float_pkg.to_float(r, 11, 52));
end to_double;
function from_double(d: double_t) return real is
begin
  return float_pkg.to_real(float_pkg.to_float(d, 11, 52));
end from_double;
function from_double (d: double_t) return std_logic_vector is begin
  return std_logic_vector(d);
end from_double;                                                            
function slv(a: double_array_t) return std_logic_vector is
  variable v: std_logic_vector((a'length * 64) - 1 downto 0);
begin
  for i in 0 to a'right loop
    v((a'length - i) * 64 - 1 downto (a'length - i) * 64 - 64) := from_double(a(i));
  end loop;
  return v;
end slv;
function to_slv(a: double_array_t) return std_logic_vector is
begin
  return slv(a);
end to_slv;
function to_double_array(a: std_logic_vector) return double_array_t is
  variable v: double_array_t(0 to a'length/64-1);
begin
  for i in 0 to v'right loop
    v(i) := double_t(a(a'length-i*64-1 downto a'length-(i+1)*64));
  end loop;
  return v;
end to_double_array;  
  
end types;
