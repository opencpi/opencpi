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

#def configure_logging(level=None, output_fd=sys.stderr):
"""
Initialize the root logging module such that:
        OCPI_LOG_LEVEL <  8 : only log WARNINGS, ERRORS and CRITICALs
   8 <= OCPI_LOG_LEVEL <  10: also log INFOs
        OCPI_LOG_LEVEL >= 10: also log DEBUGs
This can be used in other modules to change the default log level of the
logging module. For example, from another module:
>>> import logging
>>> import ocpiutil
>>> os.environ['OCPI_LOG_LEVEL'] = "11"
>>> ocpiutil.configure_logging(output_fd=sys.stdout)
<logging.RootLogger...>

# Now, logging will be determined based on OCPI_LOG_LEVEL environment
# variable. So, the following will only print if OCPI_LOG_LEVEL is >= 8
>>> logging.info("info message")

# You should see 'OCPI:INFO: info message', but it will print to stderr, not stdout
"""
level = None
output_fd=sys.stderr
logging.basicConfig(stream=output_fd, format='OCPI:%(levelname)s: %(message)s')
rootlogger = logging.getLogger()
if level:
    rootlogger.setLevel(level=level)
else:
    log_level = os.environ.get('OCPI_LOG_LEVEL')
    if not log_level or int(log_level) < 8:
        rootlogger.setLevel(level=logging.WARNING)
    elif int(log_level) < 10:
        rootlogger.setLevel(level=logging.INFO)
    else:
        rootlogger.setLevel(level=logging.DEBUG)
OCPIUTIL_LOGGER = rootlogger

from .file import *
from .project import *
from .report import *
