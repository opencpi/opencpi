#!/bin/sh
set -e
cd git/linux-xlnx
git log --tags --simplify-by-decoration --pretty="format:%ai %d" | grep tag: | sort -n

