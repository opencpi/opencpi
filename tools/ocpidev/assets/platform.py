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

from .abstract import *
from .assembly import *
from _opencpi.hdltargets import HdlToolFactory, HdlToolSet
import os
import sys
import logging
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util
import json

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

        self.platform_list = []
        if kwargs.get("init_hdlplats", False):
            logging.debug("Project constructor creating HdlPlatformWorker Objects")
            for plat_directory in self.get_valid_platforms():
                # Only construct platforms that were requested and listed in hdl_platforms
                if os.path.basename(plat_directory) in [plat.name for plat in self.hdl_platforms]:
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

        # Collect list of assembly directories
        for name in make_platforms:
            platform_list.append(self.directory + "/" + name)
        return platform_list

    def show_utilization(self):
        """
        Show utilization separately for each hdl-platform in this collection
        """
        for platform in self.platform_list:
            platform.show_utilization()

    #placeholder function
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("HdlPlatformsCollection.build() is not implemented")

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
        self.platform = hdltargets.HdlToolFactory.factory("hdlplatform", self.name)
        self.init_configs()

    def init_configs(self):
        """
        Collect the list of build configurations for this Platform Worker.
        Construct an HdlPlatformWorkerConfig for each and add to the self.configs map.
        """
        # Get the list of Configurations from make
        plat_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                mk_arg="ShellHdlPlatformVars=1", verbose=True)
        if "Configurations" not in plat_vars:
            raise ocpiutil.OCPIException("Could not get list of HDL Platform Configurations " +
                                         "from \"" + self.directory + "/Makefile\"")
        # This should be a list of Configuration NAMES
        config_list = plat_vars["Configurations"]
        # Directory for each config is <platform-worker-dir>/config-<configuration>
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
                # Add this dataset to the list of utilization dictionaries. It will serve
                # as a single data-point/row in the report
                sub_report.assign_for_all_points(key="Configuration", value=cfg_name)
                util_report += sub_report
        return util_report

