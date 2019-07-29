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
---------------------------------------------------------------------------------
--
-- Mult-Bit Synchronizer using Gray codes
--
-- Description:
--  Close-loop solution
--
--  A clock synchronization FIFO where the enqueue and dequeue sides are in
--  different clock domains.
--  There are no restrictions w.r.t. clock frequencies.
--  The depth of the FIFO must be a power of 2 (2,4,8,...) since the
--  indexing uses a Gray code counter.
--  Full and Empty signals are pessimistic, that is, they are asserted
--  immediately when the FIFO becomes FULL or EMPTY, but their deassertion
--  is delayed due to synchronization latency.
--
-- Generics:
--  WIDTH : width of data mult-bit data signal. >= 1 (default 1)
--  DEPTH : number of data words to store. Must be power of 2. >= 2 (default 2)
--
-- Latency:
--  dst_OUT : 1 src_CLK + 4 dst_CLk
--
-- Background:
--  - VHDL replacement for core/hdl/primitives/bsv/imports/SyncFIFO.v
--  - Gray coding synchronizer are discussed in Section 5.7 of
--  http://www.sunburst-design.com/papers/CummingsSNUG2008Boston_CDC.pdf
--
---------------------------------------------------------------------------------

library IEEE, ocpi;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity fifo is
  generic (
    WIDTH       : natural := 1;
    DEPTH       : natural := 2);           --minimum 2
  port (
    src_CLK     : in  std_logic;
    src_RST     : in  std_logic;
    src_ENQ     : in  std_logic;
    src_in      : in  std_logic_vector(WIDTH-1 downto 0);
    src_FULL_N  : out std_logic;
    dst_CLK     : in  std_logic;
    dst_DEQ     : in  std_logic;
    dst_out     : out std_logic_vector(WIDTH-1 downto 0);
    dst_EMPTY_N : out std_logic);
end entity fifo;

architecture rtl of fifo is

  constant INDXWIDTH : natural := ocpi.util.width_for_max(DEPTH-1);          --minimum 1
  function msb12set return std_logic_vector is
    variable v : std_logic_vector(INDXWIDTH downto 0) := (others => '0');
  begin
    v(INDXWIDTH downto INDXWIDTH-1) := "11";
    return v;
  end msb12set;
  -- FIFO Memory
  type t_fifoMem is array (0 to DEPTH-1) of std_logic_vector(WIDTH-1 downto 0);
