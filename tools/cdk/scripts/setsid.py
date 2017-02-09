#!/usr/bin/env python
import os,sys,signal

child=0
# Since we have to disassociate the child into its own session and pgrp, we need to forward
# some signals to it
def handle(signal,frame):
  global child
  #sys.stderr.write("CAUGHT " + str(signal) + " for " + str(child) + "\n")
  if child != 0:
      #sys.stderr.write("SIGNALING CHILD " + str(signal) + "\n");
      os.killpg(child,signal)
signal.signal(signal.SIGINT, handle)
signal.signal(signal.SIGTERM, handle)
signal.signal(signal.SIGTSTP, handle)
#signal.signal(signal.SIGSTOP, handle)
if len(sys.argv)<2:
    sys.exit("No arguments")

# Create a child process in all cases so we can ctrl-c it even if it is disassociated
child = os.fork()
if child == 0:
    os.setsid()
    try:
        os.execvp(sys.argv[1], sys.argv[1:])
    except OSError as e:
        sys.exit(str(e))
    sys.exit("execvp")
#sys.stderr.write("my PID " + str(os.getpid()) + " child " + str(child) + "\n")
if child == -1:
   sys.exit("fork")

# We're the parent

try:
  t=os.wait()
except OSError:
  #sys.stderr.write("Error waiting for child process\n");
  sys.exit(1)
#sys.stderr.write("CHILD: " + str(t) + '\n')
if t[0] != child:
    sys.exit("Found an unknown child process from os.wait()");
sys.exit(1 if t[1] else 0)

