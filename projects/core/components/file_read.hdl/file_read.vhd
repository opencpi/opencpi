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

-- TODO/FIXME:  eliminate the dead cycle between output messages
--              implement the "repeat" property/feature
--              implement the "granularity" feature: requires buffering
--              should badMessage set attention?
--              use precise bursts on output when messagesInFile
--              implement endians

library ieee, ocpi, util;
use ieee.std_logic_1164.all, ieee.numeric_std.all, std.textio.all;  
use util.util.all, ocpi.types.all, work.file_read_worker_defs.all;

architecture rtl of file_read_worker is
  -- for file I/O and using util.cwd module
  constant pathLength      : natural := props_in.fileName'right;
  signal cwd               : string_t(0 to props_out.cwd'right); -- from cwd module to props_out
  file   data_file         : char_file_t;
  -- our state machine
  signal init_r            : boolean     := false;           -- did one-time init
  signal som_next_r        : boolean     := false;           -- is staged data a SOM?
  signal bytesLeft_r       : ulong_t     := (others => '0'); -- bytes left in current message
  signal ready_r           : boolean     := false;           -- have staged data for out_out
  signal eof_r             : boolean     := false;           -- we have seen EOF on input
  -- registers driving ctl_out
  signal finished_r        : boolean     := false;           -- to drive ctl_out.finished
  -- registers driving props_out
  signal bad_r             : boolean     := false;           -- to drive props_out.badMessage
  signal messagesWritten_r : ulonglong_t := (others => '0'); -- drive props_out.messagesWritten
  signal bytesRead_r       : ulonglong_t := (others => '0'); -- drive props_out.bytesRead
  -- registers driving out_out - initialized just for simulator cleanliness, not functionality
  signal data_r            : ulong_t     := (others => '0');     -- to drive out_out.data;
  signal opcode_r          : uchar_t     := (others => '0');     -- to drive out_out.opcode
  signal byte_enable_r     : std_logic_vector(3 downto 0) := "0000"; -- drive out.byte_enable
  signal som_r             : boolean     := false;               -- to driver out_out.som
  signal eom_r             : boolean     := false;               -- to driver out_out.eom
  signal valid_r           : boolean     := false;               -- to driver out_out.valid
  -- combi indication that we are giving to output port in the current cycle
  signal giving            : boolean;
begin
   ctl_out.finished          <= to_bool(finished_r and not ready_r);
   props_out.cwd             <= cwd;
   props_out.bytesRead       <= bytesRead_r;
   props_out.messageswritten <= messagesWritten_r;
   props_out.badMessage      <= to_bool(bad_r);
   out_out.give              <= to_bool(giving);
   out_out.som               <= to_bool(som_r);
   out_out.eom               <= to_bool(eom_r);
   out_out.valid             <= to_bool(valid_r);
   out_out.opcode            <= out_Opcode_t(resize(opcode_r,out_out.opcode'length));
   out_out.byte_enable       <= byte_enable_r;
   out_out.data              <= std_logic_vector(data_r);
   giving                    <= out_in.ready and ready_r;
   out_out.eof               <= to_bool(eof_r);
  -- get access to the CWD for pathname resolution (and as a readable property)
  cwd_i : component util.util.cwd
    generic map(length     => cwd'right)
    port    map(cwd        => cwd);

   process (ctl_in.clk) is
     variable ulong_bytes       : natural; -- how many bytes from read_ulong
     variable ulong_byte_enable : std_logic_vector(3 downto 0); -- byte enables from read_ulong
     variable eom               : boolean; -- temp indicating we want eom
     -- impure function that also sets the above two process variables
     impure function read_ulong(max2read : natural; eof_bad : boolean) return ulong_t is
       variable data : character;
       variable ulong : ulong_t := (others => '0');
       variable n2read : natural := max2read;
     begin
       if n2read > 4 then
         n2read := 4;
       end if;
       ulong_bytes := 0;
       ulong_byte_enable := "0000";
       for i in 0 to n2read-1 loop
         if endfile(data_file) then
           bad_r <= eof_bad;
           return ulong;
         end if;
         read(data_file, data);
         ulong(8*i+7 downto 8*i) := to_unsigned(character'pos(data), 8);
         ulong_bytes := ulong_bytes + 1;
         ulong_byte_enable(i) := '1';
       end loop;
       return ulong;
     end read_ulong;
     -- check for EOF and do the right thing.
     procedure finish(msg : string) is begin
       report "EOF on input file: " & msg;
       close_file(data_file, props_in.fileName);
       finished_r <= true;
       if not props_in.suppressEOF then
         eof_r <= true;
       -- This code was from the version 0/1 implementation.  We don't want to do this any more
       -- with version 2 since version two allows for opcode zero ZLMs to be just another message
       -- else
       --   -- We must send a zero-length message
       --   opcode_r <= props_in.opcode;
       --   bytesLeft_r <= (others => '0');
       --   som_next_r <= true;
       end if;
     end finish;
   begin    
     if rising_edge(ctl_in.clk) then
       if its(ctl_in.reset) then
         init_r            <= false;
         som_next_r        <= false;
         bytesLeft_r       <= (others => '0');
         ready_r           <= false;
         eof_r             <= false;
         finished_r        <= false;
         bad_r             <= false;
         messagesWritten_r <= (others => '0');
         bytesRead_r       <= (others => '0');
         data_r            <= (others => '0');     -- to drive out_out.data;
         opcode_r          <= (others => '0');     -- to drive out_out.opcode
         byte_enable_r     <= "0000";              -- drive out.byte_enable
         som_r             <= false;               -- to driver out_out.som
         eom_r             <= false;               -- to driver out_out.eom
         valid_r           <= false;               -- to driver out_out.valid
       elsif its(ctl_in.is_operating) and (not finished_r or ready_r) then
         if giving and eom_r then
           ready_r <= false;
         end if;
         if not init_r then  
           -- do the one time (after is_operating so properties are set)
           open_file(data_file, cwd, props_in.fileName, read_mode);
           opcode_r <= props_in.opcode;
           init_r <= true;
         elsif finished_r then
         elsif bytesLeft_r = 0 and not som_next_r then
           -- between messages - see if we can start one
           if endfile(data_file) then
             if not ready_r then -- don't finish until nothing is staged to send
               finish("between messages");
             end if;
           else
             -- Starting a message
             som_next_r <= true;
             if its(props_in.messagesInFile) then
               bytesLeft_r <= read_ulong(4, true);
               opcode_r <= resize(read_ulong(4, true),opcode_r'length);
               if bad_r then
                 finish("Bad message header");
               end if;
             else
               bytesLeft_r <= props_in.messageSize;
             end if;
           end if;
         elsif giving or not ready_r then
           -- there was nothing previously staged, or we are giving it now anyway
           ready_r    <= true;
           som_r      <= som_next_r;
           som_next_r <= false;
           eom        := false;
           if bytesLeft_r = 0 then -- we are staging an EOF/zlm.  There is no file I/O to do
             eom        := true;
             valid_r    <= false;
             if eof_r then -- a ZLM at EOF means we're done
               finished_r <= true;
             end if;
           elsif endfile(data_file) then -- EOF mid-message, w/ no data: shouldn't happen
             if its(props_in.messagesInFile) then
               report "Unexpected EOF mid-message" severity failure;
             else
               -- Can't use: finish("Short message at EOF");
               eom        := true;
               som_r      <= true;
               valid_r    <= false;
               finished_r <= true;
               file_close(data_file);
             end if;
           else -- we want data (message_length_r != 0), and there is data (not eof)
             data_r        <= read_ulong(to_integer(bytesLeft_r), false);
             byte_enable_r <= ulong_byte_enable;
             eom           := ulong_bytes = bytesLeft_r or endfile(data_file);
             bytesLeft_r   <= bytesLeft_r - ulong_bytes;
             valid_r       <= true;
             bytesRead_r <= bytesRead_r + ulong_bytes;
           end if;
           if eom then
             messagesWritten_r <= messagesWritten_r + 1;
           end if;
           eom_r <= eom;
         end if;  -- end of producing output
       else -- end of operating and not finished
         -- this is needed for the very last ZLM at EOF
         ready_r <= false;
       end if;
     end if; -- end of clock edge
   end process;
end rtl;
