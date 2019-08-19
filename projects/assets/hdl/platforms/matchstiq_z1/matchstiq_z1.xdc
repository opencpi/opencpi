# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

############################################################################
# Clock constraints                                                        #
############################################################################
# 10 ns period = 100000 KHz
create_clock -name clk_fpga_0 -period 10.000 [get_nets {ftop/pfconfig_metadata_out*[clk]}]

# GPS interface (matchstiq_z1)
set_property PACKAGE_PIN AB12 [get_ports {gps_uart_rx}];  # output from GPS module (NMEA)
set_property PULLUP      true [get_ports {gps_uart_rx}];
set_property PACKAGE_PIN AA12 [get_ports {gps_uart_tx}];  # input to GPS module

# Lime SPI interface
set_property PACKAGE_PIN V8   [get_ports {lime_spi_reset}];

set_property PACKAGE_PIN V9   [get_ports {lime_spi_sdo}];
set_property PACKAGE_PIN W8   [get_ports {lime_spi_sdio}];
set_property PACKAGE_PIN AB10 [get_ports {lime_spi_sen}];
set_property PACKAGE_PIN V10  [get_ports {lime_spi_sclk}];

# i2c interface (matchstiq_z1 - tristate by default)
set_property PACKAGE_PIN AB19 [get_ports {matchstiq_z1_i2c_SCL}];
set_property PULLUP      true [get_ports {matchstiq_z1_i2c_SCL}];

set_property PACKAGE_PIN AB20 [get_ports {matchstiq_z1_i2c_SDA}];
set_property PULLUP      true [get_ports {matchstiq_z1_i2c_SDA}];

############################################################################
# Differential Clock input from Clock 0 port on Si5338 Clock Generator
# These clocks are passed through a IBUFGDS and out to the Lime IC on
# the rx_clk_out pin. This clocking scheme matches the Epiq reference
# design
############################################################################
set_property PACKAGE_PIN Y18  [get_ports {SI5338_CLK0A}];
set_property DIFF_TERM   true [get_ports {SI5338_CLK0A}];

set_property PACKAGE_PIN AA18 [get_ports {SI5338_CLK0B}];
set_property DIFF_TERM   true [get_ports {SI5338_CLK0B}];

#Clock output to Lime RX interface
set_property PACKAGE_PIN AA16 [get_ports {LIME_RX_CLK}];

#Clock input from Lime RX interface. Aligned with rxIn pins
set_property PACKAGE_PIN Y9   [get_ports {lime_adc_RX_CLK_OUT}];
set_property PULLUP      true [get_ports {lime_adc_RX_CLK_OUT}];
set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets {lime_adc_RX_CLK_OUT}];

#Clock output to Lime TX interface. Derived from rx_clk_in pin
set_property PACKAGE_PIN U20  [get_ports {lime_dac_TX_CLK}];

#Lime TX interface enable. Routed to GPIO pin 19 on rear debug connector
#set_property PACKAGE_PIN U6  [get_ports {lime_tx_txen}];


# Routed to GPIO pin 19 on the rear debug connector
set_property PACKAGE_PIN U6  [get_ports {MATCHSTIQ_Z1_GPIO_SLOT_FPGA_GPIO1}];
# Routed to GPIO pin 3 on the rear debug connector
set_property PACKAGE_PIN Y10  [get_ports {MATCHSTIQ_Z1_GPIO_SLOT_FPGA_GPIO2}];
# Routed to GPIO pin 4 on the rear debug connector
set_property PACKAGE_PIN Y6  [get_ports {MATCHSTIQ_Z1_GPIO_SLOT_FPGA_GPIO3}];


#TX Interface
set_property PACKAGE_PIN AA14 [get_ports {lime_dac_TX_IQ_SEL}];

