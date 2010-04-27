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
# CVS Version Tag: $Id: orb.tcl,v 1.16 2008/11/14 02:06:58 Owner Exp $
#
# ----------------------------------------------------------------------
# The ORB
# ----------------------------------------------------------------------
#

namespace eval Combat {
    namespace eval CORBA {
	variable ORB_conn_cond

	itcl::class ORB {
	    private common requests
	    private common callbacks
	    private common reqid 0
	    private common reqids_timer
	    private common reqids_timedout
	    private common reqids_completed
	    private common fake_asyncs
	    private common objprocid 0
	    public common refs_fwd
	    public common refs_rev
	    public common initial_pseudo_references
	    public common initial_references
	    public common default_init_ref ""
	    public common iiop_serverports
	    public common iiop_servers
	    public common ior_template
	    public common hostname

	    #
	    # send_request and receive_reply are low-level functions
	    # that process a single GIOP request/response.
	    #
	    # They are encapsulated by the higher-level functions
	    # invoke_sync, invoke_async, get_async_reply and
	    # poll_async_reply, which transparently perform a retry
	    # upon LOCATION_FORWARD etc.
	    #

	    private proc send_request {rid response object operation rtype \
			plevel params values {exceptions {}}} {
		incr plevel
		set conn [$object connect]

		if {$response} {
		    set id [$conn send_request $response $object \
			    $operation $plevel $params $values {} \
			    "::Combat::CORBA::ORB::callback $rid"]
		} else {
		    set id [$conn send_request $response $object \
			    $operation $plevel $params $values]
		}

		if {$response} {
		    $conn ref
		    set requests($rid) [list $object $operation $conn $id \
			    $rtype $params $values $exceptions -1]
		} else {
		    set requests($rid) [list $object {} {} $id {} \
			    {} {} {} 0]
		}

		#
		# trigger callback if !response_expected
		#

		if {!$response} {
		    after 0 "::Combat::CORBA::ORB::callback $rid 0"
		}
	    }

	    private proc receive_reply {rid plevel} {
		if {![info exists requests($rid)]} {
		    ::corba::throw [list IDL:omg.org/CORBA/BAD_PARAM:1.0 \
			    [list minor 0 completion_status COMPLETED_MAYBE]]
		}

		set data $requests($rid)
		set object    [lindex $data 0]
		set operation [lindex $data 1]
		set conn      [lindex $data 2]
		set connid    [lindex $data 3]
		set rtype     [lindex $data 4]
		set params    [lindex $data 5]
		set values    [lindex $data 6]
		set exceptions [lindex $data 7]
		set status    [lindex $data 8]

		if {$status != -1} {
		    # This invocation has already completed
		    set res [lindex $data 9]
		    unset requests($rid)
		    return [list $status $res]
		}

		incr plevel

		if {[catch {
		    set res [$conn receive_reply $connid $rtype $plevel \
				 $params $values $exceptions]
		} oops]} {
		    #
		    # This should only occur when the request was canceled.
		    #

		    set res [list -1]
		}

		$conn deref

		set status [lindex $res 0]
		switch -- $status {
		    0 -
		    1 -
		    2 {
			# After a successful method invocation, we set a
			# flag that allows the object to reuse its entire
			# set of profiles after disconnecting
			$object configure -reset_profile 1
		    }
		    3 { # LOCATION_FORWARD
			#
			# A LOCATION_FORWARD is also indicated, without a
			# new IOR, upon receiving a CloseConnection message.
			#
			if {[llength $res] == 2} {
			    $object forward [lindex $res 1]
			} else {
			    $object next_profile
			}
		    }
		    4 { # LOCATION_FORWARD_PERM
			$object forward [lindex $res 1] 1
		    }
		    5 { # NEEDS_ADDRESSING_MODE
			$object configure -AddressingDisposition [lindex $res 1]
		    }
		}

		#
		# Resend request
		#

		if {$status == 3 || $status == 4 || $status == 5} {
		    unset requests($rid)

		    if {[catch {
			send_request $rid 1 $object $operation $rtype \
			    $plevel $params $values $exceptions
		    } res]} {
			#
			# Re-sending the request has failed, e.g., because we
			# have run out of forwarding addresses.  Re-instate the
			# request data so that a future receive_reply will
			# return the appropriate exception.
			#

			set requests($rid) [list $object $operation $conn $connid \
						$rtype $params $values $exceptions \
						2 $res]

			return 2
		    }

		    return 0
		}

		if {$status != -1} {
		    unset requests($rid)
		}

		return $res
	    }

	    #
	    # Synchronous invocation of requests
	    #

	    public proc invoke_sync {response object operation rtype \
			plevel params values {exceptions {}} {timeout 0}} {
		incr plevel
		set rid [incr reqid]
		send_request $rid $response $object $operation $rtype \
		    $plevel $params $values $exceptions
		#
		# oneway ?
		#

		if {!$response} {
		    unset requests($rid)
		    return
		}

		#
		# Set up id mapping
		#

		set reqids_completed($rid) 0

		#
		# Set up timeout
		#

		if {$timeout} {
		    set reqids_timer($rid) [after $timeout "::Combat::CORBA::ORB::timeout $reqid"]
		}

		#
		# wait for reply
		#

		vwait [itcl::scope reqids_completed($rid)]

		if {![info exists reqids_completed($rid)]} {
		    ::corba::throw [list IDL:omg.org/CORBA/BAD_INV_ORDER:1.0 \
					[list minor 0 completion_status COMPLETED_MAYBE]]
		}

		#
		# Handle timeout
		#

		if {[info exists reqids_timedout($rid)]} {
		    unset reqids_timedout($rid)
		    unset reqids_completed($rid)
		    ::corba::throw [list IDL:omg.org/CORBA/TIMEOUT:1.0 \
			    [list minor 0 completion_status COMPLETED_MAYBE]]
		}

		#
		# Get reply
		#

		set res [receive_reply $rid $plevel]

		#
		# Clean up
		#

		unset reqids_completed($rid)

		#
		# Evaluate response
		#

		set status [lindex $res 0]
		switch -- $status {
		    0 { # NO_EXCEPTION
			return [lindex $res 1]
		    }
		    1 { # USER_EXCEPTION
			error [lindex $res 1]
		    }
		    2 { # SYSTEM_EXCEPTION
			error [lindex $res 1]
		    }
		    default {
			::corba::throw [list IDL:omg.org/CORBA/INTERNAL:1.0 \
				[list minor 0 completion_status COMPLETED_NO \
				debug "status is $status"]]
		    }
		}

		::corba::throw [list IDL:omg.org/CORBA/INTERNAL:1.0 \
			[list minor 0 completion_status COMPLETED_NO]]
	    }

	    #
	    # Asynchronous invocation
	    #

	    public proc invoke_async {response object operation rtype \
		    plevel params values {exceptions {}} {callback {}} \
		    {timeout 0}} {
		incr plevel
		set rid [incr reqid]
		set reqids_completed($rid) 0

		send_request $rid $response $object $operation $rtype \
		    $plevel $params $values $exceptions

		if {$callback != ""} {
		    set callbacks($rid) $callback
		}

		if {$timeout} {
		    set reqids_timer($rid) [after $timeout "::Combat::CORBA::ORB::timeout $reqid"]
		}

		return $reqid
	    }

	    public proc fake_async {status result {callback {}}} {
		incr reqid
		set fake_asyncs($reqid) [list $status $result $callback]

		if {$callback != ""} {
		    set callbacks($reqid) $callback
		    after 0 "::Combat::CORBA::ORB::callback $reqid $status"
		}

		return $reqid
	    }

	    public proc get_async_reply {rid plevel} {
		incr plevel

		#
		# See if the request has already timed out
		#

		if {[info exists reqids_timedout($rid)]} {
		    unset reqids_timedout($rid)
		    ::corba::throw [list IDL:omg.org/CORBA/TIMEOUT:1.0 \
			    [list minor 0 completion_status COMPLETED_MAYBE]]
		}

		if {[info exists fake_asyncs($rid)]} {
		    set res $fake_asyncs($rid)
		    unset fake_asyncs($rid)
		    set status [lindex $res 0]

		    switch -- $status {
			0 { # NO_EXCEPTION
			    return [lindex $res 1]
			}
			1 { # USER_EXCEPTION
			    error [lindex $res 1]
			}
			2 { # SYSTEM_EXCEPTION
			    error [lindex $res 1]
			}
		    }

		    error "oops"
		}

		if {![info exists reqids_completed($rid)]} {
		    ::corba::throw [list IDL:omg.org/CORBA/BAD_PARAM:1.0 \
			    [list minor 0 completion_status COMPLETED_MAYBE]]
		}

		#
		# wait for reply
		#

		if {!$reqids_completed($rid)} {
		    vwait [itcl::scope reqids_completed($rid)]
		}

		if {![info exists reqids_completed($rid)]} {
		    ::corba::throw [list IDL:omg.org/CORBA/BAD_INV_ORDER:1.0 \
					[list minor 0 completion_status COMPLETED_MAYBE]]
		}

		#
		# Handle timeout
		#

		if {[info exists reqids_timedout($rid)]} {
		    unset reqids_timedout($rid)
		    unset reqids_completed($rid)
		    ::corba::throw [list IDL:omg.org/CORBA/TIMEOUT:1.0 \
			    [list minor 0 completion_status COMPLETED_MAYBE]]
		}

		#
		# Get reply
		#

		set res [receive_reply $rid $plevel]

		#
		# Clean up
		#

		unset reqids_completed($rid)

		#
		# Evaluate response
		#

		set status [lindex $res 0]
		switch -- $status {
		    0 { # NO_EXCEPTION
			return [lindex $res 1]
		    }
		    1 { # USER_EXCEPTION
			error [lindex $res 1]
		    }
		    2 { # SYSTEM_EXCEPTION
			error [lindex $res 1]
		    }
		    default {
			::corba::throw [list IDL:omg.org/CORBA/INTERNAL:1.0 \
				[list minor 0 completion_status COMPLETED_NO \
				debug "status is $status"]]
		    }
		}

		::corba::throw [list IDL:omg.org/CORBA/INTERNAL:1.0 \
			[list minor 0 completion_status COMPLETED_NO]]
	    }

	    public proc poll_async_reply {rid} {
		#
		# reqids_completed($rid) exists for all outstanding remote
		# requests.
		#

		if {[info exists reqids_completed($rid)]} {
		    return $reqids_completed($rid)
		}

		#
		# We should only get here for fake async requests, which
		# are always ready.
		#

		if {![info exists fake_async($rid)]} {
		    ::corba::throw [list IDL:omg.org/CORBA/BAD_PARAM:1.0 \
			    [list minor 0 completion_status COMPLETED_MAYBE]]
		}

		return 1
	    }

	    public proc cancel_async_request {rid} {
		if {[info exists reqids_timedout($rid)]} {
		    unset reqids_timedout($rid)
		    unset reqids_completed($rid)
		    return
		}

		if {[info exists fake_asyncs($rid)]} {
		    unset $fake_asyncs($rid)
		    return
		}

		if {![info exists reqids_completed($rid)]} {
		    ::corba::throw [list IDL:omg.org/CORBA/BAD_PARAM:1.0 \
			    [list minor 0 completion_status COMPLETED_MAYBE]]
		}

		#
		# Cancel timeout
		#

		if {[info exists reqids_timer($rid)] && \
			![info exists reqids_timedout($rid)]} {
		    after cancel $reqids_timer($rid)
		    unset reqids_timer($rid)
		}

		#
		# Clean up
		#

		set data $requests($rid)
		unset requests($rid)

		if {[info exists reqids_completed($rid)]} {
		    unset reqids_completed($rid)
		}

		if {[info exists callbacks($rid)]} {
		    unset callbacks($rid)
		}

		#
		# Inform the connection that this request is canceled
		#

		set conn [lindex $data 2]
		set connid [lindex $data 3]
		$conn cancel_request $connid
		$conn deref
	    }

	    #
	    # Incoming invocation from GIOP
	    #

	    public proc invoke {response_expected \
		    conn request_id object_key \
		    operation decoder contexts} {

		#
		# gotta hand this over to an object adapter
		#

		::Combat::PortableServer::RootPOA::invoke $response_expected \
			$conn $request_id $object_key \
			$operation $decoder $contexts
	    }

	    public proc locate {conn request_id object_key} {
		#
		# gotta hand this over to an object adapter
		#

		::Combat::PortableServer::RootPOA::locate \
			$conn $request_id $object_key
	    }

	    #
	    # Callback from GIOP connection
	    #

	    public proc callback {rid status} {
		#
		# Transparently try again in case of LOCATION_FORWARD,
		# LOCATION_FORWARD_PERM or NEEDS_ADDRESSING_MODE. This
		# retry is performed by receive_reply.
		#

		if {$status == 3 || $status == 4 || $status == 5} {
		    if {[receive_reply $rid 0] == 0} {
			#
			# Ok, forwarded the request.
			#

			return
		    }

		    #
		    # Forwarding the request has failed.  Fall through and
		    # act as if the request has completed.
		    #
		}

		#
		# This request has completed
		#

		set reqids_completed($rid) 1
		set ::Combat::CORBA::ORB_conn_cond 1

		#
		# Cancel timeout
		#

		if {[info exists reqids_timer($rid)] && \
			![info exists reqids_timedout($rid)]} {
		    after cancel $reqids_timer($rid)
		    unset reqids_timer($rid)
		}

		#
		# Call user callback
		#

		if {[info exists callbacks($rid)]} {
		    set callback $callbacks($rid)
		    unset callbacks($rid)
		    uplevel #0 "$callback $rid"
		}
	    }

	    #
	    # Callback from "after" upon timeout
	    #

	    public proc timeout {rid} {
		#
		# Forget everything we ever knew about this request
		#

		set reqids_timedout($rid) 1
		set reqids_completed($rid) 1
		set data $requests($rid)
		unset requests($rid)
		unset reqids_timer($rid)

		#
		# Inform the connection that this request is canceled
		#

		set conn [lindex $data 2]
		set connid [lindex $data 3]
		$conn cancel_request $connid
		$conn deref

		#
		# Call this request's callback so that it learns about it
		#

		if {[info exists callbacks($rid)]} {
		    set callback $callbacks($rid)
		    unset callbacks($rid)
		    uplevel #0 "$callback $rid"
		}

		set ::Combat::CORBA::ORB_conn_cond 1
	    }

	    #
	    # Object Reference Handling
	    #

	    public proc MakeObjProc {obj} {
		incr objprocid
		set ref "_combat_obj_$objprocid"
		uplevel #0 proc $ref {args} \{ \
			eval ::Combat::CORBA::ORB::object_invoke $obj \$args \
			\}
		set refs_fwd($ref) $obj
		set refs_rev($obj) $ref
		return $ref
	    }

