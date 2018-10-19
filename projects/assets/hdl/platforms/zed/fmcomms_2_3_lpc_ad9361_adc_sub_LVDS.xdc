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

# this is the zed XDC file that should be generated when instantiating the:
# ad9361_adc_sub worker
# on the fmcomms_2_3_lpc card

# Extracted from the zedboard_master_XDC_RevC_D_v3.xdc
############################################################################
# Clock constraints                                                        #
############################################################################
# 10 ns period = 100000 KHz
create_clock -name clk_fpga_0 -period 10.000 [get_pins {ftop/pfconfig_i/zed_i/worker/ps/ps/PS7_i/FCLKCLK[0]}]

# ----------------------------------------------------------------------------
# Clock constraints - ad9361_data_sub.hdl
# ----------------------------------------------------------------------------

# FMCOMMS2/3 AD9361 DATA_CLK_P


# create_clock command defaults to 50% duty cycle when -waveform is not specified

# from AD9361 datasheet
#set AD9361_LVDS_t_CP_ns 4.069
#create_clock -period $AD9361_LVDS_t_CP_ns -name FMC_LA00_CC_P [get_ports {FMC_LA00_CC_P}]

# max supported AD9361 DATA_CLK period of ad9361_adc_sub.hdl on
# Zedboard/FMCOMMS2/3 for which slack will be 0
create_clock -period 5.712 -name FMC_LA00_CC_P [get_ports {FMC_LA00_CC_P}]

# FMCOMMS2/3 AD9361 FB_CLK_P (forwarded version of DATA_CLK_P)
create_generated_clock -name FMC_LA08_P -source [get_pins {ftop/pfconfig_i/FMC_ad9361_data_sub_i/worker/mode7.dac_clock_forward/C}] -divide_by 1 -invert [get_ports {FMC_LA08_P}]

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

# ----------------------------------------------------------------------------
# FMC Expansion Connector - Bank 13
# ---------------------------------------------------------------------------- 
set_property PACKAGE_PIN R7 [get_ports {FMC_SCL}];  # "FMC-SCL"
set_property PACKAGE_PIN U7 [get_ports {FMC_SDA}];  # "FMC-SDA"

# ----------------------------------------------------------------------------
# FMC Expansion Connector - Bank 33
# ---------------------------------------------------------------------------- 
set_property PACKAGE_PIN AB14 [get_ports {fmc_prsnt}];  # "FMC-PRSNT"

