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
Defintion of rcc and hdl workers and related classes
"""

import os
import sys
import logging
import json
from abc import abstractmethod
from xml.etree import ElementTree as ET
import _opencpi.hdltargets as hdltargets
import _opencpi.util as ocpiutil
from .abstract import HDLBuildableAsset, ReportableAsset
from .component import ShowableComponent

class Worker(ShowableComponent):
    """
    Any OpenCPI Worker. This class is authoring model agnostic and represents all workers of any
    type. In general, something is a worker if it has an OWD (OpenCPI Worker Description File),
    and implements an OCS (OpenCPI Component Specification).
    """
    def __init__(self, directory, name=None, **kwargs):
        if not name:
            name = os.path.basename(os.path.realpath(directory)).rsplit('.', 1)[0]
        rchoped_name = ocpiutil.rchop(name, "." + self.get_authoring_model(directory))
        self.ocpigen_xml = (directory + "/" + rchoped_name + ".xml")
        super().__init__(directory, name, **kwargs)

    @staticmethod
    def get_authoring_model(directory):
        """
        Each worker has an Authoring Model. Given a worker directory, return its Authoring Model.
        """
        # Worker directories end in ".<authoring-model>", so use splitext to get the
        # directories extension
        _, ext = os.path.splitext(os.path.realpath(ocpiutil.rchop(directory, '/')))
        # Return the extension/model without the leading period
        return ext[1:]

    def show(self, details, verbose, **kwargs):
        """
        Print out the ports, properties, and slaves of a given worker in the format that is
        provided by the caller

        Function attributes:
          details    - the mode to print out the information in table or simple are the only valid
                       options
          verbose    - integer for verbosity level 0 is default and lowest and anything above 1
                       shows struct internals and hidden properties
          kwargs     - no extra kwargs arguments expected
        """
        json_dict = self._get_show_dict(verbose)
        # add worker specific stuff to the dictionary
        prop_dict = json_dict["properties"]
        for prop in self.property_list:
            if verbose > 0 or prop.get("hidden", "0") == "0":
                prop_dict[prop["name"]]["isImpl"] = prop.get("isImpl", "0")
                access_dict = prop_dict[prop["name"]]["accessibility"]
                if prop_dict[prop["name"]]["isImpl"] == "0":
                    access_dict["specinitial"] = prop.get("specinitial", "0")
                    access_dict["specparameter"] = prop.get("specparameter", "0")
                    access_dict["specwritable"] = prop.get("specwritable", "0")
                    access_dict["specreadback"] = prop.get("specreadable", "0")
                    access_dict["specvolitile"] = prop.get("specvolitile", "0")
        slave_dict = {}
        for slave in self.slave_list:
            slave_dict[slave] = {"name": slave}
        json_dict["slaves"] = slave_dict

        if details == "simple" or details == "table":
            print("Component: " + json_dict["name"] + " Package ID: " + json_dict["package_id"])
            print("Directory: " + json_dict["directory"])
            self._show_ports_props(json_dict, details, verbose, True)
            if json_dict.get("slaves"):
                rows = [["Slave Name"]]
                for slave in json_dict["slaves"]:
                    rows.append([json_dict["slaves"][slave]["name"]])
                ocpiutil.print_table(rows, underline="-")
        else:
            json.dump(json_dict, sys.stdout)
            print()

    @staticmethod
    def get_working_dir(name, library, hdl_library, hdl_platform):
        """
        return the directory of a Worker given the name (name) and
        library specifiers (library, hdl_library, hdl_platform)
        """
        # if more then one of the library location variable are not None it is an error
        if len(list(filter(None, [library, hdl_library, hdl_platform]))) > 1:
            ocpiutil.throw_invalid_libs_e()
        valid_dirtypes = ["project", "libraries", "library", "worker", "hdl-platform"]
        cur_dirtype = ocpiutil.get_dirtype()
        if cur_dirtype not in valid_dirtypes:
            ocpiutil.throw_not_valid_dirtype_e(valid_dirtypes)
        if library:
            if not library.startswith("components"):
                library = "components/" + library
            return ocpiutil.get_path_to_project_top() + "/" + library + "/" + name
        elif hdl_library:
            return ocpiutil.get_path_to_project_top() + "/hdl/" + hdl_library + "/" + name
        elif hdl_platform:
            return (ocpiutil.get_path_to_project_top() + "/hdl/platforms/" + hdl_platform +
                    "/devices/" + name)
        elif name:
            if cur_dirtype == "hdl-platform":
                return "devices/" + name
            elif cur_dirtype == "project":
                if ocpiutil.get_dirtype("components") == "libraries":
                    ocpiutil.throw_specify_lib_e()
                return "components/" + name
            else:
                return name
        else: ocpiutil.throw_not_blank_e("worker", "name", True)

# Placeholder class
class RccWorker(Worker):
    """
    This class represents a RCC worker.
    """
    def __init__(self, directory, name=None, **kwargs):
        super().__init__(directory, name, **kwargs)

class HdlCore(HDLBuildableAsset):
    """
    This represents any build-able HDL Asset that is core-like (i.e. is not a primitive library).

    For synthesis tools, compilation of a HdlCore generally results in a netlist.
        Note: for simulation tools, this criteria generally cannot be used because netlists
              are not commonly used for compilation targeting simulation
    """
    def __init__(self, directory, name=None, **kwargs):
        super().__init__(directory, name, **kwargs)

    #placeholder function
    def build(self):
        """
        This function will build the asset, must be implemented by the child class
        """
        raise NotImplementedError("HdlCore.build() is not implemented")

# TODO There was a design mistake here when implementing HdlWorker, HdlLibraryWorker
#      and HdlPlatformWorker. HdlWorker should represent the most basic HDL worker
#      (e.g. an application worker that might be in a library), and HdlPlatformWorker
#      should inherit from HdlWorker. All assets that inherit from HdlWorker should
#      support build configurations. This includes HdlPlatformWorker, but note that
#      a Platform Configuration is NOT a build configuration of the Platform Worker.
#      this is somewhat confusing terminology, but a Platform Configuration is just
#      an assembly containing the Platform Worker that happens to live in the Platform
#      worker directory. That being said, a Platform Worker should ALSO be able to
#      support plain old BUILD configurations (e.g. where you would have target-1-*
#      at the top level of the Platform Worker directory). So, HdlWorker should have
#      an init_build_configs() function, and HdlPlatformWorker should inherit that
#      function from HdlWorker while ALSO implementing a init_platform_configs().
#      other asset-types should also probably inherit from HdlWorker such as
#      hdlAssembly, HdlContainer....
#      TLDR TODO: Merge HdlWorker and HdlLibraryWorker classes, rename init_configs()
#                 to init_build_configs(), rename HdlPlatformWorker's init_configs()
#                 to init_platform_configs() since it now inherits init_build_configs().
#                 Have HdlAssembly and/or its sub-classes inherit from HdlWorker.

# pylint:disable=too-many-ancestors
class HdlWorker(Worker, HdlCore):
    """
    This class represents a HDL worker.
    Examples are HDL Library Worker, HDL Platform Worker ....
    """
    def __init__(self, directory, name=None, **kwargs):
        super().__init__(directory, name, **kwargs)

    @abstractmethod
    def init_configs(self):
        """
        This function initializes the configurations of an HDL worker
        This must be implemented by the child class
            (e.g. HdlLibraryWorker and HdlPlatformWorker)
        """
        raise NotImplementedError("HdlWorker.init_configs() is not implemented")
# pylint:enable=too-many-ancestors

# pylint:disable=too-many-ancestors
class HdlLibraryWorker(HdlWorker, ReportableAsset):
    """
    An HDL Library worker is any HDL Worker that lives in a component/worker library.
    In general, this is any HDL Worker that is not an HDL Platform Worker.
    This is not a perfect name for this asset-type, but it is accurate. This is any
    HDL worker that lives in a library.

    HdlLibraryWorker instances have configurations stored in "configs" which maps configuration
    index to HdlLibraryWorkerConfig instance.
    """
    def __init__(self, directory, name=None, **kwargs):
        """
        Construct HdlLibraryWorker instance, and initialize configurations of this worker.
        Forward kwargs to configuration initialization.
        """
        super().__init__(directory, name, **kwargs)
        self.configs = {}
        self.init_configs(**kwargs)

    def build(self):
        """
        This function will build the asset, must be implemented by the child class
        """
        raise NotImplementedError("HdlLibraryWorker.build() is not implemented")

    def init_configs(self, **kwargs):
        """
        Parse this worker's build XML and populate its "configs" dictionary
        with mappings of <config-index> -> <config-instance>
        """
        # Determine if the build XML is named .build or -build.xml
        if os.path.exists(self.directory + "/" + os.path.basename(self.name) + "-build.xml"):
            build_xml = self.directory + "/" + self.name + "-build.xml"
        elif os.path.exists(self.directory + "/" + self.name + ".build"):
            build_xml = self.directory + "/" + self.name + ".build"
        else:
            # If neither is found, there is no build XML and so we assume there is only one config
            # and assign it index 0
            self.configs[0] = HdlLibraryWorkerConfig(directory=self.directory, name=self.name,
                                                     config_index=0, **kwargs)
            return

        # Begin parsing the build XML
        root = ET.parse(build_xml).getroot()
        #TODO confirm root.tag is build?

        # Find each build configuration, get the ID, get all parameters (place in dict),
        # construct the HdlLibraryWorkerConfig instance, and add it to the "configs" dict
        for config in root.findall("configuration"):
            config_id = config.get("id")
            # Confirm the ID is an integer
            if config_id is not None:
                if not ocpiutil.isint(config_id):
                    raise ocpiutil.OCPIException("Invalid configuration ID in build XML \"" +
                                                 build_xml + "\".")
            # Find elements with type "parameter", and load them into the param_dict
            # as name -> value
            param_dict = {}
            for param in config.findall("parameter") + config.findall("Parameter"):
                pname = param.get("name")
                value = param.get("value")
                param_dict[pname] = value

            # Initialize the config instance with this worker's directory and name, and the
            # configuration's ID and parameter dictionary
            if config_id:
                self.configs[int(config_id)] = HdlLibraryWorkerConfig(directory=self.directory,
                                                                      name=self.name,
                                                                      config_index=int(config_id),
                                                                      config_params=param_dict,
                                                                      **kwargs)

    def get_config_params_report(self):
        """
        Create a Report instance containing an entry for each configuration of this worker.
        Return that report. The Report's data_points member is an array that will hold
        a data-point (stored as a dictionary) for each configuration. The keys of each
        data-point/dict will be "Configuration" or parameter name, and the values are
        configuration index or parameter values.
        """
        # Initialize a report with headers matching "Configuration" and the parameter names
        report = ocpiutil.Report(ordered_headers=["Configuration"] +
                                 list(self.configs[0].param_dict.keys()))

        # For each configuration, construct a data-point with Configuration=index
        # and entries for each parameter key/value (just copy param_dict)
        for idx, config in self.configs.items():
            params = config.param_dict.copy()
            params["Configuration"] = idx
            # Append this data-point to the report
            report.append(params)
        return report

    def show_config_params_report(self):
        """
        Print out the Report of this Worker's configuration parameters.
        Each row will represent a single configuration, with each column representing
        either the Configuration index or a parameter value.

        Modes can be:
            table: plain text table to terminal
            latex: print table in LaTeX format to configurations.inc file in this
                   HdlLibraryWorker's directory
        """
        # TODO should this function and its output modes be moved into a super-class?
        dirtype = ocpiutil.get_dirtype(self.directory)
        caption = "Table of Worker Configurations for " + str(dirtype) + ": " + str(self.name)
        if self.output_format == "table":
            print(caption)
            # Print the resulting Report as a table
            self.get_config_params_report().print_table()
        elif self.output_format == "latex":
            logging.info("Generating " + caption)
            # Record the report in LaTeX in a configurations.inc file for this asset
            util_file_path = self.directory + "/configurations.inc"
            with open(util_file_path, 'w') as util_file:
                # Get the LaTeX table string, and write it to the configurations file
                latex_table = self.get_config_params_report().get_latex_table(caption=caption)
                # Only write to the file if the latex table is non-empty
                if latex_table != "":
                    util_file.write(latex_table)
                    logging.info("  LaTeX Configurations Table was written to: " + util_file_path +
                                 "\n")
        else:
            raise ocpiutil.OCPIException("Valid formats for showing worker configurations are \"" +
                                         ", ".join(self.valid_formats) + "\", but \"" +
                                         self.output_format + "\" was chosen.")


    def get_utilization(self):
        """
        Get any utilization information for this Platform Worker's Configurations

        The returned Report contains a data-point (dict) for each Configuration, stored in the
        Report instance's data_points array. Each data-point maps dimension/header to value for
        that configuration.
        """
        # Add to the default list of reportable synthesis items to report on
        ordered_headers = ["Configuration"] + hdltargets.HdlReportableToolSet.get_ordered_items()
        sort_priority = ["Configuration"] + hdltargets.HdlReportableToolSet.get_sort_priority()
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

    def show_utilization(self):
        """
        Show this worker's configurations with their parameter settings.
        Also show this worker's utilization report.
        """
        self.show_config_params_report()
        super().show_utilization()
# pylint:enable=too-many-ancestors

#TODO should implement HdlBuildableAsset
class HdlLibraryWorkerConfig(HdlCore, ReportableAsset):
    """
    A configuration of an HdlLibraryWorker. An instance
    of this class represents one combination of an HDL worker's
    build-time parameters.
    """
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HdlLibraryWorkerConfig member data and calls the super class __init__.
        valid kwargs handled at this level are:
            config_index (int) - index of this worker configuration. This dictates where the
                                 configuration's generated files will live and which build
                                 parameters map to this configuration.
        """
        super().__init__(directory, name, **kwargs)
        # We expect the config_index to be passed in via kwargs
        # These are generally defined in the worker build XML
        self.index = kwargs.get("config_index", 0)
        # The worker sub-directory starts with 'target'.
        # It is then followed by the configuration index,
        # unless the index is 0.
        if self.index == 0:
            self.subdir_prefix = directory + "/target-"
        else:
            self.subdir_prefix = directory + "/target-" + str(self.index) + "-"
        # The config_params will contain build parameters for this configuration
        # in the form: parameter-name -> value
        self.param_dict = kwargs.get("config_params", {})

    def build(self):
        """
        This function will build the asset, must be implemented by the child class
        """
        raise NotImplementedError("build() is not implemented")

    def get_utilization(self):
        """
        Get the utilization Report instance for this worker configuration
        Do so for each target provided all within a single Report

        Since a Worker Configuration is a synthesis asset, the utilization report will
        be generated with mode=synth
        """
        # Get the default list of reportable synthesis items to report on
        ordered_headers = hdltargets.HdlReportableToolSet.get_ordered_items()
        sort_priority = hdltargets.HdlReportableToolSet.get_sort_priority()
        # Initialize an empty data-set with these default headers
        util_report = ocpiutil.Report(ordered_headers=ordered_headers, sort_priority=sort_priority)
        # Add data-points to this report/set for each target
        for tgt in self.hdl_targets:
            tgtdir = self.subdir_prefix + tgt.name
            if isinstance(tgt.toolset, hdltargets.HdlReportableToolSet):
                util_report += tgt.toolset.construct_report_item(directory=tgtdir, target=tgt,
                                                                 mode="synth")
        return util_report
