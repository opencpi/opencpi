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
-- Programmable FIR Filter : Systolic Structure for Symmetric Taps (Even #)
-------------------------------------------------------------------------------
--
-- File: fir_systolic_sym_even.vhd
--
-- Description:
--
-- This FIR Filter implementation makes use of the symmetric systolic
-- structure contained in macc_systolic_sym to construct a filter with an even
-- number of taps and symmetry about its midpoint. A similar design for an odd
-- number of taps is found in fir_systolic_sym_odd.
--
-- By pre-adding data values that are destined for identically valued filter
-- coefficients, each filter section handles two taps using just one
-- multiplier. This cuts the number of multiplier resources required in half.
-- Furthermore, the pipelined architecture eliminates the need for a large
-- adder tree for output summations at the expense of some additional
-- propagation delay.
--
-- This filter is an ideal solution for applications requiring a data rate
-- that is close to the fmax of the target architecture. For applications
-- where the data is coming in well under 50% of fmax, a better solution may
-- be found by using the sequential or semi-parallel filter structures in:
--
--    fir_sequential.vhd
--    fir_semiparallel.vhd
--
-- While these alternate designs do not take advantage of tap symmetry, they
-- do employ efficient timesharing approaches for resources so that one
-- multiplier may be used to compute the results for many taps. For
-- applications where fmax / (data rate) is larger than 16 or so, the
-- sequential designs allow for the efficient storage of coefficients in
-- block RAM resources.
--
-- To avoid confusion over the number of taps in this design, the filter is
-- specified by the NUM_SECTIONS parameter, where
--
--   # of taps = 2 * NUM_SECTIONS
--
-- This eliminates the possibility of specifying an odd number of taps which
-- would be erroneous for this FIR structure.
--
-- The example diagram below shows what an 8-tap filter would look like
-- after specifying NUM_SECTIONS = 4. Note that each multiplier handles two
-- coefficients.
--
--    8-tap symmetric FIR
--    ===================
--
--           *---(8xR)----*------------*------------*------------*
--           |            |            |            |            |
--           |            |            |            |            |
--   DIN >---*--R--R--*---|--R--R--*---|--R--R--*---|--R--R--*   |
--                    |   |        |   |        |   |        |   |
--                    |   R        |   R        |   R        |   R
--                    |   |        |   |        |   |        |   |
--                   (+)--*       (+)--*       (+)--*       (+)--*
--                    |            |            |            |
--                    R            R            R            R
--             h(0/7) |     h(1/6) |     h(2/5) |     h(3/4) |
--                R--(X)       R--(X)       R--(X)       R--(X)
--                    |            |            |            |
--                    R            R            R            R
--                    |            |            |            |
--             0 >---(+)---R------(+)---R------(+)---R------(+)---R------> DOUT
--
--     R   : Register
--    (X)  : Multiplier
--    (+)  : Adder
--    h(*) : Filter taps h(n-1), h(n-2), ..., h(1), h(0)
--
-- This repeatable structure translates well into existing FPGA platforms.
-- The (8xR) structure is merely an N-clock delay. It will either be
-- implemented in dedicated delay circuits such as Xilinx's SRL16 or optimized
-- with the 2-clock delays contained in each section. Both DSP blocks offered
-- in current Altera and Xilinx FPGA's contain most of the resources for each
-- section in hard IP.
--
-- The following generic parameters define the construction of the FIR:
--
--    NUM_SECTIONS : The number of filter sections. For a number of taps
--                 : equal to N, ths would be set to N/2.
--    DATA_WIDTH   : Width of the filter input and final output data
--    COEFF_WIDTH  : Width of the programmable filter coefficients
--    ACC_PREC     : Accumulator Precision. This should be set to at least
--                 : DATA_WIDTH + COEFF_WIDTH + 1 + log2(NUM_SECTIONS)
--                 : to account for bit growth in each successive stage.
--    DATA_ADDR    : Address of the coefficient Data port
--    PTR_ADDR     : Address of the coefficient Pointer port
--    USE_COEFF_PTR : Write to a single address to load coefficients vs.
--                  : write the unique address for each coefficient
--
-- Coefficient Access:
--
-- When USE_COEFF_PTR = TRUE access to filter coefficients is implemented
-- through the coefficient pointer/data ports. A write to the pointer port
-- will clear the pointer and set access for coefficient zero. Any access to
-- the data port will increment the pointer, which rolls over back to zero
-- after reaching NUM_SECTIONS-1. When USE_COEFF_PTR = FALSE the DATA_ADDR and
-- PTR_ADDR values are not used. Instead, the ADDR input is used to index each
-- coefficient with an independent address.
--
-- Group Delay and Propagation Delay:
--
-- This filter will produce valid outputs one clock after each valid
-- input, but care must be exercised when attempting to align outputs
-- according to the filter's actual group delay and propagation delay.
--
-- For an FIR filter with symmetric impulse response we are guaranteed
-- to have linear phase response and thus constant group delay vs
-- frequency. In general the group delay will be equal to (N-1)/2 where
-- N is the number of filter taps.
--
-- The filter topology itself will add some propagation delay to the
-- response. For this design the total delay from an impulse input to the
-- beginning of the impulse response will be N/2 + 4 samples
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity fir_systolic_sym_even is

generic(

   NUM_SECTIONS  : positive;
   DATA_WIDTH    : positive;
   COEFF_WIDTH   : positive;
   ACC_PREC      : positive;
   DATA_ADDR     : std_logic_vector(15 downto 0);
   PTR_ADDR      : std_logic_vector(15 downto 0);
   USE_COEFF_PTR : boolean := true);

port(

   CLK        : in std_logic;
   RST        : in std_logic;
   DIN        : in std_logic_vector(DATA_WIDTH-1 downto 0);
   DIN_VLD    : in std_logic;
   DOUT       : out std_logic_vector(DATA_WIDTH-1 downto 0);
   DOUT_VLD   : out std_logic;

   ADDR       : in std_logic_vector(15 downto 0);
   RDEN       : in std_logic;
   WREN       : in std_logic;
   RDATA      : out std_logic_vector(COEFF_WIDTH-1 downto 0);
   WDATA      : in std_logic_vector(COEFF_WIDTH-1 downto 0));

end fir_systolic_sym_even;

architecture rtl of fir_systolic_sym_even is

   constant NUM_COEFF_c : integer := NUM_SECTIONS*2;

   component macc_systolic_sym
   generic (
      DATA_WIDTH   : positive;
      COEFF_WIDTH  : positive;
      ACC_PREC     : positive);
   port (
      CLK        : in std_logic;
      RST        : in std_logic;
      DIN_VLD    : in std_logic;
      DLY_IN_F   : in std_logic_vector(DATA_WIDTH-1 downto 0);
      DLY_IN_R   : in std_logic_vector(DATA_WIDTH-1 downto 0);
      DLY_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
      COEFF      : in std_logic_vector(COEFF_WIDTH-1 downto 0);
      ACC_IN     : in std_logic_vector(ACC_PREC-1 downto 0);
      ACC_OUT    : out std_logic_vector(ACC_PREC-1 downto 0));
   end component;

   type coeff_array_t is array (natural range <>) of std_logic_vector(COEFF_WIDTH-1 downto 0);
   type data_array_t  is array (natural range <>) of std_logic_vector(DATA_WIDTH-1 downto 0);
   type accum_array_t is array (natural range <>) of std_logic_vector(ACC_PREC-1 downto 0);

   signal coeff_ptr   : std_logic_vector(15 downto 0);
   signal coeff_wren  : std_logic_vector(NUM_SECTIONS-1 downto 0);
   signal coeff_reg   : coeff_array_t(0 to NUM_SECTIONS-1);
   signal rev_dly     : data_array_t(0 to NUM_COEFF_c) := (others => (others => '0'));
   signal dly_chain   : data_array_t(0 to NUM_SECTIONS) := (others => (others => '0'));
   signal acc_chain   : accum_array_t(0 to NUM_SECTIONS) := (others => (others => '0'));
   signal dout_scaled : std_logic_vector(2*ACC_PREC-1 downto 0);
   signal din_vld_z1  : std_logic := '0';

begin

   ----------------------------------------------------------------------------
   -- Coefficient Pointer
   ----------------------------------------------------------------------------

   -- A write to the pointer port will clear the pointer to zero
   -- Any access to the data port will increment the pointer
   -- The pointer rolls over after reaching NUM_SECTIONS-1

   ptr_logic: if USE_COEFF_PTR generate
      process (CLK)
      begin
         if rising_edge(CLK) then
            if (RST = '1') then
               coeff_ptr <= (others => '0');
            elsif (ADDR = PTR_ADDR) and (WREN = '1') then
               coeff_ptr <= (others => '0');
            elsif (ADDR = DATA_ADDR) and ((RDEN = '1') or (WREN = '1')) then
               if (unsigned(coeff_ptr) = NUM_SECTIONS-1) then
                  coeff_ptr <= (others => '0');
               else
                  coeff_ptr <= std_logic_vector(unsigned(coeff_ptr)+1);
               end if;
            end if;
         end if;
      end process;
   end generate ptr_logic;

   ----------------------------------------------------------------------------
   -- Coefficient Registers
   ----------------------------------------------------------------------------

   coeffreg_i: for i in 0 to NUM_SECTIONS-1 generate

      ptr_wren: if USE_COEFF_PTR generate
         coeff_wren(i) <= '1' when ( to_integer(unsigned(coeff_ptr)) = i ) and (WREN = '1') else '0';
      end generate ptr_wren;

      addr_wren: if not USE_COEFF_PTR generate
         coeff_wren(i) <= '1' when ( to_integer(unsigned(ADDR)) = i ) and (WREN = '1') else '0';
      end generate addr_wren;

      coeffreg: process (CLK)
      begin
         if rising_edge(CLK) then
            if (RST = '1') then
               coeff_reg(i) <= (others => '0');
            elsif (coeff_wren(i) = '1') then
               coeff_reg(i) <= WDATA;
            end if;
         end if;
      end process coeffreg;

   end generate coeffreg_i;

   ptr_rdata: if USE_COEFF_PTR generate
      RDATA <= coeff_reg( to_integer( unsigned(coeff_ptr) ) );
   end generate ptr_rdata;

   addr_rdata: if not USE_COEFF_PTR generate
      addr_rdata_reg: process (CLK)
      begin
         if rising_edge(CLK) then
            if (RST = '1') then
               RDATA <= (others => '0');
            else
               RDATA <= coeff_reg( to_integer( unsigned(ADDR) ) );
            end if;
         end if;
      end process addr_rdata_reg;
   end generate addr_rdata;

   ----------------------------------------------------------------------------
   -- Delay incoming data by N for reverse path
   ----------------------------------------------------------------------------

   rev_dly(0) <= DIN;

   revdly_i: for i in 1 to NUM_COEFF_c generate
      revdly: process (CLK)
      begin
         if rising_edge(CLK) then
            if (DIN_VLD = '1') then
               rev_dly(i) <= rev_dly(i-1);
            end if;
         end if;
      end process revdly;
   end generate revdly_i;

   ----------------------------------------------------------------------------
   -- Filter
   ----------------------------------------------------------------------------

   dly_chain(0) <= DIN;
   acc_chain(0) <= (others => '0');

   macc_i: for i in 0 to NUM_SECTIONS-1 generate

      macc: macc_systolic_sym
      generic map (
         DATA_WIDTH   => DATA_WIDTH,
         COEFF_WIDTH  => COEFF_WIDTH,
         ACC_PREC     => ACC_PREC)
      port map (
         CLK        => CLK,
         RST        => RST,
         DIN_VLD    => DIN_VLD,
         DLY_IN_F   => dly_chain(i),
         DLY_IN_R   => rev_dly(NUM_COEFF_c),
         DLY_OUT    => dly_chain(i+1),
         COEFF      => coeff_reg(i),
         ACC_IN     => acc_chain(i),
         ACC_OUT    => acc_chain(i+1));

   end generate macc_i;

   ----------------------------------------------------------------------------
   -- Final Output
   ----------------------------------------------------------------------------

   -- Take bottom DATA_WIDTH bits from accum for output
   -- No need to register or round this

   DOUT <= acc_chain(NUM_SECTIONS)(DATA_WIDTH-1 downto 0);

   process (CLK)
   begin
      if rising_edge(CLK) then
         DOUT_VLD <= DIN_VLD;
      end if;
   end process;

end rtl;

