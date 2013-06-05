#!/bin/sh

grep warning: | \
 grep -v '/include/omniORB4/' | \
 grep -v "as unsigned due to prototype" | \
 grep -v "as signed due to prototype" | \
 grep -v "with different width due to prototype" | \
 grep -v "_c.cxx"



