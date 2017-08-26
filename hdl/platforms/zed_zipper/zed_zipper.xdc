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

# Extracted form the zedboard_master_XDC_RevC_D_v3.xdc
############################################################################
# Clock constraints                                                        #
############################################################################
# 20 ns period = 50000 KHz
create_clock -name clk_fpga_0 -period 20.000 [get_nets {ftop/pfconfig_cp_out*[clk]}]

# ----------------------------------------------------------------------------
# User LEDs - Bank 33
# ---------------------------------------------------------------------------- 
set_property PACKAGE_PIN T22 [get_ports {led[0]}];  # "LD0"
set_property PACKAGE_PIN T21 [get_ports {led[1]}];  # "LD1"
set_property PACKAGE_PIN U22 [get_ports {led[2]}];  # "LD2"
set_property PACKAGE_PIN U21 [get_ports {led[3]}];  # "LD3"
set_property PACKAGE_PIN V22 [get_ports {led[4]}];  # "LD4"
set_property PACKAGE_PIN W22 [get_ports {led[5]}];  # "LD5"
set_property PACKAGE_PIN U19 [get_ports {led[6]}];  # "LD6"
set_property PACKAGE_PIN U14 [get_ports {led[7]}];  # "LD7"

############################################################################
# Myriad Zipper Constraints                                                #
############################################################################
# Pins of the lime_dac device

set_property PACKAGE_PIN C17 [get_ports {lime_tx_txen}];
set_property PACKAGE_PIN C18 [get_ports {lime_dac_TX_IQ_SEL}];

set_property PACKAGE_PIN C22 [get_ports {lime_dac_TXD[0]}];  # "FMC-LA25_N"
set_property PACKAGE_PIN D22 [get_ports {lime_dac_TXD[1]}];  # "FMC-LA25_P"
set_property PACKAGE_PIN F19 [get_ports {lime_dac_TXD[2]}];  # "FMC-LA22_N"
set_property PACKAGE_PIN G19 [get_ports {lime_dac_TXD[3]}];  # "FMC-LA22_P"
set_property PACKAGE_PIN G21 [get_ports {lime_dac_TXD[4]}];  # "FMC-LA20_N"
set_property PACKAGE_PIN G20 [get_ports {lime_dac_TXD[5]}];  # "FMC-LA20_P"

set_property PACKAGE_PIN K21 [get_ports {lime_dac_TXD[6]}];   # "FMC-LA16_N"
set_property PACKAGE_PIN J20 [get_ports {lime_dac_TXD[7]}];   # "FMC-LA16_P"
set_property PACKAGE_PIN P21 [get_ports {lime_dac_TXD[8]}];   # "FMC-LA12_N"
set_property PACKAGE_PIN P20 [get_ports {lime_dac_TXD[9]}];   # "FMC-LA12_P"
set_property PACKAGE_PIN J22 [get_ports {lime_dac_TXD[10]}];  # "FMC-LA08_N"
set_property PACKAGE_PIN J21 [get_ports {lime_dac_TXD[11]}];  # "FMC-LA08_P"

# Pins of the lime_adc device
set_property PACKAGE_PIN P17 [get_ports {lime_adc_RX_CLK_IN}];  # "FMC-LA02_P"
set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets {lime_adc_RX_CLK_IN_IBUF}];
set_property PACKAGE_PIN N20 [get_ports {lime_rx_rxen}];        # "FMC-LA01_CC_N"
set_property PACKAGE_PIN N19 [get_ports {lime_adc_RX_IQ_SEL}];  # "FMC-LA01_CC_P" - corrected 6/6/16 GE

# Timing Constraints ...
create_clock -name LIME_ADC_RX_CLK_IN -period 20.000 -waveform {0 10.000} [get_ports {lime_adc_RX_CLK_IN}]

