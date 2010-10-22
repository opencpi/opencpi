
# #####
#
#  Copyright (c)  Frank Pilhofer, combat@fpx.de
#
#  Please visit the Combat Web site at http://www.fpx.de/Combat/ for
#  more information.
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
# ======================================================================
# This file is part of Combat/Tcl, a Tcl CORBA Object Request Broker
# ======================================================================
#
# CVS Version Tag: $Id: object.tcl,v 1.13 2008/11/14 02:06:58 Owner Exp $
#
# ----------------------------------------------------------------------
# CORBA::Object class
# ----------------------------------------------------------------------
#

namespace eval Combat {
    namespace eval CORBA {
        itcl::class Object {
            public variable ior
            public variable fwd_ior
            public variable conn
            public variable object_key
            public variable profile_index
            public variable orig_profile_index
            public variable AddressingDisposition
            public variable CodeSetInfo
            public variable type_id
            public variable reset_profile
            public variable timeout

            constructor {} {
                set fwd_ior ""
                set ior ""
                set conn ""
                set type_id ""
                set CodeSetInfo ""
                set profile_index 0
                set orig_profile_index 0
                set reset_profile 0
                set timeout 0
            }

            destructor {
                if {$conn != ""} {
                    $conn deref
                }
                if {$fwd_ior != ""} {
                    itcl::delete object $fwd_ior
                }
                itcl::delete object $ior
            }

            public method get_ior {} {
                return $ior
            }

            public method get_fwd_ior {} {
                if {$fwd_ior != ""} {
                    return $fwd_ior
                }
                return $ior
            }

            public method forward {newior {permanent {}}} {
                if {$permanent != ""} {
                    if {$fwd_ior != ""} {
                        itcl::delete object $fwd_ior
                    }
                    itcl::delete object $ior
                    set ior $newior
                    set fwd_ior ""
                } else {
                    if {$fwd_ior != ""} {
                        itcl::delete object $fwd_ior
                    }
                    set fwd_ior $newior
                    set orig_profile_index $profile_index
                }
                set profile_index 0
                disconnect
            }

            public method unforward {} {
                if {$fwd_ior != ""} {
                    itcl::delete object $fwd_ior
                    set fwd_ior ""
                }
                set profile_index [expr {$orig_profile_index + 1}]
                set orig_profile_index 0
                disconnect
            }

            #
            # switch to the next profile, unforwarding if none is left
            #

            public method next_profile {} {
                incr profile_index
                if {$fwd_ior != ""} {
                    if {$profile >= [llength [$fwd_ior cget -profiles]]} {
                        unforward
                    }
                }
                disconnect
            }

            public method connect {} {
                if {$conn != ""} {
                    if {[$conn cget -broken] == 0} {
                        return $conn
                    }
                    disconnect
                }

                if {$reset_profile} {
                    set profile_index 0
                    set orig_profile_index 0
                    set reset_profile 0
                }

                set first_tested_index $profile_index
                
                set conn ""
                set theior [get_fwd_ior]
                set profiles [$theior cget -profiles]

                while {$profile_index < [llength $profiles]} {
                    set profile [lindex $profiles $profile_index]

                    if {![catch {set data [$profile connect]}]} {
                        set conn [lindex $data 0]
                        set object_key [lindex $data 1]
                        set CodeSetInfo [lindex $data 2]
                        set AddressingDisposition 0

                        #
                        # find CodeSetInfo in "top level" MuCompProf
                        #

                        if {$CodeSetInfo == ""} {
                            set CodeSetInfo [$theior getCodesetInfo]
                        }

                        return $conn
                    }

                    incr profile_index
                }

                #
                # if we were forwarded, and all profiles in fwd_ior have
                # failed, unforward to the original address and retry
                #

                if {$fwd_ior != ""} {
                    unforward
                    return [connect]
                }

                set reset_profile 1

                #
                # If we have just tested all profiles, then this is a
                # hard failure. Otherwise, be a little more optimistic.
                #

                if {$first_tested_index == 0} {
                    ::corba::throw [list IDL:omg.org/CORBA/COMM_FAILURE:1.0 \
                            [list minor 0 completion_status COMPLETED_NO]]
                }

                ::corba::throw [list IDL:omg.org/CORBA/TRANSIENT:1.0 \
                        [list minor 0 completion_status COMPLETED_NO]]
            }

            public method disconnect {} {
                if {$conn != ""} {
                    $conn deref
                    set conn ""
                }
            }

            public method _is_a {repoid} {
                set repoid [::Combat::SimpleTypeRepository::getRepoid $repoid]
                set res [::Combat::CORBA::ORB::invoke_sync 1 $this \
                        _is_a boolean 0 {{in string}} [list $repoid]]
                if {$res == 1 && $type_id == ""} {
                    set type_id $repoid
                } elseif {$res == 1 && \
                        ![::Combat::SimpleTypeRepository::_is_a \
                        $type_id $repoid]} {
                    set type_id $repoid
                }
                return $res
            }

            public method _get_interface {} {
                set res [::Combat::CORBA::ORB::invoke_sync 1 $this \
                        _interface Object 0 {} {}]
                return $res
            }

            public method _non_existent {} {
                set res [::Combat::CORBA::ORB::invoke_sync 1 $this \
                        _non_existent boolean 0 {} {}]
                return $res
            }

            public method _is_equivalent {other} {
                if {$other == "0"} {
                    return 0
                }
                if {$other == $this} {
                    return 1
                }

                #
                # test IIOP profiles for equality
                #

                set other_ior [$other get_ior]
                if {$ior != "" && $other_ior != ""} {
                    set p1 ""
                    set p2 ""

                    foreach profile [$ior cget -profiles] {
                        if {[$profile cget -tag] == 0} {
                            set p1 $profile
                            break
                        }
                    }

                    foreach profile [$other_ior cget -profiles] {
                        if {[$profile cget -tag] == 0} {
                            set p2 $profile
                            break
                        }
                    }

                    if {$p1 != "" && $p2 != ""} {
                        if {[$p1 cget -host] == [$p2 cget -host] && \
                                [$p1 cget -port] == [$p2 cget -port] && \
                                [string compare [$p1 cget -object_key] \
                                [$p2 cget -object_key]] == 0} {
                            return 1
                        }
                    }
                }


                ::corba::throw [list IDL:omg.org/CORBA/TRANSIENT:1.0 \
                        [list minor 0 completion_status COMPLETED_NO]]
            }

            public method UpdateType {} {
                set theior [get_fwd_ior]
                if {$theior == ""} {
                    return 0
                }
                if {$type_id == ""} {
                    set type_id [$theior cget -type_id]
                }
                if {$type_id == ""} {
                    return 0
                }
                return 1
            }
        }
    }
}