# ----------------------------------------------------------------------------
# FMC Expansion Connector - Bank 34
# ---------------------------------------------------------------------------- 
set_property PACKAGE_PIN L19 [get_ports {FMC_CLK0_N}];  # "FMC-CLK0_N"
set_property PACKAGE_PIN L18 [get_ports {FMC_CLK0_P}];  # "FMC-CLK0_P"
set_property PACKAGE_PIN M20 [get_ports {FMC_LA00_CC_N}];  # "FMC-LA00_CC_N"
set_property PACKAGE_PIN M19 [get_ports {FMC_LA00_CC_P}];  # "FMC-LA00_CC_P"
set_property PACKAGE_PIN N20 [get_ports {FMC_LA01_CC_N}];  # "FMC-LA01_CC_N"
set_property PACKAGE_PIN N19 [get_ports {FMC_LA01_CC_P}];  # "FMC-LA01_CC_P" - corrected 6/6/16 GE
set_property PACKAGE_PIN P18 [get_ports {FMC_LA02_N}];  # "FMC-LA02_N"
set_property PACKAGE_PIN P17 [get_ports {FMC_LA02_P}];  # "FMC-LA02_P"
set_property PACKAGE_PIN P22 [get_ports {FMC_LA03_N}];  # "FMC-LA03_N"
set_property PACKAGE_PIN N22 [get_ports {FMC_LA03_P}];  # "FMC-LA03_P"
set_property PACKAGE_PIN M22 [get_ports {FMC_LA04_N}];  # "FMC-LA04_N"
set_property PACKAGE_PIN M21 [get_ports {FMC_LA04_P}];  # "FMC-LA04_P"
set_property PACKAGE_PIN K18 [get_ports {FMC_LA05_N}];  # "FMC-LA05_N"
set_property PACKAGE_PIN J18 [get_ports {FMC_LA05_P}];  # "FMC-LA05_P"
set_property PACKAGE_PIN L22 [get_ports {FMC_LA06_N}];  # "FMC-LA06_N"
set_property PACKAGE_PIN L21 [get_ports {FMC_LA06_P}];  # "FMC-LA06_P"
set_property PACKAGE_PIN T17 [get_ports {FMC_LA07_N}];  # "FMC-LA07_N"
set_property PACKAGE_PIN T16 [get_ports {FMC_LA07_P}];  # "FMC-LA07_P"
set_property PACKAGE_PIN J22 [get_ports {FMC_LA08_N}];  # "FMC-LA08_N"
set_property PACKAGE_PIN J21 [get_ports {FMC_LA08_P}];  # "FMC-LA08_P"
set_property PACKAGE_PIN R21 [get_ports {FMC_LA09_N}];  # "FMC-LA09_N"
set_property PACKAGE_PIN R20 [get_ports {FMC_LA09_P}];  # "FMC-LA09_P"
set_property PACKAGE_PIN T19 [get_ports {FMC_LA10_N}];  # "FMC-LA10_N"
set_property PACKAGE_PIN R19 [get_ports {FMC_LA10_P}];  # "FMC-LA10_P"
set_property PACKAGE_PIN N18 [get_ports {FMC_LA11_N}];  # "FMC-LA11_N"
set_property PACKAGE_PIN N17 [get_ports {FMC_LA11_P}];  # "FMC-LA11_P"
set_property PACKAGE_PIN P21 [get_ports {FMC_LA12_N}];  # "FMC-LA12_N"
set_property PACKAGE_PIN P20 [get_ports {FMC_LA12_P}];  # "FMC-LA12_P"
set_property PACKAGE_PIN M17 [get_ports {FMC_LA13_N}];  # "FMC-LA13_N"
set_property PACKAGE_PIN L17 [get_ports {FMC_LA13_P}];  # "FMC-LA13_P"
set_property PACKAGE_PIN K20 [get_ports {FMC_LA14_N}];  # "FMC-LA14_N"
set_property PACKAGE_PIN K19 [get_ports {FMC_LA14_P}];  # "FMC-LA14_P"
set_property PACKAGE_PIN J17 [get_ports {FMC_LA15_N}];  # "FMC-LA15_N"
set_property PACKAGE_PIN J16 [get_ports {FMC_LA15_P}];  # "FMC-LA15_P"
set_property PACKAGE_PIN K21 [get_ports {FMC_LA16_N}];  # "FMC-LA16_N"
set_property PACKAGE_PIN J20 [get_ports {FMC_LA16_P}];  # "FMC-LA16_P"

