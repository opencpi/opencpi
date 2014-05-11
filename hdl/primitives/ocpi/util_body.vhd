library ieee; use ieee.math_real.all;
package body util is
function width_for_max(n : natural) return natural is begin
  return natural(ceil(log2(real(n+1))));
end width_for_max;
function slv(v : std_logic_vector) return std_logic_vector is
begin
  return v;
end slv;
function slv(b : std_logic) return std_logic_vector is
  variable v : std_logic_vector(0 downto 0) := (others => b);
begin
  return v;
end slv;
function slv0(n : natural) return std_logic_vector is
  variable v : std_logic_vector(n-1 downto 0) := (others => '0');
begin
  return v;
end slv0;
function slvn(n, width : natural) return std_logic_vector is
  variable v : unsigned(width-1 downto 0) := to_unsigned(n, width);
begin
  return std_logic_vector(v);
end slvn;
function bit2unsigned(b : std_logic; len : natural := 1)
  return unsigned is
begin
  if b = '0' then
    return to_unsigned(0, len);
  else
    return to_unsigned(1, len);
  end if;
end bit2unsigned;
function swap(d : dword_t) return dword_t is
begin
  return dword_t'(d(7 downto 0) & d(15 downto 8) & d(23 downto 16) & d(31 downto 24));
end swap;
end util;

