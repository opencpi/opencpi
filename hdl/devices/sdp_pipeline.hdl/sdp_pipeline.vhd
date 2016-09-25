-- THIS FILE WAS ORIGINALLY GENERATED ON Thu Jun 16 23:55:16 2016 EDT
-- BASED ON THE FILE: sdp_pipeline.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: sdp_pipeline
-- Note THIS IS THE OUTER skeleton, since the 'outer' attribute was set.

-- header
-- sdp_header_width:
-- op 2
-- xid 3
-- lead 2
-- trail 2
-- node 4
-- count 12
-- addr 10 + 14
-- extaddr 10: total is 61 + 32 + 1 is 0 to 93

library IEEE, ocpi, sdp, util;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, sdp.sdp.all;
architecture rtl of sdp_pipeline_rv is
  constant sdp_width_c : positive := to_integer(sdp_width);
  constant width_c : positive := sdp_width_c * dword_size + sdp_header_ndws*32 + 1;
  subtype data_t is std_logic_vector(width_c-1 downto 0);
  signal up_enq, up_deq, up_full, up_empty : bool_t;
  signal up_data_in, up_data_out : data_t;
  signal down_enq, down_deq, down_full, down_empty : bool_t;
  signal down_data_in, down_data_out : data_t;
  function sdp2slv(dws : dword_array_t; sdp : sdp_t) return data_t is
    variable v : data_t;
  begin
    for i in 0 to sdp_width_c-1 loop
      v(i*32+31 downto i*32) := dws(i);
    end loop;
    for i in 0 to sdp_header_ndws-1 loop
      v((i+sdp_width_c)*32+31 downto (i+sdp_width_c)*32) := header2dws(sdp.header)(i);
    end loop;
    v(v'left) := sdp.eop;
    return v;
  end sdp2slv;
  function slv2sdp_data(slv : data_t) return dword_array_t is
    variable v : dword_array_t(0 to sdp_width_c-1);
  begin
    for i in 0 to sdp_width_c-1 loop
      v(i) := slv(i*32+31 downto i*32);
    end loop;
    return v;
  end slv2sdp_data;
  function slv2sdp(slv : data_t; empty : bool_t; enq : bool_t) return sdp_t is
    variable v : dword_array_t(0 to sdp_header_ndws-1);
    variable sdp : sdp_t;
  begin
    for i in 0 to sdp_header_ndws-1 loop
      v(i) := slv((i+sdp_width_c)*32+31 downto (i+sdp_width_c)*32);
    end loop;
    sdp.header := dws2header(v);
    sdp.eop := slv(slv'left);
    sdp.valid := not empty;
    sdp.ready := enq;
    return sdp;
  end slv2sdp;
begin
  -- data path conversions between bits (for fifo) and dwords
  up_data_in  <= sdp2slv(down_in_data, down_in.sdp);
  up_out_data <= slv2sdp_data(up_data_out);
  up_out.sdp  <= slv2sdp(up_data_out, up_empty, down_enq);

  down_data_in  <= sdp2slv(up_in_data, up_in.sdp);
  down_out_data <= slv2sdp_data(down_data_out);
  down_out.sdp  <= slv2sdp(down_data_out, down_empty, up_enq);

  -- the unfortunate part of this is that we can't assign whole records because
  -- the take/ready signals are in the opposite directions for other reasons

  -- First the flow-through signals not subject to pipelining
  down_out.clk        <= up_in.clk;
  down_out.reset      <= up_in.reset;
  down_out.id         <= up_in.id;
  up_out.dropCount    <= down_in.dropCount;

  up_enq              <= down_in.sdp.valid and not up_full;
  up: component util.util.srl_fifo
    generic map(width  => width_c)
    port map   (clk    => up_in.clk,
                reset  => up_in.reset,
                enq    => up_enq,
                deq    => up_in.sdp.ready,
                input  => up_data_in,
                full   => up_full,
                empty  => up_empty,
                output => up_data_out);

  down_enq           <= up_in.sdp.valid and not down_full;
  down: component util.util.srl_fifo
    generic map(width  => width_c)
    port map   (clk    => up_in.clk,
                reset  => up_in.reset,
                enq    => down_enq,
                deq    => down_in.sdp.ready,
                input  => down_data_in,
                full   => down_full,
                empty  => down_empty,
                output => down_data_out);

  ---- downstream handshake up to accept a transfer into the downstream pipeline register
  --load_down           <= up_in.sdp.valid and (not down_r.valid or
  --                                            (down_r.valid and down_in.sdp.ready));
  --up_out.sdp.ready    <= load_down;
  --down_out.sdp.header <= down_r.header;
  --down_out.sdp.eop    <= down_r.eop;
  --down_out.sdp.valid  <= down_r.valid;
  --down_out_data       <= down_data_r;
  ---- upstream handshake up to accept a transfer into the upstream pipeline register
  --load_up             <= down_in.sdp.valid and (not up_r.valid or
  --                                              (up_r.valid and up_in.sdp.ready));
  --down_out.sdp.ready  <= load_up;
  --up_out.sdp.header   <= up_r.header;
  --up_out.sdp.eop      <= up_r.eop;
  --up_out.sdp.valid    <= up_r.valid;
  --up_out_data         <= up_data_r;
  --process (up_in.clk) is
  --begin
  --  if rising_edge(up_in.clk) then
  --    if its(up_in.reset) then
  --      down_r.valid <= bfalse;
  --      up_r.valid  <= bfalse;
  --    else
  --      -- The downstream signals, conditionally set.
  --      if its(load_down) then
  --        -- load downstream pipeline register
  --        down_r.header <= up_in.sdp.header;
  --        down_r.eop    <= up_in.sdp.eop;  
  --        down_r.valid  <= btrue;
  --        down_data_r   <= up_in_data;
  --      elsif down_r.valid and down_in.sdp.ready then
  --        down_r.valid  <= bfalse;
  --      end if;
  --      if its(load_up) then
  --        -- load downstream pipeline register
  --        up_r.header <= down_in.sdp.header;
  --        up_r.eop    <= down_in.sdp.eop;  
  --        up_r.valid  <= btrue;
  --        up_data_r   <= down_in_data;
  --      elsif up_r.valid and up_in.sdp.ready then
  --        up_r.valid  <= bfalse;
  --      end if;
  --    end if; -- end of not reset
  --  end if; -- end of rising edge
  --end process;
  --up : component FIFO2 is
  --  generic map(width => sdp_width * dword_bits)
  --  port map(CLK =>,
  --           RST =>,
  --           D_IN =>,
  --           ENQ =>,
  --           FULL_N =>,
  --           D_OUT =>,
  --           DEQ =>,
  --           EMPTY_N =>,
  --           CLR =>);
end rtl;