# ----------------------------------------------------------------------------
# FMC Expansion Connector - Bank 35
# ---------------------------------------------------------------------------- 
set_property PACKAGE_PIN C19 [get_ports {FMC_CLK1_N}];  # "FMC-CLK1_N"
set_property PACKAGE_PIN D18 [get_ports {FMC_CLK1_P}];  # "FMC-CLK1_P"
set_property PACKAGE_PIN B20 [get_ports {FMC_LA17_CC_N}];  # "FMC-LA17_CC_N"
set_property PACKAGE_PIN B19 [get_ports {FMC_LA17_CC_P}];  # "FMC-LA17_CC_P"
set_property PACKAGE_PIN C20 [get_ports {FMC_LA18_CC_N}];  # "FMC-LA18_CC_N"
set_property PACKAGE_PIN D20 [get_ports {FMC_LA18_CC_P}];  # "FMC-LA18_CC_P"
set_property PACKAGE_PIN G16 [get_ports {FMC_LA19_N}];  # "FMC-LA19_N"
set_property PACKAGE_PIN G15 [get_ports {FMC_LA19_P}];  # "FMC-LA19_P"
set_property PACKAGE_PIN G21 [get_ports {FMC_LA20_N}];  # "FMC-LA20_N"
set_property PACKAGE_PIN G20 [get_ports {FMC_LA20_P}];  # "FMC-LA20_P"
set_property PACKAGE_PIN E20 [get_ports {FMC_LA21_N}];  # "FMC-LA21_N"
set_property PACKAGE_PIN E19 [get_ports {FMC_LA21_P}];  # "FMC-LA21_P"
set_property PACKAGE_PIN F19 [get_ports {FMC_LA22_N}];  # "FMC-LA22_N"
set_property PACKAGE_PIN G19 [get_ports {FMC_LA22_P}];  # "FMC-LA22_P"
set_property PACKAGE_PIN D15 [get_ports {FMC_LA23_N}];  # "FMC-LA23_N"
set_property PACKAGE_PIN E15 [get_ports {FMC_LA23_P}];  # "FMC-LA23_P"
set_property PACKAGE_PIN A19 [get_ports {FMC_LA24_N}];  # "FMC-LA24_N"
set_property PACKAGE_PIN A18 [get_ports {FMC_LA24_P}];  # "FMC-LA24_P"
set_property PACKAGE_PIN C22 [get_ports {FMC_LA25_N}];  # "FMC-LA25_N"
set_property PACKAGE_PIN D22 [get_ports {FMC_LA25_P}];  # "FMC-LA25_P"
set_property PACKAGE_PIN E18 [get_ports {FMC_LA26_N}];  # "FMC-LA26_N"
set_property PACKAGE_PIN F18 [get_ports {FMC_LA26_P}];  # "FMC-LA26_P"
set_property PACKAGE_PIN D21 [get_ports {FMC_LA27_N}];  # "FMC-LA27_N"
set_property PACKAGE_PIN E21 [get_ports {FMC_LA27_P}];  # "FMC-LA27_P"
set_property PACKAGE_PIN A17 [get_ports {FMC_LA28_N}];  # "FMC-LA28_N"
set_property PACKAGE_PIN A16 [get_ports {FMC_LA28_P}];  # "FMC-LA28_P"
set_property PACKAGE_PIN C18 [get_ports {FMC_LA29_N}];  # "FMC-LA29_N"
set_property PACKAGE_PIN C17 [get_ports {FMC_LA29_P}];  # "FMC-LA29_P"
set_property PACKAGE_PIN B15 [get_ports {FMC_LA30_N}];  # "FMC-LA30_N"
set_property PACKAGE_PIN C15 [get_ports {FMC_LA30_P}];  # "FMC-LA30_P"
set_property PACKAGE_PIN B17 [get_ports {FMC_LA31_N}];  # "FMC-LA31_N"
set_property PACKAGE_PIN B16 [get_ports {FMC_LA31_P}];  # "FMC-LA31_P"
set_property PACKAGE_PIN A22 [get_ports {FMC_LA32_N}];  # "FMC-LA32_N"
set_property PACKAGE_PIN A21 [get_ports {FMC_LA32_P}];  # "FMC-LA32_P"
set_property PACKAGE_PIN B22 [get_ports {FMC_LA33_N}];  # "FMC-LA33_N"
set_property PACKAGE_PIN B21 [get_ports {FMC_LA33_P}];  # "FMC-LA33_P"

#create_pblock pblock_zed_i; add_cells_to_pblock [get_pblocks pblock_zed_i] [get_cells [list {ftop/pfconfig_i/zed_i}]]
#resize_pblock [get_pblocks pblock_zed_i] -add {SLICE_X26Y88:SLICE_X49Y110}

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

# Set the bank voltage for IO Bank 34 to 2.5V by default.
# set_property IOSTANDARD LVCMOS33 [get_ports -of_objects [get_iobanks 34]];
set_property IOSTANDARD LVCMOS25 [get_ports -of_objects [get_iobanks 34]];

