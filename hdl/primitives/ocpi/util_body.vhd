library ieee; use ieee.math_real.all;
package body util is
function width_for_max(n : natural) return natural is begin
  return natural(ceil(log2(real(n+1))));
end width_for_max;
end util;

