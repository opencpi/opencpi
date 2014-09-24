-------------------------------------------------------------------------------
-- Programmable FIR Filter - reduced to inner core by JEK: removed FIFO and tap memory
-------------------------------------------------------------------------------
--
-- File: fir_prog.vhd
-- Company: Geon Technologies, LLC
-- Engineer: Bob Martin
--
-- Description:
--
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity fir_inner is

generic(
   NUM_TAPS      : positive := 128;
   NUM_TAP_GRPS  : positive := 8;
   TAPS_PER_GRP  : positive := 128/8;
   TAP_IDX_WIDTH : natural  := 4;
   TAP_WIDTH     : positive := 16;
   MULT_PREC     : positive := 18;
   ADD_PREC      : positive := 20;
   DIN_WIDTH     : positive := 16;
   DOUT_WIDTH    : positive := 16);

port(
   CLK        : in std_logic;
   RST        : in std_logic;
   DIN        : in std_logic_vector(DIN_WIDTH-1 downto 0);
   DIN_VLD    : in std_logic;
   DOUT       : out std_logic_vector(DOUT_WIDTH-1 downto 0);
   DOUT_VLD   : out std_logic;

-- + added the tap array as a direct input to this module
   packed_taps: in std_logic_vector(0 to NUM_TAP_GRPS * TAP_WIDTH-1);
   tap_subidx : out std_logic_vector(TAP_IDX_WIDTH-1 downto 0));
end fir_inner;

architecture rtl of fir_inner is

   constant ZERO            : std_logic_vector(63 downto 0) := (others => '0');
   constant NUM_MULTS       : integer := NUM_TAP_GRPS;
   constant TAP_NUM_WIDTH   : integer := integer(ceil(log(real(NUM_TAPS))/log(2.0)));
   constant TAP_GRP_WIDTH   : integer := integer(ceil(log(real(NUM_TAP_GRPS))/log(2.0)));
   constant MULT_OUT_WIDTH  : integer := TAP_WIDTH + DIN_WIDTH;
   constant ADD_OUT_WIDTH   : integer := integer(realmax(real(MULT_PREC),real(ADD_PREC)));

   type taps_t              is array (0 to NUM_MULTS-1) of std_logic_vector(TAP_WIDTH-1 downto 0);
   type mult_out_t          is array (0 to NUM_MULTS-1) of std_logic_vector(MULT_OUT_WIDTH-1 downto 0);
   type mult_out_scale_t    is array (0 to NUM_MULTS-1) of std_logic_vector(MULT_PREC-1 downto 0);
   type mult_reg_t          is array (0 to NUM_TAPS-1) of std_logic_vector(MULT_PREC-1 downto 0);
   type add_out_t           is array (0 to NUM_TAPS-2) of std_logic_vector(ADD_OUT_WIDTH-1 downto 0);
   type add_reg_t           is array (0 to NUM_TAPS-1) of std_logic_vector(ADD_OUT_WIDTH-1 downto 0);
   type state_t             is (IDLE, PROC);

   signal taps              : taps_t := (others => (others => '0'));
   signal mult_out          : mult_out_t := (others => (others => '0'));
   signal mult_out_scale    : mult_out_scale_t := (others => (others => '0'));
   signal mult_reg          : mult_reg_t := (others => (others => '0'));
   signal add_out           : add_out_t := (others => (others => '0'));
   signal add_reg           : add_reg_t := (others => (others => '0'));
   signal cstate            : state_t;
   signal nstate            : state_t;

   signal fir_in            : std_logic_vector(DIN_WIDTH-1 downto 0);
   signal buff_empty        : std_logic;
   signal tap_idx           : std_logic_vector(TAP_IDX_WIDTH-1 downto 0);
   signal tap_idx_inc       : std_logic;
   signal dout_en_z1        : std_logic;
   signal proc_state        : std_logic;
   signal tap_idx_z1        : std_logic_vector(TAP_IDX_WIDTH-1 downto 0);
   signal tap_idx_z2        : std_logic_vector(TAP_IDX_WIDTH-1 downto 0);
   signal proc_state_z1     : std_logic;
   signal dout_en           : std_logic;
   signal mult_out_lat      : std_logic_vector(TAPS_PER_GRP-1 downto 0);

begin

-- + Directly wire input to "fir_in"
  fir_in <= DIN;
  buff_empty <= not DIN_VLD;
