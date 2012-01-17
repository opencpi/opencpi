#! /usr/bin/env python

from distutils.core import setup
import sys

install_location = '/sdr'

sys.argv.append('--install-lib='+install_location)

setup(name='ossie_demo', description='ossie_demo',data_files=[(install_location+'/waveforms/ossie_demo',['ossie_demo.sad.xml', 'ossie_demo_DAS.xml'])])
