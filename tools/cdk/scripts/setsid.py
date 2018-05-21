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

import os
import sys
import signal

child = 0
# Since we have to disassociate the child into its own session and pgrp, we need to forward
# some signals to it


def handle(signal, frame):
    global child
    # sys.stderr.write("CAUGHT " + str(signal) + " for " + str(child) + "\n")
    if child != 0:
        #sys.stderr.write("SIGNALING CHILD " + str(signal) + "\n");
        os.killpg(child, signal)


signal.signal(signal.SIGINT, handle)
signal.signal(signal.SIGTERM, handle)
signal.signal(signal.SIGTSTP, handle)
# signal.signal(signal.SIGSTOP, handle)
if len(sys.argv) < 2:
    sys.exit("No arguments")

# SSH_ASKPASS is ignored unless DISPLAY is set.
# That is the normal use case for this utility script.
if os.getenv('DISPLAY') is None:
    os.environ['DISPLAY'] = "phony"

# Create a child process in all cases so we can ctrl-c it even if it is disassociated
child = os.fork()
if child == 0:
    os.setsid()
    try:
        os.execvp(sys.argv[1], sys.argv[1:])
    except OSError as err:
        sys.exit(str(err))
    sys.exit("execvp")
# sys.stderr.write("my PID " + str(os.getpid()) + " child " + str(child) + "\n")
if child == -1:
    sys.exit("fork")

# We're the parent
try:
    t = os.wait()
except OSError:
    # sys.stderr.write("Error waiting for child process\n");
    sys.exit(1)
# sys.stderr.write("CHILD: " + str(t) + '\n')
if t[0] != child:
    sys.exit("Found an unknown child process from os.wait()")
sys.exit(1 if t[1] else 0)
