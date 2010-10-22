
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
# Application Factory
# ----------------------------------------------------------------------
#
# This file implements an Application Factory substitute that can be
# used in the absence of a Core Framework:
#
#   cfutil::ApplicationFactory \#auto eds sadFileName {verbose 0}
#
#   Input:
#     eds: List of Executable Device object references.
#     sadFileName: The name of an SAD file in the local file system.
#
# The ApplicationFactory class has a single method:
#
#   ApplicationFactory::create name {initConfiguration {}} {deviceAssignments {}}
#
#   Input:
#     name: The application name.
#     initConfiguration: CF::Properties to configure the application.
#     deviceAssignments: CF::DeviceAssignmentSequence.
#   Returns:
#     If successful, this returns an object of type cfutil::Application,
#     which implements most of the CF::Application API.
#
# The ApplicationFactory has certain limitations:
#
# - Device assignments are solely based on the OCPIContainerType, OCPIDeviceId,
#   OCPIDeviceType, os_name and processor_name attributes.
# - Other capacities are not supported (would require knowledge of the devices'
#   SPD files, which are not (readily) available without a Core Framwork that
#   provides a unified file system).
# - Connections of type "devicethatloadedthiscomponentref" or "deviceusedby-
#   thiscomponentref" are not supported, for the same reason.
# - Connections of type "domainfinder" are not supported.  (They can't be
#   supported but could be emulated.)
# - Connections of type "findby" are not supported yet.
#
# Two helper functions are also provided:
#
#   cfutil::findExecutableDevicesInNamingContext nc
#
#     Given the naming context reference nc, returns a list of all
#     ExecutableDevices that are bound to this context.
#
#   cfutil::findExecutableDevicesInDomainManager dm
#
#     Given the domain manager dm, returns a list of all Executable
#     Devices that are registered in this domain.
#
# ----------------------------------------------------------------------
#

namespace eval cfutil {}

catch {itcl::delete class cfutil::ComponentInstanceFactory}

itcl::class cfutil::ComponentInstanceFactory {
    public variable m_factory                ;# ApplicationFactory instance
    public variable m_device                ;# ExecutableDevice
    public variable m_deviceName
    public variable m_instantiationId
    public variable m_executableInfo        ;# list of {codeFileName type entrypoint}
    public variable m_execParams        ;# CF::Properties
    public variable m_configParams        ;# CF::Properties
    public variable m_worker
    public variable m_pid
    public variable m_verbose

    constructor {factory device instantiationId executableInfo execParams configParams {verbose 0}} {
        set m_factory $factory
        set m_device $device
        set m_deviceName [$device label]
        set m_instantiationId $instantiationId
        set m_executableInfo $executableInfo
        set m_execParams $execParams
        set m_configParams $configParams
        set m_verbose $verbose

        if {$verbose > 2} {
            puts "ComponentInstanceFactory:"
            puts "  Instantiation Id: $m_instantiationId"
            puts "            Device: $m_deviceName"
            puts "   Executable Info: $m_executableInfo"
            puts "       Exec Params: $m_execParams"
            puts "     Config Params: $m_configParams"
        }
    }

    public method getComponent {} {
        return $m_worker
    }

    public method load {} {
        foreach fileInfo $m_executableInfo {
            set fileName [lindex $fileInfo 0]
            set type [lindex $fileInfo 1]

            switch -- $type {
                Executable {
                    set kind "EXECUTABLE"
                }
                SharedLibrary {
                    set kind "SHARED_LIBRARY"
                }
                KernelModule {
                    set kind "KERNEL_MODULE"
                }
                Driver {
                    set kind "DRIVER"
                }
                default {
                    error "invalid code type \"$spdtype\""
                }
            }

            if {$m_verbose} {
                puts -nonewline "Loading $type \"$fileName\" to device [$m_device label] ... "
                flush stdout
            }

            cfutil::loadWorker $m_device $fileName $kind

            if {$m_verbose} {
                puts "done."
            }
        }
    }

    public method execute {} {
        #
        # The last m_executableInfo element ought to have an entrypoint.
        #

        foreach fileInfo $m_executableInfo {}
        set entrypoint [lindex $fileInfo 2]

        if {$m_verbose} {
            puts -nonewline "Executing \"$entrypoint\" ... "
            flush stdout
        }

        #
        # cfutil::executeWorker wants a simple name/value list.
        #

        set execParams [list]

        foreach epv $m_execParams {
            array set ep $epv
            lappend execParams $ep(id) [lindex $ep(value) 1]
        }

        set m_worker [cfutil::executeWorker $m_device $entrypoint $m_instantiationId $execParams m_pid]
        $m_worker initialize

        if {[llength $m_configParams]} {
            $m_worker configure $m_configParams
        }

        if {$m_verbose} {
            puts "(pid $m_pid) ... done."
        }
    }

    public method terminate {} {
        corba::release $m_worker

        if {$m_verbose} {
            puts -nonewline "Terminating \"$m_pid\" ... "
            flush stdout
        }

        $m_device terminate $m_pid

        if {$m_verbose} {
            puts "done."
        }
    }

    public method unload {} {
        #
        # Unload files in reverse order.
        #

        set numFiles [llength $m_executableInfo]

        while {$numFiles > 0} {
            incr numFiles -1
            set fileInfo [lindex $m_executableInfo $numFiles]
            set fileName [lindex $fileInfo 0]
            set fileTail [file tail $fileName]

            if {$m_verbose} {
                puts -nonewline "Unloading \"$fileTail\" ... "
                flush stdout
            }

            $m_device unload $fileTail

            if {$m_verbose} {
                puts "done."
            }
        }
    }
}

catch {itcl::delete class cfutil::Application}

