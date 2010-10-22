
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
