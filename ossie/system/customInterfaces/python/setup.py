#! /usr/bin/env python

from distutils.core import setup
import sys

ossiecipyidl = ['ossie/custominterfaces', 'ossie/custominterfaces/customInterfaces', 'ossie/custominterfaces/customInterfaces__POA', 'ossie/custominterfaces/PortTypes', 'ossie/custominterfaces/PortTypes__POA']

setup(
        name='ossiecipyidl',
        version='0.8.0',
        description='OSSIE Custom Interfaces Python IDL bindings',
        packages=ossiecipyidl,
    )
