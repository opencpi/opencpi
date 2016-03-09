-- Extend the sdp into the named pipes from the simulator for extending the sdp to software
library ieee, ocpi, platform, util;
use IEEE.std_logic_1164.all, IEEE.numeric_std.all,
  ocpi.all, ocpi.types.all, ocpi.util.all,
  platform.all, work.sdp.all, util.util.all;
entity sdp_sim is
  generic(ocpi_debug   :     bool_t;
          sdp_width    :     uchar_t);
  port(   clk          : in  std_logic;
          reset        : in  bool_t;
          sdp_in       : in  s2m_t;
          sdp_out      : out m2s_t;
          sdp_in_data  : in  dword_array_t(0 to to_integer(sdp_width)-1);
          sdp_out_data : out dword_array_t(0 to to_integer(sdp_width)-1));
end entity;
architecture rtl of sdp_sim is
  -- The ctl pipe gives us credits to spin or to read from the sw-to-sdp pipe, or says "stop"
  signal ctl_name       : string_t(0 to util.util.plusarg_length);
  file   ctl_file       : char_file_t;
  -- The ack pipe tells the sim server when we have run through the spin credits
  signal ack_name       : string_t(0 to util.util.plusarg_length);
  file   ack_file       : char_file_t;
  -- The SDP messages from SW that we inject into the sdp
  signal sw2sim_name    : string_t(0 to util.util.plusarg_length);
  file   sw2sim_file    : char_file_t;
  -- The SDP messages to SW that we have received from the sdp
  signal sim2sw_name      : string_t(0 to util.util.plusarg_length);
  file   sim2sw_file      : char_file_t;
  -- State
  signal init_r             : boolean := false;
  signal spin_credit_r      : natural := 2;
  -- State for SW->SDP flow
  signal sw2sdp_credit_r    : natural := 0;
  signal sw2sdp_header_r    : dword_array_t(0 to sdp_header_ndws - 1)
                              := (others => (others => '0'));
  signal sw2sdp_data_r      : dword_array_t(0 to to_integer(sdp_width) - 1)
                              := (others => (others => '0'));
  signal sw2sdp_complete_r  : boolean := false; -- there is an sdp frame ready to send
  signal sw2sdp_length_r    : natural := 0;
  signal sw2sdp_in_header_r : boolean := true;
  signal sw2sdp_index_r     : natural := 0;
  signal sw2sdp_eop_r       : boolean := false; -- frame is eop
  signal sw2sdp_dw_r        : dword_t;
  -- State for SDP->SW flow
  signal sdp2sw_in_header_r : boolean := true;
  signal sdp2sw_length_r    : natural := 0; -- payload length, decremented
  signal sdp2sw_index_r     : natural := 0; -- index into the frame or the header
  signal sdp2sw_last_dw     : boolean;
  signal sdp2sw_header_dws  : dword_array_t(0 to sdp_header_ndws - 1);
  -- signals for writing to pipes
  signal ack_valid_r        : bool_t := bfalse;
  signal ack_flush_r        : bool_t := bfalse;
  signal ack_close_r        : bool_t := bfalse;
  signal sim2sw_valid_r     : bool_t := bfalse;
  signal sim2sw_flush_r     : bool_t := bfalse;
  signal sim2sw_close_r     : bool_t := bfalse;
  signal sim2sw_data_r      : dword_t;
