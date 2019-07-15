# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.
"""
Defining rcc/hdl platform related classes
"""

import os
import sys
import logging
import json
import _opencpi.util as ocpiutil
import _opencpi.hdltargets as hdltargets
from .abstract import ShowableAsset, HDLBuildableAsset, ReportableAsset, Asset
from .assembly import HdlAssembly
from .worker import HdlWorker
from .factory import AssetFactory


class RccPlatformsCollection(ShowableAsset):
    """
    Collection of HDL Platform Workers. This class represents the hdl/platforms directory.
    """
    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        self.check_dirtype("rcc-platforms", directory)
        super().__init__(directory, name, **kwargs)

        self.platform_list = []
        if kwargs.get("init_hdlplats", False):
            logging.debug("Project constructor creating HdlPlatformWorker Objects")
            for plat_directory in self.get_valid_platforms():
                self.platform_list.append(AssetFactory.factory("rcc-platform", plat_directory,
                                                               **kwargs))

    def get_valid_platforms(self):
        """
        Probes file-system in order to determine the list of active platforms in the
        platforms collection
        """
        return [(self.directory + "/" + dir) for dir in os.listdir(self.directory)
                if os.path.isdir(self.directory + "/" + dir)]

    def show(self, details, verbose, **kwargs):
        """
        Show all of the Rcc platforms in this collection
        """
        raise NotImplementedError("show() is not implemented")