itcl::class cfutil::Application {
    public variable profile
    public variable name
    public variable m_factory
    public variable m_instances
    public variable m_connections
    public variable m_components
    public variable m_assemblyController
    public variable m_externalPorts

    constructor {factory theName instances connections controllerId externalPorts} {
        set profile [$factory cget -softwareProfile]
        set name $theName
        set m_assemblyController 0
        set m_factory $factory
        set m_connections $connections
        array set m_instances $instances
        array set m_externalPorts $externalPorts

        foreach instance [array names m_instances] {
            set af $m_instances($instance)
            set m_components($instance) [$af getComponent]

            if {$instance eq $controllerId} {
                set m_assemblyController $m_components($instance)
            }
        }
    }

    destructor {
        foreach instance [array names m_instances] {
            itcl::delete object $m_instances($instance)
        }
    }

    public method configure {props} {
        $m_assemblyController configure $props
    }

    public method query {propsName} {
        upvar 1 $propsName props
        $m_assemblyController query props
    }

    public method start {} {
        $m_assemblyController start
    }

    public method stop {} {
        $m_assemblyController stop
    }

    public method getPort {portName} {
        if {![info exists m_externalPorts($portName)]} {
            error "unknown port"
        }

        foreach {type componentId} $m_externalPorts($portName) {}
        set component $m_components($componentId)

        switch -- $type {
            "uses" -
            "provides" {
                set port [$component getPort $portName]
            }
            "supports" {
                set port [corba::duplicate $component]
            }
            default {
                error "oops: $type"
            }
        }

        return $port
    }

    public method releaseObject {} {
        set oopses [list]

        foreach conn $m_connections {
            set usesPortObj [lindex $conn 0]
            set connectionId [lindex $conn 1]
            catch {$usesPortObj disconnectPort $connectionId}
            corba::release $usesPortObj
        }

        foreach workerId [array names m_components] {
            set worker $m_components($workerId)
            if {[catch {$worker releaseObject} oops]} {
                lappend oopses $oops
            }
        }

        foreach instance [array names m_instances] {
            set af $m_instances($instance)
            if {[catch {$af terminate} oops]} {
                lappend oopses $oops
            }
            if {[catch {$af unload} oops]} {
                lappend oopses $oops
            }
        }

        itcl::delete object $this

        if {[llength $oopses]} {
            error $oopses
        }
    }
}

catch {itcl::delete class cfutil::ApplicationFactory}

