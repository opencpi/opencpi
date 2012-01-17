#! /usr/bin/env python

from distutils.core import setup
import sys

ossiesipyidl = ['ossie/standardinterfaces', 'ossie/standardinterfaces/standardInterfaces', 'ossie/standardinterfaces/CF', 'ossie/standardinterfaces/CF__POA', 'ossie/standardinterfaces/PortTypes', 'ossie/standardinterfaces/PortTypes__POA', 'ossie/standardinterfaces/standardInterfaces__POA']

setup(
        name='ossiesipyidl',
        version='0.8.0',
        description='OSSIE Standard Interfaces Python IDL bindings',
        packages=ossiesipyidl,
    )
