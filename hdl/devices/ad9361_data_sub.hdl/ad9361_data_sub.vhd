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

-- THIS FILE WAS ORIGINALLY GENERATED ON Fri Jun 30 18:51:05 2017 EDT
-- BASED ON THE FILE: ad9361_data_sub.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ad9361_data_sub

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of ad9361_data_sub_worker is

  -- TODO/FIXME - support bidirectional ports once framework support for parameterizable signal directions exists
  -- TSINOUT_1 primitive determines these values
  --constant OUTPUT_DISABLE : std_logic := '0';
  --constant OUTPUT_ENABLE  : std_logic := '1';

  constant adc_c : natural := 0;
  constant dac_c : natural := 1;

  constant iostandard_is_lvds : boolean := DIFFERENTIAL_p = btrue;


  signal DATA_CLK_P_s : std_logic := '0';
  signal P0_D_s    : std_logic_vector(P0_D'length-1 downto 0) := (others => '0');
  signal P1_D_s    : std_logic_vector(P1_D'length-1 downto 0) := (others => '0');

  signal port0     : std_logic_vector(P0_D'length-1 downto 0) := (others => '0');
  signal port1     : std_logic_vector(P1_D'length-1 downto 0) := (others => '0');
  -- TODO/FIXME - support bidirectional ports once framework support for parameterizable signal directions exists
  --signal P0_D_I_s  : std_logic_vector(P0_D'length-1 downto 0) := (others => '0');
  --signal P0_D_OE_s : std_logic_vector(P0_D'length-1 downto 0) := (others => '0');
  --signal P0_D_O_s  : std_logic_vector(P0_D'length-1 downto 0) := (others => '0');
  --signal P1_D_I_s  : std_logic_vector(P1_D'length-1 downto 0) := (others => '0');
  --signal P1_D_OE_s : std_logic_vector(P1_D'length-1 downto 0) := (others => '0');
  --signal P1_D_O_s  : std_logic_vector(P1_D'length-1 downto 0) := (others => '0');
  --signal port0_i   : std_logic_vector(P0_D'length-1 downto 0) := (others => '0');
  --signal port0_oe  : std_logic_vector(P0_D'length-1 downto 0) := (others => '0');
  --signal port0_o   : std_logic_vector(P0_D'length-1 downto 0) := (others => '0');
  --signal port1_i   : std_logic_vector(P1_D'length-1 downto 0) := (others => '0');
  --signal port1_oe  : std_logic_vector(P1_D'length-1 downto 0) := (others => '0');
  --signal port1_o   : std_logic_vector(P1_D'length-1 downto 0) := (others => '0');

begin

  dev_data_clk_out(adc_c).data_clk_p <= DATA_CLK_P_s;
  dev_data_clk_out(dac_c).data_clk_p <= DATA_CLK_P_s;

  noswap: if P0_P1_ARE_SWAPPED_p = bfalse generate
    -- this dev signal is used to (eventually) tell higher level proxies about
    -- the assumptions that were held when this worker was written
    dev_cfg_data_port_out.p0_p1_are_swapped <= '0';

    noswap_lvds : if iostandard_is_lvds = true generate
      P0_D_s <= port0;
      port1  <= P1_D_s;
    end generate;
    noswap_cmos : if iostandard_is_lvds = false generate
    -- TODO/FIXME - support CMOS modes once framework support for parameterizable signal directions exists
    --  P0_D_I_s  <= port0_i;
    --  P0_D_OE_s <= port0_oe;
    --  port0_o   <= P0_D_O_s;
    --  P1_D_OE_s <= port1_oe;
    --  P1_D_I_s  <= port1_i;
    --  port1_o   <= P1_D_O_s;
    end generate;
  end generate;
  swap: if P0_P1_ARE_SWAPPED_p = btrue generate
  -- TODO/FIXME - support port swapping once framework support for parameterizable signal directions exists
  -- this dev signal is used to (eventually) tell higher level proxies about
  -- the assumptions that were held when this worker was written
  --  dev_cfg_data_port_out.p0_p1_are_swapped <= '1';
  --
  --  P1_D_I_s  <= port0_i;
  --  P1_D_OE_s <= port0_oe;
  --  port0_o   <= P1_D_O_s;
  --  P0_D_OE_s <= port1_oe;
  --  P0_D_I_s  <= port1_i;
  --  port1_o   <= P0_D_O_s;
  end generate;

  -- TODO/FIXME - support CMOS modes once framework support for parameterizable signal directions exists
  cmos : if iostandard_is_lvds = false generate
  -- these dev signals are used to (eventually) tell higher level proxies about
  -- the assumptions that were held when this worker was written
  -- dev_cfg_data_port_out.iostandard_is_lvds <= '0';
  --
  --  single : if PORT_CONFIG_p = single_e generate
  --    half : if DUPLEX_CONFIG_p = half_duplex_e generate
  --      port_loop : for idx in port0_oe'length-1 downto 0 generate
  --        port0_oe(idx) <= OUTPUT_DISABLE when props_in.port_direction = rx_e else OUTPUT_ENABLE;
  --      end generate;
  --      port1_oe <= (others => OUTPUT_DISABLE);
  --      dev_data_adc_out.data <= "000000000000" & port0_o;
  --      port0_i <= dev_data_dac_in.data(11 downto 0);
  --    end generate;
  --    full : if DUPLEX_CONFIG_p = full_duplex_e generate
  --      port0_oe <= (5 downto 0  => OUTPUT_DISABLE,
  --                   11 downto 6 => OUTPUT_ENABLE);
  --      port1_oe <= (others => OUTPUT_DISABLE);
  --      dev_data_adc_out.data <= "000000000000000000" & port0_o(5 downto 0);
  --      port0_i(11 downto 6) <= dev_data_dac_in.data(5 downto 0);
  --    end generate;
  --  end generate;
  --  dual : if PORT_CONFIG_p = dual_e generate
  --    half : if DUPLEX_CONFIG_p = half_duplex_e generate
  --      port0_loop : for idx in port0_oe'length-1 downto 0 generate
  --        port0_oe(idx) <= OUTPUT_DISABLE when props_in.port_direction = rx_e else OUTPUT_ENABLE;
  --      end generate;
  --      port1_loop : for idx in port1_oe'length-1 downto 0 generate
  --        port0_oe(idx) <= OUTPUT_DISABLE when props_in.port_direction = rx_e else OUTPUT_ENABLE;
  --      end generate;
  --      dev_data_adc_out.data <= port0_o & port1_o;
  --      port0_i <= dev_data_dac_in.data(23 downto 12);
  --      port1_i <= dev_data_dac_in.data(11 downto 0);
  --    end generate;
  --    full : if DUPLEX_CONFIG_p = full_duplex_e generate
  --      port0_oe <= (others => OUTPUT_DISABLE);
  --      port1_oe <= (others => OUTPUT_ENABLE);
  --      dev_data_adc_out.data <= "000000000000" & port0_o;
  --      port1_i <= dev_data_dac_in.data(11 downto 0);
  --    end generate;
  --  end generate;
  --  signals : entity work.signals_cmos
  --    port map(
  --      w_DATA_CLK_P   => DATA_CLK_P_s,
  --      w_P0_D_I       => P0_D_I_s,
  --      w_P0_D_OE      => P0_D_OE_s,
  --      w_P0_D_O       => P0_D_O_s,
  --      w_P1_D_I       => P1_D_I_s,
  --      w_P1_D_OE      => P1_D_OE_s,
  --      w_P1_D_O       => P1_D_O_s,
  --      DATA_CLK_P     => DATA_CLK_P,
  --      DATA_CLK_N     => DATA_CLK_N,
  --      P0_D           => P0_D,
  --      P1_D           => P1_D);
  end generate;
  lvds : if iostandard_is_lvds = true generate
    -- these dev signals are used to (eventually) tell higher level proxies about
    -- the assumptions that were held when this worker was written
    dev_cfg_data_port_out.iostandard_is_lvds <= '1';

    dev_data_adc_out.data <= "000000000000000000" & port1(11) & -- P1_D11/RX_D5_P
                                                    port1(9 ) & -- P1_D9/RX_D4_P
                                                    port1(7 ) & -- P1_D7/RX_D3_P
                                                    port1(5 ) & -- P1_D5/RX_D2_P
                                                    port1(3 ) & -- P1_D3/RX_D1_P
                                                    port1(1 );  -- P1_D1/RX_D0_P
    port0 <= (11 => dev_data_dac_in.data(5), -- P0_D11/TX_D5_P
              10 => '0',
              9  => dev_data_dac_in.data(4), -- P0_D9/TX_D4_P
              8  => '0',
              7  => dev_data_dac_in.data(3), -- P0_D7/TX_D3_P
              6  => '0',
              5  => dev_data_dac_in.data(2), -- P0_D5/TX_D2_P
              4  => '0',
              3  => dev_data_dac_in.data(1), -- P0_D3/TX_D1_P
              2  => '0',
              1  => dev_data_dac_in.data(0), -- P0_D1/TX_D0_P
              0  => '0');
    signals : entity work.signals_lvds
      port map(
        w_DATA_CLK_P   => DATA_CLK_P_s,
        w_P0_D         => P0_D_s,
        w_P1_D         => P1_D_s,
        DATA_CLK_P     => DATA_CLK_P,
        DATA_CLK_N     => DATA_CLK_N,
        P0_D           => P0_D,
        P1_D           => P1_D);
  end generate;

end rtl;