class HdlPlatformsCollection(HDLBuildableAsset, ReportableAsset):
    """
    Collection of HDL Platform Workers. This class represents the hdl/platforms directory.
    """

    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HdlPlatformsCollection member data  and calls the super class __init__.
        Throws an exception if the directory passed in is not a valid hdl-platforms directory.
        valid kwargs handled at this level are:
            init_hdl_plats   (T/F) - Instructs the method whether to construct all HdlPlatformWorker
                                     objects contained in the project (at least those with a
                                     corresponding build platform listed in self.hdl_platforms)
        """
        self.check_dirtype("hdl-platforms", directory)
        super().__init__(directory, name, **kwargs)
        self.hdl_plat_strs = kwargs.get("hdl_plats", None)
        self.platform_list = []
        if kwargs.get("init_hdlplats", False):
            logging.debug("Project constructor creating HdlPlatformWorker Objects")
            for plat_directory in self.get_valid_platforms():
                # Only construct platforms that were requested and listed in hdl_platforms
                plat_in_list = (os.path.basename(plat_directory) in
                                [plat.name for plat in self.hdl_platforms])
                if "local" in self.hdl_plat_strs or plat_in_list:
                    self.platform_list.append(AssetFactory.factory("hdl-platform", plat_directory,
                                                                   **kwargs))

    def get_valid_platforms(self):
        """
        Probes make in order to determine the list of active platforms in the
        platforms collection
        """
        platform_list = []
        logging.debug("Getting valid platforms from: " + self.directory + "/Makefile")
        make_platforms = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                     mk_arg="ShellPlatformsVars=1 showplatforms",
                                                     verbose=True)["HdlPlatforms"]

        # Collect list of platform directories
        for name in make_platforms:
            platform_list.append(self.directory + "/" + name)
        return platform_list

    def show_utilization(self):
        """
        Show utilization separately for each hdl-platform in this collection
        """
        for platform in self.platform_list:
            platform.show_utilization()

    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("HdlPlatformsCollection.build() is not implemented")

    @staticmethod
    def get_working_dir(name, library, hdl_library, hdl_platform):
        """
        return the directory of an HDL Platform Collection given the name (name) and
        library specifiers (library, hdl_library, hdl_platform)
        """
        ocpiutil.check_no_libs("hdl-platforms", library, hdl_library, hdl_platform)
        if name: ocpiutil.throw_not_blank_e("hdl-platforms", "name", False)
        if ocpiutil.get_dirtype() not in ["project", "hdl-platforms"]:
            ocpiutil.throw_not_valid_dirtype_e(["project", "hdl-platforms"])
        return ocpiutil.get_path_to_project_top() + "/hdl/platforms"

# pylint:disable=too-many-ancestors
class HdlPlatformWorker(HdlWorker, ReportableAsset):
    """
    An HDL Platform Worker is a special case of HDL Worker that defines an HDL Platform.
    HDL Platforms have named build Configurations. HDL Platform Workers and Configurations
    can only be built for the HDL Platform that the worker defines.

    Each instance has a dictionary of configurations. This dict is of the form:
    self.configs = {<config-name> : <HdlPlatformWorkerConfig-instance>}

    Each instance is bound to a single HdlPlatform instance (self.platform)

    """

    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HdlPlatformWorker member data  and calls the super class __init__. Throws an
        exception if the directory passed in is not a valid hdl-platform directory. Initialize
        platform configurations for this platform worker.
        valid kwargs handled at this level are:
            None
        """
        self.check_dirtype("hdl-platform", directory)
        if name is None:
            name = os.path.basename(directory)
        super().__init__(directory, name, **kwargs)
        self.configs = {}
        self.package_id = None
        config_list = self.get_make_vars()
        self.platform = hdltargets.HdlToolFactory.factory("hdlplatform", self.name, self.package_id)
        #TODO this should be guarded by a init kwarg variable, not always needed i.e. show project
        self.init_configs(config_list)


    def build(self):
        """
        This function will build the HdlPlatformWorker
        """
        raise NotImplementedError("build() is not implemented")

    def get_make_vars(self):
        """
        Collect the list of build configurations and package id for this Platform Worker.
        """
        # Get the list of Configurations from make
        logging.debug("Get the list of platform Configurations from make")
        try:
            plat_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                    mk_arg="ShellHdlPlatformVars=1 showinfo",
                                                    verbose=False)
        except ocpiutil.OCPIException:
            # if the make call causes and error assume configs are blank
            plat_vars = {"Configurations" : "", "Package":"N/A"}
        if "Configurations" not in plat_vars:
            raise ocpiutil.OCPIException("Could not get list of HDL Platform Configurations " +
                                         "from \"" + self.directory + "/Makefile\"")
        self.package_id = plat_vars["Package"]
        # This should be a list of Configuration NAMES
        config_list = plat_vars["Configurations"]
        return config_list

    def init_configs(self, config_list):
        """
        Construct an HdlPlatformWorkerConfig for each and add to the self.configs map.
        """
        # Directory for each config is <platform-worker-directory>/config-<configuration>
        cfg_prefix = self.directory + "/config-"
        for config_name in config_list:
            # Construct the Config instance and add to map
            self.configs[config_name] = HdlPlatformWorkerConfig(directory=cfg_prefix + config_name,
                                                                name=config_name,
                                                                platform=self.platform)
    def get_utilization(self):
        """
        Get any utilization information for this Platform Worker's Configurations

        The returned Report contains a data-point (dict) for each Configuration, stored in the
        Report instance's data_points array. Each data-point maps dimension/header to value for
        that configuration.
        """
        # Add to the default list of reportable synthesis items to report on
        ordered_headers = ["Configuration"] + hdltargets.HdlReportableToolSet.get_ordered_items()
        sort_priority = hdltargets.HdlReportableToolSet.get_sort_priority() + ["Configuration"]
        # Initialize an empty data-set with these default headers
        util_report = ocpiutil.Report(ordered_headers=ordered_headers, sort_priority=sort_priority)
        # Sort based on configuration name
        for cfg_name in sorted(self.configs):
            # Get the dictionaries of utilization report items for this Platform Worker.
            # Each dictionary returned corresponds to one implementation of this
            # container, and serves as a single data-point/row.
            # Add all data-points for this container to the running list
            sub_report = self.configs[cfg_name].get_utilization()
            if sub_report:
                # We want to add the container name as a report element
                # Add this data-set to the list of utilization dictionaries. It will serve
                # as a single data-point/row in the report
                sub_report.assign_for_all_points(key="Configuration", value=cfg_name)
                util_report += sub_report
        return util_report

    @staticmethod
    def get_working_dir(name, library, hdl_library, hdl_platform):
        """
        return the directory of a HDL Platform given the name (name) and
        library specifiers (library, hdl_library, hdl_platform)
        """
        ocpiutil.check_no_libs("hdl-platform", library, hdl_library, hdl_platform)
        if not name: ocpiutil.throw_not_blank_e("hdl-platform", "name", True)
        if ocpiutil.get_dirtype() not in ["project", "hdl-platforms", "hdl-platform"]:
            ocpiutil.throw_not_valid_dirtype_e(["project", "hdl-platforms", "hdl-platform"])
        return ocpiutil.get_path_to_project_top() + "/hdl/platforms/" + name
