
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
# CVS Version Tag: $Id: poa.tcl,v 1.14 2008/11/14 02:06:58 Owner Exp $
#
# ----------------------------------------------------------------------
# The POA
# ----------------------------------------------------------------------
#

namespace eval Combat {
    namespace eval PortableServer {
        variable POAImplName "default"
        variable theRootPOA 0
        variable theCurrent 0
        variable initialized 0

        #
        # helper
        #

        proc NormalizeName {servant} {
            if {[string range $servant 0 3] == "::::"} {
                return [string range $servant 2 end]
            } elseif {[string range $servant 0 1] != "::"} {
                return "::$servant"
            }
            return $servant
        }
        
        #
        # PortableServer::POAManager
        #

        itcl::class POAManager {
            private variable state
            private variable managed

            constructor {} {
                set state HOLDING
                set managed [list]
            }

            private method change_state {new_state \
                    {etherealize_objects 0} \
                    {wait_for_completion 0}} {
                if {$state == "INACTIVE"} {
                    corba::throw IDL:omg.org/PortableServer/POAManager/AdapterInactive:1.0
                }

                set state $new_state

                foreach poa $managed {
                    $poa poa_manager_callback $state $etherealize_objects \
                            $wait_for_completion
                }
            }

            public method add_managed_poa {poa} {
                lappend managed $poa
            }

            public method del_managed_poa {poa} {
                for {set idx 0} {$idx < [llength $managed]} {incr idx} {
                    if {[string equal [lindex $managed $idx] $poa]} {
                        set managed [lreplace $managed $idx $idx]
                        return
                    }
                }
            }

            public method get_state {} {
                return $state
            }

            public method activate {} {
                change_state ACTIVE
            }

            public method hold_requests {wait_for_completion} {
                change_state HOLDING $wait_for_completion
            }

            public method discard_requests {wait_for_completion} {
                change_state DISCARDING $wait_for_completion
            }

            public method deactivate {wait_for_completion etherealize_objs} {
                change_state INACTIVE $wait_for_completion $etherealize_objs
            }
        }

        #
        # PortableServer::POACurrent
        #

        itcl::class POACurrent {
            private variable state

            constructor {} {
                set state [list]
            }

            public method set_state {poa id servant} {
                lappend state [list $poa $id $servant]
            }

            public method unset_state {} {
                set state [lreplace $state end end]
            }

            public method get_POA {} {
                if {[llength $state] == 0} {
                    corba::throw IDL:omg.org/PortableServer/Current/NoContext:1.0
                }
                return [lindex [lindex $state end] 0]
            }

            public method get_object_id {} {
                if {[llength $state] == 0} {
                    corba::throw IDL:omg.org/PortableServer/Current/NoContext:1.0
                }
                return [lindex [lindex $state end] 1]
            }

            public method get_reference {} {
                if {[llength $state] == 0} {
                    corba::throw IDL:omg.org/PortableServer/Current/NoContext:1.0
                }
                set poa [get_POA]
                set oid [get_object_id]
                return [$poa id_to_reference $oid]
            }

            public method get_servant {} {
                if {[llength $state] == 0} {
                    corba::throw IDL:omg.org/PortableServer/Current/NoContext:1.0
                }
                return [lindex [lindex $state end] 2]
            }
        }

