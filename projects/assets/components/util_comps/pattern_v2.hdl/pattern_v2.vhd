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
-- Pattern_v2
-------------------------------------------------------------------------------
--
-- Description:
--
-- The pattern v2 component provides the ability to output a pattern of messages
-- by allowing the user to create a record of messages each having a configurable
-- number of bytes and associated 8 bit opcode. Through a set of properties, the
-- component may send messages (data and opcode) up to the amount dictated by
-- the build-time parameters. The messages property defines the record of messages
-- to send, as well as, defines the number of data bytes and an opcode for each
-- message.
--
-- For example:
-- When messages = {4, 255}, one message will be sent having 4
-- bytes of data and an opcode of 255. When messages = {8, 251}, {6, 250}, two
-- messages will be sent, the first having 8 bytes of data and an opcode of 251,
-- and the second message having 6 bytes of data and an opcode of 250.
--
-- Data to be sent with a message is defined by the data property and is referred
-- to as the data buffer. The number of data words in the data buffer is the
-- number of data bytes for the messages. The component offers an additional
-- feature when there are multiple messages via the dataRepeat property which
-- indicates whether the a message starts at the beginning of the data buffer,
-- or continues from its current index within the buffer.
--
-- For example:
-- Given messages = {4, 251},{8, 252},{12, 253},{16, 254},{20, 255}
-- If dataRepeat = true, then numDataWords is 5. To calculate the numDataWords
-- when dataRepeat is true, divide the largest message size (in bytes) by 4.
-- Dividing by four required because the data is output as a 4 byte data
-- word. Since the largest message size in the given messages assignment is 20,
-- 20/4 = 5. When numDataWords = 5, then a valid data assignment would be
-- data = {0, 1, 2, 3, 4}, and the data within each
-- message would look like: msg1 = {0}, msg2 = {0, 1}, msg3 = {0, 1, 2},
-- msg4 = {0, 1, 2, 3}, msg5 = {0, 1, 2, 3, 4}
-- If dataRepeat = false, then numDataWords is 15. To calculate the numDataWords
-- when dataRepeat is false, divide
-- the sum of all the message sizes (in bytes) by 4. Dividing by four is required
-- because the data is output as a 4 byte data word. Since the sum of all message
-- sizes in the given messages assignment is (4+8+12+16+20)/4 = 15.
-- When numDataWords = 15, then a valid data assignment would be
-- data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
-- and the data within each message would look like:
-- msg1 = {0}, msg2 = {1, 2}, msg3 = {3, 4, 5}, msg4 = {6, 7, 8, 9},
-- msg5 = {10, 11, 12, 13, 14}
-- There is also a messagesToSend property that must be less than or equal to the
-- the number of messages to send (numMessagesMax). The worker will check for
-- this and report an error if messagesToSend is greater than numMessagesMax.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.util.all; use ocpi.types.all; use ocpi.wci.all;

architecture rtl of worker is

-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Signals for pattern logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
constant c_msg_ptr_width    : natural := width_for_max(to_integer(numMessagesMax*2) - 1); --Multiply numMessagesMax by 2 because there are 2 message fields
constant c_data_ptr_width   : natural := width_for_max(to_integer(numDataWords) - 1);
signal s_byte_enable        : std_logic_vector(3 downto 0) := (others => '0');
signal s_opcode             : std_logic_vector(7 downto 0) := (others => '0');
signal s_bytes_left         : ulong_t := (others => '0'); -- Keep track of how many bytes that are left to send for current message
signal s_messagesToSend     : ulong_t := (others => '0');
signal s_msg_ptr            : unsigned(c_msg_ptr_width-1 downto 0) := (others => '0'); -- Pointer for messages buffer
signal s_data_ptr           : unsigned(c_data_ptr_width-1 downto 0) := (others => '0'); -- Pointer for data buffer
signal s_dataSent           : ulong_t := (others => '0');
signal s_messagesSent       : ulong_t := (others => '0');
signal s_finished           : std_logic := '0';   -- Used to to stop sending messages
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Signals for combinatorial logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal s_som_r               : std_logic := '0'; -- Used to drive s_out_som
signal s_som_next_r          : std_logic := '0'; -- Used for logic for starting a message
signal s_eom_r               : std_logic := '0'; -- Used to drive s_out_eom
signal s_ready_r             : std_logic := '0'; -- Used to determine if ready to send message data
signal s_valid_r             : std_logic := '0'; -- Used for combinatorial logic for out_valid
signal s_give                : std_logic := '0'; -- Used for combinatorial logic for out_valid
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Mandatory output port logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal s_data_ready_for_out_port : std_logic := '0';
signal s_out_meta_is_reserved    : std_logic := '0';
signal s_out_som                 : std_logic := '0';
signal s_out_eom                 : std_logic := '0';
signal s_out_valid               : std_logic := '0';
signal s_out_eof                 : std_logic := '0';

-- Takes in the argument bytes_left and outputs the appropriate byte_enable
function read_bytes (bytes_left : ulong_t) return std_logic_vector is
    variable v_result : std_logic_vector(3 downto 0) := (others => '0');
    begin
        if bytes_left >= 4 then
           v_result := "1111";
        elsif bytes_left = 3 then
           v_result := "0111";
        elsif bytes_left = 2 then
           v_result := "0011";
        elsif bytes_left = 1 then
           v_result := "0001";
        elsif bytes_left = 0 then
           v_result := (others => '0');
        end if;
        return v_result;
 end function read_bytes;

begin


props_out.messagesToSend <= props_in.messagesToSend when ctl_in.state = INITIALIZED_e else
                            s_messagesToSend;