itcl::class cfutil::ApplicationFactory {
    public variable name
    public variable identifier
    public variable softwareProfile

    public variable m_executableDevices        ;# array id -> info
    public variable m_sad                ;# DOM
    public variable m_spd                ;# array file name -> DOM
    public variable m_scd                ;# array file name -> DOM
    public variable m_prf                ;# array file name -> DOM
    public variable m_spdRev                ;# array DOM -> file name
    public variable m_componentFile        ;# array id -> file name
    public variable m_verbose

    public common s_ocpiContainerTypeUUID "DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43"
    public common s_ocpiDeviceIdUUID "DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2"
    public common s_ocpiDeviceTypeUUID "DCE:c788404e-b9f5-4532-8c7d-3588d328fff0"
    public common s_osNameUUID "DCE:80bf17f0-6c7f-11d4-a226-0050da314cd6"
    public common s_processorNameUUID "DCE:9b445600-6c7f-11d4-a226-0050da314cd6"

    constructor {eds sadFileName {verbose 0}} {
        package require tdom

        set m_verbose $verbose
        set m_sad ""

        array set m_executableDevices {}
        array set m_spd {}
        array set m_scd {}
        array set m_prf {}

        if {[catch {
            gatherInfoForExecutableDevices $eds
            loadSoftwareApplicationDescriptor $sadFileName
        } oops]} {
            set savedInfo $::errorInfo
            cleanup
            error $oops $savedInfo
        }
    }

    destructor {
        cleanup
    }

    public method create {name {initConfiguration {}} {inputDeviceAssignments {}}} {
        set sadRoot [$m_sad documentElement]
        set partitioning [getFirstChild $sadRoot "partitioning"]

        foreach ida $inputDeviceAssignments {
            array set inputDeviceAssignment $ida
            set componentId $inputDeviceAssignment(componentId)
            set deviceId $inputDeviceAssignment(assignedDeviceId)
            set deviceAssignment($componentId) $deviceId
        }

        #
        # 1. Find the assembly controller.
        #

        set assemblyController [getFirstChild $sadRoot "assemblycontroller"]
        set componentInstantiationRef [getFirstChild $assemblyController "componentinstantiationref"]
        set assemblyControllerId [$componentInstantiationRef getAttribute "refid"]
        set assemblyControllerConfigParams [list]

        #
        # 2. Find a good home for each component instance.
        #

        foreach componentPlacement [getAllChildren $partitioning "componentplacement"] {
            set componentFileRef [getFirstChild $componentPlacement "componentfileref"]
            set refId [$componentFileRef getAttribute "refid"]

            if {![info exists m_componentFile($refId)]} {
                error "invalid componentfileref \"$refId\""
            }

            set componentFile $m_componentFile($refId)
            set spd [$m_spd($componentFile) documentElement]
            catch {array unset deviceSet}

            #
            # List of {deviceId implementationDOM}
            #

            set compatibleDevices [findCompatibleDevices $spd]

            foreach compatibleDevice $compatibleDevices {
                set deviceSet([lindex $compatibleDevice 0]) [lindex $compatibleDevice 1]
            }

            foreach componentInstantiation [getAllChildren $componentPlacement "componentinstantiation"] {
                set instanceId [$componentInstantiation getAttribute "id"]

                if {[info exists deviceAssignments($instanceId)]} {
                    set assignedDevice $deviceAssignments($instanceId)

                    if {![info exists deviceSet($assignedDevice)]} {
                        error "invalid device assignment for componenent instantiation \"$instanceId\""
                    }
                } else {
                    if {![llength $compatibleDevices]} {
                        error "no compatible devices found for component instantiation \"$instanceId\""
                    }

                    set assignedDevice [lindex [array names deviceSet] 0]
                }

                if {$m_verbose > 1} {
                    puts "Instance \"$instanceId\" assigned to device \"$assignedDevice\""
                }

                set implementation $deviceSet($assignedDevice)
                set execParams [collectExecutableProperties $componentInstantiation $spd $implementation]
                set configParams [collectConfigurableProperties $componentInstantiation $spd $implementation]
                set executableInfo [collectExecutableInfo $componentFile $implementation]

                if {$instanceId eq $assemblyControllerId} {
                    set assemblyControllerConfigParams $configParams
                    set configParams [list]
                }

                array set edInfo $m_executableDevices($assignedDevice)
                set componentIdentifier "$instanceId:$name"
                set instanceFactory($instanceId) [namespace current]::[cfutil::ComponentInstanceFactory \#auto $this $edInfo(ior) $componentIdentifier $executableInfo $execParams $configParams $m_verbose]
            }
        }

        foreach hostCollocation [getAllChildren $partitioning "hostcollocation"] {
            set hostCollocationName [$hostCollocation getAttribute "id" ""]
            set componentPlacements [getAllChildren $hostCollocation "componentplacement"]
            set aggregateCapacities [list]
            catch {array unset aggregateDeviceSet}
            set first 1

            foreach componentPlacement $componentPlacements {
                set componentFileRef [getFirstChild $componentPlacement "componentfileref"]
                set refId [$componentFileRef getAttribute "refid"]

                if {![info exists m_componentFile($refId)]} {
                    error "invalid componentfileref \"$refId\""
                }

                set componentFile $m_componentFile($refId)
                set spd [$m_spd($componentFile) documentElement]

                set compatibleDevices [findCompatibleDevices $spd]

                if {$first} {
                    set first 0
                    foreach compatibleDevice $compatibleDevices {
                        set aggregateDeviceSet([lindex $compatibleDevice 0]) [lindex $compatibleDevice 1]
                    }
                } else {
                    catch {array unset deviceSet}
                    foreach compatibleDevice $compatibleDevices {
                        set deviceSet([lindex $compatibleDevice 0]) [lindex $compatibleDevice 1]
                    }
                    foreach deviceId [array names aggregateDeviceSet] {
                        if {![info exists deviceSet($deviceId)]} {
                            unset aggregateDeviceSet($deviceId)
                        }
                    }
                }

                foreach componentInstantiation [getAllChildren $componentPlacement "componentinstantiation"] {
                    set instanceId [$componentInstantiation getAttribute "id"]

                    if {[info exists deviceAssignments($instanceId)]} {
                        set assignedDevice $deviceAssignments($instanceId)

                        if {![info exists aggregateDeviceSet($assignedDevice)]} {
                            error "invalid device assignment for componenent instantiation \"$instanceId\""
                        }

                        foreach deviceId [array names aggregateDeviceSet] {
                            if {$deviceAssignments(instanceId) ne $deviceId} {
                                unset aggregateDeviceSet($deviceId)
                            }
                        }
                    }
                }
            }

            #
            # We now have a list of devices that are compatible with all
            # instances.  For the moment, we only consider the first one.
            #

            set compatibleDeviceIds [array names aggregateDeviceSet]

            if {![llength $compatibleDeviceIds]} {
                error "no devices found for hostcollocation \"$hostCollocationName\""
            }

            set assignedDevice [lindex $compatibleDeviceIds 0]

            foreach componentPlacement $componentPlacements {
                set componentFileRef [getFirstChild $componentPlacement "componentfileref"]
                set refId [$componentFileRef getAttribute "refid"]
                set componentFile $m_componentFile($refId)
                set spd [$m_spd($componentFile) documentElement]
                catch {array unset deviceSet}
                set compatibleDevices [findCompatibleDevices $spd]

                foreach compatibleDevice $compatibleDevices {
                    set deviceSet([lindex $compatibleDevice 0]) [lindex $compatibleDevice 1]
                }

                foreach componentInstantiation [getAllChildren $componentPlacement "componentinstantiation"] {
                    set instanceId [$componentInstantiation getAttribute "id"]
                    set implementation $deviceSet($assignedDevice)

                    if {$m_verbose > 1} {
                        puts "Instance \"$instanceId\" assigned to device \"$assignedDevice\""
                    }

                    set execParams [collectExecutableProperties $componentInstantiation $spd $implementation]
                    set configParams [collectConfigurableProperties $componentInstantiation $spd $implementation]
                    set executableInfo [collectExecutableInfo $componentFile $implementation]

                    if {$instanceId eq $assemblyControllerId} {
                        set assemblyControllerConfigParams $configParams
                        set configParams [list]
                    }

                    array set edInfo $m_executableDevices($assignedDevice)
                    set componentIdentifier "$instanceId:$name"
                    set instanceFactory($instanceId) [namespace current]::[cfutil::ComponentInstanceFactory \#auto $this $edInfo(ior) $componentIdentifier $executableInfo $execParams $configParams $m_verbose]
                }
            }
        }

        #
        # 3. Load and execute components.
        #

        set runningComponents [list]

        corba::try {
            foreach instanceId [array names instanceFactory] {
                set factory $instanceFactory($instanceId)
                $factory load

                corba::try {
                    $factory execute
                } catch {... oops} {
                    set savedInfo $::errorInfo
                    catch {$factory unload}
                    error $oops $savedInfo
                }

                lappend runningComponents $factory
                set components($instanceId) [$factory getComponent]
            }
        } catch {... oops} {
            set savedInfo $::errorInfo

            foreach factory $runningComponents {
                catch {$factory terminate}
                catch {$factory unload}
            }

            foreach instanceId [array names instanceFactory] {
                set factory $instanceFactory($instanceId)
                itcl::delete object $factory
            }

            error $oops $savedInfo
        }

        set establishedConnections [list]

        corba::try {
            #
            # 4. Make connections.
            #

            if {[hasChild $sadRoot "connections"]} {
                set connections [getFirstChild $sadRoot "connections"]

                foreach connectInterface [getAllChildren $connections "connectinterface"] {
                    set connectionId [$connectInterface getAttribute "id" "default-connection-id"]

                    #
                    # Get uses port.
                    #

                    set usesPort [getFirstChild $connectInterface "usesport"]
                    set usesIdentifier [getFirstChild $usesPort "usesidentifier"]
                    set usesPortName [getTextContent $usesIdentifier]

                    if {[hasChild $usesPort "componentinstantiationref"]} {
                        set componentInstantiationRef [getFirstChild $usesPort "componentinstantiationref"]
                        set usesPortComponentId [$componentInstantiationRef getAttribute "refid"]
                        set usesPortComponent $components($usesPortComponentId)
                        set usesPortObj [$usesPortComponent getPort $usesPortName]
                    } else {
                        error "empty or unsupported usesport element"
                    }

                    #
                    # Get provider port.
                    #

                    if {[hasChild $connectInterface "providesport"]} {
                        set providesPort [getFirstChild $connectInterface "providesport"]
                        set providesIdentifier [getFirstChild $providesPort "providesidentifier"]
                        set providesPortName [getTextContent $providesIdentifier]

                        if {[hasChild $providesPort "componentinstantiationref"]} {
                            set providesType "component"
                            set componentInstantiationRef [getFirstChild $providesPort "componentinstantiationref"]
                            set providesPortComponentId [$componentInstantiationRef getAttribute "refid"]
                            set providesPortComponent $components($providesPortComponentId)
                            set providesPortObj [$providesPortComponent getPort $providesPortName]
                        } else {
                            error "empty or unsupported providesport element"
                        }
                    } elseif {[hasChild $connectInterface "componentsupportedinterface"]} {
                        set componentSupportedInterface [getFirstChild $connectInterface "componentsupportedinterface"]
                        set providesPortName "<componentsupportedinterface>"
                        set providesType "component"

                        if {[hasChild $componentSupportedInterface "componentinstantiationref"]} {
                            set componentInstantiationRef [getFirstChild $componentSupportedInterface "componentinstantiationref"]
                            set providesPortComponentId [$componentInstantiationRef getAttribute "refid"]
                            set providesPortObj [corba::duplicate $components($providesPortComponentId)]
                        } else {
                            error "empty or unsupported componentsupportedinterface element"
                        }
                    } else {
                        error "missing or unsupported provider port"
                    }

                    #
                    # Connect the two.
                    #

                    if {$m_verbose} {
                        puts -nonewline "Connecting component \"$usesPortComponentId\" uses port \"$usesPortName\" "
                        puts -nonewline "to $providesType \"$providesPortComponentId\" port \"$providesPortName\" ... "
                        flush stdout
                    }

                    $usesPortObj connectPort $providesPortObj $connectionId

                    if {$m_verbose} {
                        puts "done."
                    }

                    lappend establishedConnections [list $usesPortObj $connectionId]
                    corba::release $providesPortObj
                }
            }

            #
            # 5. Configure the assembly controller.
            #

            array set acConfig {}

            foreach prop $assemblyControllerConfigParams {
                array set cpInfo $prop
                set acConfig($cpInfo(id)) $cpInfo(value)
            }

            foreach prop $initConfiguration {
                array set cpInfo $prop
                set acConfig($cpInfo(id)) $cpInfo(value)
            }

            set acConfigs [list]

            foreach propName [array names acConfig] {
                lappend acConfigs [list id $propName value $acConfig($propName)]
            }

            if {[llength $acConfigs]} {
                set assemblyController $components($assemblyControllerId)

                if {$m_verbose} {
                    puts -nonewline "Configuring the assembly controller ... "
                    flush stdout
                }

                if {$m_verbose > 1} {
                    puts ""
                    foreach propName [array names acConfig] {
                        puts "  $propName = \"[lindex $acConfig($propName) 1]\""
                    }
                }

                $assemblyController configure $acConfigs

                if {$m_verbose} {
                    puts "done."
                }
            }

            #
            # 6. Collect external port information.
            #

            set externalPortInfo [list]

            if {[hasChild $sadRoot "externalports"]} {
                set externalPorts [getFirstChild $sadRoot "externalports"]

                foreach port [getAllChildren $externalPorts "port"] {
                    set componentInstantiationRef [getFirstChild $port "componentinstantiationref"]
                    set componentId [$componentInstantiationRef getAttribute "refid"]

                    if {[hasChild $port "usesidentifier"]} {
                        set usesIdentifier [getFirstChild $port "usesidentifier"]
                        set portType "uses"
                        set portName [getTextContent $usesIdentifier]
                    } elseif {[hasChild $port "providesidentifier"]} {
                        set providesIdentifier [getFirstChild $port "providesidentifier"]
                        set portType "provides"
                        set portName [getTextContent $providesIdentifier]
                    } elseif {[hasChild $port "supportedidentifier"]} {
                        set supportedIdentifier [getFirstChild $port "supportedidentifier"]
                        set portType "supports"
                        set portName [getTextContent $providesIdentifier]
                    } else {
                        error "empty or unsupported externalports element"
                    }

                    lappend externalPortInfo $portName [list $portType $componentId]
                }
            }
        } catch {... oops} {
            set savedInfo $::errorInfo

            foreach conn $establishedConnections {
                set usesPortObj [lindex $conn 0]
                set connectionId [lindex $conn 1]
                catch {$usesPortObj disconnectPort $connectionId}
                corba::release $usesPortObj
            }

            foreach instanceId [array names instanceFactory] {
                set factory $instanceFactory($instanceId)
                catch {$factory terminate}
                catch {$factory unload}
                itcl::delete object $factory
            }

            error $oops $savedInfo
        }

        #
        # All done!  Now that was easy, wasn't it?
        #

        set instances [array get instanceFactory]
        return [namespace current]::[cfutil::Application \#auto $this $name $instances $establishedConnections $assemblyControllerId $externalPortInfo]
    }

    #
    # Returns a CF::Properties list of the assembly controller's properties.
    # Property values may be missing.
    #

    public method getConfigurableProperties {} {
        set sadRoot [$m_sad documentElement]
        set partitioning [getFirstChild $sadRoot "partitioning"]
        set assemblyController [getFirstChild $sadRoot "assemblycontroller"]
        set componentInstantiationRef [getFirstChild $assemblyController "componentinstantiationref"]
        set assemblyControllerId [$componentInstantiationRef getAttribute "refid"]

        #
        # 1. Find the assembly controller.
        #

        set found 0

        foreach componentPlacement [getAllChildren $partitioning "componentplacement"] {
            set componentFileRef [getFirstChild $componentPlacement "componentfileref"]
            set refId [$componentFileRef getAttribute "refid"]

            if {![info exists m_componentFile($refId)]} {
                error "invalid componentfileref \"$refId\""
            }

            set componentFile $m_componentFile($refId)
            set spd [$m_spd($componentFile) documentElement]

            foreach componentInstantiation [getAllChildren $componentPlacement "componentinstantiation"] {
                set instanceId [$componentInstantiation getAttribute "id"]

                if {$instanceId eq $assemblyControllerId} {
                    set found 1
                    break
                }
            }

            if {$found} {
                break
            }
        }

        if {!$found} {
            foreach hostCollocation [getAllChildren $partitioning "hostcollocation"] {
                set componentPlacements [getAllChildren $hostCollocation "componentplacement"]

                foreach componentPlacement $componentPlacements {
                    set componentFileRef [getFirstChild $componentPlacement "componentfileref"]
                    set refId [$componentFileRef getAttribute "refid"]

                    if {![info exists m_componentFile($refId)]} {
                        error "invalid componentfileref \"$refId\""
                    }

                    set componentFile $m_componentFile($refId)
                    set spd [$m_spd($componentFile) documentElement]

                    foreach componentInstantiation [getAllChildren $componentPlacement "componentinstantiation"] {
                        set instanceId [$componentInstantiation getAttribute "id"]

                        if {$instanceId eq $assemblyControllerId} {
                            set found 1
                            break
                        }
                    }

                    if {$found} {
                        break
                    }
                }

                if {$found} {
                    break
                }
            }
        }

        if {!$found} {
            error "Assembly controller instance not found"
        }

        #
        # At this point, $spd and $componentInstantiation is set.
        #

        return [collectConfigurableProperties $componentInstantiation $spd {} 1]
    }

    public method cleanup {} {
        foreach file [array names m_spd] {
            $m_spd($file) delete
        }

        foreach file [array names m_scd] {
            $m_scd($file) delete
        }

        foreach file [array names m_prf] {
            $m_prf($file) delete
        }

        if {$m_sad ne ""} {
            $m_sad delete
        }

        foreach ed [array names m_executableDevices] {
            array set edInfo $m_executableDevices($ed)
            corba::release $edInfo(ior)
        }
    }

    public method loadSoftwareApplicationDescriptor {sadFileName} {
        set softwareProfile $sadFileName

        if {$m_verbose} {
            puts -nonewline "Parsing SAD file \"[file tail $sadFileName]\" ... "
            flush stdout
        }

        set f [open $softwareProfile]

        if {[catch {set m_sad [dom parse -channel $f]} oops]} {
            set savedInfo $::errorInfo
            close $f
            error $oops $savedInfo
        }

        close $f

        if {$m_verbose} {
            puts "done."
        }

        set sadRoot [$m_sad documentElement]
        set sadType [$sadRoot nodeName]

        if {$sadType ne "softwareassembly"} {
            error "not a Software Assembly Descriptor but a \"$sadType\" file"
        }

        set name [$sadRoot getAttribute "name" ""]
        set identifier [$sadRoot getAttribute "id"]

        set componentFiles [getFirstChild $sadRoot "componentfiles"]
        foreach componentFile [getAllChildren $componentFiles "componentfile"] {
            set componentFileId [$componentFile getAttribute "id"]
            set localFile [getFirstChild $componentFile "localfile"]
            set localFileName [$localFile getAttribute "name"]
            set spdFileName [resolveFileName $softwareProfile $localFileName]
            loadSoftwarePackageDescriptor $spdFileName
            set m_componentFile($componentFileId) $spdFileName
        }
    }

    public method loadSoftwarePackageDescriptor {spdFileName} {
        if {[info exists m_spd($spdFileName)]} {
            return
        }

        if {$m_verbose} {
            puts -nonewline "Parsing SPD file \"[file tail $spdFileName]\" ... "
            flush stdout
        }

        set f [open $spdFileName]

        if {[catch {set spd [dom parse -channel $f]} oops]} {
            set savedInfo $::errorInfo
            close $f
            error $oops $savedInfo
        }

        close $f

        if {$m_verbose} {
            puts "done."
        }

        set spdRoot [$spd documentElement]
        set m_spd($spdFileName) $spd
        set m_spdRev($spdRoot) $spdFileName
        set spdType [$spdRoot nodeName]

        if {$spdType ne "softpkg"} {
            error "\"$spdFileName\" is not a Software Package Descriptor but a \"$spdType\" file"
        }

        #
        # Load the (optional) per-package property file.
        #

        foreach propertyFile [getAllChildren $spdRoot "propertyfile"] {
            set localFile [getFirstChild $propertyFile "localfile"]
            set localFileName [$localFile getAttribute "name"]
            set prfFileName [resolveFileName $spdFileName $localFileName]
            loadPropertyFile $prfFileName
        }

        #
        # Load the (optional) Software Component Descriptor file.
        #

        foreach descriptor [getAllChildren $spdRoot "descriptor"] {
            set localFile [getFirstChild $descriptor "localfile"]
            set localFileName [$localFile getAttribute "name"]
            set scdFileName [resolveFileName $spdFileName $localFileName]
            loadSoftwareComponentDescriptor $scdFileName
        }

        foreach implementation [getAllChildren $spdRoot "implementation"] {
            #
            # Check that the code file exists.
            #

            set code [getFirstChild $implementation "code"]
            set localFile [getFirstChild $code "localfile"]
            set localFileName [$localFile getAttribute "name"]
            set codeFileName [resolveFileName $spdFileName $localFileName]

            if {![file exists $codeFileName]} {
                error "\"$codeFileName\" not found"
            }

            #
            # Load the (optional) implementation-specific property file.
            #

            foreach propertyFile [getAllChildren $implementation "propertyfile"] {
                set localFile [getFirstChild $propertyFile "localfile"]
                set localFileName [$localFile getAttribute "name"]
                set prfFileName [resolveFileName $spdFileName $localFileName]
                loadPropertyFile $prfFileName
            }

            #
            # Load any softpkgref packages.
            #

            foreach dependency [getAllChildren $implementation "dependency"] {
                if {[hasChild $dependency "softpkgref"]} {
                    set softpkgRef [getFirstChild $dependency "softpkgref"]
                    set localFile [getFirstChild $softpkgRef "localfile"]
                    set localFileName [$localFile getAttribute "name"]
                    set otherSpdFileName [resolveFileName $spdFileName $localFileName]
                    loadSoftwarePackageDescriptor $otherSpdFileName
                }
            }
        }
    }

    public method loadSoftwareComponentDescriptor {scdFileName} {
        if {[info exists m_scd($scdFileName)]} {
            return
        }

        if {$m_verbose} {
            puts -nonewline "Parsing SCD file \"[file tail $scdFileName]\" ... "
            flush stdout
        }

        set f [open $scdFileName]

        if {[catch {set scd [dom parse -channel $f]} oops]} {
            set savedInfo $::errorInfo
            close $f
            error $oops $savedInfo
        }

        close $f

        if {$m_verbose} {
            puts "done."
        }

        set m_scd($scdFileName) $scd
        set scdRoot [$scd documentElement]
        set scdType [$scdRoot nodeName]

        if {$scdType ne "softwarecomponent"} {
            error "\"$scdFileName\" is not a Software Component Descriptor but a \"$scdType\" file"
        }

        #
        # Load the (optional) property file.
        #

        foreach propertyFile [getAllChildren $scdRoot "propertyfile"] {
            set localFile [getFirstChild $propertyFile "localfile"]
            set localFileName [$localFile getAttribute "name"]
            set prfFileName [resolveFileName $scdFileName $localFileName]
            loadPropertyFile $prfFileName
        }
    }

    public method loadPropertyFile {prfFileName} {
        if {[info exists m_prf($prfFileName)]} {
            return
        }

        if {$m_verbose} {
            puts -nonewline "Parsing PRF file \"[file tail $prfFileName]\" ... "
            flush stdout
        }

        set f [open $prfFileName]

        if {[catch {set prf [dom parse -channel $f]} oops]} {
            set savedInfo $::errorInfo
            close $f
            error $oops $savedInfo
        }

        close $f

        if {$m_verbose} {
            puts "done."
        }

        set m_prf($prfFileName) $prf
        set prfRoot [$prf documentElement]
        set prfType [$prfRoot nodeName]

        if {$prfType ne "properties"} {
            error "\"$prfFileName\" is not a Properties Descriptor but a \"$prfType\" file"
        }
    }

    #
    # Input: spdFileName, implementation DOM
    # Returns: list of {codeFileName type entrypoint}
    #

    public method collectExecutableInfo {spdFileName implementation} {
        set result [list]

        #
        # Process softpkgref packages first.
        #

        foreach dependency [getAllChildren $implementation "dependency"] {
            if {[hasChild $dependency "softpkgref"]} {
                set softpkgRef [getFirstChild $dependency "softpkgref"]
                set localFile [getFirstChild $softpkgRef "localfile"]
                set localFileName [$localFile getAttribute "name"]
                set otherSpdFileName [resolveFileName $spdFileName $localFileName]
                set otherSpd [$m_spd($otherSpdFileName) documentElement]

                if {[hasChild $softpkgRef "implref"]} {
                    set implref [getFirstChild $softpkgRef "implref"]
                    set implrefId [$implref getAttribute "refid"]
                } else {
                    set implrefId ""
                }

                foreach otherSpdImplementation [getAllChildren $otherSpd "implementation"] {
                    set otherSpdImplId [$otherSpdImplementation getAttribute "id"]

                    if {$implrefId ne "" && $implrefId != $otherSpdImplId} {
                        continue
                    }

                    set otherDependencies [collectExecutableInfo $otherSpdFileName $otherSpdImplementation]

                    foreach otherDependency $otherDependencies {
                        lappend result $otherDependency
                    }
                }
            }
        }

        set code [getFirstChild $implementation "code"]
        set type [$code getAttribute "type" "Executable"]
        set localFile [getFirstChild $code "localfile"]
        set localFileName [$localFile getAttribute "name"]
        set codeFileName [resolveFileName $spdFileName $localFileName]

        if {[hasChild $code "entrypoint"]} {
            set entrypoint [getFirstChild $code "entrypoint"]
            set entryName [getTextContent $entrypoint]
            lappend result [list $codeFileName $type $entryName]
        } elseif {$type eq "Executable"} {
            #
            # Use the file name with "execute"
            #
            lappend result [list $codeFileName $type [file tail $localFileName]]
        } else {
            lappend result [list $codeFileName $type]
        }

        return $result
    }

    #
    # Input: componentinstantiation DOM, SPD DOM, implementation DOM all
    # Returns: CF::Properties
    #
    # componentInstantiation and implementation may be blank to consider
    # the SPD only.
    #
    # If all=0, returns only properties that have values
    # If all=1, returns all properties, value may be empty
    #

    public method collectConfigurableProperties {componentInstantiation spd implementation {all 0}} {
        return [collectProperties $componentInstantiation $spd $implementation "configure" 1 $all]
    }

    public method collectExecutableProperties {componentInstantiation spd implementation {all 0}} {
        return [collectProperties $componentInstantiation $spd $implementation "execparam" 1 $all]
    }

    public method collectProperties {componentInstantiation spd implementation kind writableOnly all} {
        #
        # Process the PRF file that's linked from the SPD file via the SCD file.
        #

        if {[hasChild $spd "descriptor"]} {
            set descriptor [getFirstChild $spd "descriptor"]
            set localFile [getFirstChild $descriptor "localfile"]
            set localFileName [$localFile getAttribute "name"]
            set scdFileName [resolveFileName $m_spdRev($spd) $localFileName]
            set scd [$m_scd($scdFileName) documentElement]

            if {[hasChild $scd "propertyfile"]} {
                set propertyFile [getFirstChild $scd "propertyfile"]
                set localFile [getFirstChild $propertyFile "localfile"]
                set localFileName [$localFile getAttribute "name"]
                set prfFileName [resolveFileName $scdFileName $localFileName]
                set prf [$m_prf($prfFileName) documentElement]

                foreach {name type} [collectPropertyTypes $prf $kind $writableOnly] {
                    set propertyType($name) $type
                }

                foreach {name value} [collectPropertyValues $prf $kind $writableOnly] {
                    set propertyValue($name) $value
                }
            }
        }

        #
        # Process the PRF file that's linked from the SPD's top level.
        #

        if {[hasChild $spd "propertyfile"]} {
            set propertyFile [getFirstChild $spd "propertyfile"]
            set localFile [getFirstChild $propertyFile "localfile"]
            set localFileName [$localFile getAttribute "name"]
            set prfFileName [resolveFileName $m_spdRev($spd) $localFileName]
            set prf [$m_prf($prfFileName) documentElement]

            foreach {name type} [collectPropertyTypes $prf $kind $writableOnly] {
                set propertyType($name) $type
            }

            foreach {name value} [collectPropertyValues $prf $kind $writableOnly] {
                set propertyValue($name) $value
            }
        }

        #
        # Process the PRF file that's linked from the implementation element.
        #

        if {$implementation ne "" && [hasChild $implementation "propertyfile"]} {
            set propertyFile [getFirstChild $spd "propertyfile"]
            set localFile [getFirstChild $propertyFile "localfile"]
            set localFileName [$localFile getAttribute "name"]
            set prfFileName [resolveFileName $m_spdRev($spd) $localFileName]
            set prf [$m_prf($prfFileName) documentElement]

            foreach {name type} [collectPropertyTypes $prf $kind $writableOnly] {
                set propertyType($name) $type
            }

            foreach {name value} [collectPropertyValues $prf $kind $writableOnly] {
                set propertyValue($name) $value
            }
        }

        #
        # Process the properties from the componentinstantiation
        #

        if {$componentInstantiation ne "" && [hasChild $componentInstantiation "componentproperties"]} {
            set componentProperties [getFirstChild $componentInstantiation "componentproperties"]

            foreach simpleref [getAllChildren $componentProperties "simpleref"] {
                set name  [$simpleref getAttribute "refid"]
                set value [$simpleref getAttribute "value"]
                set propertyValue($name) $value
            }

            foreach simplesequenceref [getAllChildren $componentProperties "simplesequenceref"] {
                set name   [$simplesequenceref getAttribute "refid"]
                set values [getFirstChild $simplesequenceref "values"]
                set value [list]
                foreach v [getAllChildren $values "value"] {
                    lappend value [getTextContent $v]
                }
                set propertyValue($name) $value
            }
        }

        #
        # Return a list of the properties that have values.
        #

        set properties [list]

        foreach id [array names propertyType] {
            if {[info exists propertyValue($id)]} {
                set any [list $propertyType($id) $propertyValue($id)]
                lappend properties [list id $id value $any]
            } elseif {$all} {
                lappend properties [list id $id value [list $propertyType($id)]]
            }
        }

        return $properties
    }

    public method collectPropertyTypes {prf requestedKind {writableOnly 0}} {
        set propertyTypes [list]

        foreach simple [getAllChildren $prf "simple"] {
            set id [$simple getAttribute "id"]
            set type [$simple getAttribute "type"]
            set mode [$simple getAttribute "mode" "readwrite"]

            if {$requestedKind eq "configure" && $writableOnly && $mode eq "readonly"} {
                continue
            }

            switch -- $type {
                boolean -
                char -
                double -
                float -
                short -
                long -
                octet -
                string {
                    set corbaType $type
                }
                ulong {
                    set corbaType "unsigned long"
                }
                ushort {
                    set corbaType "unsigned short"
                }
                default {
                    error "unsupported type \"$type\""
                }
            }

            set kindFound 0

            foreach kind [getAllChildren $simple "kind"] {
                set kindtype [$kind getAttribute "kindtype" "configure"]
                if {$kindtype eq $requestedKind} {
                    set kindFound 1
                    break
                }
            }

            if {$kindFound} {
                lappend propertyTypes $id $corbaType
            }
        }

        foreach simplesequence [getAllChildren $prf "simplesequence"] {
            set id [$simple getAttribute "id"]
            set type [$simple getAttribute "type"]
            set mode [$simple getAttribute "mode" "readwrite"]

            if {$requestedKind eq "configure" && $writableOnly && $mode eq "readonly"} {
                continue
            }

            switch -- $type {
                boolean -
                char -
                double -
                float -
                short -
                long -
                octet -
                string {
                    set corbaType $type
                }
                ulong {
                    set corbaType "unsigned long"
                }
                ushort {
                    set corbaType "unsigned short"
                }
                default {
                    error "unsupported type \"$type\""
                }
            }

            set kindFound 0

            foreach kind [getAllChildren $simple "kind"] {
                set kindtype [$kind getAttribute "kindtype" "configure"]
                if {$kindtype eq $requestedKind} {
                    set kindFound 1
                    break
                }
            }

            if {$kindFound} {
                lappend propertyTypes $id [list sequence $corbaType]
            }
        }

        return $propertyTypes
    }

    public method collectPropertyValues {prf requestedKind {writableOnly 0}} {
        set propertyValues [list]

        foreach simple [getAllChildren $prf "simple"] {
            if {![hasChild $simple "value"]} {
                continue
            }

            set id [$simple getAttribute "id"]
            set mode [$simple getAttribute "mode" "readwrite"]

            if {$requestedKind eq "configure" && $writableOnly && $mode eq "readonly"} {
                continue
            }

            set value [getFirstChild $simple "value"]
            set kindFound 0

            foreach kind [getAllChildren $simple "kind"] {
                set kindtype [$kind getAttribute "kindtype" "configure"]
                if {$kindtype eq $requestedKind} {
                    set kindFound 1
                    break
                }
            }

            if {$kindFound} {
                lappend propertyValues $id [getTextContent $value]
            }
        }

        foreach simplesequence [getAllChildren $prf "simplesequence"] {
            if {![hasChild $simple "values"]} {
                continue
            }

            set id [$simple getAttribute "id"]
            set mode [$simple getAttribute "mode" "readwrite"]

            if {$requestedKind eq "configure" && $writableOnly && $mode eq "readonly"} {
                continue
            }

            set values [getFirstChild $simple "values"]
            set kindFound 0

            foreach kind [getAllChildren $simple "kind"] {
                set kindtype [$kind getAttribute "kindtype" "configure"]
                if {$kindtype eq $requestedKind} {
                    set kindFound 1
                    break
                }
            }

            if {!$kindFound} {
                continue
            }

            set value [list]

            foreach v [getAllChildren $values "value"] {
                lappend value [getTextContent $v]
            }

            lappend propertyValues $id [list sequence $type]
        }

        return $propertyValues
    }

    #
    # Input: SPD DOM.  Returns list of {deviceId implementationDOM}.
    #

    public method findCompatibleDevices {spd} {
        set compatibleDevices [list]
        set id [$spd getAttribute "id"]

        if {$m_verbose > 2} {
            puts "  Finding compatible devices for package \"$id\""
        }

        #
        # Note: this only works for the capacities that we have queried
        # earlier.
        #

        foreach implementation [getAllChildren $spd "implementation"] {
            set implId [$implementation getAttribute "id"]
            catch {array unset requiredCapacity}

            if {$m_verbose > 3} {
                puts "    Finding compatible devices for implementation \"$implId\""
            }

            foreach os [getAllChildren $implementation "os"] {
                set os_name [$os getAttribute "name"]
                set requiredCapacity($s_osNameUUID) $os_name
            }

            foreach processor [getAllChildren $implementation "processor"] {
                set processor_name [$processor getAttribute "name"]
                set requiredCapacity($s_processorNameUUID) $processor_name
            }

            foreach dependency [getAllChildren $implementation "dependency"] {
                foreach propertyref [getAllChildren $dependency "propertyref"] {
                    set refid [$propertyref getAttribute "refid"]
                    set value [$propertyref getAttribute "value"]
                    set requiredCapacity($refid) $value
                }
            }

            if {$m_verbose > 3} {
                puts "      Implementation \"$implId\" requires the following capacities:"

                foreach c [array names requiredCapacity] {
                    puts "        $c = \"$requiredCapacity($c)\""
                }
            }

            catch {array unset edInfo}

            foreach deviceId [array names m_executableDevices] {
                catch {array unset deviceCapacity}
                array set deviceCapacity $m_executableDevices($deviceId)
                set isCompatible 1

                foreach capacity [array names requiredCapacity] {
                    if {![info exists deviceCapacity($capacity)]} {
                        if {$m_verbose > 3} {
                            puts "      Device \"$deviceId\" does not have the $capacity capacity."
                        }

                        set isCompatible 0
                        break
                    }

                    if {$deviceCapacity($capacity) != $requiredCapacity($capacity)} {
                        if {$m_verbose > 3} {
                            puts "      Device \"$deviceCapacity(label)\" does not match the $capacity capacity ($deviceCapacity($capacity) != $requiredCapacity($capacity))"
                        }

                        set isCompatible 0
                        break
                    }
                }

                #
                # Ensure that a non-OCPI component is not assigned to a OCPI
                # device, except to a GPP device.
                #

                if {$isCompatible && \
                        ![info exists requiredCapacity($s_ocpiContainerTypeUUID)] && \
                        [info exists deviceCapacity($s_ocpiContainerTypeUUID)] && \
                        $deviceCapacity($s_ocpiContainerTypeUUID) != "GPP"} {
                    if {$m_verbose > 3} {
                        puts "      OCPI device \"$deviceCapacity(label)\" is not compatible with non-OCPI component."
                    }

                    set isCompatible 0
                }

                if {$isCompatible} {
                    if {$m_verbose > 3} {
                        puts "      Implementation \"$implId\" is compatible with device \"$deviceCapacity(label)\""
                    }

                    lappend compatibleDevices [list $deviceId $implementation]
                }
            }
        }

        #
        # If we haven't found any compatible devices yet, then assign a
        # non-OCPI component to any non-OCPI executable device, just in
        # chance.
        #
        # The problem is that a non-OCPI component still advertises its
        # os and processor dependencies, but non-OCPI executable devices
        # don't advertise their capabilities by way of properties.
        #

        if {![llength $compatibleDevices]} {
            puts "Checking again."
            foreach implementation [getAllChildren $spd "implementation"] {
                set implId [$implementation getAttribute "id"]
                set isOcpiComponent 0
                puts "Checking implementation $implId again."

                foreach dependency [getAllChildren $implementation "dependency"] {
                    foreach propertyref [getAllChildren $dependency "propertyref"] {
                        set refid [$propertyref getAttribute "refid"]
                        if {$refid eq $s_ocpiContainerTypeUUID} {
                            set isOcpiComponent 1
                            break
                        }
                    }
                    if {$isOcpiComponent} {
                        break
                    }
                }

                puts "Checking implementation $implId again: $isOcpiComponent."

                if {!$isOcpiComponent} {
                    set isOcpiDevice 0

                    foreach deviceId [array names m_executableDevices] {
                        catch {array unset deviceCapacity}
                        array set deviceCapacity $m_executableDevices($deviceId)
                        puts "Checking implementation $implId against device $deviceCapacity(label)."

                        if {[info exists deviceCapacity($s_ocpiContainerTypeUUID)]} {
                            puts "Device $deviceCapacity(label) is a OCPI device."
                            set isOcpiDevice 1
                        } else {
                            set isOcpiDevice 0
                        }

                        if {!$isOcpiDevice} {
                            if {$m_verbose > 3} {
                                puts "      Non-OCPI implementation \"$implId\" assumed compatible with non-OCPI device \"$deviceCapacity(label)\""
                            }

                            lappend compatibleDevices [list $deviceId $implementation]
                        }
                    }
                }
            }

        }

        if {$m_verbose > 2} {
            puts -nonewline "  Package \"$id\" is compatible with devices:"
            foreach compatibleDevice $compatibleDevices {
                puts -nonewline " "
                puts -nonewline [lindex $compatibleDevice 0]
            }
            puts "."
        }

        return $compatibleDevices
    }

    public method gatherInfoForExecutableDevices {eds} {
        foreach ed $eds {
            corba::try {
                set identifier [$ed identifier]
                set info [getInfoForExecutableDevice $ed]
                lappend info "ior" [corba::duplicate $ed]

                if {$m_verbose} {
                    catch {array unset edInfo}
                    array set edInfo $info

                    if {[info exists edInfo($s_ocpiContainerTypeUUID)]} {
                        switch -- $edInfo($s_ocpiContainerTypeUUID) {
                            GPP {
                                puts -nonewline "Found GPP Executable Device \"$edInfo(label)\", "
                                puts -nonewline "device id \"$edInfo($s_ocpiDeviceIdUUID)\" "
                                puts "($edInfo($s_osNameUUID)/$edInfo($s_processorNameUUID))."
                            }
                            RCC {
                                puts -nonewline "Found RCC Executable Device \"$edInfo(label)\", "
                                puts -nonewline "device id \"$edInfo($s_ocpiDeviceIdUUID)\" "
                                puts "($edInfo($s_osNameUUID)/$edInfo($s_processorNameUUID))."
                            }
                            RPL {
                                puts -nonewline "Found RPL Executable Device \"$edInfo(label)\", "
                                puts -nonewline "device id \"$edInfo($s_ocpiDeviceIdUUID)\", "
                                puts "($edInfo($s_ocpiDeviceTypeUUID))."
                            }
                        }
                    } else {
                        puts "Found Executable Device \"$edInfo(label)\"."
                    }
                }

                set m_executableDevices($identifier) $info
            } finally {
                catch {corba::release $obj}
            }
        }
    }

    public proc getInfoForExecutableDevice {ed} {
        #
        # Human-readable name.
        #

        set info [list label [$ed label]]

        #
        # Query the Executable Device's OCPIContainerType property.
        #

        set props [list [list id "DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43" value {null {}}]]

        corba::try {
            $ed query props
        } catch {::CF::UnknownProperties} {
            #
            # Guess this is neither an SCA RCC nor RPL Executable Device.
            #

            return $info
        }

        set ocpiContainerType [lindex [lindex [lindex $props 0] 3] 1]
        lappend info $s_ocpiContainerTypeUUID $ocpiContainerType

        switch -- $ocpiContainerType {
            "GPP" {
                set ids [list $s_ocpiDeviceIdUUID $s_osNameUUID $s_processorNameUUID]
            }
            "RCC" {
                set ids [list $s_ocpiDeviceIdUUID $s_osNameUUID $s_processorNameUUID]
            }
            "RPL" {
                set ids [list $s_ocpiDeviceIdUUID $s_ocpiDeviceTypeUUID]
            }
            default {
                #
                # What kind of device is this?
                #

                error "invalid container type $ocpiContainerType"
            }
        }

        set props [list]

        foreach id $ids {
            lappend props [list id $id value {null {}}]
        }

        $ed query props

        foreach prop $props {
            lappend info [lindex $prop 1] [lindex [lindex $prop 3] 1]
        }

        return $info
    }

    public proc getTextContent {node} {
        set result ""
        foreach textNode [$node childNodes] {
            append result [$textNode nodeValue]
        }
        return $result
    }

    public proc hasChild {node type} {
        foreach n [$node childNodes] {
            if {[$n nodeName] eq $type} {
                return 1
            }
        }

        return 0
    }

    public proc getFirstChild {node type} {
        foreach n [$node childNodes] {
            if {[$n nodeName] eq $type} {
                return $n
            }
        }

        error "node $node no child node of type \"$type\""
    }

    public proc getAllChildren {node type} {
        set res [list]

        foreach n [$node childNodes] {
            if {[$n nodeName] eq $type} {
                lappend res $n
            }
        }

        return $res
    }

    public proc resolveFileName {baseName relName} {
        return [file join [file dirname $baseName] $relName]
    }
}

proc cfutil::findExecutableDevicesInNamingContext {nc} {
    catch {$nc _is_a ::CosNaming::NamingContext}
    $nc list 0 bl bi

    if {$bi == 0} {
        return [list]
    }

    set allExecutableDevices [list]

    corba::try {
        while {[$bi next_n 100 bl]} {
            foreach b $bl {
                array set binding $b

                if {$binding(binding_type) == "ncontext"} {
                    continue
                }

                set obj [$nc resolve $binding(binding_name)]

                corba::try {
                    if {[$obj _is_a IDL:CF/ExecutableDevice:1.0]} {
                        lappend allExecutableDevices [corba::duplicate $obj]
                    }
                } finally {
                    corba::release $obj
                }
            }
        }
    } finally {
        catch {$bi destroy}
    }

    return $allExecutableDevices
}

proc cfutil::findExecutableDevicesInDomainManager {dm} {
    set devmgrs [$dm deviceManagers]
    set allExecutableDevices [list]

    foreach devmgr $devmgrs {
        corba::try {
            set devs [$devmgr registeredDevices]
        } catch {...} {
            continue
        }

        foreach obj $devs {
            corba::try {
                if {[$obj _is_a IDL:CF/ExecutableDevice:1.0]} {
                    lappend allExecutableDevices [corba::duplicate $obj]
                }
            } catch {...} {
            }

            corba::release $obj
        }

        corba::release $devmgr
    }

    return $allExecutableDevices
}

