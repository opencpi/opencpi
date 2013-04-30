-- THIS FILE WAS ORIGINALLY GENERATED ON Mon Mar 18 13:03:40 2013 EDT
-- BASED ON THE FILE: gen/file_read.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: file_read

library IEEE; 
  use IEEE.std_logic_1164.all; 
  use ieee.numeric_std.all;
  use ieee.std_logic_textio.all;
  use ieee.math_real.all;
  use std.textio.all;  
library ocpi; 
  use ocpi.types.all; -- remove this to avoid all ocpi name collisions
  use work.file_read_worker_defs.all;

--entity file_read_worker is
--  port(
--    -- Signals for control and configuration.  See record types above.
--    ctl_in               : in  worker_ctl_in_t;
--    ctl_out              : out worker_ctl_out_t := (btrue, bfalse, bfalse, bfalse);
--    -- Input values and strobes for this worker's writable properties
--    props_in             : in  worker_props_in_t;
--    -- Outputs for this worker's volatile, readable properties
--    props_out            : out worker_props_out_t;
--    -- Signals for WSI output port named "out".  See record types above.
--    out_in               : in  worker_out_in_t;
--    out_out              : out worker_out_out_t);
--end entity file_read_worker;

architecture behavior of file_read_worker is

  signal bytes_remaining_1 : ulong_t:= (others => '0');  -- left in message PLUS 1
  signal som_now, som_next : std_logic            := '0';
  signal bytes_read        : ulonglong_t:= (others => '0');
  signal messages_written  : ulonglong_t:= (others => '0');
  signal opcode_r          : out_opcode_t; -- opcode from file that is 32 bits
  signal data_r            : std_logic_vector(31 downto 0);
  signal byte_enable       : std_logic_vector(3 downto 0);
  signal give_now          : bool_t;
  signal eom_now           : bool_t;
  signal done              : bool_t := bfalse;  -- to indicate file is done and closed
  signal init              : bool_t := bfalse;  -- to initialize file name from props once
begin
   props_out.bytesread       <= bytes_read;
   props_out.messageswritten <= messages_written;
   out_out.opcode            <= opcode_r;
   out_out.byte_enable       <= byte_enable;
   out_out.data              <= data_r;
   out_out.valid             <= give_now;
   out_out.som               <= som_now;
   out_out.give              <= give_now;
   out_out.eom               <= eom_now;

   process (ctl_in.clk) is
     type char_file_t is file of character;
     file data_file          : char_file_t;
     variable i              : integer range 0 to props_in.filename'right := 0;
     variable file_name      : string(1 to props_in.filename'right);--:= (others => '0');
     variable status         : file_open_status;
     variable length_to_read : natural;
     variable nbytes         : natural;
     variable data           : character;
     variable eom            : bool_t;
     variable bad            : bool_t;
   begin    
     if rising_edge(ctl_in.clk) then
       give_now <= bfalse;
       eom_now <= bfalse;
       if its(ctl_in.reset) then
         init             <= bfalse;
         done             <= bfalse;
         bytes_read       <= (others => '0');
         messages_written <= (others => '0');
         som_next         <= '0';
       elsif its(ctl_in.is_operating and not done) then
         if not init then
           -- convert file name for function file_open
           for i in (props_in.fileName'left) to (props_in.fileName'right-1) loop
             file_name(i+1) := To_character(props_in.fileName(i));     
           end loop;
           file_open(status, data_file, file_name, read_mode);
           if status = open_ok then
             init <= btrue;
             report "File opened successfully:";
             report file_name;
           else
             done <= btrue;
             report "File could not be opened:";
             report file_name;
           end if;
         elsif bytes_remaining_1 = 0 then
           -- start a new message
           if its(props_in.messagesInFile) then
             if endfile(data_file) then
               -- clean eof, no more messages
               file_close(data_file);
               done <= btrue;
             else
               nbytes := 0;
               for i in 0 to 3 loop
                 if endfile(data_file) then
                   bad := btrue;
                   exit;
                 else
                   nbytes := nbytes * 256;
                   read(data_file, data);
                   nbytes := nbytes + character'pos(data);
                 end if;
               end loop;
               if not bad then
                 bytes_remaining_1 <= to_unsigned(nbytes + 1, bytes_remaining_1'length);
                 for i in 0 to 3 loop
                   if endfile(data_file) then
                     bad := btrue;
                     exit;
                   elsif i = 0 then
                     read(data_file, data);
                     opcode_r <= std_logic_vector(to_unsigned(character'pos(data),8));
                   end if;
                 end loop;
               end if;
               if its(bad) then
                 report "Bad message header at EOF";
                 done <= btrue;
               else
                 som_next <= '1';
               end if;
             end if;
           else                         -- else of messagesinfile
             opcode_r <= out_opcode_t(props_in.opcode);
             bytes_remaining_1  <= props_in.messageSize + 1;
             som_next <= '1';
             report "prepared for new message";
           end if;
         elsif its(out_in.ready) and bytes_remaining_1 /= 0 then
           -- output is ready and we should read and send some bytes
           byte_enable <= (others => '0');
           nbytes := 0;
           eom := bfalse;
           if bytes_remaining_1 = 1 then
             eom := btrue;
           elsif endfile(data_file) then
             file_close(data_file);
             done <= '1';
             report "File closed";
             eom := btrue;
           else
             length_to_read := to_integer(bytes_remaining_1) - 1;
             if length_to_read > 4 then
               length_to_read := 4;
             end if;
             for i in 0 to length_to_read - 1 loop
               if endfile(data_file) then
                 byte_enable(i) <= '0';
                 data_r(i*8+7 downto i*8) <= (others => '0');  -- for non-byte-enabled
               else
                 read(data_file, data);
                 data_r(i*8+7 downto i*8) <= std_logic_vector(to_unsigned(character'pos(data),8));
                 byte_enable(i) <= '1';
                 nbytes := nbytes + 1;
               end if;
             end loop;
             bytes_remaining_1 <= bytes_remaining_1 - nbytes;
             bytes_read <= bytes_read + nbytes;
             if endfile(data_file) then
               eom := btrue;
               done <= btrue;
             end if;
           end if;
           give_now <= btrue;
           som_now <= som_next;
           som_next <= '0';
           if its(eom) then
             eom_now <= btrue;
             bytes_remaining_1 <= (others => '0');
             messages_written <= messages_written + 1;
           end if;
         end if;
       end if;
     end if;
   end process;
end behavior;
