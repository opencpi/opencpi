
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

# ----------------------------------------------------------------------------
#  progressdlg.tcl
#  This file is part of Unifix BWidget Toolkit
# ----------------------------------------------------------------------------
#  Index of commands:
#     - ProgressDlg::create
# ----------------------------------------------------------------------------

namespace eval ProgressDlg {
    Widget::define ProgressDlg progressdlg Dialog ProgressBar

    Widget::bwinclude ProgressDlg Dialog :cmd \
        remove {
            -modal -image -bitmap -side -anchor -cancel -default
            -homogeneous -padx -pady -spacing
        }

    Widget::bwinclude ProgressDlg ProgressBar .frame.pb \
        remove {-orient -width -height}

    Widget::declare ProgressDlg {
        {-width        TkResource 25 0 label}
        {-height       TkResource 2  0 label}
        {-textvariable TkResource "" 0 label}
        {-font         TkResource "" 0 label}
        {-stop         String "" 0}
        {-command      String "" 0}
    }

    Widget::addmap ProgressDlg :cmd .frame.msg \
        {-width {} -height {} -textvariable {} -font {} -background {}}
}


# ----------------------------------------------------------------------------
#  Command ProgressDlg::create
# ----------------------------------------------------------------------------
proc ProgressDlg::create { path args } {
    array set maps [list ProgressDlg {} :cmd {} .frame.msg {} .frame.pb {}]
    array set maps [Widget::parseArgs ProgressDlg $args]

    eval [list Dialog::create] $path $maps(:cmd) \
        [list -image [Bitmap::get hourglass] \
             -modal none -side bottom -anchor e -class ProgressDlg]

    Widget::initFromODB ProgressDlg "$path#ProgressDlg" $maps(ProgressDlg)

    wm protocol $path WM_DELETE_WINDOW {;}

    set frame [Dialog::getframe $path]
    bind $frame <Destroy> [list Widget::destroy $path\#ProgressDlg]
    $frame configure -cursor watch

    eval [list label $frame.msg] $maps(.frame.msg) \
        -relief flat -borderwidth 0 \
            -highlightthickness 0 -anchor w -justify left
    pack $frame.msg -side top -pady 3m -anchor nw -fill x -expand yes

    eval [list ProgressBar::create] $frame.pb $maps(.frame.pb) -width 100
    pack $frame.pb -side bottom -anchor w -fill x -expand yes

    set stop [Widget::cget "$path#ProgressDlg" -stop]
    set cmd  [Widget::cget "$path#ProgressDlg" -command]
    if { $stop != "" && $cmd != "" } {
        Dialog::add $path -text $stop -name $stop -command $cmd
    }
    Dialog::draw $path
    BWidget::grab local $path

    return [Widget::create ProgressDlg $path 0]
}


# ----------------------------------------------------------------------------
#  Command ProgressDlg::configure
# ----------------------------------------------------------------------------
proc ProgressDlg::configure { path args } {
    return [Widget::configure "$path#ProgressDlg" $args]
}


# ----------------------------------------------------------------------------
#  Command ProgressDlg::cget
# ----------------------------------------------------------------------------
proc ProgressDlg::cget { path option } {
    return [Widget::cget "$path#ProgressDlg" $option]
}