begin
  ctl_name_i    : plusarg generic map(name => "ctl")    port map(val => ctl_name);
  ack_name_i    : plusarg generic map(name => "ack")    port map(val => ack_name);
  sw2sim_name_i : plusarg generic map(name => "sw2sim") port map(val => sw2sim_name);
  sim2sw_name_i : plusarg generic map(name => "sim2sw") port map(val => sim2sw_name);
  ack_write_i:    flush_writer
    generic map(nbytes => 1, namelength => util.util.plusarg_length)
    port    map(clk => clk, reset => reset, name => ack_name,
                valid => ack_valid_r, flush => ack_flush_r, close => ack_close_r,
                data => from_char(to_char('1')));
  sim2sw_write_i: flush_writer
    generic map(nbytes => 4, namelength => util.util.plusarg_length)
    port    map(clk => clk, reset => reset, name => sim2sw_name,
                valid => sim2sw_valid_r, flush => sim2sw_flush_r, close => sim2sw_close_r,
                data => sim2sw_data_r);
  sdp_out.clk        <= clk;
  sdp_out.reset      <= reset;
  sdp_out.id         <= (others => '0');  -- set in case directly wired to control plane
  sdp_out.sdp.header <= dws2header(sw2sdp_header_r);
  sdp_out.sdp.valid  <= to_bool(sw2sdp_complete_r);
  sdp_out.sdp.eop    <= to_bool(sw2sdp_eop_r);
  sdp_out.sdp.ready  <= to_bool(sdp_in.sdp.valid and
                                ((sdp2sw_in_header_r and
                                  sdp2sw_index_r = sdp_header_ndws-1 and
                                  payload_in_dws(sdp_in.sdp.header) = 0) or
                                 (not sdp2sw_in_header_r and sdp2sw_last_dw)));
  sdp_out_data       <= sw2sdp_data_r;

  sdp2sw_last_dw     <= sdp2sw_index_r = sdp_width-1 or sdp2sw_length_r = 1;
  sdp2sw_header_dws  <= header2dws(sdp_in.sdp.header);

  process (clk) is
    variable sw2sdp_dw : dword_t;
    variable sdp2sw_dw : dword_t;
    variable ndws      : natural;
    variable credit    : integer;
    impure function read_byte(file f : char_file_t; msg : string) return natural is
      variable data : character;
    begin
      if endfile(f) then
        report "Unexpected EOF on " & msg; -- this is benign - it just indicates pipe empty
      end if;
--      report "Reading a byte from " & msg;
      read(f, data);
--      report "Got a byte from " & msg & " value " & integer'image(character'pos(data));
      return character'pos(data);
    end read_byte;
    impure function read_short(file f : char_file_t) return natural is
      variable byte : natural := read_byte(f, "short1");
    begin
      return byte + read_byte(f, "short2") * 256;
    end read_short;
    procedure write_dw(dw : dword_t) is
    begin
      sim2sw_valid_r <= btrue;
      sim2sw_data_r <= dw;
--      for i in 0 to 3 loop
--        -- report "Writing byte";
--        write(sim2sw_file, to_character(char_t(dw(i*8+7 downto i*8))));
--      end loop;
    end write_dw;
    impure function header_dws(dw : dword_t) return dword_array_t is
      variable dws : dword_array_t(0 to sdp_header_ndws-1);
    begin
      if sdp_header_ndws = 1 then
        dws(0) := dw;
      else
        dws(0 to sdp_header_ndws-2) := sw2sdp_header_r(0 to sdp_header_ndws-2);
        dws(sdp_header_ndws-1) := dw;
      end if;
      return dws;
    end header_dws;
  begin
    if rising_edge(clk) and not its(reset) then
      ack_valid_r        <= bfalse;
      ack_flush_r        <= bfalse;
      ack_close_r        <= bfalse;
      sim2sw_valid_r     <= bfalse;
      sim2sw_flush_r     <= bfalse;
      sim2sw_close_r     <= bfalse;
      if not init_r then
        -- We assume all these names are absolute and need no CWD
        open_file(ctl_file,    to_string("",0), ctl_name,    read_mode);
--      open_file(ack_file,    to_string("",0), ack_name,    write_mode);
        open_file(sw2sim_file, to_string("",0), sw2sim_name, read_mode);
--      open_file(sim2sw_file,   to_string("",0), sim2sw_name,   write_mode);
        init_r <= true;
      else
        -- Process the controls from the simulations server
        credit := sw2sdp_credit_r;
        if spin_credit_r > 0 then
          if spin_credit_r = 3 then
--            report "ack";
--            write(ack_file, '1');
            ack_valid_r <= btrue;
            ack_flush_r <= btrue;
          end if;
          spin_credit_r <= spin_credit_r - 1;
        elsif not sdp_in.sdp.valid then
          case read_byte(ctl_file, "control") is
            when 0      => spin_credit_r <= read_byte(ctl_file, "spin");
            when 1      =>
              credit := credit + read_short(ctl_file);
            when 255    => 
              file_close(ctl_file);
--            file_close(ack_file);
              file_close(sw2sim_file);
