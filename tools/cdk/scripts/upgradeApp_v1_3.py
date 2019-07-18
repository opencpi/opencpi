#!/usr/bin/env python3
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

from __future__ import print_function
from lxml import etree as ET
from os.path import basename
from os.path import dirname
import sys
import ocpiutil
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util as ocpiutil

def createPackageDict():
    packDict = {"ocpi":"ocpi.core"}
    packDict["ocpiassets"] = "ocpi.assets"
    return packDict

def createCompDict():
    compDict = dict()
    compDict["ocpi.file_read"] = "ocpi.core.file_read"
    compDict["ocpi.file_write"] = "ocpi.core.file_write"
    compDict["ocpi.bias"] = "ocpi.core.bias"
    compDict["ocpi.hello_world"] = "ocpi.core.hello_world"
    compDict["ocpi.biasFGM"] = "ocpi.core.biasFGM"
    compDict["ocpiassets.devices.tx"] = "ocpi.core.tx"
    compDict["ocpiassets.devices.rx"] = "ocpi.core.rx"
    compDict["ocpiassets.platforms.picoflexor.devices.IQ_Time_gen"] = "ocpi.bsp_picoflexor.platforms.picoflexor.devices.IQ_Time_gen"
    compDict["ocpiassets.platforms.picoflexor.devices.drs_controller"] = "ocpi.bsp_picoflexor.platforms.picoflexor.devices.drs_controller"
    compDict["ocpiassets.platforms.picoflexor.devices.IQ_Time_Demux"] = "ocpi.bsp_picoflexor.platforms.picoflexor.devices.IQ_Time_Demux"
    compDict["ocpiassets.platforms.picoflexor.devices.iqAndTime_sink"] = "ocpi.bsp_picoflexor.platforms.picoflexor.devices.iqAndTime_sink"
    compDict["ocpiassets.platforms.picoflexor.devices.iqAndTime_source"] = "ocpi.bsp_picoflexor.platforms.picoflexor.devices.iqAndTime_source"
    compDict["ocpi.capture"] = "ocpi.assets.capture"
    compDict["ocpi.cons"] = "ocpi.assets.cons"
    compDict["ocpi.copy"] = "ocpi.assets.copy"
    compDict["ocpi.pattern"] = "ocpi.assets.pattern"
    compDict["ocpi.prod"] = "ocpi.assets.prod"
    compDict["ocpi.ptest"] = "ocpi.assets.ptest"
    compDict["ocpi.cos"] = "ocpi.assets.cos"
    compDict["ocpi.sin"] = "ocpi.assets.sin"
    compDict["ocpi.time_gen"] = "ocpi.assets.time_gen"
    compDict["ocpi.zcons"] = "ocpi.core.zcons"
    compDict["ocpi.zcloop"] = "ocpi.core.zcloop"
    compDict["ocpi.zcprod"] = "ocpi.core.zcprod"
    compDict["ocpiassets.comms_comps.mfsk_mapper"] = "ocpi.assets.comms_comps.mfsk_mapper"
    compDict["ocpiassets.dsp_comps.baudTracking"] = "ocpi.assets.dsp_comps.baudTracking"
    compDict["ocpiassets.dsp_comps.cic_dec"] = "ocpi.assets.dsp_comps.cic_dec"
    compDict["ocpiassets.dsp_comps.cic_int"] = "ocpi.assets.dsp_comps.cic_int"
    compDict["ocpiassets.dsp_comps.complex_mixer"] = "ocpi.assets.dsp_comps.complex_mixer"
    compDict["ocpiassets.dsp_comps.dc_offset_filter"] = "ocpi.assets.dsp_comps.dc_offset_filter"
    compDict["ocpiassets.dsp_comps.fir_real_sse"] = "ocpi.assets.dsp_comps.fir_real_sse"
    compDict["ocpiassets.dsp_comps.fir_complex_sse"] = "ocpi.assets.dsp_comps.fir_complex_sse"
    compDict["ocpiassets.dsp_comps.iq_imbalance"] = "ocpi.assets.dsp_comps.iq_imbalance"
    compDict["ocpiassets.dsp_comps.phase_to_amp_cordic"] = "ocpi.assets.dsp_comps.phase_to_amp_cordic"
    compDict["ocpiassets.dsp_comps.pr_cordic"] = "ocpi.assets.dsp_comps.pr_cordic"
    compDict["ocpiassets.dsp_comps.real_digitizer"] = "ocpi.assets.dsp_comps.real_digitizer"
    compDict["ocpiassets.dsp_comps.rp_cordic"] = "ocpi.assets.dsp_comps.rp_cordic"
    compDict["ocpiassets.misc_comps.backpressure"] = "ocpi.misc_comps.backpressure"
    compDict["ocpiassets.misc_comps.data_src"] = "ocpi.misc_comps.data_src"
    compDict["ocpiassets.misc_comps.vita49_remover"] = "ocpi.misc_comps.vita49_remover"
    compDict["ocpiassets.util_comps.advanced_pattern"] = "ocpi.util_comps.advanced_pattern"
    compDict["ocpiassets.util_comps.agc_real"] = "ocpi.util_comps.agc_real"
    compDict["ocpiassets.util_comps.combiner_2_to_1"] = "ocpi.util_comps.combiner_2_to_1"
    compDict["ocpiassets.util_comps.fifo"] = "ocpi.util_comps.fifo"
    compDict["ocpiassets.util_comps.IQ_Time_Demux"] = "ocpi.util_comps.IQ_Time_Demux"
    compDict["ocpiassets.util_comps.IQ_Time_gen"] = "ocpi.util_comps.IQ_Time_gen"
    compDict["ocpiassets.util_comps.file_write_demux"] = "ocpi.util_comps.file_write_demux"
    compDict["ocpiassets.util_comps.resource_consumer_example"] = "ocpi.util_comps.resource_consumer_example"
    compDict["ocpiassets.util_comps.resource_example"] = "ocpi.util_comps.resource_example"
    compDict["ocpiassets.util_comps.socket_write"] = "ocpi.util_comps.socket_write"
    compDict["ocpiassets.util_comps.timestamper"] = "ocpi.util_comps.timestamper"
    compDict["ocpiassets.util_comps.zero_pad"] = "ocpi.util_comps.zero_pad"
    compDict["ocpiassets.util_comps.zero_padding"] = "ocpi.util_comps.zero_padding"
    compDict["ocpiassets.platforms.matchstiq_z1.devices.matchstiq_z1_avr_proxy"] = "ocpi.assets.platforms.matchstiq_z1.devices.matchstiq_z1_avr_proxy"
    compDict["ocpiassets.platforms.matchstiq_z1.devices.matchstiq_z1_avr"] = "ocpi.assets.platforms.matchstiq_z1.devices.matchstiq_z1_avr"
    compDict["ocpiassets.platforms.matchstiq_z1.devices.matchstiq_z1_i2c"] = "ocpi.assets.platforms.matchstiq_z1.devices.matchstiq_z1_i2c"
    compDict["ocpiassets.platforms.matchstiq_z1.devices.matchstiq_z1_pca9535_proxy"] = "ocpi.assets.platforms.matchstiq_z1.devices.matchstiq_z1_pca9535_proxy"
    compDict["ocpiassets.devices.gps_uart"] = "ocpi.assets.devices.gps_uart"
    compDict["ocpiassets.devices.si5338"] = "ocpi.assets.devices.si5338"
    compDict["ocpiassets.devices.lime_dac_em"] = "ocpi.assets.devices.lime_dac_em"
    compDict["ocpiassets.devices.qdac_ts"] = "ocpi.assets.devices.qdac_ts"
    compDict["ocpiassets.devices.tmp100_proxy"] = "ocpi.assets.devices.tmp100_proxy"
    compDict["ocpiassets.devices.tmp100"] = "ocpi.assets.devices.tmp100"
    compDict["ocpiassets.devices.pca9534"] = "ocpi.assets.devices.pca9534"
    compDict["ocpiassets.devices.pca9535"] = "ocpi.assets.devices.pca9535"
    compDict["ocpi.devices.ad9361_config_proxy"] = "ocpi.assets.devices.ad9361_config_proxy"
    compDict["ocpi.devices.qadc"] = "ocpi.assets.devices.qadc"
    compDict["ocpi.devices.qdac"] = "ocpi.assets.devices.qdac"
    compDict["ocpi.devices.rf_rx"] = "ocpi.assets.devices.rf_rx"
    compDict["ocpi.devices.rf_rx_proxy"] = "ocpi.assets.devices.rf_rx_proxy"
    compDict["ocpi.devices.rf_tx"] = "ocpi.assets.devices.rf_tx"
    compDict["ocpi.devices.rf_tx_proxy"] = "ocpi.assets.devices.rf_tx_proxy"
    compDict["ocpiassets.devices.i2c_sim.simdevices.i2c_sim_subdevice"] = "ocpi.assets.devices.i2c_sim_subdevice"
    compDict["ocpiassets.devices.i2c_sim.simdevices.i2c_sim_master_16b_props"] = "assets.devices.i2c_sim.simdevices.i2c_sim_master_16b_props"
    compDict["ocpiassets.devices.i2c_sim.simdevices.i2c_sim_master_mix_props"] = "assets.devices.i2c_sim.simdevices.i2c_sim_master_mix_props"
    compDict["ocpiassets.devices.i2c_sim.simdevices.i2c_sim_master_16b_ext_wr_props"] = "assets.devices.i2c_sim.simdevices.i2c_sim_master_16b_ext_wr_props"
    compDict["ocpiassets.devices.i2c_sim.simdevices.i2c_sim_subdevice"] = "assets.devices.i2c_sim.simdevices.i2c_sim_subdevice"
    compDict["ocpiassets.devices.i2c_sim.simdevices.i2c_em"] = "assets.devices.i2c_sim.simdevices.i2c_em"
    compDict["ocpi.devices.ad7291"] = "ocpi.assets.devices.ad7291"
    compDict["ocpi.devices.ad9361_adc_sub"] = "ocpi.assets.devices.ad9361_adc_sub"
    compDict["ocpi.devices.ad9361_config"] = "ocpi.assets.devices.ad9361_config"
    compDict["ocpi.devices.ad9361_dac_sub"] = "ocpi.assets.devices.ad9361_dac_sub"
    compDict["ocpi.devices.ad9361_data_sub"] = "ocpi.assets.devices.ad9361_data_sub"
    compDict["ocpi.devices.ad9361_spi"] = "ocpi.assets.devices.ad9361_spi"
    compDict["ocpi.devices.flash"] = "ocpi.assets.devices.flash"
    compDict["ocpi.devices.stm_mc24c02"] = "ocpi.assets.devices.stm_mc24c02"
    compDict["ocpi.devices.lime_spi"] = "ocpi.assets.devices.lime_spi"
    return compDict

