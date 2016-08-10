-- THIS FILE WAS ORIGINALLY GENERATED ON Sun Apr  5 09:03:54 2015 EDT
-- BASED ON THE FILE: sdp_node.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: sdp_node
-- Note THIS IS THE OUTER skeleton, since the 'outer' attribute was set.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of sdp_node_rv is
  -- Combinatorial state for splitting downstream messages to "client" or "down"
  signal for_client       : bool_t; -- the message coming from up_in is for the client
  -- State for joining upstream messsages from "client" or "down".
  signal up_starting      : bool_t;
  signal up_ending        : bool_t;
  signal up_from_client   : bool_t;
  signal up_active_r      : bool_t; -- We are sending a message to up_out, not the first cycle
  signal up_from_client_r : bool_t; -- The current (or last) message is from client.
begin
  -------------------------------------------------------------------------  
  -- The first function is to route messages arriving on the up_in channel
  -- to either the client_out channel or the down_out channel.
  -- Since the header is stable for the length of the message, its easy.
  -- The "ready" signal is routed back to the "up" channel from the recipient.
  for_client <= to_bool(up_in.sdp.header.node = up_in.id);
  -- Copy the incoming upstream info to the client, qualifying the valid signal
  client_out.clk <= up_in.clk;
  client_out.reset <= up_in.reset;
  client_out.id <= up_in.id;
  client_out.sdp.header <= up_in.sdp.header;
  client_out.sdp.eop <= up_in.sdp.eop;
  client_out_data <= up_in_data;
  client_out.sdp.valid <= up_in.sdp.valid and for_client;
  -- Copy the incoming upstream info downstream, qualifying the valie signal
  down_out.clk <= up_in.clk;
  down_out.reset <= up_in.reset;
  down_out.id <= up_in.id + 1;
  down_out.sdp.header <= up_in.sdp.header;
  down_out.sdp.eop <= up_in.sdp.eop;
  down_out_data <= up_in_data;
  down_out.sdp.valid <= up_in.sdp.valid and not for_client;
  -- Accept the incoming frame from the recipient
  up_out.sdp.ready <= client_in.sdp.ready when its(for_client) else down_in.sdp.ready;
  up_out.dropCount <= down_in.dropCount;
  -------------------------------------------------------------------------  
  -- The "joining" function, from down and/or client to upstream
  -- This needs memory to implement fairness (LRU) so it has some state
  -- It also must keep the routing stable within a message.

  -- Are we starting an upstream message in THIS cycle?
  up_starting <= not up_active_r and (client_in.sdp.valid or down_in.sdp.valid);
  -- Are we ending a message in this cycle?
  up_ending <= client_in.sdp.valid and client_in.sdp.eop and up_in.sdp.ready
               when its(up_from_client) else
               down_in.sdp.valid and down_in.sdp.eop and up_in.sdp.ready;
  -- Is the current (or starting) message from client?
  up_from_client <= up_from_client_r when its(up_active_r) else
                    client_in.sdp.valid and (not down_in.sdp.valid or not up_from_client_r);
  -- mux into up_out channel
  up_out.sdp.header    <= client_in.sdp.header when its(up_from_client) else down_in.sdp.header;
  up_out.sdp.eop       <= client_in.sdp.eop    when its(up_from_client) else down_in.sdp.eop;
  up_out.sdp.valid     <= client_in.sdp.valid  when its(up_from_client) else down_in.sdp.valid;
  up_out_data          <= client_in_data       when its(up_from_client) else down_in_data;

  down_out.sdp.ready   <= up_in.sdp.ready and down_in.sdp.valid and not up_from_client;
  client_out.sdp.ready <= up_in.sdp.ready and client_in.sdp.valid and up_from_client;

  -- Our state machine for upstream messages - keep track of 2 FFs.
  work : process(up_in.clk)
  begin
    if rising_edge(up_in.clk) then
      if its(up_in.reset) then
        up_active_r      <= bfalse;
        up_from_client_r <= bfalse;
      else
        if up_starting and not its(up_ending) then
          up_from_client_r <= up_from_client; -- make this sticky through a message
          if not up_ending then
            up_active_r <= btrue;
          end if;
        end if;
        if its(up_ending) then
          up_active_r <= bfalse;
        end if;
      end if; -- end not reset
    end if; -- end rising edge
  end process;
end rtl;
