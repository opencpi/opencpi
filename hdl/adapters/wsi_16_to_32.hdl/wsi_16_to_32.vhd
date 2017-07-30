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

-- THIS FILE WAS ORIGINALLY GENERATED ON Sun Jan 20 14:00:41 2013 EST
-- BASED ON THE FILE: wsi_16_to_32.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: wsi_32_to_16

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

--entity wsi_16_to_32_worker is
--  port(
--    -- Clock(s) not associated with one specific port:
--    wci_Clk               : in std_logic;
--    -- Signals for WSI input port named "in".  See record types above.
--    in_in              : in  worker_in_in_t;
--    in_out             : out worker_in_out_t;
--    -- Signals for WSI output port named "out".  See record types above.
--    out_in             : in  worker_out_in_t;
--    out_out            : out worker_out_out_t);
--end entity wsi_16_to_32_worker;

--  type worker_in_in_t is record
--    reset            : Bool_t;           -- this port is being reset from the outside peer
--    ready            : Bool_t;           -- this port is ready for data to be taken
--                                         -- one or more of: som, eom, valid are true
--    data             : std_logic_vector(15 downto 0);
--    byte_enable      : std_logic_vector(1 downto 0);
--    som, eom, valid  : Bool_t;           -- valid means data and byte_enable are present
--  end record worker_in_in_t;
--  type worker_in_out_t is record
--    take             : Bool_t;           -- take data now from this port
--                                         -- can be asserted when ready is true
--  end record worker_in_out_t;
--
--  -- The following record(s) are for the inner/worker interfaces for port "out"
--  type worker_out_in_t is record
--    reset            : Bool_t;           -- this port is being reset from the outside peer
--    ready            : Bool_t;           -- this port is ready for data to be given
--  end record worker_out_in_t;
--  type worker_out_out_t is record
--    give             : Bool_t;           -- give data now to this port
--                                         -- can be asserted when ready is true
--    data             : std_logic_vector(31 downto 0);
--    byte_enable      : std_logic_vector(3 downto 0);
--    som, eom, valid  : Bool_t;            -- one or more must be true when 'give' is asserted
--  end record worker_out_out_t;


architecture rtl of wsi_16_to_32_worker is

  signal data_r        : std_logic_vector(15 downto 0):= (others => '0');
  signal byte_enable_r : std_logic_vector(1 downto 0):= "00";
  signal som_r         : bool_t:= bfalse;
  signal captured_r    : bool_t:= bfalse;
  signal capture       : bool_t;
  signal giving        : bool_t;
begin
  capture <= in_in.ready and not captured_r and in_in.valid and not in_in.eom;
  giving  <= out_in.ready and in_in.ready and not capture;
  in_out.take         <= in_in.ready and (out_in.ready or capture);
  out_out.give        <= giving;
  out_out.data        <= in_in.data & data_r when its(captured_r)
                         else (in_in.data'range => '0') & in_in.data;
  out_out.byte_enable <= in_in.byte_enable & byte_enable_r when its(captured_r)
                         else (in_in.byte_enable'range => '0') & in_in.byte_enable;
  out_out.som         <= in_in.som or som_r;
  out_out.eom         <= in_in.eom;
  out_out.valid       <= in_in.valid or captured_r;

  process (wci_clk) is
  begin
    if rising_edge(wci_clk) then
      if its(wci_reset) then
        captured_r    <= bfalse;
      elsif its(capture) then
        data_r        <= in_in.data;
        byte_enable_r <= in_in.byte_enable;
        som_r         <= in_in.som;
        captured_r    <= btrue;
      elsif its(giving) then
        captured_r    <= bfalse;
      end if;
    end if;
  end process;
end rtl;