def update(origComponent, compDict):
    retVal = origComponent
    if (origComponent in compDict):
        retVal = compDict[origComponent]
    return retVal

#main
if len(sys.argv) < 2 :
    print("ERROR: need to specify the application xml")
    sys.exit(1)

inFile = sys.argv[1]

if len(sys.argv) == 3 :
    if (sys.argv[2] == "same"):
        outFileName = inFile
    else:
        outFileName = sys.argv[2]
else:
    outFileName = dirname(inFile) + "/updated_" + basename(inFile)

inTree = ET.parse(inFile)
compDict = createCompDict()
packDict = createPackageDict()

try:
  oPack = inTree.getroot().attrib["package"]
  packageLC = True
except:
    try:
        oPack = inTree.getroot().attrib["Package"]
        packageLC = False
    except:
        oPack = ""
        packageLC = None

if (oPack != ""):
    uPack = update(oPack, packDict)
    if (uPack != oPack):
        prompt = "do you want to change Package: " + oPack + " to: " + uPack
        if ocpiutil.get_ok(prompt=prompt):
            if (packageLC): 
                inTree.getroot().attrib["package"] = uPack
            else : 
                inTree.getroot().attrib["Package"] = uPack

for child in inTree.findall("Instance") + inTree.findall("instance"):
    uComp = update(child.attrib["Component"], compDict)
    if (child.attrib["Component"] != uComp):
        prompt = "do you want to change Component: " + child.attrib["Component"] + " to: " + uComp
        if ocpiutil.get_ok(prompt=prompt):
            child.attrib["Component"] = uComp

print("writing file:" + outFileName)
myFile = open(outFileName, 'w')
myFile.write(ET.tostring(inTree,pretty_print=True))