# whenever we instantiate any of the ad9361 workers for FMCOMMS2/3, we really
# should set the IOSTANDARD for all of the FMCOMMS2/3 pins for the given slot

# note that the fmcomms_2_3_lpc card definition forces LVDS mode, so we do the
# same here since this constraints file is fmcomms_2_3_lpc card-specific

# ----------------------------------------------------------------------------
# IOSTANDARD constraints - ad9361_config.hdl
# ----------------------------------------------------------------------------

set_property IOSTANDARD LVCMOS25 [get_ports {FMC_LA16_N}]; # FMCOMMS2/3 AD9361 TXNRX
set_property IOSTANDARD LVCMOS25 [get_ports {FMC_LA16_P}]; # FMCOMMS2/3 AD9361 ENABLE

# ----------------------------------------------------------------------------
# IOSTANDARD constraints - ad9361_data_sub.hdl
# ----------------------------------------------------------------------------

set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA00_CC_P}]; # FMCOMMS3 DATA_CLK_P
set_property DIFF_TERM 1 [get_ports {FMC_LA00_CC_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA01_CC_P}]; # FMCOMMS3 RX_FRAME_P
set_property DIFF_TERM 1 [get_ports {FMC_LA01_CC_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA02_P}]; # FMCOMMS3 RX_D0
set_property DIFF_TERM 1 [get_ports {FMC_LA02_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA03_P}]; # FMCOMMS3 RX_D1
set_property DIFF_TERM 1 [get_ports {FMC_LA03_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA04_P}]; # FMCOMMS3 RX_D2
set_property DIFF_TERM 1 [get_ports {FMC_LA04_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA05_P}]; # FMCOMMS3 RX_D3
set_property DIFF_TERM 1 [get_ports {FMC_LA05_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA06_P}]; # FMCOMMS3 RX_D4
set_property DIFF_TERM 1 [get_ports {FMC_LA06_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA07_P}]; # FMCOMMS3 RX_D5
set_property DIFF_TERM 1 [get_ports {FMC_LA07_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA08_P}]; # FMCOMMS3 TX_FB_CLK_P
set_property DIFF_TERM 1 [get_ports {FMC_LA08_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA09_P}]; # FMCOMMS3 TX_FRAME_P
set_property DIFF_TERM 1 [get_ports {FMC_LA09_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA11_P}]; # FMCOMMS3 TX_D0
set_property DIFF_TERM 1 [get_ports {FMC_LA11_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA12_P}]; # FMCOMMS3 TX_D1
set_property DIFF_TERM 1 [get_ports {FMC_LA12_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA13_P}]; # FMCOMMS3 TX_D2
set_property DIFF_TERM 1 [get_ports {FMC_LA13_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA10_P}]; # FMCOMMS3 TX_D3
set_property DIFF_TERM 1 [get_ports {FMC_LA10_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA14_P}]; # FMCOMMS3 TX_D4
set_property DIFF_TERM 1 [get_ports {FMC_LA14_P}]
set_property IOSTANDARD LVDS_25 [get_ports {FMC_LA15_P}]; # FMCOMMS3 TX_D5
set_property DIFF_TERM 1 [get_ports {FMC_LA15_P}]
# set_property IOSTANDARD LVCMOS18 [get_ports -of_objects [get_iobanks 34]];

# Set the bank voltage for IO Bank 35 to 2.5V by default.
# set_property IOSTANDARD LVCMOS33 [get_ports -of_objects [get_iobanks 35]];
set_property IOSTANDARD LVCMOS25 [get_ports -of_objects [get_iobanks 35]];
# set_property IOSTANDARD LVCMOS18 [get_ports -of_objects [get_iobanks 35]];

# Note that the bank voltage for IO Bank 13 is fixed to 3.3V on ZedBoard. 
set_property IOSTANDARD LVCMOS33 [get_ports -of_objects [get_iobanks 13]];

