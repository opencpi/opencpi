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

-- RAW arbitrator/multiplexor.  Allows for multiple users, servicing them round-robin
library IEEE, ocpi;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
use ocpi.types.all, ocpi.util.all, ocpi.all;
entity raw_arb is
  generic (
    nusers        : positive := 1
  );
  port (
    clk           : in  std_logic;
    reset         : in  bool_t;
    from_users    : in  wci.raw_prop_out_array_t(0 to nusers-1);
    to_users      : out wci.raw_prop_in_array_t(0 to nusers-1);
    from_device   : in  wci.raw_prop_in_t;
    to_device     : out wci.raw_prop_out_t;
    index         : out integer range 0 to nusers-1
  );
end entity raw_arb;
architecture rtl of raw_arb is
  subtype user_t is unsigned(width_for_max(nusers-1)-1 downto 0);
  subtype user_nat_t is natural range 0 to nusers-1;
  signal idx          : user_nat_t;
  signal mydone       : bool_t;
  signal who_has_it_r : user_t;
  signal active_r     : bool_t;
  signal present      : bool_array_t(0 to nusers-1);
  -- Find first present user
  function first_present(present : bool_array_t(0 to nusers-1)) return user_nat_t is
  begin
    for i in 0 to nusers-1 loop
      if its(present(i)) then
        return i;
      end if;
    end loop;
    return 0;
  end first_present;
  impure function next_user return user_nat_t is
  begin
    if who_has_it_r = nusers-1 then
      return 0;
    else
      return to_integer(who_has_it_r + 1);
    end if;
  end next_user;
  impure function any_ready return bool_t is
  begin
    for i in 0 to nusers-1 loop
      if present(i) and its(from_users(i).raw.is_read or from_users(i).raw.is_write) then
        return btrue;
      end if;
    end loop;
    return bfalse;
  end any_ready;
  impure function all_reset return bool_t is
  begin
    for i in 0 to nusers-1 loop
      if its(present(i) and not from_users(i).reset) then
        return bfalse;
      end if;
    end loop;
    return btrue;
  end all_reset;
begin
  idx          <= to_integer(who_has_it_r);
  index <= idx;
  g0: for i in 0 to nusers-1 generate
    present(i)          <= from_users(i).present;
    to_users(i).present(0 to present'right) <= present;
    g1: if present'right < to_users(i).present'right generate
      to_users(i).present(present'right+1 to to_users(i).present'right) <= (others => bfalse);
    end generate g1;
    g2: if nusers < wci.raw_max_devices generate
      to_users(i).present(nusers to wci.raw_max_devices-1) <= (others => '0');
    end generate g2;
    to_users(i).raw.data    <= from_device.raw.data; -- broadcast data to all
    to_users(i).raw.done    <= to_bool(from_device.raw.done and who_has_it_r = i);
    to_users(i).raw.error   <= to_bool(from_device.raw.error and who_has_it_r = i);
  end generate g0;
  to_device.present <= btrue;
  to_device.reset   <= all_reset;
  to_device.raw     <= from_users(idx).raw;
  arb : process(clk) is begin
    if rising_edge(clk) then
      if its(reset) then
        who_has_it_r <= to_unsigned(first_present(present),user_t'length);
        active_r     <= bfalse;
      elsif its(from_device.raw.done) then -- rotate after a successful cycle.
        who_has_it_r <= to_unsigned(next_user, user_t'length);
        active_r     <= bfalse;
      elsif not its(active_r) then
        if from_users(idx).raw.is_read or from_users(idx).raw.is_write then
          active_r     <= btrue;
        else
          who_has_it_r <= to_unsigned(next_user, user_t'length);          
        end if;
      end if;
    end if;
  end process arb;
end rtl;
