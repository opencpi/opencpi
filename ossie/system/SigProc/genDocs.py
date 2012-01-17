#!/usr/bin/env python

import os

def convertEPStoPNG(imgfilename):
    '''converts .eps file to .png'''
    # Try using command-line "convert"
    epsfilename = imgfilename
    pngfilename = os.path.splitext(imgfilename)[0] + '.png'
    if os.system('convert ' + epsfilename + ' ' + pngfilename) != 0:
        print "could not convert " + epsfilename + " using ImageMagick"
        return False
    return True

def convertDirectoryEPStoPNG(imgdir):
    '''converts .eps files to .png for a given directory'''
    for i in os.listdir(imgdir):
        imgfile = os.path.splitext(i)
        if imgfile[1].lower() == ".eps":
            print "converting " + imgdir + i + "..."
            if convertEPStoPNG(imgdir + i) != True:
                print "error"

if __name__ == "__main__":
    print "generating documentation..."

    # convert all .eps files in directory to .png
    convertDirectoryEPStoPNG('img/')

    # run doxygen on default Doxyfile
    os.system('doxygen Doxyfile')

