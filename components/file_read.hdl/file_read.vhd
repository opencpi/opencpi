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
   ctl_out.finished          <= to_bool(finished_r);
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
       variable ulong : ulong_t;
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
       file_close(data_file);
       if its(props_in.suppressEOF) then
         finished_r <= true; -- nothing to do at this EOF except be finished
       else
         -- We must send a zero-length message
         opcode_r <= props_in.opcode;
         bytesLeft_r <= (others => '0');
         som_next_r <= true;
       end if;
     end finish;
   begin    
     if rising_edge(ctl_in.clk) then
       if its(ctl_in.reset) then
         init_r            <= false;
         som_next_r        <= false;
         bytesLeft_r       <= (others => '0');
         ready_r           <= false;
         finished_r        <= false;
         bad_r             <= false;
         messagesWritten_r <= (others => '0');
         bytesRead_r       <= (others => '0');
       elsif its(ctl_in.is_operating) and not finished_r then    
         if giving and eom_r then
           ready_r <= false;
         end if;
         if not init_r then  
           -- do the one time (after is_operating so properties are set)
           open_file(data_file, cwd, props_in.fileName, read_mode);
           opcode_r <= props_in.opcode;
           init_r <= true;
         elsif bytesLeft_r = 0 and not som_next_r then
           -- between messages - see if we can start one
           if endfile(data_file) then
             finish("between messages");
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
           if bytesLeft_r = 0 then -- we are staging an EOF/zlm.  There is no file I/O
             eom        := true;
             valid_r    <= false;
             finished_r <= true;
           elsif endfile(data_file) then -- EOF mid-message, w/ no data: shouldn't happen
             report "Unexpected EOF" severity failure;
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