set_property PACKAGE_PIN Y21  [get_ports {lime_dac_TXD[0]}];
set_property PACKAGE_PIN Y14  [get_ports {lime_dac_TXD[1]}];
set_property PACKAGE_PIN Y20  [get_ports {lime_dac_TXD[2]}];
set_property PACKAGE_PIN AA13 [get_ports {lime_dac_TXD[3]}];
set_property PACKAGE_PIN AB22 [get_ports {lime_dac_TXD[4]}];
set_property PACKAGE_PIN Y13  [get_ports {lime_dac_TXD[5]}];
set_property PACKAGE_PIN AA22 [get_ports {lime_dac_TXD[6]}];
set_property PACKAGE_PIN AB17 [get_ports {lime_dac_TXD[7]}];
set_property PACKAGE_PIN AB21 [get_ports {lime_dac_TXD[8]}];
set_property PACKAGE_PIN AA17 [get_ports {lime_dac_TXD[9]}];
set_property PACKAGE_PIN AA21 [get_ports {lime_dac_TXD[10]}];
set_property PACKAGE_PIN AB16 [get_ports {lime_dac_TXD[11]}];

#RX Interface
set_property PACKAGE_PIN V20  [get_ports {lime_adc_RX_IQ_SEL}];

set_property PACKAGE_PIN W22  [get_ports {lime_adc_RXD[0]}];
set_property PACKAGE_PIN T22  [get_ports {lime_adc_RXD[1]}];
set_property PACKAGE_PIN V22  [get_ports {lime_adc_RXD[2]}];
set_property PACKAGE_PIN U22  [get_ports {lime_adc_RXD[3]}];
set_property PACKAGE_PIN W21  [get_ports {lime_adc_RXD[4]}];
set_property PACKAGE_PIN U17  [get_ports {lime_adc_RXD[5]}];
set_property PACKAGE_PIN W20  [get_ports {lime_adc_RXD[6]}];
set_property PACKAGE_PIN V17  [get_ports {lime_adc_RXD[7]}];
set_property PACKAGE_PIN AA19 [get_ports {lime_adc_RXD[8]}];
set_property PACKAGE_PIN T21  [get_ports {lime_adc_RXD[9]}];
set_property PACKAGE_PIN Y19  [get_ports {lime_adc_RXD[10]}];
set_property PACKAGE_PIN U21  [get_ports {lime_adc_RXD[11]}];

#1PPS output from GPS module
set_property PACKAGE_PIN Y11  [get_ports {GPS_1PPS_IN}];
set_property PULLUP      true [get_ports {GPS_1PPS_IN}];
#set_property PACKAGE_PIN AB11  [get_ports {EXT_1PPS_IN}];
#set_property PULLUP      true [get_ports {EXT_1PPS_IN}];
set_property PACKAGE_PIN AA11 [get_ports {EXT_1PPS_OUT}];
set_property PACKAGE_PIN V13  [get_ports {GPS_FIX_IND}];

#ATLAS LEDs
set_property PACKAGE_PIN T4   [get_ports {ATLAS_LEDS[2]}];
set_property DRIVE       12   [get_ports {ATLAS_LEDS[2]}]
set_property PACKAGE_PIN U4   [get_ports {ATLAS_LEDS[1]}];
set_property DRIVE       12   [get_ports {ATLAS_LEDS[1]}]
set_property PACKAGE_PIN V4   [get_ports {ATLAS_LEDS[0]}];
set_property DRIVE       12   [get_ports {ATLAS_LEDS[0]}]

#create_pblock pblock_matchstiq_z1_i; add_cells_to_pblock [get_pblocks pblock_matchstiq_z1_i] [get_cells [list {ftop/pfconfig_i/matchstiq_z1_i}]];
#resize_pblock [get_pblocks pblock_matchstiq_z1_i] -add {SLICE_X26Y88:SLICE_X49Y110};


# All IOSTANDARDS should be set here. The LAST IOSTANDARD applied
# to a port sticks, so setting them above may have no effect.
#
# All of the ports use LVCMOS25, except a few.
# So, we set them all to LVCMOS25, and then overwrite the two
# SI5338 exceptions.
set_property IOSTANDARD LVCMOS25 [get_ports];
set_property IOSTANDARD LVDS_25  [get_ports {SI5338_CLK0*}];