set_property PACKAGE_PIN E18 [get_ports {lime_adc_RXD[0]}];   # "FMC-LA26_N"
set_property PACKAGE_PIN F18 [get_ports {lime_adc_RXD[1]}];   # "FMC-LA26_P"
set_property PACKAGE_PIN D15 [get_ports {lime_adc_RXD[2]}];   # "FMC-LA23_N"
set_property PACKAGE_PIN E15 [get_ports {lime_adc_RXD[3]}];   # "FMC-LA23_P"
set_property PACKAGE_PIN B20 [get_ports {lime_adc_RXD[4]}];   # "FMC-LA17_CC_N"
set_property PACKAGE_PIN B19 [get_ports {lime_adc_RXD[5]}];   # "FMC-LA17_CC_P"
set_property PACKAGE_PIN M17 [get_ports {lime_adc_RXD[6]}];   # "FMC-LA13_N"
set_property PACKAGE_PIN L17 [get_ports {lime_adc_RXD[7]}];   # "FMC-LA13_P"
set_property PACKAGE_PIN R21 [get_ports {lime_adc_RXD[8]}];   # "FMC-LA09_N"
set_property PACKAGE_PIN R20 [get_ports {lime_adc_RXD[9]}];   # "FMC-LA09_P"
set_property PACKAGE_PIN K18 [get_ports {lime_adc_RXD[10]}];  # "FMC-LA05_N"
set_property PACKAGE_PIN J18 [get_ports {lime_adc_RXD[11]}];  # "FMC-LA05_P"

# Pins of the lime_spi subdevice
set_property PACKAGE_PIN L21 [get_ports {lime_spi_reset}];  # "FMC-LA06_P"
set_property PACKAGE_PIN C20 [get_ports {lime_spi_sdio}];   # "FMC-LA18_CC_N"
set_property PACKAGE_PIN K20 [get_ports {lime_spi_sdo}];    # "FMC-LA14_N"
set_property PACKAGE_PIN D20 [get_ports {lime_spi_sclk}];   # "FMC-LA18_CC_P"
set_property PACKAGE_PIN K19 [get_ports {lime_spi_sen}];    # "FMC-LA14_P"

# Pins of the si5351 device
# The I2C pin assignments are reversed from the FMC slot pin names
# because the Zipper reverses the pins between the FMC and the Si5351 I2C
# drive=12, IOSTANDARD=LVCMOS25, SLEW=SLOW
set_property PACKAGE_PIN U7   [get_ports {zipper_i2c_scl}];  # "FMC-SDA"
set_property SLEW        SLOW [get_ports {zipper_i2c_scl}];
set_property DRIVE       12   [get_ports {zipper_i2c_scl}];
set_property PULLUP      true [get_ports {zipper_i2c_scl}];

set_property PACKAGE_PIN R7   [get_ports {zipper_i2c_sda}];  # "FMC-SCL"
set_property SLEW        SLOW [get_ports {zipper_i2c_sda}];
set_property DRIVE       12   [get_ports {zipper_i2c_sda}];
set_property PULLUP      true [get_ports {zipper_i2c_sda}];

# ----------------------------------------------------------------------------
# IOSTANDARD Constraints
#
# Note that these IOSTANDARD constraints are applied to all IOs currently
# assigned within an I/O bank.  If these IOSTANDARD constraints are 
# evaluated prior to other PACKAGE_PIN constraints being applied, then 
# the IOSTANDARD specified will likely not be applied properly to those 
# pins.  Therefore, bank wide IOSTANDARD constraints should be placed 
# within the XDC file in a location that is evaluated AFTER all 
# PACKAGE_PIN constraints within the target bank have been evaluated.
#
# Un-comment one or more of the following IOSTANDARD constraints according to
# the bank pin assignments that are required within a design.
# ---------------------------------------------------------------------------- 

# Note that the bank voltage for IO Bank 33 is fixed to 3.3V on ZedBoard. 
set_property IOSTANDARD LVCMOS33 [get_ports -of_objects [get_iobanks 33]];

# Set the bank voltage for IO Bank 34 to 1.8V by default.
# set_property IOSTANDARD LVCMOS33 [get_ports -of_objects [get_iobanks 34]];
set_property IOSTANDARD LVCMOS25 [get_ports -of_objects [get_iobanks 34]];
# set_property IOSTANDARD LVCMOS18 [get_ports -of_objects [get_iobanks 34]];

# Set the bank voltage for IO Bank 35 to 1.8V by default.
# set_property IOSTANDARD LVCMOS33 [get_ports -of_objects [get_iobanks 35]];
set_property IOSTANDARD LVCMOS25 [get_ports -of_objects [get_iobanks 35]];
# set_property IOSTANDARD LVCMOS18 [get_ports -of_objects [get_iobanks 35]];

# Note that the bank voltage for IO Bank 13 is fixed to 3.3V on ZedBoard. 
set_property IOSTANDARD LVCMOS33 [get_ports -of_objects [get_iobanks 13]];


