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

set_property BITSTREAM.GENERAL.CRC Enable [current_design]
set_property BITSTREAM.CONFIG.CCLKPIN PullUp [current_design]
set_property BITSTREAM.CONFIG.M0PIN PullUp [current_design]
set_property BITSTREAM.CONFIG.M1PIN PullUp [current_design]
set_property BITSTREAM.CONFIG.M2PIN PullUp [current_design]
set_property BITSTREAM.CONFIG.PROGPIN PullUp [current_design]
set_property BITSTREAM.CONFIG.DONEPIN PullUp [current_design]
set_property BITSTREAM.CONFIG.INITPIN PullUp [current_design]
set_property BITSTREAM.CONFIG.TCKPIN PullUp [current_design]
set_property BITSTREAM.CONFIG.TDIPIN PullUp [current_design]
set_property BITSTREAM.CONFIG.TDOPIN PullUp [current_design]
set_property BITSTREAM.CONFIG.TMSPIN PullUp [current_design]
set_property BITSTREAM.CONFIG.UNUSEDPIN PullDown [current_design]
set_property BITSTREAM.CONFIG.USERID 0xFEEDFACE [current_design]
set_property BITSTREAM.CONFIG.CONFIGFALLBACK Disable [current_design]
set_property BITSTREAM.CONFIG.OVERTEMPPOWERDOWN Disable [current_design]
set_property BITSTREAM.CONFIG.DCIUPDATEMODE AsRequired [current_design]
set_property BITSTREAM.STARTUP.STARTUPCLK Cclk [current_design]
set_property BITSTREAM.STARTUP.DONE_CYCLE 4 [current_design]
set_property BITSTREAM.STARTUP.GTS_CYCLE 5 [current_design]
set_property BITSTREAM.STARTUP.GWE_CYCLE 6 [current_design]
set_property BITSTREAM.STARTUP.LCK_CYCLE NoWait [current_design]
set_property BITSTREAM.STARTUP.MATCH_CYCLE NoWait [current_design]
set_property BITSTREAM.READBACK.SECURITY None [current_design]
set_property BITSTREAM.STARTUP.DONEPIPE No [current_design]
set_property BITSTREAM.Config.DRIVEDONE No [current_design]
# Default is no. Property is read-only
#set_property BITSTREAM.ENCRYPTION.ENCRYPT No [current_design]
