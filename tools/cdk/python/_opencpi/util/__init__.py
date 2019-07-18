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
"""
ocpiutil module definition file will directly setup the logging module and define the exception
class that is used throught
"""

import logging
import sys
import os

__all__ = [
    "file",
    "project",
    "report",
    ]


class OCPIException(Exception):
    """
    A exception class that we can throw and catch within OpenCPI code while ignoring other exception
    types. This class inherits from the built-in Exception class and doesn't extend it in any way
    """
    pass


# Initialize the root logging module such that:
#        OCPI_LOG_LEVEL <  8 : only log WARNINGS, ERRORS and CRITICALs
#   8 <= OCPI_LOG_LEVEL <  10: also log INFOs
#        OCPI_LOG_LEVEL >= 10: also log DEBUGs
# Now, logging will be determined based on OCPI_LOG_LEVEL environment
# variable. So, the following will only print if OCPI_LOG_LEVEL is >= 8
# >>> logging.info("info message")
# You should see 'OCPI:INFO: info message', but it will print to stderr, not stdout

OL_LEVEL = None
logging.basicConfig(stream=sys.stderr, format='OCPI:%(levelname)s: %(message)s')
OL_ROOTLOGGER = logging.getLogger()
if OL_LEVEL:
    OL_ROOTLOGGER.setLevel(level=OL_LEVEL)
else:
    OL_LOG_LEVEL = os.environ.get('OCPI_LOG_LEVEL')
    if not OL_LOG_LEVEL or int(OL_LOG_LEVEL) < 8:
        OL_ROOTLOGGER.setLevel(level=logging.WARNING)
    elif int(OL_LOG_LEVEL) < 10:
        OL_ROOTLOGGER.setLevel(level=logging.INFO)
    else:
        OL_ROOTLOGGER.setLevel(level=logging.DEBUG)
OCPIUTIL_LOGGER = OL_ROOTLOGGER

# pylint:disable=wildcard-import
from .file import *
from .project import *
from .report import *
# pylint:enable=wildcard-import