# pylint:enable=too-many-ancestors

class HdlPlatformWorkerConfig(HdlAssembly):
    """
    HDL Platform Worker Configurations are build-able HDL Assemblies that contain their
    HDL Platform Worker as well as HDL Device Workers. Each configuration can only be
    built for the HDL Platform defined by the configuration's HDL Platform Worker.

    Each instance has a target-<hdl-target> sub-directory where build artifacts can be found.

    Each instance is bound to a single HdlPlatform instance (self.platform)
    """
    def __init__(self, directory, name, **kwargs):
        """
        Initializes HdlPlatformWorkerConfig member data and calls the super class __init__.
        valid kwargs handled at this level are:
            platform (HdlPlatform) - The HdlPlatform object that is bound to this configuration.
        """
        super().__init__(directory, name, **kwargs)
        self.platform = kwargs.get("platform", None)
        if self.platform is None:
            raise ocpiutil.OCPIException("HdlPlatformWorkerConfig cannot be constructed without " +
                                         "being provided a platform")
        self.subdir_prefix = directory + "/target-" + self.platform.target.name

    #placeholder function
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("HdlPlatformWorkerConfig.build() is not implemented")

    def get_utilization(self):
        """
        Get any utilization information for this instance

        The returned Report contains a single data-point (dict) for this Configuration,
        stored in the Report instance's data_points array.
        The data-point maps dimension/header to value for that configuration.

        Since a Platform Configuration is a synthesis asset, the utilization report will
        be generated with mode=synth
        """
        # We report for this Config's HDL Platform's toolset
        toolset = self.platform.target.toolset
        if isinstance(toolset, hdltargets.HdlReportableToolSet):
            return toolset.construct_report_item(self.subdir_prefix, target=self.platform.target,
                                                 mode="synth")
        else:
            return ocpiutil.Report()

class Platform(Asset):
    """
    Base Class for both rcc and hdl platforms
    """
    @classmethod
    def show_all(cls, details):
        """
        shows the list of all rcc and hdl platforms in the format specified from details
        (simple, table, or json)
        """
        if details == "simple":
            print("RCC:")
            RccPlatform.show_all(details)
            print("HDL:")
            HdlPlatform.show_all(details)
        elif details == "table":
            #need to combine rcc and hdl into a single table
            rcc_table = RccPlatform.get_all_table(RccPlatform.get_all_dict())
            rcc_table[0].insert(1, "Type")
            rcc_table[0].append("HDL Part")
            rcc_table[0].append("HDL Vendor")
            for my_list in rcc_table[1:]:
                my_list.append("N/A")
            for my_list in rcc_table[1:]:
                my_list.append("N/A")
            for my_list in rcc_table[1:]:
                my_list.insert(1, "rcc")
            hdl_table = HdlPlatform.get_all_table(HdlPlatform.get_all_dict())
            for my_list in hdl_table[1:]:
                my_list.insert(1, "hdl")
            for my_list in hdl_table[1:]:
                rcc_table.append(my_list)
            ocpiutil.print_table(rcc_table, underline="-")

        elif details == "json":
            rcc_dict = RccPlatform.get_all_dict()
            hdl_dict = HdlPlatform.get_all_dict()

            plat_dict = {"rcc":rcc_dict, "hdl":hdl_dict}
            json.dump(plat_dict, sys.stdout)
            print()

