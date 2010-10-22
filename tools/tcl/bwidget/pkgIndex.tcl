
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

if {[catch {package require Tcl}]} return
package ifneeded BWidget 1.8 "\
    package require Tk 8.1.1;\
    [list tclPkgSetup $dir BWidget 1.8 {
{arrow.tcl source {ArrowButton ArrowButton::create ArrowButton::use}}
{labelframe.tcl source {LabelFrame LabelFrame::create LabelFrame::use}}
{labelentry.tcl source {LabelEntry LabelEntry::create LabelEntry::use}}
{bitmap.tcl source {Bitmap::get Bitmap::use}}
{button.tcl source {Button Button::create Button::use}}
{buttonbox.tcl source {ButtonBox ButtonBox::create ButtonBox::use}}
{combobox.tcl source {ComboBox ComboBox::create ComboBox::use}}
{label.tcl source {Label Label::create Label::use}}
{entry.tcl source {Entry Entry::create Entry::use}}
{pagesmgr.tcl source {PagesManager PagesManager::create PagesManager::use}}
{notebook.tcl source {NoteBook NoteBook::create NoteBook::use}}
{panedw.tcl source {PanedWindow PanedWindow::create PanedWindow::use}}
{scrollw.tcl source {ScrolledWindow ScrolledWindow::create ScrolledWindow::use}}
{scrollview.tcl source {ScrollView ScrollView::create ScrollView::use}}
{scrollframe.tcl source {ScrollableFrame ScrollableFrame::create ScrollableFrame::use}}
{panelframe.tcl source {PanelFrame PanelFrame::create PanelFrame::use}}
{progressbar.tcl source {ProgressBar ProgressBar::create ProgressBar::use}}
{progressdlg.tcl source {ProgressDlg ProgressDlg::create ProgressDlg::use}}
{passwddlg.tcl source {PasswdDlg PasswdDlg::create PasswdDlg::use}}
{dragsite.tcl source {DragSite::register DragSite::include DragSite::use}}
{dropsite.tcl source {DropSite::register DropSite::include DropSite::use}}
{separator.tcl source {Separator Separator::create Separator::use}}
{spinbox.tcl source {SpinBox SpinBox::create SpinBox::use}}
{statusbar.tcl source {StatusBar StatusBar::create StatusBar::use}}
{titleframe.tcl source {TitleFrame TitleFrame::create TitleFrame::use}}
{mainframe.tcl source {MainFrame MainFrame::create MainFrame::use}}
{listbox.tcl source {ListBox ListBox::create ListBox::use}}
{tree.tcl source {Tree Tree::create Tree::use}}
{color.tcl source {SelectColor SelectColor::menu SelectColor::dialog SelectColor::setcolor}}
{dynhelp.tcl source {DynamicHelp::configure DynamicHelp::use DynamicHelp::register DynamicHelp::include DynamicHelp::add DynamicHelp::delete}}
{dialog.tcl source {Dialog Dialog::create Dialog::use}}
{messagedlg.tcl source {MessageDlg MessageDlg::create MessageDlg::use}}
{font.tcl source {SelectFont SelectFont::create SelectFont::use SelectFont::loadfont}}
{widgetdoc.tcl source {Widget::generate-doc Widget::generate-widget-doc}}
{wizard.tcl source {Wizard Wizard::create Wizard::use SimpleWizard ClassicWizard}}
{xpm2image.tcl source {xpm-to-image}}
}]; \
        [list namespace eval ::BWIDGET {}]; \
        [list set ::BWIDGET::LIBRARY $dir]; \
    [list source [file join $dir widget.tcl]]; \
    [list source [file join $dir init.tcl]]; \
    [list source [file join $dir utils.tcl]]; \
"
