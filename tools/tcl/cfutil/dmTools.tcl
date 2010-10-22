
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
# Various Domain Manager tools
# ----------------------------------------------------------------------
#

namespace eval cfutil {}
namespace eval cfutil::dm {}

#
# ----------------------------------------------------------------------
# Install an application from a SAD file in the remote file system.
# ----------------------------------------------------------------------
#

proc cfutil::dm::installApplication {dm sad} {
    #
    # The domain manager's "installApplication" does not return the
    # application factory.
    #
    # So after installing the application, we fish its list of
    # application factories for the one we just installed.
    #

    $dm installApplication $sad

    set afs [$dm applicationFactories]
    set found 0

    foreach afi $afs {
        corba::try {
            set profile [$afi softwareProfile]

            if {[string range $profile 0 7] == "<profile"} {
                regexp {filename(?: )*=(?: )*\"([^\"]*)\"} $profile match fileName
            } else {
                set fileName $profile
            }

            if {$fileName == $sad} {
                set af [corba::duplicate $afi]
                set found 1
                break
            }
        } catch {...} {
        }
    }

    foreach afi $afs {
        corba::release $afi
    }

    if {!$found} {
        error "Application Factory not found after successful installation"
    }

    return $af
}

#
# ----------------------------------------------------------------------
# Install an application from a local Zip file.
# ----------------------------------------------------------------------
#

proc cfutil::dm::uploadAndInstall {dm zip dir} {
    set fm [$dm fileMgr]
    cfutil::uploadZip $fm $zip $dir

    if {[catch {
        set sad [cfutil::fs::find $fm $dir "*.sad.xml" 0 1]
    }]} {
        corba::release $fm
        error "Zip file did not contain an SAD"
    }

    corba::release $fm
    return [installApplication $dm $sad]
}

#
# ----------------------------------------------------------------------
# Find a device by its label.
# ----------------------------------------------------------------------
#

proc cfutil::dm::findDeviceByWhat {dm name value} {
    set devmgrs [$dm deviceManagers]
    set found 0

    foreach devmgr $devmgrs {
        corba::try {
            set devs [$devmgr registeredDevices]
        } catch {...} {
            continue
        }

        foreach devi $devs {
            corba::try {
                set dl [$devi $name]
            } catch {...} {
                continue
            }

            if {$dl == $value} {
                set dev [corba::duplicate $devi]
                set found 1
                break
            }
        }

        foreach devi $devs {
            corba::release $devi
        }

        if {$found} {
            break
        }
    }

    foreach devmgr $devmgrs {
        corba::release $devmgr
    }

    if {!$found} {
        error "no device with $name==$value found"
    }

    return $dev
}

proc cfutil::dm::findDeviceByLabel {dm label} {
    return [findDeviceByWhat $dm label $label]
}

proc cfutil::dm::findDeviceByIdentifier {dm identifier} {
    return [findDeviceByWhat $dm identifier $identifier]
}
