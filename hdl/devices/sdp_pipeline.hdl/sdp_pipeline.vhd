-- THIS FILE WAS ORIGINALLY GENERATED ON Thu Jun 16 23:55:16 2016 EDT
-- BASED ON THE FILE: sdp_pipeline.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: sdp_pipeline
-- Note THIS IS THE OUTER skeleton, since the 'outer' attribute was set.

library IEEE, ocpi, sdp;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, sdp.sdp.all;
architecture rtl of sdp_pipeline_rv is
  -- downstream
  signal down_r      : sdp_t;
  signal down_data_r : dword_array_t(down_out_data'range);
  signal load_down   : bool_t;
  -- upstream
  signal up_r        : sdp_t;
  signal up_data_r   : dword_array_t(up_out_data'range);
  signal load_up     : bool_t;
begin
  -- the nasty part of this is that we can't assign whole records because
  -- the take signals are in the opposite directions FIX THIS!

  -- First the flow-through signals not subject to pipelining
  down_out.clk        <= up_in.clk;
  down_out.reset      <= up_in.reset;
  down_out.id         <= up_in.id;
  -- downstream handshake up to accept a transfer into the downstream pipeline register
  load_down           <= up_in.sdp.valid and (not down_r.valid or
                                              (down_r.valid and down_in.sdp.ready));
  up_out.sdp.ready    <= load_down;
  down_out.sdp.header <= down_r.header;
  down_out.sdp.eop    <= down_r.eop;
  down_out.sdp.valid  <= down_r.valid;
  down_out_data       <= down_data_r;
  -- upstream handshake up to accept a transfer into the upstream pipeline register
  load_up             <= down_in.sdp.valid and (not up_r.valid or
                                                (up_r.valid and up_in.sdp.ready));
  down_out.sdp.ready  <= load_up;
  up_out.sdp.header   <= up_r.header;
  up_out.sdp.eop      <= up_r.eop;
  up_out.sdp.valid    <= up_r.valid;
  up_out_data         <= up_data_r;
  process (up_in.clk) is
  begin
    if rising_edge(up_in.clk) then
      if its(up_in.reset) then
        down_r.valid <= bfalse;
        up_r.valid  <= bfalse;
      else
        -- The downstream signals, conditionally set.
        if its(load_down) then
          -- load downstream pipeline register
          down_r.header <= up_in.sdp.header;
          down_r.eop    <= up_in.sdp.eop;  
          down_r.valid  <= btrue;
          down_data_r   <= up_in_data;
        elsif down_r.valid and up_in.sdp.ready then
          down_r.valid  <= bfalse;
        end if;
        if its(load_up) then
          -- load downstream pipeline register
          up_r.header <= down_in.sdp.header;
          up_r.eop    <= down_in.sdp.eop;  
          up_r.valid  <= btrue;
          up_data_r   <= down_in_data;
        elsif up_r.valid and down_in.sdp.ready then
          up_r.valid  <= bfalse;
        end if;
      end if; -- end of not reset
    end if; -- end of rising edge
  end process;
end rtl;