# pylint:disable=too-few-public-methods
class Target(object):
    """
    Base Class for both rcc and hdl targets
    """
    @classmethod
    def show_all(cls, details):
        """
        shows the list of all rcc and hdl targets in the format specified from details
        (simple, table, or json)
        """
        if details == "simple" or details == "table":
            print("RCC:")
            RccTarget.show_all(details)
            print("HDL:")
            HdlTarget.show_all(details)
        elif details == "json":
            rcc_dict = RccTarget.get_all_dict()
            hdl_dict = HdlTarget.get_all_dict()

            target_dict = {"rcc":rcc_dict, "hdl":hdl_dict}
            json.dump(target_dict, sys.stdout)
            print()
# pylint:enable=too-few-public-methods

class RccPlatform(Platform):
    """
    An OpenCPI Rcc software platform
    """
    def __init__(self, directory, name, **kwargs):
        """
        Constructor for RccPlatform no extra values from kwargs processed in this constructor
        """
        self.check_dirtype("rcc-platform", directory)
        super().__init__(directory, name, **kwargs)

    def __str__(self):
        return self.name

    @classmethod
    def get_all_dict(cls):
        """
        returns a dictionary with all available rcc platforms from the RccAllPlatforms make variable
        """
        rcc_dict = ocpiutil.get_make_vars_rcc_targets()
        try:
            rcc_plats = rcc_dict["RccAllPlatforms"]
        except TypeError:
            raise ocpiutil.OCPIException("No RCC platforms found. Make sure the core project is " +
                                         "registered or in the OCPI_PROJECT_PATH.")
        plat_dict = {}
        for plat in rcc_plats:
            plat_dict[plat] = {}
            plat_dict[plat]["target"] = rcc_dict["RccTarget_" + plat][0]
            proj_top = ocpiutil.get_project_package(rcc_dict["RccPlatDir_" + plat][0])
            plat_dict[plat]["package_id"] = proj_top + ".platform." + plat
            plat_dict[plat]["directory"] = rcc_dict["RccPlatDir_" + plat][0]
        return plat_dict

    @classmethod
    def get_all_table(cls, plat_dict):
        """
        returns a table (but does not print it) with all the rcc platforms in it
        """
        row_1 = ["Platform", "Package-ID", "Target"]
        rows = [row_1]
        for plat in plat_dict:
            rows.append([plat, plat_dict[plat]["package_id"], plat_dict[plat]["target"]])
        return rows

    @classmethod
    def show_all(cls, details):
        """
        shows all of the rcc platforms in the format that is specified using details
        (simple, table, or json)
        """
        plat_dict = cls.get_all_dict()

        if details == "simple":
            for plat in plat_dict:
                print(plat + " ", end='')
            print()
        elif details == "table":
            ocpiutil.print_table(cls.get_all_table(plat_dict), underline="-")
        elif details == "json":
            json.dump(plat_dict, sys.stdout)
            print()

class RccTarget(object):
    """
    An OpenCPI Rcc software platform (mostly meaningless just a internal for make)
    """
    def __init__(self, name, target):
        """
        Constructor for RccTarget no extra values from kwargs processed in this constructor
        """
        self.name = name
        self.target = target

    def __str__(self):
        return self.name

    @classmethod
    def get_all_dict(cls):
        """
        returns a dictionary with all available rcc targets from the RccAllPlatforms make variable
        """
        rcc_dict = ocpiutil.get_make_vars_rcc_targets()
        try:
            rcc_plats = rcc_dict["RccAllPlatforms"]

        except TypeError:
            raise ocpiutil.OCPIException("No RCC platforms found. Make sure the core project is " +
                                         "registered or in the OCPI_PROJECT_PATH.")
        target_dict = {}
        for plat in rcc_plats:
            target_dict[plat] = {}
            target_dict[plat]["target"] = rcc_dict["RccTarget_" + plat][0]
        return target_dict

    @classmethod
    def show_all(cls, details):
        """
        shows all of the rcc targets in the format that is specified using details
        (simple, table, or json)
        """
        target_dict = cls.get_all_dict()

        if details == "simple":
            for plat in target_dict:
                print(target_dict[plat]["target"] + " ", end='')
            print()
        elif details == "table":
            row_1 = ["Platform", "Target"]
            rows = [row_1]
            for plat in target_dict:
                rows.append([plat, target_dict[plat]["target"]])
            ocpiutil.print_table(rows, underline="-")
        elif details == "json":
            json.dump(target_dict, sys.stdout)
            print()

