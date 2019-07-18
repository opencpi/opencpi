#!/usr/bin/env python2
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

from __future__ import print_function
import socket
import errno
import os
import time
import signal
import sys

def do_pull(port, filename, addr='localhost', timeout=60):
  print("busy_loop_socket:{0}: port={1}, filename={2}, addr={3}, timeout={4}".format(os.getpid(), port, filename, addr, timeout))
  sys.stdout.flush()
  total_bytes = 0
  start_time = time.time()
  with open(filename, "wb") as ofile:
    while True:
      try:
        sock = socket.socket()
        sock.connect((addr,int(port)))
        break
      except socket.error as e:
        if errno.ECONNREFUSED == e.errno:
          # print("busy_loop_socket: {}:{} refused ({}s vs {}s)".format(addr,port,timeout,time.time() - start_time), file=sys.stderr)
          pass
        else:
          raise e
      if time.time() - start_time >= int(timeout):
        print("busy_loop_socket:{0}: {1}:{2} timed out ({3}s)".format(os.getpid(), addr, port, timeout), file=sys.stderr)
        return False
    print("busy_loop_socket:{0}: {1}:{2} connected ({3:0.5}s)".format(os.getpid(), addr, port, time.time() - start_time))
    while True:
      data = sock.recv(8192)  # 8K chunks
      total_bytes += len(data)
      if not data: break
      ofile.write(data)
    sock.close()
    fmt = '{3}'
    if sys.version_info >= (2, 7):
      fmt = '{3:,}'  # Pretty commas
    print(("busy_loop_socket:{0}: {1}:{2} finished with "+fmt+" bytes ({4:0.5}s)").format(os.getpid(), addr, port, total_bytes, time.time() - start_time))
    return True

def usage():
  print("""
Takes up to four arguments:
  port to connect to
  filename to write to
  address to connect to [default=localhost]
  timeout in seconds [default=60]
""", file=sys.stderr)
  sys.exit(99)

if __name__ == "__main__":
  if not 3 <= len(sys.argv) <= 5:
    usage()
  port = int(sys.argv[1])
  filename = sys.argv[2]
  addr = 'localhost'
  timeout = 60
  if len(sys.argv) >= 4:
    addr = sys.argv[3]
  if len(sys.argv) >= 5:
    timeout = int(sys.argv[4])
  signal.signal(signal.SIGHUP, signal.SIG_IGN)  # If our caller (Makefile) exits, continue flushing
  ret = False
  try:
    ret = do_pull(port=port, filename=filename, addr=addr, timeout=timeout)
  except:
    print("busy_loop_socket:{0}: {1}:{2} error: {3}".format(os.getpid(), addr, port, sys.exc_info()[0]), file=sys.stderr)
    raise
  sys.exit(not ret)  # The "not" is because 0 == success in Unix
