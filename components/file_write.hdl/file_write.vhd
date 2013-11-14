-- THIS FILE WAS ORIGINALLY GENERATED ON Mon Jun  3 17:13:21 2013 EDT
-- BASED ON THE FILE: gen/file_write.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: file_write

library IEEE; 
  use IEEE.std_logic_1164.all; 
  use ieee.numeric_std.all;
  use std.textio.all; 
library ocpi; 
  use ocpi.types.all; -- remove this to avoid all ocpi name collisions

--entity file_write_worker is
--  port(
--    -- Signals for control and configuration.  See record types above.
--    ctl_in               : in  worker_ctl_in_t;
--    ctl_out              : out worker_ctl_out_t := (btrue, bfalse, bfalse, bfalse); done, error, finished, attention
--    -- Input values and strobes for this worker's writable properties
--    props_in             : in  worker_props_in_t;
--    -- Outputs for this worker's volatile, readable properties
--    props_out            : out worker_props_out_t;
--    -- Signals for WSI input port named "in".  See record types above.
--    in_in                : in  worker_in_in_t;
--    in_out               : out worker_in_out_t);
--end entity file_write_worker;

architecture rtl of file_write_worker is

  signal bytes_remaining : std_logic_vector(31 downto 0):= (others => '0');
  signal message_length  : std_logic_vector(31 downto 0):= (others => '0');
  signal byte_count      : std_logic_vector(11 downto 0):= (others => '0'); -- max bytes per message 4096
  signal init : bool_t := bfalse;
  signal done : bool_t := btrue;
  signal msg_count : ulonglong_t := (others=> '0');
  signal opcode_pad    : std_logic_vector(23 downto 0) := (others => '0');
  signal bytes_written : ulonglong_t := (others => '0');

begin

  in_out.take <= in_in.ready;
  props_out.bytesWritten <= bytes_written; 
  props_out.messagesWritten <= msg_count; 
  
  process(ctl_in.clk)
    type msg_array is array (96*4-1 downto 0) of std_logic_vector(7 downto 0); -- stores 4096 words
    variable msg_buffer : msg_array:=(others => (others => '0'));
    type char_file_t is file of character;
    file sig_data_file     : char_file_t;
    variable status       : file_open_status;
    variable file_name     : string(1 to props_in.filename'right);
    variable i             : integer range 0 to props_in.filename'right := 0;
    variable c             : character;
    variable byte_count    : integer:=0;
    variable opcode_32     : std_logic_vector(31 downto 0):= (others => '0');
    variable byte_count_64 : ulonglong_t := (others => '0');
    variable bytes_in_msg  : std_logic_vector(31 downto 0) := (others => '0');
  begin
    
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        init      <= bfalse;
        done      <= bfalse;
        msg_count <= (others => '0');
        bytes_written <= (others => '0');
      elsif its(ctl_in.is_operating and not done) then
        if its(not init) then  
          -- convert file name for function file_open
          for i in (props_in.fileName'left) to (props_in.fileName'right - 1) loop
            file_name(i+1) := To_character(props_in.fileName(i));         
          end loop;
          
          file_open(sig_data_file, file_name, write_mode);
          if status = open_ok then
            init <= btrue;
            report "File opened successfully:";
            report file_name;
          else
            done <= btrue;
            report "File could not be opened:";
            report file_name;
          end if; 
        
        elsif its(props_in.messagesInFile) then 
          if its(in_in.som) then
            byte_count := 0;
          end if;

          if its(in_in.ready and in_in.valid) then
            msg_buffer_loop: for i in 0 to 3 loop
              if in_in.byte_enable(i) = '1' then
                msg_buffer(byte_count) := in_in.data(i*8+7 downto i*8);
                byte_count := byte_count + 1;
              end if;
            end loop; 
          end if;
          
          bytes_in_msg := std_logic_vector(to_unsigned(byte_count, bytes_in_msg'length));
          
          if its(in_in.eom) then
            -- write message length
            byte_count_64 := byte_count + byte_count_64;
            bytes_written <= byte_count_64;
            length_loop: for i in 0 to 3 loop
              c := character'val(to_integer(unsigned(bytes_in_msg(i*8+7 downto i*8))));
              write(sig_data_file,c);   
            end loop;            
            -- write opcode
            opcode_32 := std_logic_vector(resize(unsigned(in_in.opcode), opcode_32'length));
            opcode_loop: for i in 0 to 3 loop
              c := character'val(to_integer(unsigned(opcode_32(i*8+7 downto i*8))));
              write(sig_data_file,c);   
            end loop;
            -- write message data
            data_loop: for i in 0 to byte_count-1 loop
                c := character'val(to_integer(unsigned(msg_buffer(i))));
                write(sig_data_file,c);
            end loop;    
            msg_count <= msg_count + 1;
          end if;

        elsif in_in.byte_enable > "0000" and its(in_in.ready and in_in.valid) then 
          if its(in_in.eom) then
            done <= btrue;
            msg_count <= msg_count + 1;
          end if;
          data_word_loop: for i in 0 to 3 loop
            if in_in.byte_enable(i) = '1' and its(in_in.valid) then
              c := character'val(to_integer(unsigned(in_in.data(i*8+7 downto i*8))));
              write(sig_data_file,c);
              byte_count_64 := byte_count_64 + 1;
            end if;
          end loop;  
          bytes_written <= byte_count_64;
        end if; --if not init
      elsif its(done) then
        file_close(sig_data_file);
      end if; -- if ctl_in.reset
    end if; -- if rising_edge(ctl_in.clk)  

  end process;
  

end rtl;