        itcl::class POA {
            protected common unique_prefix
            protected common all_poas

            private variable thread_policy
            private variable lifespan_policy
            private variable id_uniqueness_policy
            private variable id_assignment_policy
            private variable implicit_activation_policy
            private variable servant_retention_policy
            private variable request_processing_policy

            private variable manager
            private variable default_servant
            private variable servant_manager
            private variable adapter_activator
            private variable next_oid
            private variable holding_invocations

            public variable name
            public variable fqn
            protected variable state
            protected variable parent
            protected variable children
            protected variable oaid

            #
            # map oid -> [list servant repository_id]
            #

            private variable active_object_map

            #
            # map servant -> [list of oids]
            #

            private variable active_servant_map

            #
            # methods
            #

            constructor {my_name my_manager my_policies my_parent} {
                set name $my_name
                set manager $my_manager
                set parent $my_parent

                if {$parent != 0} {
                    set pfqn [$parent cget -fqn]
                    if {$pfqn != "RootPOA"} {
                        set fqn [format "%s/%s" $fqn $name]
                    } else {
                        set fqn $name
                    }
                } else {
                    set fqn "RootPOA"
                }

                if {$manager == 0} {
                    set manager [namespace current]::[::Combat::PortableServer::POAManager \#auto]
                }

                set state [$manager get_state]
                set thread_policy ORB_CTRL_MODEL
                set lifespan_policy TRANSIENT
                set id_uniqueness_policy UNIQUE_ID
                set id_assignment_policy SYSTEM_ID
                set implicit_activation_policy NO_IMPLICIT_ACTIVATION
                set servant_retention_policy RETAIN
                set request_processing_policy USE_ACTIVE_OBJECT_MAP_ONLY

                set idx -1
                foreach policy $my_policies {
                    incr idx
                    switch -- $policy {
                        ORB_CTRL_MODEL -
                        SINGLE_THREAD_MODEL -
                        MAIN_THREAD_MODEL {
                            set thread_policy $policy
                        }
                        TRANSIENT -
                        PERSISTENT {
                            set lifespan_policy $policy
                        }
                        UNIQUE_ID -
                        MULTIPLE_ID {
                            set id_uniqueness_policy $policy
                        }
                        USER_ID -
                        SYSTEM_ID {
                            set id_assignment_policy $policy
                        }
                        RETAIN -
                        NON_RETAIN {
                            set servant_retention_policy $policy
                        }
                        USE_ACTIVE_OBJECT_MAP_ONLY -
                        USE_DEFAULT_SERVANT -
                        USE_SERVANT_MANAGER {
                            set request_processing_policy $policy
                        }
                        IMPLICIT_ACTIVATION -
                        NO_IMPLICIT_ACTIVATION {
                            set implicit_activation_policy $policy
                        }
                        default {
                            corba::throw [list IDL:omg.org/PortableServer/POA/InvalidPolicy:1.0 [list index $idx]]
                        }
                    }
                }
                    
                set default_servant 0
                set servant_manager 0
                set adapter_activator 0
                
                if {$lifespan_policy == "TRANSIENT"} {
                    if {$fqn != "RootPOA"} {
                        set oaid [format "%s/%s" $unique_prefix $fqn]
                    }
                } else {
                    if {$fqn == $::Combat::PortableServer::POAImplName} {
                        set oaid $fqn
                    } else {
                        set oaid [format "%s/%s" $::Combat::PortableServer::POAImplName $fqn]
                    }
                }
                
                if {$fqn != "RootPOA"} {
                    set all_poas($oaid) $this
                }
                
                if {$::Combat::debug(poa)} {
                    puts stderr "POA:  $fqn created, status is $state"
                }
                
                set next_oid 0
                array set children [list]
                array set active_object_map [list]
                array set active_servant_map [list]
                set holding_invocations [list]
                $manager add_managed_poa $this
            }

            public method create_POA {adapter_name a_POAManager policies} {
                if {[info exists children($adapter_name)]} {
                    corba::throw IDL:omg.org/PortableServer/POA/AdapterAlreadyExists:1.0
                }
                set newoa [namespace current]::[Combat::PortableServer::POA \
                        \#auto $adapter_name $a_POAManager $policies $this]
                set children($adapter_name) $newoa
                return $newoa
            }

            public method find_POA {adapter_name activate_it} {
                if {[info exists children($adapter_name)]} {
                    return $children($adapter_name)
                }
                if {!$activate_it || $adapter_activator == 0} {
                    corba::throw IDL:omg.org/PortableServer/POA/AdapterNonExistent:1.0
                }
                if {[catch {
                    set newoa [$adapter_activator $this $adapter_name]
                }]} {
                    corba::throw [list IDL:omg.org/CORBA/OBJ_ADAPTER \
                            [list minor 1 completion_status COMPLETED_MAYBE]]
                }
                return $newoa
            }

            public method destroy {etherealize_objects wait_for_completion} {
                error "not implemented yet"
            }

            public method the_name {} {
                return $name
            }

            public method the_parent {} {
                return $parent
            }

            public method the_children {} {
                set allpoas [list]
                foreach child [array names children] {
                    lappend allpoas $children($child)
                }
                return $allpoas
            }

            public method the_POAManager {} {
                return $manager
            }

            public method get_servant_manager {} {
                if {$request_processing_policy != "USE_SERVANT_MANAGER"} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongPolicy:1.0
                }
                return $servant_manager
            }

            public method set_servant_manager {imgr} {
                if {$request_processing_policy != "USE_SERVANT_MANAGER"} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongPolicy:1.0
                }
                set servant_manager [::Combat::PortableServer::NormalizeName $imgr]
            }

            public method get_servant {} {
                if {$request_processing_policy != "USE_DEFAULT_SERVANT"} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongPolicy:1.0
                }
                return $default_servant
            }

            public method set_servant {p_servant} {
                if {$request_processing_policy != "USE_DEFAULT_SERVANT"} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongPolicy:1.0
                }
                set default_servant [::Combat::PortableServer::NormalizeName $p_servant]
            }

            public method activate_object {servant} {
                set servant [::Combat::PortableServer::NormalizeName $servant]

                if {$id_assignment_policy != "SYSTEM_ID" || \
                        $servant_retention_policy != "RETAIN"} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongPolicy:1.0
                }

                if {$id_uniqueness_policy != "MULTIPLE_ID"} {
                    if {[info exists active_servant_map($servant)]} {
                        corba::throw IDL:omg.org/PortableServer/POA/ServantAlreadyActive:1.0
                    }   
                }

                incr next_oid

                if {$lifespan_policy == "PERSISTENT"} {
                    set oid [format "%s.%d" $unique_prefix $next_oid]
                } else {
                    set oid [format "%d" $next_oid]
                }

                if {[catch {
                    set repoidorscopedname [$servant _Interface]
                }]} {
                    if {$::Combat::debug(poa)} {
                        puts stderr "POA:  $fqn cannot activate $servant: _Interface failed"
                    }
                    corba::throw [list IDL:omg.org/CORBA/BAD_PARAM \
                            [list minor 1 completion_status COMPLETED_NO]]
                }

                if {[catch {
                    set type_id [::Combat::SimpleTypeRepository::getRepoid $repoidorscopedname]
                }]} {
                    if {$::Combat::debug(poa)} {
                        puts stderr "POA:  $fqn error: no type information for $repoidorscopedname"
                    }

                    corba::throw [list IDL:omg.org/CORBA/INTF_REPOS \
                            [list minor 1 completion_status COMPLETED_NO]]
                }

