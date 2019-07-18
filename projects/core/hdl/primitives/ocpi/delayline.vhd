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

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; use ocpi.util.all;

entity delayline is
  generic (
    LATENCY : integer  := 1
  );
  port (
    CLK                : in std_logic;
    RESET              : in Bool_t;
    IS_OPERATING       : in Bool_t;
    IN_READY           : in Bool_t;
    IN_SOM             : in Bool_t;
    IN_EOM             : in Bool_t;
    IN_OPCODE          : in std_logic_vector;
    IN_VALID           : in Bool_t;
    IN_BYTE_ENABLE     : in std_logic_vector;
    IN_DATA            : in std_logic_vector;
    OUT_READY          : in Bool_t;
    OUT_SOM            : out Bool_t;
    OUT_EOM            : out Bool_t;
    OUT_OPCODE         : out std_logic_vector;
    OUT_VALID          : out Bool_t;
    OUT_BYTE_ENABLE    : out std_logic_vector;
    OUT_DATA           : out std_logic_vector
  );
end delayline;

architecture rtl of delayline is

  type data_array is array (natural range <>) of std_logic_vector(IN_DATA'range);
  type byte_enable_array is array (natural range <>) of std_logic_vector(IN_BYTE_ENABLE'range);
  type opcode_array is array (natural range <>) of std_logic_vector(IN_OPCODE'range);

  signal som              : std_logic_vector(LATENCY-1 downto 0);
  signal eom              : std_logic_vector(LATENCY-1 downto 0);
  signal valid            : std_logic_vector(LATENCY-1 downto 0);
  signal data             : data_array(LATENCY-1 downto 0);
  signal byte_enable      : byte_enable_array(LATENCY-1 downto 0);
  signal opcode           : opcode_array(LATENCY-1 downto 0);

begin

-----------------------------------------------------------------------------
-- Delay line to match the latency of the primitive for non-sample data
-----------------------------------------------------------------------------

  latency_eq_one_gen : if LATENCY = 1 generate
    delayLine : process (CLK)
    begin
      if rising_edge(CLK) then
        if (RESET = '1') then
          som              <= (others => '0');
          eom              <= (others => '0');
          opcode           <= (others => (others => '0'));
          valid            <= (others => '0');
          data             <= (others => (others => '0'));
          byte_enable      <= (others => (others => '0'));
        elsif (IS_OPERATING = '1' and OUT_READY = '1') then
          if IN_READY = '1' then
            som(0)         <= IN_SOM;
            eom(0)         <= IN_EOM;
            opcode(0)      <= IN_OPCODE;
            byte_enable(0) <= IN_BYTE_ENABLE;
            valid(0)       <= IN_VALID;
            data(0)        <= IN_DATA;
          else
            som            <= (others => '0');
            eom            <= (others => '0');
            opcode         <= (others => (others => '0'));
            valid          <= (others => '0');
            data           <= (others => (others => '0'));
            byte_enable    <= (others => (others => '0'));            
          end if;    
        end if;
      end if;
    end process delayLine;
  end generate latency_eq_one_gen;

  latency_gt_one_gen : if LATENCY > 1 generate
    delayLine : process (CLK)
    begin
      if rising_edge(CLK) then
        if (RESET = '1') then
          som                                 <= (others => '0');
          eom                                 <= (others => '0');
          opcode                              <= (others => (others => '0'));
          valid                               <= (others => '0');
          data                                <= (others => (others => '0'));
          byte_enable                         <= (others => (others => '0'));
        elsif (IS_OPERATING = '1' and OUT_READY = '1') then
          if IN_READY = '1' then
            som                               <= som(som'high-1 downto 0) & IN_SOM;
            eom                               <= eom(eom'high-1 downto 0) & IN_EOM;
            byte_enable                       <= byte_enable(byte_enable'high-1 downto 0) & IN_BYTE_ENABLE;
            opcode                            <= opcode(opcode'high-1 downto 0) & IN_OPCODE;
            valid                             <= valid(valid'high-1 downto 0) & IN_VALID;
            data                              <= data(data'high-1 downto 0) & IN_DATA;
          else
            som                               <= som(som'high-1 downto 0) & '0';
            eom                               <= eom(eom'high-1 downto 0) & '0';
            valid                             <= valid(valid'high-1 downto 0) & '0';
            --Upper array elements
            byte_enable(opcode'high downto 1) <= byte_enable(byte_enable'high-1 downto 0);
            opcode(opcode'high downto 1)      <= opcode(opcode'high-1 downto 0);
            data(opcode'high downto 1)        <= data(data'high-1 downto 0);
            --Bottom array element
            opcode(0)                         <= (others => '0');
            data(0)                           <= (others => '0');
            byte_enable(0)                    <= (others => '0');            
          end if;
        end if;
      end if;
    end process delayLine;
  end generate latency_gt_one_gen;

OUT_SOM    <= som(som'high);
OUT_EOM    <= eom(eom'high);
OUT_OPCODE <= opcode(opcode'high);
OUT_VALID   <= valid(valid'high);
OUT_BYTE_ENABLE  <= byte_enable(byte_enable'high);
OUT_DATA     <=   data(data'high);

end rtl;