--  signal fifoMem  : t_fifoMem := (others => (others => '0'));
  signal fifoMem  : t_fifoMem;
  signal dDoutReg : std_logic_vector(WIDTH-1 downto 0);

  -- Enqueue Pointer support
  signal sGEnqPtr, sGEnqPtr1          : std_logic_vector(INDXWIDTH+1 downto 0);
  signal sNotFullReg                  : std_logic;
  signal sNextNotFull, sFutureNotFull : std_logic;

  -- Dequeue Pointer support
  signal dGDeqPtr, dGDeqPtr1 : std_logic_vector(INDXWIDTH+1 downto 0);
  signal dNotEmptyReg        : std_logic;
  signal dNextNotEmpty       : std_logic;

  -- Reset generation
  signal dRST : std_logic;

  -- flops to sychronize enqueue and dequeue point across domains
  signal dSyncReg1, dEnqPtr : std_logic_vector(INDXWIDTH downto 0);
  signal sSyncReg1, sDeqPtr : std_logic_vector(INDXWIDTH downto 0);

  signal sEnqPtrIndx, dDeqPtrIndx : std_logic_vector(INDXWIDTH-1 downto 0);

  function or_reduce (arg : std_logic_vector)
    return std_logic is
    variable Upper, Lower : std_logic;
    variable Half         : integer;
    variable BUS_int      : std_logic_vector (arg'length - 1 downto 0);
    variable Result       : std_logic;
  begin
    if (arg'length < 1) then            -- In the case of a NULL range
      Result := '0';
    else
      BUS_int := to_ux01 (arg);
      if (BUS_int'length = 1) then
        Result := BUS_int (BUS_int'left);
      elsif (BUS_int'length = 2) then
        Result := BUS_int (BUS_int'right) or BUS_int (BUS_int'left);
      else
        Half   := (BUS_int'length + 1) / 2 + BUS_int'right;
        Upper  := or_reduce (BUS_int (BUS_int'left downto Half));
        Lower  := or_reduce (BUS_int (Half - 1 downto BUS_int'right));
        Result := Upper or Lower;
      end if;
    end if;
    return Result;
  end function or_reduce;

  function f_incrGray(grayin : std_logic_vector(INDXWIDTH downto 0);
                      parity : std_logic)
    return std_logic_vector is
    variable i         : integer;
    variable tempshift : std_logic_vector(INDXWIDTH downto 0);
    variable flips     : std_logic_vector(INDXWIDTH downto 0);
    variable incrGray  : std_logic_vector(INDXWIDTH downto 0);
  begin
    flips(0) := not parity;
    for i in 1 to INDXWIDTH loop
      tempshift := std_logic_vector(signed(grayin) sll (2 + INDXWIDTH - i));
      flips(i)  := parity and grayin(i-1) and (not or_reduce(tempshift));
    end loop;
    tempshift        := std_logic_vector(signed(grayin) sll 2);
    flips(INDXWIDTH) := parity and not (or_reduce(tempshift));
    incrGray         := flips xor grayin;
    return incrGray;
  end function;

  function f_incrGrayP(grayPin : std_logic_vector(INDXWIDTH+1 downto 0))
    return std_logic_vector is
    variable g         : std_logic_vector(INDXWIDTH downto 0);
    variable p         : std_logic;
    variable i         : std_logic_vector(INDXWIDTH downto 0);
    variable incrGrayP : std_logic_vector(INDXWIDTH+1 downto 0);
  begin
    g         := grayPin(INDXWIDTH+1 downto 1);
    p         := grayPin(0);
    i         := f_incrGray(g, p);
    incrGrayP := i & not p;
    return incrGrayP;
  end function;

begin

  assert (DEPTH = 2**INDXWIDTH)
    report "ERROR sync_fifo_ic.vhd: INDXWIDTH and DEPTH do not match. DEPTH must equal 2 ** INDXWIDTH"
    severity failure;

  -- Resets
  dRST        <= src_RST;
  -- Outputs
  dst_OUT    <= dDoutReg;
  dst_EMPTY_N <= dNotEmptyReg;
  src_FULL_N  <= sNotFullReg;

  -- Indexes are truncated from the Gray counter with parity
  sEnqPtrIndx <= sGEnqPtr(INDXWIDTH-1 downto 0);
  dDeqPtrIndx <= dGDeqPtr(INDXWIDTH-1 downto 0);

  -- Fifo memory write
  process (src_CLK)
  begin
    if rising_edge(src_CLK) then
      if (src_ENQ = '1') then
        fifoMem(to_integer(unsigned(sEnqPtrIndx))) <= src_IN;
      end if;
    end if;
  end process;

  ------------------------------------------------------------------------
  -- Enqueue Pointer and increment logic
  sNextNotFull   <= '1' when ((sGEnqPtr(INDXWIDTH+1 downto 1) xor msb12set) /= sDeqPtr)  else '0';
  sFutureNotFull <= '1' when ((sGEnqPtr1(INDXWIDTH+1 downto 1) xor msb12set) /= sDeqPtr) else '0';

  process (src_CLK, src_RST)
  begin
    if (src_RST = '1') then
      sGEnqPtr    <= (others                      => '0');
      sGEnqPtr1   <= (sGEnqPtr1'length-1 downto 2 => '0') & "11";
      sNotFullReg <= '0';  -- Mark as full during reset to avoid spurious loads
    elsif rising_edge(src_CLK) then
      if (src_ENQ = '1') then
        sGEnqPtr    <= sGEnqPtr1;
        sGEnqPtr1   <= f_incrGrayP(sGEnqPtr1);
        sNotFullReg <= sFutureNotFull;
      else
        sNotFullReg <= sNextNotFull;
      end if;
    end if;
  end process;

  -- Enqueue pointer synchronizer to dCLK
  process (dst_CLK, dRST)
  begin
    if (dRST = '1') then
      dSyncReg1 <= (others => '0');
      dEnqPtr   <= (others => '0');
    elsif rising_edge(dst_CLK) then
      dSyncReg1 <= sGEnqPtr(INDXWIDTH+1 downto 1);  -- Clock domain crossing
      dEnqPtr   <= dSyncReg1;
    end if;
  end process;

  -- Enqueue Pointer and increment logic
  dNextNotEmpty <= '1' when (dGDeqPtr(INDXWIDTH+1 downto 1) /= dEnqPtr) else '0';

  process (dst_CLK, dRST)
  begin
    if (dRST = '1') then
      dGDeqPtr     <= (others                      => '0');
      dGDeqPtr1    <= (dGDeqPtr1'length-1 downto 2 => '0') & "11";
      dNotEmptyReg <= '0';
    elsif rising_edge(dst_CLK) then
      if ((dNotEmptyReg = '0' or dst_DEQ = '1') and dNextNotEmpty = '1') then
        dGDeqPtr     <= dGDeqPtr1;
        dGDeqPtr1    <= f_incrGrayP(dGDeqPtr1);
        dDoutReg     <= fifoMem(to_integer(unsigned(dDeqPtrIndx)));
        dNotEmptyReg <= '1';
      elsif (dst_DEQ = '1' and dNextNotEmpty = '0') then
        dNotEmptyReg <= '0';
      end if;
    end if;
  end process;

  -- Dequeue pointer synchronized to sCLK
  process (src_CLK, src_RST)
  begin
    if (src_RST = '1') then
      sSyncReg1 <= (others => '0');
      sDeqPtr   <= (others => '0');
    elsif rising_edge(src_CLK) then
      sSyncReg1 <= dGDeqPtr(INDXWIDTH+1 downto 1);  -- clock domain crossing
      sDeqPtr   <= sSyncReg1;
    end if;
  end process;

end architecture rtl;
