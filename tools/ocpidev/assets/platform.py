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
import os
import sys
import logging
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util

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