--            file_close(sim2sw_file);
              ack_close_r    <= btrue;
              sim2sw_close_r <= btrue;
              report "NONE. End of simulation." severity failure;
            -- std.env.finish(2); VHDL 2008...
            when others => report "unknown control byte" severity failure;
          end case;
        end if;
        -- The processing from sw to sdp.
        if sw2sdp_complete_r and sdp_in.sdp.ready then
          -- The SDP has accepted a frame, we'll default this to off
          sw2sdp_complete_r <= false;
        end if;
        if sw2sdp_credit_r > 0 and (not sw2sdp_complete_r or sdp_in.sdp.ready) then
          credit := credit - 1;
          --sw2sdp_credit_r <= sw2sdp_credit_r - 1;
          for i in 0 to 3 loop
            sw2sdp_dw(i*8+7 downto i*8) :=
              std_logic_vector(to_unsigned(read_byte(sw2sim_file, "sw2sdp"), 8));
          end loop;
          sw2sdp_dw_r <= sw2sdp_dw;
          if sw2sdp_in_header_r then
            sw2sdp_header_r(sw2sdp_index_r) <= sw2sdp_dw;
            if sw2sdp_index_r = sdp_header_ndws-1 then
              -- we're done with the header
              if sw2sdp_index_r = 0 then
                ndws := to_integer(payload_in_dws(sw2sdp_dw));
              else
                ndws := to_integer(payload_in_dws(sw2sdp_header_r(0)));
              end if;
              if ndws = 0 then
                -- zero payload, we're done
                sw2sdp_index_r     <= 0;
                sw2sdp_complete_r  <= true;
                sw2sdp_eop_r       <= true;
              else
                sw2sdp_length_r    <= ndws;
                sw2sdp_index_r     <= start_dw(dws2header(header_dws(sw2sdp_dw)), sdp_width);
                sw2sdp_in_header_r <= false;
                sw2sdp_eop_r       <= ndws <= sdp_width;
              end if;
            else
              sw2sdp_index_r <= sw2sdp_index_r + 1;
            end if;
          else
            sw2sdp_data_r(sw2sdp_index_r) <= sw2sdp_dw;
            sw2sdp_length_r <= sw2sdp_length_r - 1;
            if sw2sdp_length_r = 1 then
              sw2sdp_index_r     <= 0;
              sw2sdp_in_header_r <= true;
              sw2sdp_complete_r  <= true;
              sw2sdp_eop_r       <= true;
            elsif sw2sdp_index_r = sdp_width-1 then
              sw2sdp_index_r    <= 0;
              sw2sdp_complete_r <= true;
            else
              sw2sdp_index_r <= sw2sdp_index_r + 1;
            end if;
          end if;
        end if;
        sw2sdp_credit_r <= credit;
        -- The processing from sdp to sw
        if its(sdp_in.sdp.valid) then -- we will write a word from the current frame
          sim2sw_valid_r <= btrue;
          if sdp2sw_in_header_r then
            -- write_dw(sdp2sw_header_dws(sdp2sw_index_r));
            sim2sw_data_r   <= sdp2sw_header_dws(sdp2sw_index_r);
            sdp2sw_length_r <= to_integer(payload_in_dws(sdp_in.sdp.header));
            if sdp2sw_index_r = sdp_header_ndws-1 then
              if payload_in_dws(sdp_in.sdp.header) /= 0 then
                sdp2sw_in_header_r <= false;
                sdp2sw_index_r <= start_dw(sdp_in.sdp.header, sdp_width);
              else
                sim2sw_flush_r <= btrue;
                sdp2sw_index_r <= 0;
              end if;
            else
              sdp2sw_index_r <= sdp2sw_index_r + 1;
            end if;
          else
            -- write_dw(sdp_in_data(sdp2sw_index_r));
            sim2sw_data_r <= sdp_in_data(sdp2sw_index_r);
            if sdp2sw_last_dw then -- end of frame from sdp
              sim2sw_flush_r <= btrue;
              sdp2sw_index_r <= 0;
              if sdp2sw_length_r = 1 then
                sdp2sw_in_header_r <= true;
              end if;
            else
              sdp2sw_index_r <= sdp2sw_index_r + 1;
            end if;
            sdp2sw_length_r <= sdp2sw_length_r - 1;
          end if;
        end if;
      end if; -- not reset of termination
    end if; -- rising edge
  end process;
end rtl;