class HdlPlatform(Platform):
    """
    HdlPlatform
    A HDL Platform (e.g. zed, ml605, alst4, modelsim) has an exact-part number
    (e.g xc7z020-1-clg484) and a corresponding target (e.g zynq, virtex6,
    stratix4, modelsim). A flag is set which indicates whether this platform
    has been built yet.

    Example (doctest):
        >>> platform0 = HdlPlatform("myplat0", target1, "exactpart0")
        >>> platform1 = HdlPlatform("myplat1", target1, "exactpart1")
        >>> platform2 = HdlPlatform("myplat2", target2, "exactpart2")
        >>> [platform0.name, platform0.target.name, str(platform0.target.toolset)]
        ['myplat0', 'mytgt1', 'mytool1']
        >>> [platform1.name, platform1.target.name, str(platform1.target.toolset)]
        ['myplat1', 'mytgt1', 'mytool1']
        >>> [platform2.name, platform2.target.name, str(platform2.target.toolset)]
        ['myplat2', 'mytgt2', 'mytool2']
        >>> platform0.exactpart
        'exactpart0'
    """
    #TODO change to use kwargs too many arguments
    def __init__(self, name, target, exactpart, directory, built=False, package_id=None):
        """
        HdlPlatform constructor
        """
        super().__init__(directory, name)
        self.target = target
        self.exactpart = exactpart
        self.built = built
        self.dir = ocpiutil.rchop(directory, "/lib")
        if self.dir and not package_id:
            self.package_id = ocpiutil.set_vars_from_make(self.dir + "/Makefile",
                                                          "ShellHdlPlatformVars=1 showpackage",
                                                          "verbose")["Package"][0]
        else:
            self.package_id = ""

    def __str__(self):
        return self.name

    def __lt__(self, other):
        if self.target.vendor < other.target.vendor:
            return True
        elif self.target.vendor == other.target.vendor:
            return str(self) < str(other)
        else:
            return False

    def get_toolset(self):
        """
        Return the toolset for this target
        """
        return self.target.toolset

    #TODO this can be static its not using cls
    @classmethod
    def get_all_dict(cls):
        """
        returns a dictionary with all available hdl platforms from the HdlAllPlatforms make variable
        """
        all_plats = hdltargets.HdlToolFactory.get_or_create_all("hdlplatform")
        plat_dict = {}
        for plat in all_plats:
            plat_dict[plat.name] = {}
            plat_dict[plat.name]["vendor"] = plat.target.vendor
            plat_dict[plat.name]["target"] = plat.target.name
            plat_dict[plat.name]["part"] = plat.exactpart
            plat_dict[plat.name]["built"] = plat.built
            plat_dict[plat.name]["directory"] = plat.dir
            plat_dict[plat.name]["tool"] = plat.target.toolset.name
            plat_dict[plat.name]["package_id"] = plat.package_id

        return plat_dict

    #TODO this can be static its not using cls
    @classmethod
    def get_all_table(cls, plat_dict):
        """
        returns a table (but does not print it) with all the hdl platforms in it
        """
        row_1 = ["Platform", "Package-ID", "Target", "Part", "Vendor", "Toolset"]
        rows = [row_1]
        for plat in plat_dict:
            built = ""
            if not plat_dict[plat]["built"]:
                built = "*"
            rows.append([plat + built, plat_dict[plat]["package_id"], plat_dict[plat]["target"],
                         plat_dict[plat]["part"], plat_dict[plat]["vendor"],
                         plat_dict[plat]["tool"]])
        return rows

    @classmethod
    def show_all(cls, details):
        """
        shows all of the hdl platforms in the format that is specified using details
        (simple, table, or json)
        """
        plat_dict = cls.get_all_dict()

        if details == "simple":
            for plat in plat_dict:
                print(plat + " ", end='')
            print()
        elif details == "table":
            ocpiutil.print_table(cls.get_all_table(plat_dict), underline="-")
            print("* An asterisk indicates that the platform has not been built yet.\n" +
                  "  Assemblies and tests cannot be built until the platform is built.\n")
        elif details == "json":
            json.dump(plat_dict, sys.stdout)
            print()

