
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
#  labelentry.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id: labelentry.tcl,v 1.6 2003/10/20 21:23:52 damonc Exp $
# ------------------------------------------------------------------------------
#  Index of commands:
#     - LabelEntry::create
#     - LabelEntry::configure
#     - LabelEntry::cget
#     - LabelEntry::bind
# ------------------------------------------------------------------------------

namespace eval LabelEntry {
    Widget::define LabelEntry labelentry Entry LabelFrame

    Widget::bwinclude LabelEntry LabelFrame .labf \
        remove {-relief -borderwidth -focus} \
        rename {-text -label} \
        prefix {label -justify -width -anchor -height -font -textvariable}

    Widget::bwinclude LabelEntry Entry .e \
        remove {-fg -bg} \
        rename {-foreground -entryfg -background -entrybg}

    Widget::addmap LabelEntry "" :cmd {-background {}}

    Widget::syncoptions LabelEntry Entry .e {-text {}}
    Widget::syncoptions LabelEntry LabelFrame .labf {-label -text -underline {}}

    ::bind BwLabelEntry <FocusIn> [list focus %W.labf]
    ::bind BwLabelEntry <Destroy> [list LabelEntry::_destroy %W]
}


# ------------------------------------------------------------------------------
#  Command LabelEntry::create
# ------------------------------------------------------------------------------
proc LabelEntry::create { path args } {
    array set maps [list LabelEntry {} :cmd {} .labf {} .e {}]
    array set maps [Widget::parseArgs LabelEntry $args]

    eval [list frame $path] $maps(:cmd) -class LabelEntry \
            -relief flat -bd 0 -highlightthickness 0 -takefocus 0
    Widget::initFromODB LabelEntry $path $maps(LabelEntry)
        
    set labf  [eval [list LabelFrame::create $path.labf] $maps(.labf) \
                   [list -relief flat -borderwidth 0 -focus $path.e]]
    set subf  [LabelFrame::getframe $labf]
    set entry [eval [list Entry::create $path.e] $maps(.e)]

    pack $entry -in $subf -fill both -expand yes
    pack $labf  -fill both -expand yes

    bindtags $path [list $path BwLabelEntry [winfo toplevel $path] all]

    return [Widget::create LabelEntry $path]
}


# ------------------------------------------------------------------------------
#  Command LabelEntry::configure
# ------------------------------------------------------------------------------
proc LabelEntry::configure { path args } {
    return [Widget::configure $path $args]
}


# ------------------------------------------------------------------------------
#  Command LabelEntry::cget
# ------------------------------------------------------------------------------
proc LabelEntry::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command LabelEntry::bind
# ------------------------------------------------------------------------------
proc LabelEntry::bind { path args } {
    return [eval [list ::bind $path.e] $args]
}


#------------------------------------------------------------------------------
#  Command LabelEntry::_path_command
#------------------------------------------------------------------------------
proc LabelEntry::_path_command { path cmd larg } {
    if { [string equal $cmd "configure"] ||
         [string equal $cmd "cget"] ||
         [string equal $cmd "bind"] } {
        return [eval [list LabelEntry::$cmd $path] $larg]
    } else {
        return [eval [list $path.e:cmd $cmd] $larg]
    }
}


proc LabelEntry::_destroy { path } {
    Widget::destroy $path
}
