#
# ----------------------------------------------------------------------
# Execute a worker.
#
# device:  The Executable Device.
# name:    The file name or entrypoint (device specific).
# cid:     Worker identifier.
# params:  A set of name/value pairs, i.e., a list with the names
#          as odd-numbered and the values as even-numbered items.
# pidvar:  The name of a variable to assign the process id to
#          (out parameter).  The process id can later be used to
#          terminate the worker.
# timeout: How long to wait for the worker to bind to our naming
#          context (in seconds).
#
# Returns the worker's object reference or throws an error.
# ----------------------------------------------------------------------
#

proc cfutil::executeWorker {device name cid params {pidvar ""} {timeout 60}} {
    upvar $pidvar pid

    #
    # Sanity check: Is this an executable device?
    #

    if {![$device _is_a IDL:CF/ExecutableDevice:1.0]} {
	error "not an executable device"
    }

    #
    # Start our Naming Context.
    #

    set mnc [cfutil::WaitForNameServiceBinding \#auto "worker"]
    set ncref [$mnc getContext]
    set ncior [corba::object_to_string $ncref]
    corba::release $ncref

    #
    # Make up a nice list of execution parameters.
    #

    set execParams [list]
    lappend execParams [list id "NAMING_CONTEXT_IOR" value [list string $ncior]]
    lappend execParams [list id "NAME_BINDING" value [list string "worker"]]
    lappend execParams [list id "COMPONENT_IDENTIFIER" value [list string $cid]]

    foreach {id value} $params {
	lappend execParams [list id $id value [list string $value]]
    }

    #
    # Execute the worker.
    #

    corba::try {
	set pid [$device execute $name [list] $execParams]
    } catch {... ex} {
	itcl::delete object $mnc
	corba::throw $ex
    }

    #
    # Wait for the worker to bind to our Naming Context.
    #

    set gotBinding [$mnc waitForBinding $timeout]

    if {!$gotBinding} {
	itcl::delete object $mnc
	error "worker started with pid $pid but did not bind to naming context"
    }

    set obj [$mnc getBinding]
    itcl::delete object $mnc

    #
    # Tell Combat that this is a CF::Resource object, just in case.
    #

    $obj _is_a IDL:CF/Resource:1.0
    return $obj
}
