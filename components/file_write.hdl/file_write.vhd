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

-- TODO/FIXME:  use precise bursts to eliminate buffering when they are there
--              implement both endians

library ieee, ocpi, util;
use ieee.std_logic_1164.all, ieee.numeric_std.all, std.textio.all; 
use ocpi.types.all, ocpi.wci.all, util.util.all;

architecture rtl of file_write_worker is
  -- for file I/O and using util.cwd module
  constant pathLength      : natural := props_in.fileName'right;
  signal cwd               : string_t(0 to props_out.cwd'right);
  file   data_file         : char_file_t;
  -- our state machine
  signal init_r            : boolean := false;
  signal messageLength_r   : ulong_t := (others => '0');
  -- registers driving ctl_out
  signal finished_r        : boolean := false;
  -- registers driving props_out
  signal messagesWritten_r : ulonglong_t := (others => '0');
  signal bytesWritten_r    : ulonglong_t := (others => '0');
  -- pull a byte out of a dword and convert to char type. FIXME: endian!
  function char(dw : ulong_t; pos : natural) return character is begin
    return to_character(char_t(dw(pos*8+7 downto pos*8)));
  end char;
begin
  -- worker outputs
  ctl_out.finished          <= to_bool(finished_r);
  props_out.cwd             <= cwd;
  props_out.bytesWritten    <= bytesWritten_r; 
  props_out.messagesWritten <= messagesWritten_r;
  in_out.take               <= in_in.ready;

  -- get access to the CWD for pathname resolution (and as a readable property)
  cwd_i : component util.util.cwd
    generic map(length     => cwd'right)
    port    map(cwd        => cwd);

  process(ctl_in.clk)
    variable c              : character;
    variable msg_buffer     : string(1 to 16*1024);
    variable new_msg_length : natural;
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        init_r            <= false;
        messageLength_r   <= (others => '0');
        finished_r        <= false;
        messagesWritten_r <= (others => '0');
        bytesWritten_r    <= (others => '0');
      elsif ctl_in.control_op = STOP_e or ctl_in.control_op = RELEASE_e then
        finished_r <= true;
        close_file(data_file, props_in.fileName);
      elsif its(ctl_in.is_operating) and not finished_r then
        if not init_r then  
          open_file(data_file, cwd, props_in.fileName, write_mode);
          init_r <= true;
        elsif its(in_in.ready) then
          new_msg_length := to_integer(messageLength_r);
          if its(in_in.valid) and in_in.byte_enable /= "0000" then
            -- There is data: either write it directly or put it in the buffer
            for i in 0 to 3 loop
              if in_in.byte_enable(i) = '1' then -- FIXME: endian!
                c := char(ulong_t(in_in.data), i);
                if its(props_in.messagesInFile) then 
                  if new_msg_length >= msg_buffer'length then
                    report "The messagesInFile property is true so messages are being buffered";
                    report "A message is too large for the message buffer";
                    report "new_msg_length is " & integer'image(new_msg_length) severity failure;
                  else
                    msg_buffer(new_msg_length+1) := c;
                  end if;
                else
                  write(data_file, c);
                end if;
                new_msg_length := new_msg_length + 1;
              end if;
            end loop;    
          end if;
          if its(in_in.eom) then
            if new_msg_length = 0 and unsigned(in_in.opcode) = 0 and props_in.stopOnEOF then
              finished_r <= true;
              close_file(data_file, props_in.fileName);
            else
              if its(props_in.messagesInFile) then
                for i in 0 to 3 loop
                  write(data_file, char(to_ulong(new_msg_length),i));
                end loop;
                for i in 0 to 3 loop
                  write(data_file, char(ulong_t(resize(unsigned(std_logic_vector(in_in.opcode)),
                                                       ulong_t'length)), i));
                end loop;
                for i in 0 to new_msg_length-1 loop
                  write(data_file, msg_buffer(i+1));
                end loop;            
              end if;
              messagesWritten_r <= messagesWritten_r + 1; -- we count non-EOF ZLMs
            end if;
            bytesWritten_r <= bytesWritten_r + new_msg_length;
            messageLength_r <= (others => '0');
          else
            messageLength_r <= to_ulong(new_msg_length);
          end if; -- eom
        end if; --if in_in.ready
      end if; -- if operating and not finished
    end if; -- if rising_edge(ctl_in.clk)  
  end process;
end rtl;