	    public proc GetObjFromRef {ref} {
		if {$ref == "0"} {
		    return 0
		}
		if {![info exists refs_fwd($ref)]} {
		    ::corba::throw [list IDL:omg.org/CORBA/BAD_PARAM:1.0 \
			    [list minor 0 completion_status COMPLETED_NO]]
		}
		return $refs_fwd($ref)
	    }

	    public proc GetRefFromObj {obj} {
		if {$ref == "0"} {
		    return 0
		}
		if {![info exists refs_rev($obj)]} {
		    ::corba::throw [list IDL:omg.org/CORBA/BAD_PARAM:1.0 \
			    [list minor 0 completion_status COMPLETED_NO]]
		}
		return $refs_rev($obj)
	    }

	    public proc ReleaseRef {ref} {
		if {$ref == "0"} {
		    return
		}
		set obj [GetObjFromRef $ref]
		unset refs_fwd($ref)
		unset refs_rev($obj)
		itcl::delete object $obj
		uplevel #0 rename $ref \"\"
	    }

	    #
	    # Method invocation
	    #

	    public proc object_invoke {obj args} {
		set idx 0
		set async 0
		set callback ""
		set response 1
		set timeout 0

		while {$idx < [llength $args]} {
		    if {[lindex $args $idx] == "-async"} {
			set async 1
			incr idx
		    } elseif {[lindex $args $idx] == "-callback"} {
			incr idx
			set async 1
			set callback [lindex $args $idx]
			incr idx
		    } elseif {[lindex $args $idx] == "-timeout"} {
			incr idx
			set timeout [lindex $args $idx]
			incr idx
		    } else {
			break
		    }
		}

		if {!$timeout} {
		    set timeout [$obj cget -timeout]
		}

		set operation [lindex $args $idx]
		incr idx
		set values [lrange $args $idx end]

		if {$operation == ""} {
		    error "usage: $refs_rev($obj) op ?parameters ...?"
		}

		if {$operation == "_is_a" || \
			$operation == "_get_interface" || \
			$operation == "_non_existent" || \
			$operation == "_duplicate"} {
		    switch -- $operation {
			_is_a {
			    if {[llength $values] != 1} {
				error "usage: $refs_rev($obj) _is_a repoid"
			    }
			    if {!$async} {
				return [$obj _is_a [lindex $values 0]]
			    }
			    set theop "_is_a"
			    set rtype "boolean"
			    set params {{in string}}
			}
			_get_interface {
			    if {[llength $values] != 0} {
				error "usage: $refs_rev($obj) _get_interface"
			    }
			    if {!$async} {
				return [$obj _get_interface]
			    }
			    set theop "_interface"
			    set rtype "Object"
			    set params [list]
			}
			_non_existent {
			    if {[llength $values] != 0} {
				error "usage: $refs_rev($obj) _non_existent"
			    }
			    if {!$async} {
				return [$obj _non_existent]
			    }
			    set theop "_non_existent"
			    set rtype "boolean"
			    set params [list]
			}
			_duplicate {
			    return [corba::string_to_object \
				    [corba::object_to_string \
				    $refs_rev($obj)]]
			}
			default {
			    error "oops"
			}
		    }

		    if {$async} {
			return [invoke_async 1 $obj $theop $rtype 2 \
				$params $values {} $callback $timeout]
		    }

		    return [invoke_sync 1 $obj $theop $rtype 2 \
				$params $values {} $timeout]
		}

		if {$operation == "_is_equivalent"} {
		    if {[llength $values] != 1} {
			error "usage: $refs_rev($obj) _is_equivalent ref"
		    }
		    set otherobj [GetObjFromRef [lindex $values 0]]
		    if {[catch {
			set res [$obj _is_equivalent $otherobj]
		    } err]} {
			set status 2
			set res $err
		    } else {
			set status 0
		    }
		    if {$async} {
			return [fake_async $status $res $callback]
		    }
		    if {$status == 2} {
			error $res
		    }
		    return $res
		}

		if {![::Combat::SimpleTypeRepository::UpdateTypeInfoForObj $obj]} {
		    ::corba::throw [list IDL:omg.org/CORBA/INTF_REPOS:1.0 \
			    [list minor 0 completion_status COMPLETED_NO]]
		}

		set type [$obj cget -type_id]

		set opInfo [::Combat::SimpleTypeRepository::getOp \
			$type $operation]

		if {$opInfo == ""} {
		    set atInfo [::Combat::SimpleTypeRepository::getAttr \
			    $type $operation]
		} else {
		    set atInfo ""
		}

		if {$opInfo == "" && $atInfo == ""} {
		    if {![::Combat::SimpleTypeRepository::UpdateTypeInfoForObj $obj 1]} {
			::corba::throw [list IDL:omg.org/CORBA/INTF_REPOS:1.0 \
				[list minor 0 completion_status COMPLETED_NO]]
		    }
		    
		    set type [$obj cget -type_id]
		    set opInfo [::Combat::SimpleTypeRepository::getOp \
			    $type $operation]
		    set atInfo [::Combat::SimpleTypeRepository::getAttr \
			    $type $operation]
		    if {$opInfo == "" && $atInfo == ""} {
			error "error: $operation is neither operation nor attribute for $type"
		    }
		}

		if {$opInfo != ""} {
		    set rtype [lindex $opInfo 9]
		    set params [lindex $opInfo 15]
		    set exceptions [lindex $opInfo 17]

		    if {[llength $params] != [llength $values]} {
			error "error: operation $operation wants [llength $params] parameters, not [llength $values]"
		    }

		    if {[lindex $opInfo 11] == "OP_ONEWAY"} {
			set response 0
		    }

		    if {$async} {
			return [invoke_async $response $obj $operation \
				$rtype 2 $params $values $exceptions \
				$callback $timeout]
		    }

		    return [invoke_sync $response $obj $operation \
			    $rtype 2 $params $values $exceptions \
			    $timeout]
		}

		if {$atInfo != ""} {
		    set attype [lindex $atInfo 9]

		    if {[llength $values] == 0} {
			if {[llength $atInfo] > 13} {
			    set get_exceptions [lindex $atInfo 13]
			} else {
			    set get_exceptions [list]
			}
			if {$async} {
			    return [invoke_async 1 $obj _get_$operation \
				    $attype 0 {} {} $get_exceptions \
				    $callback $timeout]
			}
			return [invoke_sync 1 $obj _get_$operation \
				$attype 0 {} {} {} $timeout]
		    } elseif {[llength $values] == 1} {
			if {[lindex $atInfo 11] == "ATTR_READONLY"} {
			    error "error: attribute $operation is readonly"
			}
			if {[llength $atInfo] > 15} {
			    set set_exceptions [lindex $atInfo 15]
			} else {
			    set set_exceptions [list]
			}
			if {$async} {
			    return [invoke_async 1 $obj _set_$operation \
				    void 0 [list [list in $attype]] $values \
				    $set_exceptions $callback $timeout]
			}

			return [invoke_sync 1 $obj _set_$operation \
				void 0 [list [list in $attype]] $values \
				{} $timeout]
		    }

		    error "attribute get wants 0, attribute set 1 parameter"
		}

		error "oops, I should not be here."
	    }

