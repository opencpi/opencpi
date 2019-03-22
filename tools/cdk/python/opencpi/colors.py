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

""" Look-up Table for colors """

import os
import subprocess
import sys

BLUE = BOLD = CLS = END = GREEN = RED = UNDERLINE = YELLOW = ''
try:
    with open(os.devnull, 'w') as devnull:
        subprocess.check_output(['tput', 'lines'], stderr=devnull)
        if os.getenv('TERM', 'dumb') != "dumb":
            BLUE = subprocess.check_output(['tput', 'setaf', '4'], stderr=devnull).decode("utf-8")
            BOLD = subprocess.check_output(['tput', 'bold'], stderr=devnull).decode("utf-8")
            CLS = subprocess.check_output(['tput', 'clear'], stderr=devnull).decode("utf-8")
            END = subprocess.check_output(['tput', 'sgr0'], stderr=devnull).decode("utf-8")
            GREEN = subprocess.check_output(['tput', 'setaf', '2'], stderr=devnull).decode("utf-8")
            RED = subprocess.check_output(['tput', 'setaf', '1'], stderr=devnull).decode("utf-8")
            UNDERLINE = subprocess.check_output(['tput', 'smul'], stderr=devnull).decode("utf-8")
            YELLOW = subprocess.check_output(['tput', 'setaf', '3'], stderr=devnull).decode("utf-8")
except AttributeError:  # old python with no subprocess.check_output
    pass
except OSError:  # something weird
    pass
except subprocess.CalledProcessError:  # no tput
    pass
