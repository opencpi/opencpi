#!/usr/bin/env python
import os,sys

if len(sys.argv)<2:
    sys.exit("No arguments")

if os.getpgrp() == os.getpid() :
    f = os.fork()
    if f == -1:
        sys.exit("fork")
    if f != 0: # parent process
        sys.exit(0)

os.setsid()
try:
    os.execvp(sys.argv[1], sys.argv[1:])
except OSError as e:
    sys.exit(str(e))

sys.exit("execvp")
