#!/usr/bin/env python

import os

if __name__ == "__main__":
    print "generating documentation..."

    # run doxygen on default Doxyfile
    os.system('doxygen Doxyfile')

