#! /usr/bin/env python

from distutils.core import setup
import sys

ossiepyidl = ['ossie', 'ossie/cf', 'ossie/cf/CF', 'ossie/cf/CF__POA', 'ossie/cf/PortTypes', 'ossie/cf/PortTypes__POA', 'ossie/cf/StandardEvent', 'ossie/cf/StandardEvent__POA']

setup(
        name='ossiepyidl',
        version='0.8.0',
        description='OSSIE Python IDL bindings',
        packages=ossiepyidl,
    )