                if {![::Combat::SimpleTypeRepository::UpdateTypeInfoForId $type_id]} {
                    if {$::Combat::debug(poa)} {
                        puts stderr "POA:  $fqn error: no type information for $type_id"
                    }

                    corba::throw [list IDL:omg.org/CORBA/INTF_REPOS \
                            [list minor 1 completion_status COMPLETED_NO]]
                }

                set active_object_map($oid) [list $servant $type_id]

                if {$::Combat::debug(poa)} {
                    puts stderr "POA:  $fqn activating $oid, type $type_id"
                }

                if {[info exists active_servant_map($servant)]} {
                    lappend active_servant_map($servant) $oid
                } else {
                    set active_servant_map($servant) [list $oid]
                }
                
                return $oid
            }

            public method activate_object_with_id {oid servant} {
                set servant [::Combat::PortableServer::NormalizeName $servant]

                if {$servant_retention_policy != "RETAIN"} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongPolicy:1.0
                }
                if {[info exists active_object_map($oid)]} {
                    corba::throw IDL:omg.org/PortableServer/POA/ObjectAlreadyActive:1.0
                }
                if {$id_uniqueness_policy != "MULTIPLE_ID"} {
                    if {[info exists active_servant_map($servant)]} {
                        corba::throw IDL:omg.org/PortableServer/POA/ServantAlreadyActive:1.0
                    }   
                }


                if {[catch {
                    set repoidorscopedname [$servant _Interface]
                }]} {
                    if {$::Combat::debug(poa)} {
                        puts stderr "POA:  $fqn cannot activate $servant: _Interface failed"
                    }
                    corba::throw [list IDL:omg.org/CORBA/BAD_PARAM \
                            [list minor 1 completion_status COMPLETED_NO]]
                }

                if {[catch {
                    set type_id [::Combat::SimpleTypeRepository::getRepoid $repoidorscopedname]
                }]} {
                    if {$::Combat::debug(poa)} {
                        puts stderr "POA:  $fqn error: no type information for $repoidorscopedname"
                    }

                    corba::throw [list IDL:omg.org/CORBA/INTF_REPOS \
                            [list minor 1 completion_status COMPLETED_NO]]
                }

                set active_object_map($oid) [list $servant $type_id]

                if {$::Combat::debug(poa)} {
                    puts stderr "POA:  $fqn activating $oid, type $type_id"
                }

                if {[info exists active_servant_map($servant)]} {
                    lappend active_servant_map($servant) $oid
                } else {
                    set active_servant_map($servant) [list $oid]
                }

                return
            }

            public method deactivate_object {oid} {
                if {$servant_retention_policy != "RETAIN"} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongPolicy:1.0
                }
                if {![info exists active_object_map($oid)]} {
                    corba::throw IDL:omg.org/PortableServer/POA/ObjectNotActive:1.0
                }

                set servant [lindex $active_object_map($oid) 0]
                unset active_object_map($oid)

                if {[llength $active_servant_map($servant)] == 1} {
                    set remaining_activations 0
                    unset active_servant_map($servant)
                } else {
                    set idx [lsearch -exact $active_servant_map($servant) $servant]
                    set active_servant_map($servant) \
                            [lreplace $active_servant_map($servant) $idx $idx]
                    set remaining_activations 1
                }

                if {$::Combat::debug(poa)} {
                    puts stderr "POA:  $fqn deactivating $oid"
                }

                if {$request_processing_policy == "USE_SERVANT_MANAGER" && \
                        $servant_retention_policy == "RETAIN" && \
                        $servant_manager != 0} {
                    catch {
                        $servant_manager etherealize $oid $this $servant \
                                0 $remaining_activations
                    }
                }
            }

            public method create_reference {intf} {
                if {$id_assignment_policy != "SYSTEM_ID"} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongPolicy:1.0
                }

                incr next_oid

                if {$lifespan_policy == "PERSISTENT"} {
                    set oid [format "%s.%d" $unique_prefix $next_oid]
                } else {
                    set oid [format "%d" $next_oid]
                }

                return [create_reference_with_id $oid $intf]
            }

            public method create_reference_with_id {oid intf} {
                if {![::Combat::SimpleTypeRepository::UpdateTypeInfoForId $intf]} {
                    if {$::Combat::debug(poa)} {
                        puts stderr "POA:  $fqn error: no type information for $intf"
                    }

                    corba::throw [list IDL:omg.org/CORBA/INTF_REPOS \
                            [list minor 1 completion_status COMPLETED_NO]]
                }

                set str_template [$::Combat::CORBA::ORB::ior_template stringify]
                set template [::Combat::IOP::DestringifyIOR $str_template]
                $template configure -type_id $intf

                if {$oid == $oaid} {
                    set object_key $oid
                } else {
                    set object_key [format "%s*%s" $oaid $oid]
                }

                foreach profile [$template cget -profiles] {
                    catch {$profile configure -object_key $object_key}
                }

                set obj [namespace current]::[::Combat::CORBA::Object \#auto]
                $obj configure -ior $template
                return [::Combat::CORBA::ORB::MakeObjProc $obj]
            }

            public method servant_to_id {servant} {
                set servant [::Combat::PortableServer::NormalizeName $servant]

                if {$servant_retention_policy == "RETAIN" && \
                        $id_uniqueness_policy == "UNIQUE_ID"} {
                    if {[info exists active_servant_map($servant)]} {
                        return [lindex $active_servant_map($servant) 0]
                    }
                }
                if {$servant_retention_policy == "RETAIN" && \
                        $implicit_activation_policy == "IMPLICIT_ACTIVATION"} {
                    if {![info exists active_servant_map($servant)] || \
                            $id_uniqueness_policy == "MULTIPLE_ID"} {
                        return [activate_object $servant]
                    }
                }
                if {$request_processing_policy == "USE_DEFAULT_SERVANT"} {
                    catch {
                        set cp [$::Combat::PortableServer::theCurrent get_POA]
                        set cs [$::Combat::PortableServer::theCurrent get_servant]
                        set ci [$::Combat::PortableServer::theCurrent get_object_id]
                        if {$cp == $this && $cs == $servant && $cs == $default_servant} {
                            return $ci
                        }
                    }
                }
                corba::throw IDL:omg.org/PortableServer/POA/ServantNotActive:1.0
            }

            public method servant_to_reference {servant} {
                set servant [::Combat::PortableServer::NormalizeName $servant]

                if {$servant_retention_policy == "RETAIN" && \
                        $id_uniqueness_policy == "UNIQUE_ID"} {
                    if {[info exists active_servant_map($servant)]} {
                        return [id_to_reference [lindex $active_servant_map($servant) 0]]
                    }
                }
                if {$servant_retention_policy == "RETAIN" && \
                        $implicit_activation_policy == "IMPLICIT_ACTIVATION"} {
                    if {![info exists active_servant_map($servant)] || \
                            $id_uniqueness_policy == "MULTIPLE_ID"} {
                        return [id_to_reference [activate_object $servant]]
                    }
                }
                catch {
                    set cp [$::Combat::PortableServer::theCurrent get_POA]
                    set cs [$::Combat::PortableServer::theCurrent get_servant]
                    set ci [$::Combat::PortableServer::theCurrent get_object_id]
                    if {$cp == $this && $cs == $servant} {
                        return [id_to_reference $ci]
                    }
                }
                corba::throw IDL:omg.org/PortableServer/POA/ServantNotActive:1.0
            }

            public method reference_to_servant {reference} {
                return [id_to_servant [reference_to_id $reference]]
            }

            public method reference_to_id {reference} {
                set obj [::Combat::CORBA::ORB::GetObjFromRef $reference]
                set ior [$obj get_fwd_ior]
                set key ""
                foreach profile [$ior cget -profiles] {
                    if {![catch {set key [$profile cget -object_key]}]} {
                        if {$key == $oaid} {
                            break
                        } else {
                            set idx [string first * $key]
                            if {$idx > 0} {
                                incr idx -1
                                set theoaid [string range $key 0 $idx]
                                if {$theoaid == $oaid} {
                                    break
                                }
                            }
                        }
                    }
                    set key ""
                }
                if {$key == ""} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongAdapter:1.0
                }
                if {$key == $oaid} {
                    return $oaid
                }
                return [string range $key [expr {[string length $oaid] + 1}] end]
            }

            public method id_to_servant {oid} {
                if {$servant_retention_policy != "RETAIN" && \
                        $request_processing_policy != "USE_DEFAULT_SERVANT"} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongPolicy:1.0
                }
                if {[info exists active_object_map($oid)]} {
                    return [lindex $active_object_map($oid) 0]
                }
                if {$request_processing_policy == "USE_DEFAULT_SERVANT" && \
                        $default_servant != 0} {
                    return $default_servant
                }
                corba::throw IDL:omg.org/PortableServer/POA/ObjectNotActive:1.0
            }

            public method id_to_reference {oid} {
                if {$servant_retention_policy != "RETAIN"} {
                    corba::throw IDL:omg.org/PortableServer/POA/WrongPolicy:1.0
                }
                if {![info exists active_object_map($oid)]} {
                    corba::throw IDL:omg.org/PortableServer/POA/ObjectNotActive:1.0
                }
                set type_id [lindex $active_object_map($oid) 1]
                return [create_reference_with_id $oid $type_id]
            }

            public method poa_manager_callback {mystate etherealize_objects \
                    wait_for_completion} {
                if {$::Combat::debug(poa)} {
                    puts stderr "POA:  $fqn switching from $state to $mystate"
                }

                set state $mystate
                switch -- $state {
                    ACTIVE -
                    DISCARDING {
                        foreach inv $holding_invocations {
                            after 0 "$this $inv"
                        }
                        set holding_invocations [list]
                    }
                    INACTIVE {
                        if {$etherealize_objects} {
                            etherealize
                        }
                    }
                }
            }

            public method local_invoke {response_expected \
                    conn request_id object_key \
                    operation decoder contexts} {
                
                switch -- $state {
                    HOLDING {
                        if {$::Combat::debug(poa)} {
                            puts stderr "POA:  $fqn HOLDING $operation on $object_key"
                        }
                        lappend holding_invocations [list local_invoke \
                                $response_expected \
                                $conn $request_id $object_key \
                                $operation $decoder $contexts]
                        return
                    }
                    DISCARDING {
                        if {$::Combat::debug(poa)} {
                            puts stderr "POA:  $fqn DISCARDING $operation on $object_key"
                        }
                        if {$response_expected} {
                            $conn send_reply $request_id 2 dummy \
                                    [list IDL:omg.org/CORBA/TRANSIENT:1.0 \
                                    [list minor 0 completion_status COMPLETED_NO]] \
                                    0 "" ""
                        }
                        itcl::delete object $decoder
                        return
                    }
                    INACTIVE {
                        if {$::Combat::debug(poa)} {
                            puts stderr "POA:  $fqn INACTIVE $operation on $object_key"
                        }
                        if {$response_expected} {
                            if {$lifespan_policy == "TRANSIENT"} {
                                $conn send_reply $request_id 2 dummy \
                                        [list IDL:omg.org/CORBA/OBJECT_NOT_EXIST:1.0 \
                                        [list minor 0 completion_status COMPLETED_NO]] \
                                        0 "" ""
                            } else {
                                $conn send_reply $request_id 2 dummy \
                                        [list IDL:omg.org/CORBA/TRANSIENT:1.0 \
                                        [list minor 0 completion_status COMPLETED_NO]] \
                                        0 "" ""
                            }
                        }
                        itcl::delete object $decoder
                        return
                    }
                    ACTIVE {
                        # ok
                    }
                    default {
                        error "oops"
                    }
                }

                #
                # determine object id for invocation
                #

                if {$object_key == $oaid} {
                    set oid $object_key
                } else {
                    set oid [string range $object_key [expr {[string length $oaid] + 1}] end]
                }

                #
                # Try to find a servant
                #

                set servant 0
                set type_id ""

                #
                # check the active object map
                #

                if {$servant_retention_policy == "RETAIN"} {
                    if {[info exists active_object_map($oid)]} {
                        set servant [lindex $active_object_map($oid) 0]
                        set type_id [lindex $active_object_map($oid) 1]
                    }
                }

                #
                # if use_default_servant, use it
                #

                if {$servant == 0 && $request_processing_policy == "USE_DEFAULT_SERVANT"} {
                    if {$default_servant == 0} {
                        if {$response_expected} {
                            $conn send_reply $request_id 2 dummy \
                                    [list IDL:omg.org/CORBA/OBJ_ADAPTER:1.0 \
                                    [list minor 0 completion_status COMPLETED_NO]] \
                                    0 "" ""
                        }
                        itcl::delete object $decoder
                        return
                    }
                    set servant $default_servant
                    set repoidorscopedname [$servant _Interface]
                    set type_id [::Combat::SimpleTypeRepository::getRepoid $repoidorscopedname]
                }

                #
                # if USE_SERVANT_MANAGER, ask the manager for a servant
                #

                if {$servant == 0 && $request_processing_policy == "USE_SERVANT_MANAGER"} {
                    if {$servant_manager == 0} {
                        if {$response_expected} {
                            $conn send_reply $request_id 2 dummy \
                                    [list IDL:omg.org/CORBA/OBJ_ADAPTER:1.0 \
                                    [list minor 0 completion_status COMPLETED_NO]] \
                                    0 "" ""
                        }
                        itcl::delete object $decoder
                        return
                    }

                    if {$servant_retention_policy == "RETAIN"} {
                        if {[catch {
                            set servant [$servant_manager incarnate $oid $this]
                            set repoidorscopedname [$servant _Interface]
                            set type_id [::Combat::SimpleTypeRepository::getRepoid $repoidorscopedname]
                            activate_object_with_id $oid $servant
                        }]} {
                            if {$response_expected} {
                                $conn send_reply $request_id 2 dummy \
                                        [list IDL:omg.org/CORBA/OBJ_ADAPTER:1.0 \
                                        [list minor 0 completion_status COMPLETED_NO]] \
                                        0 "" ""
                            }
                            itcl::delete object $decoder
                            return
                        }
                    } else {
                        if {[catch {
                            set cookie 0
                            set servant [$servant_manager preinvoke $oid \
                                    $this $operation cookie]
                            set repoidorscopedname [$servant _Interface]
                            set type_id [::Combat::SimpleTypeRepository::getRepoid $repoidorscopedname]
                        }]} {
                            if {$response_expected} {
                                $conn send_reply $request_id 2 dummy \
                                        [list IDL:omg.org/CORBA/OBJ_ADAPTER:1.0 \
                                        [list minor 0 completion_status COMPLETED_NO]] \
                                        0 "" ""
                            }
                            itcl::delete object $decoder
                            return
                        }
                    }
                }

                #
                # still no servant?
                #
                
                if {$servant == 0} {
                    if {$response_expected} {
                        $conn send_reply $request_id 2 dummy \
                                [list IDL:omg.org/CORBA/OBJ_ADAPTER:1.0 \
                                [list minor 0 completion_status COMPLETED_NO]] \
                                0 "" ""
                    }
                    itcl::delete object $decoder
                    return
                }

                #
                # perform invocation
                #

                if {$::Combat::debug(poa)} {
                    puts stderr "POA:  $fqn invoking $operation on object $oid"
                }

                if {![::Combat::SimpleTypeRepository::UpdateTypeInfoForId $type_id]} {
                    if {$::Combat::debug(poa)} {
                        puts stderr "POA:  $fqn error: no type information for $type_id"
                    }

                    if {$response_expected} {
                        $conn send_reply $request_id 2 dummy \
                                [list IDL:omg.org/CORBA/INTF_REPOS:1.0 \
                                [list minor 0 completion_status COMPLETED_NO]] \
                                0 "" ""
                    }
                    itcl::delete object $decoder
                    return
                }

                if {$operation == "_is_a"} {
                    if {[catch {
                        set type [$decoder Demarshal string]
                    } err]} {
                        if {$::Combat::debug(poa)} {
                            puts stderr "POA:  $fqn demarshalling error: $err"
                        }
                        if {$response_expected} {
                            $conn send_reply $request_id 2 dummy \
                                    [list IDL:omg.org/CORBA/MARSHAL:1.0 \
                                    [list minor 0 completion_status COMPLETED_NO]] \
                                    0 "" ""
                        }
                        itcl::delete object $decoder
                        return
                    }

                    if {$type == "IDL:omg.org/Reflection/IFRProvider:1.0"} {
                        set res 1
                    } else {
                        set res [Combat::SimpleTypeRepository::_is_a $type_id $type]
                    }
                    if {$response_expected} {
                        $conn send_reply $request_id 0 boolean $res \
                                0 "" ""
                    }
                    itcl::delete object $decoder
                    return
                } elseif {$operation == "_non_existent"} {
                    if {$response_expected} {
                        $conn send_reply $request_id 0 boolean 0 \
                                0 "" ""
                    }
                    itcl::delete object $decoder
                    return
                } elseif {$operation == "_interface"} {
                    corba::try {
                        set ifr [corba::resolve_initial_references \
                                InterfaceRepository]
                    } catch {...} {
                        set ifr 0
                    }

                    if {$ifr == 0} {
                        set ifrdef 0
                    } else {
                        corba::try {
                            set ifrobj [::Combat::CORBA::ORB::GetObjFromRef $ifr]
                            set ifrdef [::Combat::CORBA::ORB::invoke_sync \
                                    1 $ifrobj lookup_id Object 0 \
                                    {{in string}} [list $type_id]]
                        } catch {...} {
                            set ifrdef 0
                        }
                    }

                    if {$response_expected} {
                        if {$ifrdef == 0} {
                            $conn send_reply $request_id 2 dummy \
                                    [list IDL:omg.org/CORBA/INTF_REPOS:1.0 \
                                    [list minor 0 completion_status COMPLETED_NO]] \
                                    0 "" ""
                        } else {
                            $conn send_reply $request_id 0 Object $ifrdef \
                                    0 "" ""
                        }
                    }

                    if {$ifr != 0} {
                        corba::release $ifr
                    }

                    if {$ifrdef != 0} {
                        corba::release $ifrdef
                    }

                    itcl::delete object $decoder
                    return
                }

                #
                # Handle Reflection::IFRProvider interface
                #

                if {$operation == "omg_get_ifr_metadata"} {
                    set value [::Combat::SimpleTypeRepository::omgGetIfrMetadata $type_id]
                    if {[catch {
                        set value [::Combat::SimpleTypeRepository::omgGetIfrMetadata $type_id]
                    }]} {
                        if {$response_expected} {
                            $conn send_reply $request_id 2 dummy \
                                    [list IDL:omg.org/CORBA/INTF_REPOS:1.0 \
                                    [list minor 0 completion_status COMPLETED_NO]] \
                                    0 "" ""
                        }
                        itcl::delete object $decoder
                        return
                    }
                    if {$response_expected} {
                        $conn send_reply $request_id 0 "any" $value 0 "" ""
                    }
                    corba::release "any" $value 
                    itcl::delete object $decoder
                    return
                } elseif {$operation == "omg_get_xml_metadata"} {
                    if {$response_expected} {
                        $conn send_reply $request_id 2 dummy \
                                [list IDL:omg.org/Reflection/FormatNotSupported:1.0 \
                                [list minor 0 completion_status COMPLETED_NO]] \
                                0 "" ""
                    }
                    itcl::delete object $decoder
                    return
                }

                $::Combat::PortableServer::theCurrent set_state \
                        $this $oid $servant

                #
                # Is it an attribute?
                #

                if {[string match _get_* $operation] || \
                        [string match _set_* $operation]} {
                    set attribute [string range $operation 5 end]
                    set atInfo [::Combat::SimpleTypeRepository::getAttr \
                            $type_id $attribute]

                    if {$atInfo == ""} {
                        if {$::Combat::debug(poa)} {
                            puts stderr "POA:  $fqn type $type_id: no such attribute: $attribute"
                        }
                        if {$response_expected} {
                            $conn send_reply $request_id 2 dummy \
                                    [list IDL:omg.org/CORBA/BAD_OPERATION:1.0 \
                                    [list minor 0 completion_status COMPLETED_NO]] \
                                    0 "" ""
                        }
                        $::Combat::PortableServer::theCurrent unset_state
                        itcl::delete object $decoder
                        return
                    }

                    set attype [lindex $atInfo 9]

                    #
                    # Handle "get" attribute
                    #

                    if {[string match _get_* $operation]} {
                        set value [$servant cget -$attribute]
                        if {$response_expected} {
                            $conn send_reply $request_id 0 $attype $value \
                                    0 "" ""
                        }
                        corba::release $attype $value
                    } else {
                        #
                        # Handle "set" attribute
                        #

                        if {[catch {
                            set value [$decoder Demarshal $attype]
                        } err]} {
                            if {$::Combat::debug(poa)} {
                                puts stderr "POA:  $fqn demarshalling error: $err"
                            }
                            if {$response_expected} {
                                $conn send_reply $request_id 2 dummy \
                                        [list IDL:omg.org/CORBA/MARSHAL:1.0 \
                                        [list minor 0 completion_status COMPLETED_NO]] \
                                        0 "" ""
                            }
                            $::Combat::PortableServer::theCurrent unset_state
                            itcl::delete object $decoder
                            return
                        }

                        $servant configure -$attribute $value
                        
                        if {$response_expected} {
                            $conn send_reply $request_id 0 void "" \
                                    0 "" ""
                        }

                        corba::release $attype $value
                    }
                } else {
                    #
                    # Handle operation
                    #

                    set opInfo [::Combat::SimpleTypeRepository::getOp \
                            $type_id $operation]
                    
                    if {$opInfo == ""} {
                        if {$::Combat::debug(poa)} {
                            puts stderr "POA:  $fqn type $type_id: no such operation: $operation"
                        }
                        if {$response_expected} {
                            $conn send_reply $request_id 2 dummy \
                                    [list IDL:omg.org/CORBA/BAD_OPERATION:1.0 \
                                    [list minor 0 completion_status COMPLETED_NO]] \
                                    0 "" ""
                        }
                        $::Combat::PortableServer::theCurrent unset_state
                        itcl::delete object $decoder
                        return
                    }
                    
                    set rtype [lindex $opInfo 9]
                    set params [lindex $opInfo 15]
                    set exceptions [lindex $opInfo 17]

                    set values [list]

                    if {[catch {
                        for {set idx 0} {$idx < [llength $params]} {incr idx} {
                            set param [lindex $params $idx]
                            set pname [format "_param%03d" $idx]

                            if {[llength $param] == 8} {
                                #
                                # It's a ParameterDescription
                                #
                                set dir [lindex $param 7]
                                set type [lindex $param 3]
                            } else {
                                #
                                # It's a simple list with the direction and type
                                #
                                set dir  [lindex $param 0]
                                set type [lindex $param 1]
                            }
                        
                            switch -- $dir {
                                PARAM_IN -
                                in {
                                    lappend values [$decoder Demarshal $type]
                                }
                                PARAM_INOUT -
                                inout {
                                    set $pname [$decoder Demarshal $type]
                                    lappend values $pname
                                }
                                PARAM_OUT -
                                out {
                                    lappend values $pname
                                }
                            }
                        }
                    } err]} {
                        if {$::Combat::debug(poa)} {
                            puts stderr "POA:  $fqn demarshalling error: $err"
                        }
                        if {$response_expected} {
                            $conn send_reply $request_id 2 dummy \
                                    [list IDL:omg.org/CORBA/MARSHAL:1.0 \
                                    [list minor 0 completion_status COMPLETED_NO]] \
                                    0 "" ""
                        }
                        $::Combat::PortableServer::theCurrent unset_state
                        itcl::delete object $decoder
                        return
                    }
                    
                    if {[catch {
                        set result [eval $servant $operation $values]
                    } error]} {
                        if {[catch {set exid [lindex $error 0]}]} {
                            set exid "(oops)"
                        }
                        set found ""
                        
                        foreach exception $exceptions {
                            if {[llength $exception] == 10} {
                                #
                                # It's an ExceptionDescription
                                #
                                set exrid [lindex $exception 3]
                                set extype [lindex $exception 9]
                            } else {
                                #
                                # It's an exception typecode
                                #
                                set exrid [lindex $exception 1]
                                set extype $exception
                            }
                            if {$exid == $exrid} {
                                set found $extype
                            }
                        }
                        
                        if {$::Combat::debug(poa)} {
                            if {$found != ""} {
                                puts stderr "POA:  $fqn operation $operation throws $exid"
                            } else {
                                global errorInfo
                                puts stderr "POA:  $fqn operation $operation: unexpected exception: $error"
                                puts stderr "$errorInfo"
                            }
                        }
                        
                        if {$response_expected} {
                            if {$found != ""} {
                                $conn send_reply $request_id 1 $found $error \
                                        0 "" ""
                            } else {
                                $conn send_reply $request_id 2 dummy \
                                        [list IDL:omg.org/CORBA/UNKNOWN:1.0 \
                                        [list minor 0 completion_status COMPLETED_MAYBE]] \
                                        0 "" ""
                            }
                        }
                    } elseif {$response_expected} {
                        $conn send_reply $request_id 0 $rtype $result \
                                0 $params $values

                        corba::release $rtype $result
                    }
                    
                    for {set idx 0} {$idx < [llength $params]} {incr idx} {
                        set param [lindex $params $idx]
                        set value [lindex $values $idx]

                        if {[llength $param] == 8} {
                            #
                            # It's a ParameterDescription
                            #
                            set dir [lindex $param 7]
                            set type [lindex $param 3]
                        } else {
                            #
                            # It's a simple list with the direction and type
                            #
                            set dir  [lindex $param 0]
                            set type [lindex $param 1]
                        }

                        switch -- $dir {
                            PARAM_IN -
                            in {
                                corba::release $type $value
                            }
                            PARAM_INOUT -
                            inout -
                            PARAM_OUT -
                            out {
                                corba::release $type [set $value]
                                unset $value
                            }
                        }
                    }
                }

                $::Combat::PortableServer::theCurrent unset_state

                if {[info exists cookie]} {
                    $servant_manager postinvoke $oid $this $operation \
                            $cookie $servant
                }

                itcl::delete object $decoder
                return
            }

            public method local_locate {conn request_id object_key} {
                switch -- $state {
                    HOLDING {
                        if {$::Combat::debug(poa)} {
                            puts stderr "POA:  $fqn HOLDING locate request on $object_key"
                        }
                        lappend holding_invocations [list local_locate \
                                $conn $request_id $object_key]
                        return
                    }
                    DISCARDING {
                        if {$::Combat::debug(poa)} {
                            puts stderr "POA:  $fqn DISCARDING locate request on $object_key"
                        }
                        $conn send_locate_reply $request_id 4 \
                                [list IDL:omg.org/CORBA/TRANSIENT:1.0 \
                                [list minor 0 completion_status COMPLETED_NO]]
                        return
                    }
                    INACTIVE {
                        if {$::Combat::debug(poa)} {
                            puts stderr "POA:  $fqn INACTIVE locate request on $object_key"
                        }
                        if {$lifespan_policy == "TRANSIENT"} {
                            $conn send_locate_reply $request_id 4 \
                                    [list IDL:omg.org/CORBA/OBJECT_NOT_EXIST:1.0 \
                                    [list minor 0 completion_status COMPLETED_NO]]
                        } else {
                            $conn send_locate_reply $request_id 4 \
                                        [list IDL:omg.org/CORBA/TRANSIENT:1.0 \
                                        [list minor 0 completion_status COMPLETED_NO]]
                        }
                        return
                    }
                    ACTIVE {
                        # ok
                    }
                    default {
                        error "oops"
                    }
                }

                #
                # determine object id for invocation
                #

                if {$object_key == $oaid} {
                    set oid $object_key
                } else {
                    set oid [string range $object_key [expr {[string length $oaid] + 1}] end]
                }

                #
                # Try to find a servant
                #

                #
                # check the active object map
                #

                if {$servant_retention_policy == "RETAIN"} {
                    if {[info exists active_object_map($oid)]} {
                        $conn send_locate_reply $request_id 1 dummy
                        return
                    }
                }

                #
                # if use_default_servant or servant manager, that's fine
                #

                if {$request_processing_policy == "USE_DEFAULT_SERVANT" || \
                        $request_processing_policy == "USE_SERVANT_MANAGER"} {
                    $conn send_locate_reply $request_id 1 dummy
                    return
                }

                #
                # no servant
                #

                $conn send_locate_reply $request_id 0 dummy
            }
        }

        itcl::class RootPOA {
            inherit ::Combat::PortableServer::POA

            constructor {} {
                ::Combat::PortableServer::POA::constructor RootPOA 0 \
                        {IMPLICIT_ACTIVATION} 0
            } {
                set unique_prefix "/[clock seconds]/[pid]"
                set oaid $unique_prefix
                set all_poas($oaid) $this
                set ::Combat::PortableServer::theRootPOA $this
            }

            public proc invoke {response_expected \
                    conn request_id object_key \
                    operation decoder contexts} {
                set idx [string first * $object_key]

                if {$idx == -1} {
                    set theoaid $object_key
                } else {
                    incr idx -1
                    set theoaid [string range $object_key 0 $idx]
                }

                if {![info exists all_poas($theoaid)]} {
                    if {$::Combat::debug(poa)} {
                        puts stderr "POA:  no POA (oaid $theoaid) for object key $object_key (operation $operation)"
                    }
                    if {$response_expected} {
                        $conn send_reply $request_id 2 dummy \
                                [list IDL:omg.org/CORBA/OBJECT_NOT_EXIST:1.0 \
                                [list minor 0 completion_status COMPLETED_NO]] \
                                0 "" ""
                    }
                    itcl::delete object $decoder
                    return
                }

                set poa $all_poas($theoaid)

                return [$poa local_invoke \
                        $response_expected \
                        $conn $request_id \
                        $object_key $operation \
                        $decoder $contexts]
            }

            public proc locate {conn request_id object_key} {
                set idx [string first * $object_key]

                if {$idx == -1} {
                    set theoaid $object_key
                } else {
                    incr idx -1
                    set theoaid [string range $object_key 0 $idx]
                }

                if {![info exists all_poas($theoaid)]} {
                    if {$::Combat::debug(poa)} {
                        puts stderr "POA:  no POA (oaid $theoaid) for object key $object_key (locate request)"
                    }

                    $conn send_locate_reply $request_id 4 \
                            [list IDL:omg.org/CORBA/OBJECT_NOT_EXIST:1.0 \
                            [list minor 0 completion_status COMPLETED_NO]] \
                            0 "" ""

                    return
                }

                set poa $all_poas($theoaid)

                return [$poa local_locate $conn $request_id $object_key]
            }
        }

        proc init {} {
            if {$::Combat::PortableServer::initialized} {
                return
            }
            set ::Combat::PortableServer::initialized 1
            set ::Combat::PortableServer::theRootPOA [namespace current]::[::Combat::PortableServer::RootPOA \#auto]
            set ::Combat::PortableServer::theCurrent [namespace current]::[::Combat::PortableServer::POACurrent \#auto]
            set ::Combat::CORBA::ORB::initial_pseudo_references(RootPOA) \
                    $::Combat::PortableServer::theRootPOA
            set ::Combat::CORBA::ORB::initial_pseudo_references(POACurrent) \
                    $::Combat::PortableServer::theCurrent
        }
    }
}

#
# Base classes
#

namespace eval PortableServer {
    itcl::class AdapterActivator {}
    itcl::class ServantManager {}
    itcl::class ServantLocator {
        inherit ::PortableServer::ServantManager
    }
    itcl::class ServantActivator {
        inherit ::PortableServer::ServantManager
    }
    itcl::class ServantBase {
        public method _default_POA {} {
            return $::Combat::PortableServer::theRootPOA
        }

        public method _this {} {
            if {![catch {
                set current [corba::resolve_initial_references POACurrent]
                set poa [$current get_POA]
                set servant [$current get_servant]
            }]} {
                if {$servant == $this} {
                    return [$current get_reference]
                }
            }
            return [[_default_POA] servant_to_reference $this]
        }
    }
}

