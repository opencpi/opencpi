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

library IEEE, ocpi, bsv;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, ocpi.util.all;

architecture rtl of timestamper2_worker is
  signal time_now_r        : std_logic_vector(63 downto 0);
  signal message_wcnt_r    : unsigned(15 downto 0);
  signal wsi_data          : std_logic_vector(31 downto 0);
  -- signal wsi_opcode        : iqstream_OpCode_t;
  signal doit              : bool_t;
  signal zlm_flag          : std_logic;
begin
  doit <= ctl_in.is_operating and in_in.ready and out_in.ready;
  zlm_flag <= in_in.ready and in_in.som and in_in.eom and not in_in.valid;
  
  RegisterTimeStamp : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(in_in.ready) and in_in.valid and message_wcnt_r=0 then --Valid message on input
        time_now_r <= std_logic_vector(time_in.seconds) & std_logic_vector(time_in.fraction);
      end if;
    end if;
  end process;
  
  -- WSI input interface outputs
  in_out.take <= doit and (
                 to_bool(message_wcnt_r>2) or --Timestamp was given
                 zlm_flag                     --ZLM
                 );
  MessageWordCount : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        message_wcnt_r <= (others => '0');
      elsif doit and in_in.valid then
        if its(in_in.eom) then
          message_wcnt_r <= (others => '0');  
        else
          message_wcnt_r <= message_wcnt_r+1;
        end if;
      end if;
    end if;
  end process;

  MessageOutputData : process(message_wcnt_r, time_now_r, in_in.data)
  begin
    case to_integer(message_wcnt_r) is
      when 1      => wsi_data <= time_now_r(63 downto 32);
      when 2      => wsi_data <= time_now_r(31 downto 0);
      when others => wsi_data <= in_in.data;
    end case;
  end process;
  
  -- WSI output interface outputs
  out_out.give <= doit and (
                  to_bool(message_wcnt_r>2) or                                                      --Data
                  (props_in.enable and (to_bool(message_wcnt_r=1) or to_bool(message_wcnt_r=2))) or --Timestamps
                  zlm_flag                                                                          --ZLM
                  );
                   
  out_out.som <= to_bool(message_wcnt_r=1) or --SOM of timestamp
                 to_bool(message_wcnt_r=3) or --SOM of data
                 zlm_flag;                    --ZLM
  
  out_out.eom <= to_bool(message_wcnt_r=2) or --EOM of timestamp
                 in_in.eom;                   --EOM of data

  out_out.data <= wsi_data; 
  out_out.valid <= doit and to_bool(message_wcnt_r>0);
  out_out.opcode <= iqstream_with_sync_Time_op_e when message_wcnt_r=1 else iqstream_with_sync_iq_op_e;
  
end rtl;
