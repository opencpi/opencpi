#!/bin/sh
# This script is a placeholder to capture the args.
exec astyle -A2 -T2 -xT8 -H -c -xL -L -w -p -U -k3 -W3 -xj -N -xn -xc -xl -xk $*
