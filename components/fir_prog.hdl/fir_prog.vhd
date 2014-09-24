-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Dec 26 16:08:01 2012 EST
-- BASED ON THE FILE: gen/fir_prog.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: fir_prog

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all; use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library fir_prog;
architecture rtl of fir_prog_worker is
  constant NUM_TAPS      : positive := props_write.taps'length;
  constant NUM_TAP_GRPS  : positive := 8; -- this is the true input we choose
  constant TAPS_PER_GRP  : positive := positive(ceil(real(NUM_TAPS)/real(NUM_TAP_GRPS)));
  constant TAP_IDX_WIDTH : natural  := natural(ceil(log(real(TAPS_PER_GRP))/log(2.0)));
  constant TAP_WIDTH     : positive := props_write.taps(0)'length;
  -- packed tap array for inner fir
  signal inner_taps      : std_logic_vector(0 to NUM_TAP_GRPS * TAP_WIDTH - 1);
  signal tap_within_grp  : std_logic_vector(TAP_IDX_WIDTH-1 downto 0); 
  signal my_take         : bool_t;
  signal my_give         : bool_t;
begin
  -- convert the wide fully registered tap output to the RAM-style storage for the inner block
  convert_taps: for i in 0 to NUM_TAP_GRPS-1 generate
    inner_taps(i * TAP_WIDTH to i * TAP_WIDTH + TAP_WIDTH-1) <=
      std_logic_vector(props_write.taps(i * TAPS_PER_GRP + to_integer(unsigned(tap_within_grp))));
  end generate;
  my_take <= in_in.ready and (not in_in.valid or out_in.ready);
  take <= my_take;
  give <= my_give;
  -- instantiate the inner block
inner: entity work.fir_inner
  generic map(NUM_TAPS       => NUM_TAPS,
              NUM_TAP_GRPS   => NUM_TAP_GRPS,
              TAPS_PER_GRP   => TAPS_PER_GRP,
              TAP_IDX_WIDTH  => TAP_IDX_WIDTH,
              TAP_WIDTH      => TAP_WIDTH)
  port map(clk         => ctl_in.clk, -- should be data clock
           rst         => in_in.reset,
           din         => in_in.data,
           din_vld     => my_take and in_in.valid,
           dout        => out_out.data,
           dout_vld    => my_give,
           packed_taps => inner_taps,
           tap_subidx  => tap_within_grp);

  -- bookkeepping for message boundaries.
  --   ping-pong counters for the number of values in the message
  --   we attempt to support ZLMs too
  process(ctl_in.clk, ctl_in.reset)
    type count_t is array (natural range <>) of unsigned(15 downto 0);
    signal counts : count_t(0 to 1);
    signal which_in, which_out : std_logic;
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset then
        counts <= (0, 0);
        which_in <= 0;
        which_out <= 0;
      elsif which_in = which_out then
        if my_take and in_in.valid then
          -- no counts move since we're incrementing and decrementing
          
        if not my_take or in_in.som or not my_give then
          if my_take then
            if in_in.som then
              counts(which_in) <= 1 when in_in.valid else 0;
            elsif in_in.valid then
              counts(which_in) <= counts(which_in) + 1;
        end if;
        if in_in.eom then
          which_in <= not which_in;
        end if;
      end if;
      if my_give then
        counts(which_out) <= counts(which_out) - 1;
        if my_eom then
          which_out <= not which_out;
        end if;
      end if;
    end if;
  end process;
end rtl;
