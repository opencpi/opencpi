
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
#  separator.tcl
#  This file is part of Unifix BWidget Toolkit
# ------------------------------------------------------------------------------
#  Index of commands:
#     - Separator::create
#     - Separator::configure
#     - Separator::cget
# ------------------------------------------------------------------------------

namespace eval Separator {
    Widget::define Separator separator

    Widget::declare Separator {
        {-background TkResource ""         0 frame}
        {-cursor     TkResource ""         0 frame}
        {-relief     Enum       groove     0 {ridge groove}}
        {-orient     Enum       horizontal 1 {horizontal vertical}}
        {-bg         Synonym    -background}
    }
    Widget::addmap Separator "" :cmd { -background {} -cursor {} }

    bind Separator <Destroy> [list Widget::destroy %W]
}


# ------------------------------------------------------------------------------
#  Command Separator::create
# ------------------------------------------------------------------------------
proc Separator::create { path args } {
    array set maps [list Separator {} :cmd {}]
    array set maps [Widget::parseArgs Separator $args]
    eval [list frame $path] $maps(:cmd) -class Separator
    Widget::initFromODB Separator $path $maps(Separator)

    if { [Widget::cget $path -orient] == "horizontal" } {
        $path configure -borderwidth 1 -height 2
    } else {
        $path configure -borderwidth 1 -width 2
    }

    if { [string equal [Widget::cget $path -relief] "groove"] } {
        $path configure -relief sunken
    } else {
        $path configure -relief raised
    }

    return [Widget::create Separator $path]
}


# ------------------------------------------------------------------------------
#  Command Separator::configure
# ------------------------------------------------------------------------------
proc Separator::configure { path args } {
    set res [Widget::configure $path $args]

    if { [Widget::hasChanged $path -relief relief] } {
        if { $relief == "groove" } {
            $path:cmd configure -relief sunken
        } else {
            $path:cmd configure -relief raised
        }
    }

    return $res
}


# ------------------------------------------------------------------------------
#  Command Separator::cget
# ------------------------------------------------------------------------------
proc Separator::cget { path option } {
    return [Widget::cget $path $option]
}