class HdlTarget(object):
    """
    HdlTarget
    A HDL target corresponds to a family (e.g. zynq, virtex6, stratix4) of parts.
    A target belongs to a vendor/top target (e.g. xilinx, altera, modelsim),
    and is associated with a toolset (e.g. vivado, quartus, xsim, modelsim).

    Example (doctest):
        >>> target0 = HdlTarget("mytgt0", "vend1", ["part0.1", "part0.2"], tool1)
        >>> target1 = HdlTarget("mytgt1", "vend1", ["part1.1", "part1.2"], tool1)
        >>> target2 = HdlTarget("mytgt2", "vend2", ["part2"], tool2)
        >>> [target1.name, target1.vendor, target1.parts, str(target1.toolset)]
        ['mytgt1', 'vend1', ['part1.1', 'part1.2'], 'mytool1']
        >>> [target2.name, target2.vendor, target2.parts, str(target2.toolset)]
        ['mytgt2', 'vend2', ['part2'], 'mytool2']
        >>> target1.name
        'mytgt1'
        >>> target1.parts
        ['part1.1', 'part1.2']
        >>> target2.vendor
        'vend2'
    """
    def __init__(self, name, vendor, parts, toolset):
        """
        Create an instance of HdlTarget.
        Give it a name and associate it with a vendor, a list of parts, and an HdlToolSet.
        """
        self.name = name
        self.vendor = vendor
        self.parts = parts
        # If the caller passed in a toolset instance instead of name, just assign
        # the instance (no need to construct or search for one). This is especially
        # useful for simple tests of this class (e.g. see doctest setup at end of file
        if isinstance(toolset, hdltargets.HdlToolSet):
            self.toolset = toolset
        else:
            self.toolset = hdltargets.HdlToolFactory.factory("hdltoolset", toolset)

    def __str__(self):
        return self.name

    def __lt__(self, other):
        if self.vendor < other.vendor:
            return True
        elif self.vendor == other.vendor:
            return str(self) < str(other)
        else:
            return False

    #TODO this can be static its not using cls
    @classmethod
    def get_all_dict(cls):
        """
        returns a dictionary with all available hdl targets from the HdlAllTargets make variable
        """
        target_dict = {}
        for vendor in hdltargets.HdlToolFactory.get_all_vendors():
            vendor_dict = {}
            for target in hdltargets.HdlToolFactory.get_all_targets_for_vendor(vendor):
                vendor_dict[target.name] = {"parts": target.parts,
                                            "tool": target.toolset.title}
            target_dict[vendor] = vendor_dict

        return target_dict

    #TODO this can be static its not using cls when get_all_dict is static
    @classmethod
    def show_all(cls, details):
        """
        shows all of the hdl targets in the format that is specified using details
        (simple, table, or json)
        """
        target_dict = cls.get_all_dict()
        if details == "simple":
            for vendor in target_dict:
                for target in target_dict[vendor]:
                    print(target + " ", end='')
            print()
        elif details == "table":
            rows = [["Target", "Parts", "Vendor", "Toolset"]]
            for vendor in target_dict:
                for target in target_dict[vendor]:
                    rows.append([target,
                                 ", ".join(target_dict[vendor][target]["parts"]),
                                 vendor,
                                 target_dict[vendor][target]["tool"]])
            ocpiutil.print_table(rows, underline="-")
        elif details == "json":
            json.dump(target_dict, sys.stdout)
            print()