props_out.messagesSent <= s_messagesSent;
props_out.dataSent <= s_dataSent;

-- Mandatory output port logic, (note that
-- s_data_ready_for_out_port MUST be clock-aligned with out_out.data)
-- (note that reserved messages will be DROPPED ON THE FLOOR)
out_out.give <= s_give;

s_give <= out_in.ready and (not s_out_meta_is_reserved) and s_data_ready_for_out_port;


s_out_meta_is_reserved <= (not s_out_som) and (not s_out_valid) and (not s_out_eom);
out_out.som   <= s_out_som;
out_out.eom   <= s_out_eom;
out_out.valid <= s_out_valid;
out_out.eof <= s_out_eof;

s_out_som <= s_som_r;

s_out_eom <= s_eom_r;

s_out_valid <= out_in.ready and s_data_ready_for_out_port and s_valid_r;

s_data_ready_for_out_port <= s_ready_r;

out_out.data <= std_logic_vector(props_in.data(to_integer(s_data_ptr)));
out_out.byte_enable <= s_byte_enable;
out_out.opcode <= s_opcode;

ctl_out.finished <= s_finished;


-- This process handles the logic for the byte enable, opcode, som, eom, valid, and give for the
-- current message. It handles decrementing the messagesToSend counter, incrementing the messages
-- buffer pointer, and s_messagesSent counter. It also handles when the worker should stop sending messages
message_logic : process(ctl_in.clk)
begin
  if rising_edge(ctl_in.clk) then
    if ctl_in.reset = '1' then
      s_bytes_left  <= (others => '0');
      s_byte_enable  <= (others => '0');
      s_opcode <= (others => '0');
      s_ready_r <= '0';
      s_eom_r <=  '0';
      s_som_r <= '0';
      s_som_next_r <= '0';
      s_valid_r <= '0';
      s_messagesToSend <= (others => '0');
      s_finished <= '0';
      s_msg_ptr <= (others => '0');
      s_messagesSent <= (others => '0');
      s_out_eof <= '0';
    -- Grab the value of messagesToSend once it gets it's initial value
    elsif (ctl_in.control_op = START_e) then
      s_messagesToSend <= props_in.messagesToSend;
    elsif (s_finished = '0' and out_in.ready = '1') then
        -- Report an error if messagesToSend is greater than numMessagesMax
        if (s_messagesToSend > numMessagesMax) then
          report "messagesToSend is greater than numMessagesMax. messagesToSend must be less than or equal to numMessagesMax" severity failure;
        end if;
        s_som_r <= s_som_next_r;
        if s_eom_r = '1' then
            s_ready_r <= '0';
        end if;
        -- No more messages to send. Send an eof
        if s_messagesToSend = 0 then
            s_byte_enable <= (others=>'0');
            s_opcode <= (others=>'0');
            s_som_r <= '0';
            s_eom_r <= '0';
            s_ready_r <= '1';
            s_valid_r <= '0';
            s_out_eof <= '1';
        -- Starting a message
        elsif (s_bytes_left = 0 and s_som_next_r = '0') then
            s_eom_r<= '0';
            s_som_next_r <= '1';
            s_bytes_left <= props_in.messages(to_integer(s_msg_ptr));
            s_opcode <= std_logic_vector(resize(props_in.messages(to_integer(s_msg_ptr+1)), s_opcode'length));
            s_byte_enable <= read_bytes(props_in.messages(to_integer(s_msg_ptr)));
        -- Handle sending a message that is a ZLM
        elsif (s_bytes_left = 0 and s_som_next_r = '1') then
            s_som_next_r <= '0';
            s_byte_enable <= (others=>'0');
            s_eom_r <= '1';
            s_ready_r <= '1';
            s_valid_r <= '0';
            s_msg_ptr <= s_msg_ptr + 2;
            s_messagesToSend <= s_messagesToSend - 1;
            s_messagesSent <=  s_messagesSent + 1;
        -- Keep track of how many bytes are left for the current message and set byte enable
        elsif (s_bytes_left > 4) then
            s_som_next_r <= '0';
            s_bytes_left <= s_bytes_left - 4;
            s_byte_enable <= (others => '1');
            s_ready_r <= '1';
            s_valid_r <= '1';
        -- At the end of the message
        elsif (s_bytes_left <= 4) then
            s_som_next_r <= '0';
            s_bytes_left <= (others=>'0');
            s_ready_r <= '1';
            s_valid_r <= '1';
            s_eom_r <= '1';
            s_msg_ptr <= s_msg_ptr + 2;
            s_messagesToSend <= s_messagesToSend - 1;
            s_byte_enable <= read_bytes(s_bytes_left);
            s_messagesSent <=  s_messagesSent + 1;
        end if;
    -- Set set finished high and s_ready_r low after EOF is sent
    elsif (s_out_eof = '1') then
        s_finished <= '1';
        s_ready_r <= '0';
    end if;
  end if;
end process;

-- The process handles incrementing the data buffer pointer and s_dataSent counter
data_logic : process(ctl_in.clk)
begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
         s_data_ptr <= (others => '0');
         s_dataSent <= (others => '0');
      elsif (s_finished = '0' and out_in.ready = '1' and s_out_valid = '1') then
        s_dataSent <= s_dataSent + 1;
        if (props_in.dataRepeat = '1' and s_eom_r = '1') then
            s_data_ptr <= (others => '0');
        elsif (s_data_ptr < numDataWords-1) then
            s_data_ptr <= s_data_ptr + 1;
        end if;
      end if;
    end if;
end process;
end rtl;
