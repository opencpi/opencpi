
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

# -----------------------------------------------------------------------------
#  passwddlg.tcl
#  This file is part of Unifix BWidget Toolkit
#   by Stephane Lavirotte (Stephane.Lavirotte@sophia.inria.fr)
#  $Id: passwddlg.tcl,v 1.11 2006/09/28 22:31:28 dev_null42a Exp $
# -----------------------------------------------------------------------------
#  Index of commands:
#     - PasswdDlg::create
#     - PasswdDlg::configure
#     - PasswdDlg::cget
#     - PasswdDlg::_verifonlogin
#     - PasswdDlg::_verifonpasswd
#     - PasswdDlg::_max
#------------------------------------------------------------------------------

namespace eval PasswdDlg {
    Widget::define PasswdDlg passwddlg Dialog LabelEntry

    Widget::bwinclude PasswdDlg Dialog :cmd \
            remove     {-image -bitmap -side -default -cancel -separator} \
            initialize {-modal local -anchor e}
    
    Widget::bwinclude PasswdDlg LabelEntry .frame.lablog \
            remove [list -command -justify -name -show -side                \
                -state -takefocus -width -xscrollcommand -padx -pady        \
                -dragenabled -dragendcmd -dragevent -draginitcmd        \
                -dragtype -dropenabled -dropcmd -dropovercmd -droptypes        \
                ] \
            prefix [list login -editable -helptext -helpvar -label      \
                -text -textvariable -underline                                \
                ] \
            initialize [list -relief sunken -borderwidth 2                \
                -labelanchor w -width 15 -loginlabel "Login"                \
                ]
    
    Widget::bwinclude PasswdDlg LabelEntry .frame.labpass                \
            remove [list -command -width -show -side -takefocus                \
                -xscrollcommand -dragenabled -dragendcmd -dragevent        \
                -draginitcmd -dragtype -dropenabled -dropcmd                \
                -dropovercmd -droptypes -justify -padx -pady -name        \
                ] \
            prefix [list passwd -editable -helptext -helpvar -label        \
                -state -text -textvariable -underline                        \
                ] \
            initialize [list -relief sunken -borderwidth 2                \
                -labelanchor w -width 15 -passwdlabel "Password"        \
                ]
    
    Widget::declare PasswdDlg {
        {-type        Enum       ok           0 {ok okcancel}}
        {-labelwidth  TkResource -1           0 {label -width}}
        {-command     String     ""           0}
    }
}


# -----------------------------------------------------------------------------
#  Command PasswdDlg::create
# -----------------------------------------------------------------------------
proc PasswdDlg::create { path args } {

    array set maps [list PasswdDlg {} :cmd {} .frame.lablog {} \
            .frame.labpass {}]
    array set maps [Widget::parseArgs PasswdDlg $args]

    Widget::initFromODB PasswdDlg "$path#PasswdDlg" $maps(PasswdDlg)

    # Extract the PasswdDlg megawidget options (those that don't map to a
    # subwidget)
    set type      [Widget::cget "$path#PasswdDlg" -type]
    set cmd       [Widget::cget "$path#PasswdDlg" -command]

    set defb -1
    set canb -1
    switch -- $type {
        ok        { set lbut {ok}; set defb 0 }
        okcancel  { set lbut {ok cancel} ; set defb 0; set canb 1 }
    }

    eval [list Dialog::create $path] $maps(:cmd) \
        [list -class PasswdDlg -image [Bitmap::get passwd] \
             -side bottom -default $defb -cancel $canb]
    foreach but $lbut {
        if { $but == "ok" && $cmd != "" } {
            Dialog::add $path -text $but -name $but -command $cmd
        } else {
            Dialog::add $path -text $but -name $but
        }
    }

    set frame [Dialog::getframe $path]
    bind $path  <Return>  ""
    bind $frame <Destroy> [list Widget::destroy $path\#PasswdDlg]

    set lablog [eval [list LabelEntry::create $frame.lablog] \
                    $maps(.frame.lablog) \
                    [list -name login -dragenabled 0 -dropenabled 0 \
                         -command [list PasswdDlg::_verifonpasswd \
                                       $path $frame.labpass]]]

    set labpass [eval [list LabelEntry::create $frame.labpass] \
                     $maps(.frame.labpass) \
                     [list -name password -show "*" \
                          -dragenabled 0 -dropenabled 0 \
                          -command [list PasswdDlg::_verifonlogin \
                                        $path $frame.lablog]]]

    # compute label width
    if {[$lablog cget -labelwidth] == 0} {
        set loglabel  [$lablog cget -label]
        set passlabel [$labpass cget -label]
        set labwidth  [_max [string length $loglabel] [string length $passlabel]]
        incr labwidth 1
        $lablog  configure -labelwidth $labwidth
        $labpass configure -labelwidth $labwidth
    }

    Widget::create PasswdDlg $path 0

    pack  $frame.lablog $frame.labpass -fill x -expand 1

    # added by bach@mwgdna.com
    #  give focus to loginlabel unless the state is disabled
    if {[$lablog cget -editable]} {
        focus $frame.lablog.e
    } else {
        focus $frame.labpass.e
    }
    set res [Dialog::draw $path]

    if { $res == 0 } {
        set res [list [$lablog.e cget -text] [$labpass.e cget -text]]
    } else {
        set res [list]
    }
    Widget::destroy "$path#PasswdDlg"
    destroy $path

    return $res
}

# -----------------------------------------------------------------------------
#  Command PasswdDlg::configure
# -----------------------------------------------------------------------------

proc PasswdDlg::configure { path args } {
    set res [Widget::configure "$path#PasswdDlg" $args]
}

# -----------------------------------------------------------------------------
#  Command PasswdDlg::cget
# -----------------------------------------------------------------------------

proc PasswdDlg::cget { path option } {
    return [Widget::cget "$path#PasswdDlg" $option]
}


# -----------------------------------------------------------------------------
#  Command PasswdDlg::_verifonlogin
# -----------------------------------------------------------------------------
proc PasswdDlg::_verifonlogin { path labpass } {
    Dialog::enddialog $path 0
}

# -----------------------------------------------------------------------------
#  Command PasswdDlg::_verifonpasswd
# -----------------------------------------------------------------------------
proc PasswdDlg::_verifonpasswd { path lablog } {
    focus $lablog
}

# -----------------------------------------------------------------------------
#  Command PasswdDlg::_max
# -----------------------------------------------------------------------------
proc PasswdDlg::_max { val1 val2 } { 
    return [expr {($val1 > $val2) ? ($val1) : ($val2)}] 
}