-- + Convert taps array since we don't have VHDL-200x..
  unpack: for i in 0 to NUM_MULTS-1 generate
    taps(i) <= packed_taps(i * TAP_WIDTH to i * TAP_WIDTH + TAP_WIDTH - 1);
  end generate;
  tap_subidx <= tap_idx;
   ----------------------------------------------------------------------------
   -- Tap Pointer
   ----------------------------------------------------------------------------

   -- The filter taps are stored in interleaved fashion among the individual
   -- tap memories associated with each multiplier used in the filter
   -- implementation. For sake of clarity in the code, the contents of each
   -- tap memory are referred to as a "tap group" and the address of each tap
   -- within a group is referred to as its "tap index".
   --
   -- As an example, a 13-tap filter using 3 multipliers would be built with
   -- 3 tap groups of 5 taps each (including two NULL taps):
   --
   --     Tap    Tap Group    Tap
   --    Group     Index     Number
   --
   --      0         0         0
   --      0         1         3
   --      0         2         6
   --      0         3         9
   --      0         4         12
   --
   --      1         0         1
   --      1         1         4
   --      1         2         7
   --      1         3         10
   --      1         4         -
   --
   --      2         0         2
   --      2         1         5
   --      2         2         8
   --      2         3         11
   --      2         4         -
   --
   -- The signals used to implement this structure are described below. The
   -- user is freed from concern over any of this of course. To begin
   -- accessing taps at tap 0 onward, the user simply writes to the PTR_ADDR
   -- address which sets tap_num = 0. Subsequent read or write accesses to the
   -- DATA_ADDR will increment tap_num and generate the corresponding tap_grp
   -- and tap_idx values.
   --
   -- user_tap_num = Actual Tap number from 0 to NUM_TAPS-1
   --                Increment with each R/W Access to DATA_ADDR
   --                Rollover at NUM_TAPS-1
   --
   -- user_tap_grp = Tap group number from 0 to NUM_MULTS-1
   --                Increment with each R/W Access to DATA_ADDR
   --                Rollover at NUM_MULTS-1
   --                Clear when tap_num rolls over
   --
   -- user_tap_idx = Tap Index number from 0 to TAPS_PER_GRP-1
   --                Increment each time tap_grp rolls over
   --                Clear when tap_num rolls over
   --

   ----------------------------------------------------------------------------
   -- Multipliers
   ----------------------------------------------------------------------------

   mult_i: for i in 0 to NUM_MULTS-1 generate

      mult_out(i) <= std_logic_vector(signed(taps(i)) * signed(fir_in));

      -- Strip off extra sign bit and keep MULT_PREC bits
      mult_out_scale(i) <= mult_out(i)(MULT_OUT_WIDTH-2 downto MULT_OUT_WIDTH-MULT_PREC-1);

   end generate;

   ----------------------------------------------------------------------------
   -- Storage for Multiplier Outputs
   ----------------------------------------------------------------------------

   mult_reg_i: for i in 0 to TAPS_PER_GRP-1 generate

      mult_reg_j: for j in 0 to NUM_MULTS-1 generate

         process (CLK)
         begin
            if rising_edge(CLK) then
               if (mult_out_lat(i) = '1') then
                  mult_reg(i*NUM_MULTS+j) <= mult_out_scale(j);
               end if;
            end if;
         end process;

      end generate;

   end generate;

   ----------------------------------------------------------------------------
   -- Adders
   ----------------------------------------------------------------------------

   -- Note: No adder for highest order tap

   adder_i: for i in 0 to NUM_TAPS-2 generate

      add_out(i) <= std_logic_vector( signed(mult_reg(i)) + signed(add_reg(i+1)) );

   end generate;

   ----------------------------------------------------------------------------
   -- Storage for Adder Outputs
   ----------------------------------------------------------------------------

   adder_reg_i: for i in 0 to NUM_TAPS-2 generate

      adder_reg: process (CLK)
      begin
         if rising_edge(CLK) then
            if (dout_en = '1') then
               add_reg(i) <= add_out(i);
            end if;
         end if;
      end process;

   end generate;

   -- Oddball reg for highest order tap (no adder)

   process (CLK)
   begin
      if rising_edge(CLK) then
         if (dout_en = '1') then
            add_reg(NUM_TAPS-1) <= std_logic_vector(resize(signed(mult_reg(NUM_TAPS-1)),ADD_OUT_WIDTH));
         end if;
      end if;
   end process;

   ----------------------------------------------------------------------------
   -- Final Output
   ----------------------------------------------------------------------------

   process (CLK)
   begin
      if rising_edge(CLK) then
         dout_en_z1 <= dout_en;
      end if;
   end process;

   -- Output precision: The adders should be set wide enough to account for
   -- in the adder chain; however, the final output width need not be that
   -- large. You don't want to scale (multiply) here, so just take the
   -- DOUT_WIDTH LSBs.

   DOUT <= add_reg(0)(DOUT_WIDTH-1 downto 0);
   DOUT_VLD <= dout_en_z1;

   ----------------------------------------------------------------------------
   -- State Machine
   ----------------------------------------------------------------------------

   process (CLK, RST)
   begin
      if (RST = '1') then
         cstate <= IDLE;
      elsif rising_edge(CLK) then
         cstate <= nstate;
      end if;
   end process;


   process (cstate, buff_empty, tap_idx)
   begin

      -- Default output assignments

      proc_state <= '0';
      tap_idx_inc <= '0';

      -- Next-State logic / Output assignments

      case (cstate) is

         when IDLE =>
            if (buff_empty = '1') then
               nstate <= IDLE;
            else
               nstate <= PROC;
            end if;

         when PROC =>
            proc_state <= '1';
            tap_idx_inc <= '1';
            if (unsigned(tap_idx) = TAPS_PER_GRP-1) and (buff_empty = '1') then
               nstate <= IDLE;
            else
               nstate <= PROC;
            end if;

      end case;

   end process;

   process (CLK, RST)
   begin
      if (RST = '1') then
         tap_idx <= (others => '0');
      elsif rising_edge(CLK) then
         if (tap_idx_inc = '1') then
            if (unsigned(tap_idx) = TAPS_PER_GRP-1) then
               tap_idx <= (others => '0');
            else
               tap_idx <= std_logic_vector(unsigned(tap_idx)+1);
            end if;
         end if;
      end if;
   end process;

   ----------------------------------------------------------------------------
   -- Generate control signals
   ----------------------------------------------------------------------------

   process (CLK)
   begin
      if rising_edge(CLK) then
         tap_idx_z1 <= tap_idx;
         tap_idx_z2 <= tap_idx_z1;
         proc_state_z1 <= proc_state;
      end if;
   end process;

   dout_en <= '1' when (unsigned(tap_idx_z2) = TAPS_PER_GRP-1) else '0';

   gen_mult_lat: for i in 0 to TAPS_PER_GRP-1 generate
      mult_out_lat(i) <= '1' when (unsigned(tap_idx_z1) = i) and (proc_state_z1 = '1') else '0';
   end generate;

   ----------------------------------------------------------------------------

end rtl;

