#
# ======================================================================
# This file is part of Combat/Tcl, a Tcl CORBA Object Request Broker
#
# Please visit the Combat Web site at http://www.fpx.de/Combat/ for
# more information.
#
# Copyright (c) Frank Pilhofer, combat@fpx.de
#
# ======================================================================
#
# CVS Version Tag: $Id: combat.tcl,v 1.8 2008/11/08 18:24:50 Owner Exp $
#
# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------
#

#
# Tcl 8.1 adds the internationalization features that are used for codeset
# negotiation. Also, [string] had some bugs with binary strings in 8.0.
#

package require Tcl 8.1

#
# Look for Itcl, or, failing that, tcl++
#

if {[catch {
    package require Itcl
}]} {
    #
    # If we can't have Itcl, can we load tcl++?
    #

    package require tcl++

    #
    # When using tcl++, fool myself into thinking that Itcl is present.
    # The original tcl++ didn't want to be so bold.
    #

    namespace eval ::itcl {
	namespace import -force ::tcl++::class
	namespace import -force ::tcl++::delete
	namespace import -force ::tcl++::scope
    }

    package provide Itcl 3.0
}

#
# Load Combat
#

set _combat_mandatory_files {
    cdr giop iop iiop codeset str object orb corba
}

set _combat_optional_files {
    poa
}

foreach _combat_file $_combat_mandatory_files {
    set _combat_fullname [file join [file dirname [info script]] \
	    [set _combat_file].tcl]
    source $_combat_fullname
}

foreach _combat_file $_combat_optional_files {
    set _combat_fullname [file join [file dirname [info script]] \
	    [set _combat_file].tcl]
    if {[file exists $_combat_fullname]} {
	source $_combat_fullname
    }
}

#
# combat Namespace
#

namespace eval combat {
    proc ir {cmd data} {
	if {$cmd != "add"} {
	    error "illegal option for combat::ir, was expecting add"
	}
	::Combat::SimpleTypeRepository::add $data
    }
}

package provide corba 0.8
package provide combat 0.8