# ----------------------------------------------------------------------------
# INPUT / OUTPUT DELAY constraints - ad9361_adc_sub.hdl
# ----------------------------------------------------------------------------

# FMCOMMS3 RX_D/RX_FRAME_P
#
# ----- from Vivado GUI:
#                             _____________
# input clock              __|             |_____________
#                   _____    :      ___    :      _______
# data              _____XXXX:XXXXXX___XXXX:XXXXXX_______
#                        :<--:---->:   :<--:-----:
#                    skew_bre skew_are skew_bfe skew_afe
#
# skew_bre : Data invalid before rising edge  (I think this really means max
#                                              amount of time that
#                                              start-of-valid for "rising" data
#                                              precedes rising edge by)
# skew_are : Data invalid after rising edge   (I think this really means max
#                                              amount of time that
#                                              start-of-valid for "rising" data
#                                              follows rising edge by)
# skew_bfe : Data invalid before falling edge (I think this really means max
#                                              amount of time that
#                                              start-of-valid for "falling" data
#                                              precedes falling edge by)
# skew_afe : Data invalid after falling edge  (I think this really means max
#                                              amount of time that
#                                              start-of-valid for "falling" data
#                                              follows falling edge by)
#
# Rise Max = (period/2) + skew_afe (I don't know if GUI is wrong, this might
#                                   need to be period/2 + skew_are)
# Rise Min = (period/2) - skew_bfe (I don't know if GUI is wrong, this might
#                                   need to be period/2 - skew_are)
# Fall Max = (period/2) + skew_are (I don't know if GUI is wrong, this might
#                                   need to be period/2 + skew_afe)
# Fall Min = (period/2) - skew_bre (I don't know if GUI is wrong, this might
#                                   need to be period/2 - skew_afe)
#
# ----- from AD9361 datasheet:
# t_DDRx_min = 0.25
# t_DDRx_max = 1.25
#
# ----- assumed ad9361_data_sub.hdl parameter property/no-OS init_param settings
# ----- (values chosen specifically to meet static timing):
# ------------------------------------------------------------------------------
# | ad9361_data_sub.hdl | no-OS init_param member | value | delay(ns)          |
# | parameter property  |                         |       |                    |
# ------------------------------------------------------------------------------
# | DATA_CLK_Delay      | rx_data_clock_delay     | 3     | 0.9                |
# | Rx_Data_Delay       | rx_data_delay           | 0     | 0.0                |
# ------------------------------------------------------------------------------
#
# ----- calculations
# skew_bre = -t_DDRx_min + (DATA_CLK_Delay-Rx_Data_Delay)*0.3 = 0.65
# skew_are =  t_DDRx_max - (DATA_CLK_Delay-Rx_Data_Delay)*0.3 = 0.35
# skew_bfe = -t_DDRx_min + (DATA_CLK_Delay-Rx_Data_Delay)*0.3 = 0.65
# skew_afe =  t_DDRx_max - (DATA_CLK_Delay-Rx_Data_Delay)*0.3 = 0.35
# Rise Max = (period/2) + skew_afe = (5.712/2) + 0.35 = 3.206
# Rise Min = (period/2) - skew_bfe = (5.712/2) - 0.65 = 2.206
# Fall Max = (period/2) + skew_are = (5.712/2) + 0.35 = 3.206
# Fall Min = (period/2) - skew_bre = (5.712/2) - 0.65 = 2.206
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA02_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA02_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA02_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA02_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA02_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA02_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA02_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA02_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA03_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA03_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA03_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA03_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA03_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA03_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA03_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA03_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA04_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA04_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA04_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA04_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA04_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA04_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA04_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA04_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA05_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA05_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA05_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA05_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA05_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA05_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA05_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA05_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA06_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA06_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA06_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA06_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA06_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA06_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA06_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA06_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA07_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA07_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -min -add_delay 2.206 [get_ports {FMC_LA07_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -max -add_delay 3.206 [get_ports {FMC_LA07_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA07_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA07_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA07_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA07_N}]
# RX_FRAME_P is sampled on the DATA_CLK_P falling edge (we use DDR primitive as a sample-in-the-middle)
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA01_CC_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA01_CC_P}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -min -add_delay 2.206 [get_ports {FMC_LA01_CC_N}]
set_input_delay -clock [get_clocks {FMC_LA00_CC_P}] -clock_fall -max -add_delay 3.206 [get_ports {FMC_LA01_CC_N}]

# ----------------------------------------------------------------------------
# INPUT / OUTPUT DELAY constraints - ad9361_data_sub.hdl
# ----------------------------------------------------------------------------

# from AD9361 datasheet
set AD9361_ENABLE_t_SC_min 1.0;
set AD9361_ENABLE_t_HC_min 0.0;

set AD9361_ENABLE_tsu_r $AD9361_ENABLE_t_SC_min;
set AD9361_ENABLE_thd_r $AD9361_ENABLE_t_HC_min;

# assume 0 for now, can measure later if necessary
set AD9361_ENABLE_trce_dly_max 0;
set AD9361_ENABLE_trce_dly_min 0;

# see Vivado constraints wizard for these formulas
set AD9361_ENABLE_Rise_Max [expr $AD9361_ENABLE_trce_dly_max + $AD9361_ENABLE_tsu_r];
set AD9361_ENABLE_Rise_Min [expr $AD9361_ENABLE_trce_dly_min - $AD9361_ENABLE_thd_r];

set_output_delay -clock [get_clocks {FMC_LA08_P}] -min -add_delay $AD9361_ENABLE_Rise_Min [get_ports {FMC_LA16_N}]
set_output_delay -clock [get_clocks {FMC_LA08_P}] -max -add_delay $AD9361_ENABLE_Rise_Max [get_ports {FMC_LA16_N}]

# from AD9361 datasheet
set AD9361_TXNRX_t_SC_min 1.0;
set AD9361_TXNRX_t_HC_min 0.0;

set AD9361_TXNRX_tsu_r $AD9361_TXNRX_t_SC_min;
set AD9361_TXNRX_thd_r $AD9361_TXNRX_t_HC_min;

# assume 0 for now, can measure later if necessary
set AD9361_TXNRX_trce_dly_max 0;
set AD9361_TXNRX_trce_dly_min 0;

# see Vivado constraints wizard for these formulas
set AD9361_TXNRX_Rise_Max [expr $AD9361_TXNRX_trce_dly_max + $AD9361_TXNRX_tsu_r];
set AD9361_TXNRX_Rise_Min [expr $AD9361_TXNRX_trce_dly_min - $AD9361_TXNRX_thd_r];

set_output_delay -clock [get_clocks {FMC_LA08_P}] -min -add_delay $AD9361_TXNRX_Rise_Min [get_ports {FMC_LA16_P}]
set_output_delay -clock [get_clocks {FMC_LA08_P}] -max -add_delay $AD9361_TXNRX_Rise_Max [get_ports {FMC_LA16_P}]

# ----------------------------------------------------------------------------
# CLOCK DOMAIN CROSSING / FALSE PATH constraints - ad9361_data_sub.hdl
# ----------------------------------------------------------------------------

# disable timing check among paths between AD9361 DATA_CLK_P and control plane clock domains (which are asynchronous)
set_clock_groups -asynchronous -group [get_clocks {FMC_LA00_CC_P}] -group [get_clocks {clk_fpga_0}]

# ----------------------------------------------------------------------------
# CLOCK DOMAIN CROSSING / FALSE PATH constraints - ad9361_adc_sub.hdl
# ----------------------------------------------------------------------------

# because RX_FRAME_P is sampled on the DATA_CLK_P falling edge (we use DDR primitive as a sample-in-the-middle), the rising edge latched output is unconnected and therefore should not be used in timing analysis
set_false_path -from [get_ports FMC_LA01_CC_P] -rise_to [get_pins ftop/pfconfig_i/FMC_ad9361_adc_sub_i/worker/supported_so_far.rx_frame_p_ddr/D]
