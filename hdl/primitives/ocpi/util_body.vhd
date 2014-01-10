library ieee; use ieee.math_real.all;
package body util is
function width_for_max(n : natural) return natural is begin
  return natural(ceil(log2(real(n+1))));
end width_for_max;
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
end util;

