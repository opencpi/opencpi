
# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

# ------------------------------------------------------------------------------
#  bitmap.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id: bitmap.tcl,v 1.4 2003/10/20 21:23:52 damonc Exp $
# ------------------------------------------------------------------------------
#  Index of commands:
#     - Bitmap::get
#     - Bitmap::_init
# ----------------------------------------------------------------------------
namespace eval Bitmap {
    Widget::define Bitmap bitmap -classonly

    variable path
    variable _bmp
    variable _types {
        photo  .gif
        photo  .ppm
        bitmap .xbm
        photo  .xpm
    }

    proc use {} {}
}


# ----------------------------------------------------------------------------
#  Command Bitmap::get
# ----------------------------------------------------------------------------
proc Bitmap::get { name } {
    variable path
    variable _bmp
    variable _types

    if {[info exists _bmp($name)]} {
        return $_bmp($name)
    }

    # --- Nom de fichier avec extension ---------------------------------
    set ext [file extension $name]
    if { $ext != "" } {
        if { ![info exists _bmp($ext)] } {
            error "$ext not supported"
        }

        if { [file exists $name] } {
            if {[string equal $ext ".xpm"]} {
                set _bmp($name) [xpm-to-image $name]
                return $_bmp($name)
            }
            if {![catch {set _bmp($name) [image create $_bmp($ext) -file $name]}]} {
                return $_bmp($name)
            }
        }
    }

    foreach dir $path {
        foreach {type ext} $_types {
            if { [file exists [file join $dir $name$ext]] } {
                if {[string equal $ext ".xpm"]} {
                    set _bmp($name) [xpm-to-image [file join $dir $name$ext]]
                    return $_bmp($name)
                } else {
                    if {![catch {set _bmp($name) [image create $type -file [file join $dir $name$ext]]}]} {
                        return $_bmp($name)
                    }
                }
            }
        }
    }

    return -code error "$name not found"
}


# ----------------------------------------------------------------------------
#  Command Bitmap::_init
# ----------------------------------------------------------------------------
proc Bitmap::_init { } {
    global   env
    variable path
    variable _bmp
    variable _types

    set path [list "." [file join $::BWIDGET::LIBRARY images]]
    set supp [image types]
    foreach {type ext} $_types {
        if { [lsearch $supp $type] != -1} {
            set _bmp($ext) $type
        }
    }
}


Bitmap::_init
