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

-------------------------------------------------------------------------------
-- Zero Pad
-------------------------------------------------------------------------------
--
-- Description:
--- The Zero Pad worker inputs samples and inserts num_zeros zeros between each
--- output sample. The DWIDTH_p parameter defines the input and output data
--- width as well as the size of the zeros inserted
--- 
--- The output message size is equal to num_zeros+1
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library util; use util.util.all;
              
architecture rtl of zero_pad_worker is
  signal message_wcnt_r    : unsigned(15 downto 0);
  signal doit              : bool_t;
  signal doit_out          : bool_t;
  signal output_valid      : std_logic;
  signal output_som        : std_logic;
  signal output_eom        : std_logic;
  signal take              : std_logic;
  signal zlm_detected      : std_logic;  
  signal data_eom          : std_logic;
begin
  doit     <= ctl_in.is_operating and in_in.ready and out_in.ready;
  doit_out <= ctl_in.is_operating and out_in.ready;
  
  -- WSI input interface outputs
  in_out.take <= take;
  take        <= doit and 
                 to_bool(message_wcnt_r=0); --Done adding zeros

  data_eom <= to_bool(message_wcnt_r=unsigned(props_in.num_zeros)); --Send eom on last zero

  -- Use zlm detector primitive to detect zlms and multicycle zlms
  zlm_detector : util.util.zlm_detector
  port map (
    clk => ctl_in.clk,
    reset => ctl_in.reset,
    som => in_in.som,
    valid => in_in.valid,
    eom => in_in.eom,
    ready => in_in.ready,
    take => take,
    eozlm_pulse => open,
    eozlm => zlm_detected);

  MessageWordCount : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        message_wcnt_r <= (others => '0');
      elsif its(doit_out) then
        if its(data_eom) then --reset on last zero
          message_wcnt_r <= (others => '0');
        elsif message_wcnt_r=0 then 
          if its(in_in.valid) and its(in_in.ready) then--increment when taking valid data
            message_wcnt_r <= message_wcnt_r+1;
          end if;
        else
          message_wcnt_r <= message_wcnt_r+1;
        end if;
      end if;
    end if;
  end process;

  MessageOutputData : process(message_wcnt_r, in_in.data)
  begin
    case to_integer(message_wcnt_r) is
      when 0      => out_out.data <= in_in.data;
      when others => out_out.data <= (others => '0');
    end case;
  end process;

    -- WSI output interface outputs
  out_out.give          <= doit_out and (output_valid or zlm_detected);
  output_valid          <= (in_in.ready and in_in.valid and to_bool(message_wcnt_r=0)) or --Sending sample
                           (to_bool(message_wcnt_r/=0));                                  --Sending zeros
  out_out.valid         <= output_valid;
  output_som            <= to_bool(message_wcnt_r=0) when its(in_in.ready) and its(in_in.valid) else zlm_detected;
  output_eom            <= data_eom when its(output_valid) else zlm_detected;
  out_out.som           <= output_som;
  out_out.eom           <= output_eom;
  out_out.byte_enable   <= (others => '1');
  out_out.opcode        <= in_in.opcode;
  
end rtl;