class HdlPlatformWorkerConfig(HdlAssembly):
    """
    HDL Platform Worker Configurations are buildable HDL Assemblies that contain their
    HDL Platform Worker as well as HDL Device Workers. Each configuration can only be
    built for the HDL Platform defined by the configuration's HDL Platform Worker.

    Each instance has a target-<hdl-target> subdirectory where build artifacts can be found.

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

class Platform(object):
    @classmethod
    def show_all(cls, details):
        if details == "simple" or details == "table":
            print("RCC:")
            RccPlatform.show_all(details)
            print("HDL:")
            HdlPlatform.show_all(details)
        elif details == "json":
            rcc_dict = RccPlatform._get_all_dict()
            hdl_dict = HdlPlatform._get_all_dict()

            plat_dict = {"rcc":rcc_dict, "hdl":hdl_dict}
            json.dump(plat_dict, sys.stdout)
            print()

class Target(object):
    @classmethod
    def show_all(cls, details):
        if details == "simple" or details == "table":
            print("RCC:")
            RccTarget.show_all(details)
            print("HDL:")
            HdlTarget.show_all(details)
        elif details == "json":
            rcc_dict = RccTarget._get_all_dict()
            hdl_dict = HdlTarget._get_all_dict()

            target_dict = {"rcc":rcc_dict, "hdl":hdl_dict}
            json.dump(target_dict, sys.stdout)
            print()

class RccPlatform(Platform):
    def __init__(self, name, target):
        self.name = name
        self.target = target

    def __str__(self):
        return self.name

    @classmethod
    def _get_all_dict(cls):
        rccDict = ocpiutil.get_make_vars_rcc_targets()
        try:
          rccPlatforms = rccDict["RccAllPlatforms"]
        except TypeError:
            raise ocpiutil.OCPIException("No RCC platforms found. Make sure the core project is " +
                                         "registered or in the OCPI_PROJECT_PATH.")
        plat_dict = {}
        for plat in rccPlatforms:
            plat_dict[plat] = {}
            plat_dict[plat]["target"] = rccDict["RccTarget_" + plat][0]
        return plat_dict

    @classmethod
    def show_all(cls, details):
        plat_dict = cls._get_all_dict()

        if details == "simple":
            for plat in plat_dict:
                print(plat + " ", end='')
            print()
        elif details == "table":
            row_1 = ["Platform", "Target"]
            rows = [row_1]
            for plat in plat_dict:
                rows.append([plat, plat_dict[plat]["target"]])
            ocpiutil.print_table(rows, underline="-")
        elif details == "json":
            json.dump(plat_dict, sys.stdout)
            print()

class RccTarget(object):
    def __init__(self, name, target):
        self.name = name
        self.target = target

    def __str__(self):
        return self.name

    @classmethod
    def _get_all_dict(cls):
        rccDict = ocpiutil.get_make_vars_rcc_targets()
        try:
          rccPlatforms = rccDict["RccAllPlatforms"]

        except TypeError:
            raise ocpiutil.OCPIException("No RCC platforms found. Make sure the core project is " +
                                         "registered or in the OCPI_PROJECT_PATH.")
        target_dict = {}
        for plat in rccPlatforms:
            target_dict[plat] = {}
            target_dict[plat]["target"] = rccDict["RccTarget_" + plat][0]
        return target_dict

    @classmethod
    def show_all(cls, details):
        target_dict = cls._get_all_dict()

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
    def __init__(self, name, target, exactpart, built=False):
        self.name = name
        self.target = target
        self.exactpart = exactpart
        self.built = built

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

    @classmethod
    def _get_all_dict(cls):
        all_plats= HdlToolFactory.get_or_create_all("hdlplatform")
        plat_dict = {}
        for plat in all_plats:
            plat_dict[plat.name] = {}
            plat_dict[plat.name]["vendor"] = plat.target.vendor
            plat_dict[plat.name]["target"] = plat.target.name
            plat_dict[plat.name]["part"] = plat.exactpart
            plat_dict[plat.name]["built"] = plat.built
            plat_dict[plat.name]["tool"] = plat.target.toolset.name

        return plat_dict

    @classmethod
    def show_all(cls, details):
        plat_dict = cls._get_all_dict()

        if details == "simple":
            for plat in plat_dict:
                print(plat + " ", end='')
            print()
        elif details == "table":
            row_1 = ["Platform", "Target", "Part", "Vendor", "Toolset"]
            rows = [row_1]
            for plat in plat_dict:
                built = ""
                if plat_dict[plat]["built"] == False:
                    built = "*"
                rows.append([plat + built, plat_dict[plat]["target"], plat_dict[plat]["part"],
                             plat_dict[plat]["vendor"], plat_dict[plat]["tool"]])
            ocpiutil.print_table(rows, underline="-")
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
        if isinstance(toolset, HdlToolSet):
            self.toolset = toolset
        else:
            self.toolset = HdlToolFactory.factory("hdltoolset", toolset)

    def __str__(self):
        return self.name

    def __lt__(self, other):
        if self.vendor < other.vendor:
            return True
        elif self.vendor == other.vendor:
            return str(self) < str(other)
        else:
            return False

    @classmethod
    def _get_all_dict(cls):
        target_dict = {}
        for vendor in HdlToolFactory.get_all_vendors():
            targetDict = {}
            for target in HdlToolFactory.get_all_targets_for_vendor(vendor):
                targetDict[target.name] = {"parts": target.parts,
                                           "tool": target.toolset.title}
            target_dict[vendor] = targetDict

        return target_dict

    @classmethod
    def show_all(cls, details):
        target_dict = cls._get_all_dict()

        if details == "simple":
            for vendor in target_dict:
                for target in target_dict[vendor]:
                    print(target + " ", end='')
            print()
        elif details == "table":
            row_1 = ["Target", "Parts", "Vendor", "Toolset"]
            rows = [row_1]
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