	    #
	    # corba::dii ?-async? ref spec args
	    #
	    # spec: [list returntype opname params exceptions]
	    #

	    public proc dii_invoke {args} {
		set idx 0
		set async 0
		set callback ""
		set response 1
		set timeout 0

		while {$idx < [llength $args]} {
		    if {[lindex $args $idx] == "-async"} {
			set async 1
			incr idx
		    } elseif {[lindex $args $idx] == "-callback"} {
			incr idx
			set async 1
			set callback [lindex $args $idx]
			incr idx
		    } elseif {[lindex $args $idx] == "-timeout"} {
			incr idx
			set timeout [lindex $args $idx]
			incr idx
		    } else {
			break
		    }
		}

		set ref [lindex $args $idx]
		set obj [GetObjFromRef $ref]
		incr idx

		set spec [lindex $args $idx]
		incr idx

		if {$idx > [llength $args]} {
		    error "too few arguments to dii_invoke"
		}

		if {[llength $spec] < 3} {
		    error "illegal dii call specification"
		}

		set rtype [lindex $spec 0]
		set operation [lindex $spec 1]
		set params [lindex $spec 2]
		set exceptions [lindex $spec 3]
		set values [lrange $args $idx end]

		if {[llength $spec] == 4 && \
			([lindex $spec 3] == "OP_ONEWAY" || \
			[lindex $spec 4] == "oneway")} {
		    set response 0
		}

		if {!$timeout} {
		    set timeout [$obj cget -timeout]
		}

		if {$async} {
		    return [invoke_async $response $obj $operation \
			    $rtype 2 $params $values $exceptions \
			    $callback $timeout]
		}

		return [invoke_sync $response $obj $operation \
			$rtype 2 $params $values $exceptions \
			$timeout]
	    }
	}
    }
}
