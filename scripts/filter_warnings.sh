#!/bin/sh

grep warning: | \
 grep -v '/include/omniORB4/' | \
 grep -v "as unsigned due to prototype" | \
 grep -v "as signed due to prototype" | \
 grep -v "with different width due to prototype" | \
 grep -v "suggest explicit braces" | \
 grep -v "suggest parentheses around" | \
 grep -v "unrecognized command line option" | \
 grep -v "_c.cxx"



