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
This module a collection of OpenCPI Asset classes

The classes in this module are used for OpenCPI project structure management

Documentation and testing:
    Documentation can be viewed by running:
        $ pydoc3 ocpiassets
Note on testing:
    When adding functions to this file, add unit tests to
    opencpi/tests/pytests/*_test.py
"""
from abc import ABCMeta, abstractmethod
import os
import logging
import copy
from functools import partial
from glob import glob
import json
import sys
import shutil
import xml.etree.ElementTree as ET
import ocpiutil
import hdltargets

class AssetFactory():
    """
    This class is used for intelligent construction of supported OpenCPI Assets. Given an asset type
    and arguments, the factory will provide an instance of the relevant class.
    """
    # __assets is a dictionary to keep track of existing instances for each asset subclass. Only
    # assets that use the __get_or_create function for construction will be tracked in this dict.
    # It can be used to determine whether an instance needs to be created or already exists.
    # {
    #     <asset-subclass> : {
    #                            <directory> : <asset-subclass-instance>
    #                        }
    # }
    __assets = {}

    @classmethod
    def factory(cls, asset_type, directory, name=None, **kwargs):
        """
        Class method that is the intended wrapper to create all instances of any Asset subclass.
        Returns a constructed object of the type specified by asset_type. Throws an exception for
        an invalid type.

        Every asset must have a directory, and may provide a name and other args.
        Some assets will be created via the corresponding subclass constructor.
        Others will use an auxiliary function to first check if a matching instance
        already exists.
        """
        if not directory:
            raise ocpiutil.OCPIException("directory passed to  AssetFactory is None.  Pass a " +
                                         "valid directory to the factory")
        # actions maps asset_type string to the function that creates objects of that type
        # Some types will use plain constructors,
        # and some will use __get_or_create with asset_cls set accordingly
        actions = {"worker":         cls.__worker_with_model,
                   "hdl-assemblies": HdlAssembliesCollection,
                   "hdl-assembly":   HdlApplicationAssembly,
                   "hdl-platforms":  HdlPlatformsCollection,
                   "hdl-platform":   HdlPlatformWorker,
                   "hdl-container":  HdlContainer,
                   "test":           Test,
                   "application":    Application,
                   "applications":   ApplicationsCollection,
                   "library":        Library,
                   #"libraries":      LibrariesCollection, # TODO implement this class
                   "project":        partial(cls.__get_or_create, Project),
                   "registry":       partial(cls.__get_or_create, Registry)}

        if asset_type not in actions.keys():
            raise ocpiutil.OCPIException("Bad asset creation, \"" + asset_type + "\" not supported")

        # Call the action for this type and hand it the arguments provided
        return actions[asset_type](directory, name, **kwargs)

    @classmethod
    def __worker_with_model(cls, directory, name=None, **kwargs):
        """
        Construct Worker subclass based on authoring model of worker
        (e.g. RccWorker or HdlLibraryWorker)
        """
        if os.path.basename(directory).endswith(".hdl"):
            return HdlLibraryWorker(directory, name, **kwargs)
        elif os.path.basename(directory).endswith(".rcc"):
            return RccWorker(directory, name, **kwargs)
        else:
            raise ocpiutil.OCPIException("Unsupported authoring model for worker located at '" +
                                         directory + "'")

    @classmethod
    def remove(cls, directory=None, instance=None):
        """
        Removes an instance from the static class variable __assets by dierectory or the instance
        itself.  Throws an exception if neither optional argument is provided.
        """
        if directory is not None:
            real_dir = os.path.realpath(directory)
            dirtype_dict = {"project": Project,
                            "registry": Registry}
            dirtype = ocpiutil.get_dirtype(real_dir)
            cls.__assets[dirtype_dict[dirtype]].pop(real_dir, None)
        elif instance is not None:
            cls.__assets[instance.__class__] = {
                k:v for k, v in cls.__assets[instance.__class__].items() if v is not instance}
        else:
            raise ocpiutil.OCPIException("Invalid use of AssetFactory.remove() both directory " +
                                         "and instance are None.")

    @classmethod
    def __get_or_create(cls, asset_cls, directory, name, **kwargs):
        """
        Given an asset subclass type, check whether an instance of the
        subclass already exists for the provided directory. If so, return
        that instance. Otherwise, call the subclass constructor and return
        the new instance.
        """
        # Determine the sub-dictionary in __assets corresponding to the provided class (asset_cls)
        if asset_cls not in cls.__assets:
            cls.__assets[asset_cls] = {}
        asset_inst_dict = cls.__assets[asset_cls]

        # Does an instance of the asset_cls subclass exist with a directory matching the provided
        # "dictionary" parameter? If so, just return that instance.
        real_dir = os.path.realpath(directory)
        if real_dir not in asset_inst_dict:
            # If not, construct a new one, add it to the dictionary, and return it
            asset_inst_dict[real_dir] = asset_cls(directory, name, **kwargs)
        return asset_inst_dict[real_dir]

class Asset(metaclass=ABCMeta):
    #TODO add project top and package id as a variable here, maybe this becomes a method instead
    """
    Parent Class for all Asset objects.  Contains a factory to create each of the asset types.
    Not officially a virtual class but objects of this class are not intended to be directly
    created.
    """
    valid_authoring_models = ["rcc", "hdl"]
    valid_settings = []

    def __init__(self, directory, name=None, **kwargs):
        """
        initializes Asset member data valid kwargs handled at this level are:
            verbose (T/F) - be verbose with output
            name - Optional argument that specifies the name of the asset if not set defaults to the
                   basename of the directory argument
            directory - The location on the file system of the asset that is being constructed.
                        both relative and global file paths are valid.
        """
        if not name:
            self.name = os.path.basename(directory)
        else:
            self.name = name
        self.directory = os.path.realpath(directory)
        self.verbose = kwargs.get("verbose", False)

    @classmethod
    def get_valid_settings(cls):
        """
        Recursive class method that gathers all the valid settings static lists of the current
        class's base classes and combines them into a single set to return to the caller
        """
        ret_val = cls.valid_settings

        for base_class in cls.__bases__:
            # prevents you from continuing up the class hierarchy to "object"
            if callable(getattr(base_class, "get_valid_settings", None)):
                # pylint:disable=no-member
                ret_val += base_class.get_valid_settings()
                # pylint:enable=no-member

        return set(ret_val)

    def get_settings(self):
        """
        Generic method that returns a dictionary of settings associated with a single run or build
        of an object.  valid settings are set at the subclass level and any member variable that
        is not in this list or is not set(equal to None) are removed from the dictionary
        """
        settings_list = copy.deepcopy(vars(self))
        # list constructor is required here because the original arg_list is being
        # changed and we can't change a variable we are iterating over
        for setting, value in list(settings_list.items()):
            if (value in [None, False]) or (setting not in self.get_valid_settings()):
                del settings_list[setting]

        return settings_list

    @staticmethod
    def check_dirtype(dirtype, directory):
        """
        checks that the directory passed in is of the type passed in and if not an exception is
        thrown
        """
        if not os.path.isdir(directory):
            raise ocpiutil.OCPIException("Expected directory of type \"" + dirtype + "\" for a " +
                                         "directory that does not exist \"" + directory + "\"")

        if ocpiutil.get_dirtype(directory) != dirtype:
            raise ocpiutil.OCPIException("Expected directory of type \"" + dirtype + "\", but " +
                                         "found type \"" + str(ocpiutil.get_dirtype(directory)) +
                                         "\" for directory \"" + directory + "\"")
    #@abstractmethod
    def delete(self, force=False):
        """
        Remove the Asset from disk.  Any additional cleanup on a per asset basis can be done in
        the child implementations of this function
        """
        if not force:
            prompt = ("removing " + ocpiutil.get_dirtype(self.directory) + " at directory: " +
                      self.directory)
            force = ocpiutil.get_ok(prompt=prompt)
        if force:
            shutil.rmtree(self.directory)

class BuildableAsset(Asset):
    """
    Virtual class that requires that any child classes implement a build method.  Contains settings
    that are specific to all assets that can be run
    """
    valid_settings = ["only_plat", "ex_plat"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes BuildableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            ex_plat (list) - list of platforms(strings) to exclude from a build
            only_plat (list) - list of the only platforms(strings) to build for
        """
        super().__init__(directory, name, **kwargs)
        self.ex_plat = kwargs.get("ex_plat", None)
        self.only_plat = kwargs.get("only_plat", None)

    @abstractmethod
    def build(self):
        """
        This function will build the asset, must be implemented by the child class
        """
        raise NotImplementedError("BuildableAsset.build() is not implemented")

class HDLBuildableAsset(BuildableAsset):
    """
    Virtual class that requires that any child classes implement a build method.  Contains settings
    that are specific to all assets that can be run
    """
    valid_settings = ["hdl_plats", "hdl_tgts"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HDLBuildableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            hdl_plats (list) - list of hdl platforms(strings) to build for
            hdl_tgts  (list) - list of hdl targets(strings) to build for
        """
        super().__init__(directory, name, **kwargs)

        # Collect the string lists of HDL Targets and Platforms from kwargs
        hdl_tgt_strs = kwargs.get("hdl_tgts", None)
        hdl_plat_strs = kwargs.get("hdl_plats", None)

        # Initialize the lists of hdl targets/platforms to empty sets
        # Note that they are sets instead of lists to easily avoid duplication
        #    Also note that there is no literal for the empty set in python3.4
        #    because {} is for dicts, so set() must be used.
        self.hdl_targets = set()
        self.hdl_platforms = set()

        # If there were HDL Targets provided, construct HdlTarget object for each
        # and add to the hdl_targets set
        if hdl_tgt_strs is not None:
            if "all" in hdl_tgt_strs:
                self.hdl_targets  = hdltargets.HdlToolFactory.get_or_create_all("hdltarget")
            else:
                for tgt in hdl_tgt_strs:
                    self.hdl_targets.add(hdltargets.HdlToolFactory.factory("hdltarget", tgt))
        # If there were HDL Platforms provided, construct HdlPlatform object for each
        # and add to the hdl_platforms set.
        # Also get the corresponding HdlTarget and add to the hdl_targets set
        if hdl_plat_strs is not None:
            if "all" in hdl_plat_strs:
                self.hdl_platforms = hdltargets.HdlToolFactory.get_or_create_all("hdlplatform")
                self.hdl_targets = hdltargets.HdlToolFactory.get_or_create_all("hdltarget")
            else:
                for plat in hdl_plat_strs:
                    plat = hdltargets.HdlToolFactory.factory("hdlplatform", plat)
                    self.hdl_platforms.add(plat)
                    self.hdl_targets.add(plat.target)

    @abstractmethod
    def build(self):
        """
        This function will build the asset, must be implemented by the child class
        """
        raise NotImplementedError("BuildableAsset.build() is not implemented")

class RCCBuildableAsset(BuildableAsset):
    """
    Virtual class that requires that any child classes implement a build method.  Contains settings
    that are specific to all assets that can be run
    """
    valid_settings = ["rcc_plat"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HDLBuildableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            rcc_plat (list) - list of rcc platforms(strings) to build for
        """
        super().__init__(directory, name, **kwargs)
        self.rcc_plat = kwargs.get("rcc_plat", None)

    @abstractmethod
    def build(self):
        """
        This function will build the asset, must be implemented by the child class
        """
        raise NotImplementedError("BuildableAsset.build() is not implemented")

class RunnableAsset(Asset):
    """
    Virtual class that requires that any child classes implement a run method.
    """
    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes RunnableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            None
        """
        super().__init__(directory, name, **kwargs)

    @abstractmethod
    def run(self):
        """
        This function will run the asset must be implemented by the child class
        """
        raise NotImplementedError("RunnableAsset.run() is not implemented")


class ShowableAsset(Asset):
    """
    Virtual class that requires that any child classes implement a show function
    """
    @abstractmethod
    def show(self, **kwargs):
        """
        This function will show this asset must be implemented by the child class
        """
        raise NotImplementedError("ShowableAsset.show() is not implemented")

    #@classmethod
    #@abstractmethod
    #def showall(cls, options, only_registry):
    #    """
    #    This function will show all assets of this type, must be implemented by the child class
    #    """
    #    raise NotImplementedError("ShowableAsset.showall() is not implemented")


class ReportableAsset(Asset):
    """
    Skeleton class providing get/show_utilization functions
    for reporting utilization of an asset.

    get_utilization is generally overridden by subclasses, but
    show_utilization is usually only overridden for subclasses that are collections
    of OpenCPI assets (e.g. ones where show_utilization is called for children assets).
    """
    valid_formats = ["table", "latex"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HDLReportableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            output_format (str) - mode to output utilization info (table, latex)
                                  output_formats not yet implenented: simple, json, csv
        """
        super().__init__(directory, name, **kwargs)

        self.output_format = kwargs.get("output_format", "table")

    def get_utilization(self):
        """
        This is a placeholder function will be the function that returns ocpiutil.Report instance
        for this asset. Subclasses should override this function to collect utilization information
        for this asset into a Report object to return.

        The returned Report contains data_points, which is an array of dictionaries. Each
        dict is essentially a data-point mapping dimension/header to value for that point.
        Reports also have metadata regarding ordering & sorting, and can be displayed in
        various formats.
        """
        raise NotImplementedError("ReportableAsset.get_utilization() is not implemented")

    def show_utilization(self):
        """
        Show the utilization Report for this asset and print/record the results.

        This default behavior is likely sufficient, but subclasses that are collections of OpenCPI
        assets may override this function to instead iterate over children assets and call their
        show_utilization functions.
        """
        # Get the directory type to add in the header/caption for the utilization info
        dirtype = ocpiutil.get_dirtype(self.directory)
        if dirtype is None:
            dirtype = "asset"
        caption = "Resource Utilization Table for " + dirtype + " \"" + self.name + "\""

        # Get the utilization using this class' hopefully overridden get_utilization() function
        util_report = self.get_utilization()
        if not util_report:
            logging.warning("Skipping " + caption + " because the report is empty for \n" +
                            "\tHDL Platforms: " + str([str(p) for p in self.hdl_platforms]) +
                            " and\n\tHDL Targets:   " + str([str(p) for p in self.hdl_targets]))
            return



        if self.output_format not in self.valid_formats:
            raise ocpiutil.OCPIException("Valid formats for showing utilization are \"" +
                                         ", ".join(self.valid_formats) + "\", but \"" +
                                         self.output_format + "\" was chosen.")
        if self.output_format == "table":
            print(caption)
            # Maybe Report.print_table() should accept caption as well?
            # print the Report as a table
            util_report.print_table()
        if self.output_format == "latex":
            logging.info("Generating " + caption)
            # Record the utilization in LaTeX in a utilization.inc file for this asset
            util_file_path = self.directory + "/utilization.inc"
            with open(util_file_path, 'w') as util_file:
                # Get the LaTeX table string, and write it to the utilization file
                latex_table = util_report.get_latex_table(caption=caption)
                # Only write to the file if the latex table is non-empty
                if latex_table != "":
                    util_file.write(latex_table)
                    logging.info("  LaTeX Utilization Table was written to: " + util_file_path + "\n")

# Skeleton class
class Worker(Asset):
    """
    Any OpenCPI Worker. This class is authoring model agnostic and represents all workers of any
    type. In general, something is a worker if it has an OWD (OpenCPI Worker Description File),
    and implements an OCS (OpenCPI Component Specification).
    """
    def __init__(self, directory, name=None, **kwargs):
        if name is None:
            name = os.path.basename(directory).rsplit('.', 1)[0]
        super().__init__(directory, name, **kwargs)

    @staticmethod
    def get_authoring_model(directory):
        """
        Each worker has an Authoring Model. Given a worker directory, return its Authoring Model.
        """
        # Worker directories end in ".<authoring-model>", so use splitext to get the
        # directories extension
        _, ext = os.path.splitext(os.path.realpath(directory))
        # Return the extension/model without the leading period
        return ext[1:]

# Placeholder class
class RccWorker(Worker):
    """
    This class represents a RCC worker.
    """
    def __init__(self, directory, name=None, **kwargs):
        super().__init__(directory, name, **kwargs)

class HdlCore(HDLBuildableAsset):
    """
    This represents any buildable HDL Asset that is core-like (i.e. is not a primitive library).

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
#                 Have HdlAssembly and/or its subclasses inherit from HdlWorker.

# Skeleton class
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

            # Initialize the config instance with this worker's dir and name, and the
            # configuration's ID and parameter dictionary
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
        # TODO should this function and its output modes be moved into a superclass?
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
                # Add this dataset to the list of utilization dictionaries. It will serve
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
        # The worker subdirectory starts with 'target'.
        # It is then followed by the configuration index,
        # unless the index is 0.
        if self.index == 0:
            self.subdir_prefix = directory + "/target-"
        else:
            self.subdir_prefix = directory + "/target-" + str(self.index) + "-"
        # The config_params will contain build parameters for this configuration
        # in the form: parameter-name -> value
        self.param_dict = kwargs.get("config_params", {})

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

class HdlAssembly(HdlCore):
    """
    HDL Assembly: any collection/integration of multiple HDL Cores/Workers
    Examples include:
                     HdlApplicationAssembly
                     HdlPlatformConfiguration
                     HdlContainer[Implementation]
    """
    def __init__(self, directory, name=None, **kwargs):
        super().__init__(directory, name, **kwargs)
        # TODO Collect list of included HdlCores


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

class HdlApplicationAssembly(HdlAssembly, ReportableAsset):
    """
    The classic user-facing HDL Assembly. This is a collection of
    HDL Application/Library Workers. An HdlApplicationAssembly
    may reference the HDL containers that it supports
    """

    def __init__(self, directory, name=None, **kwargs):
        super().__init__(directory, name, **kwargs)
        # container maps container name/XML to HdlContainer object
        self.containers = {}

    # TODO: Should we just init for all platforms in constructor, or wait for
    # user to call some function that requires containers and enforce that they
    # provide platforms as an argument?
    def collect_container_info(self, platforms):
        """
        Determine which containers exist that support the provided platforms.
        Each container will have a simple XML name and a full/implementation name.
        Each implementation is bound to a platform and platform configuration.

        Return the information in a dictionary with format:
            container_impl_dict =
                {
                    xml/simple-name : {
                                       impl-name : HdlPlatform
                                      }
                }
        """
        container_impl_dict = {}
        # Loop through the provided platforms, and determine what containers,
        # container implementations, and their corresponding platforms are
        # available to this assembly.
        for plat in platforms:
            plat_string = "HdlPlatform=" + plat.name
            # NOTE: We need to call make with a single HdlPlatform set because otherwise
            #       hdl-pre calls multiple sub-make commands and causes complications
            assemb_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                      mk_arg=plat_string +
                                                             " shellhdlassemblyvars " +
                                                             "ShellHdlAssemblyVars=1",
                                                      verbose=True)
            if "Containers" not in assemb_vars:
                raise ocpiutil.OCPIException("Could not get list of HDL containers from " +
                                             "directory\"" + self.directory + "\"")
            # We get a top-level list of containers (e.g. container XMLs)
            for cname in assemb_vars['Containers']:
                # Get the list of container IMPLEMENTATIONS for this container XML name
                container_impls_str = "HdlContainerImpls_" + cname

                # Each container will have a dictionary for its implementations
                # Each dict will map implementation-name to supported platform
                if cname not in container_impl_dict:
                    # initialize implementation dict to {} for this container
                    container_impl_dict[cname] = {}

                # Ensure that the list of container implementations is found
                if container_impls_str in assemb_vars:
                    # For each container implementation, determine the corresponding HDL platform
                    for impl in assemb_vars[container_impls_str]:
                        container_plat = "HdlPlatform_" + impl
                        # Construct this implementation's platform and add the mapping to the
                        # implementation dictionary
                        plat_obj = hdltargets.HdlToolFactory.factory("hdlplatform",
                                                                     assemb_vars[container_plat][0])
                        container_impl_dict[cname][impl] = plat_obj

            container_impls_str = "HdlBaseContainerImpls"
            # Need to do one more pass to collect information about the base container (if used)
            # and collect its implementations as well
            if container_impls_str in assemb_vars and assemb_vars['HdlBaseContainerImpls']:
                if "base" not in container_impl_dict:
                    # initialize the implementation dict to {} for the base container
                    container_impl_dict["base"] = {}
                # For each implementation of the base container, determine the corresponding
                # HDL platform
                for cname in assemb_vars[container_impls_str]:
                    # Construct this implementation's platform and add the mapping to the
                    # implementation dictionary
                    container_plat = "HdlPlatform_" + cname
                    plat_obj = hdltargets.HdlToolFactory.factory("hdlplatform",
                                                                 assemb_vars[container_plat][0])
                    container_impl_dict["base"][cname] = plat_obj

        return container_impl_dict

    def init_containers(self, platforms):
        """
        Initialize the container instances mapped to by this assembly.
        First collect the container information, next construct each instance.

        Result is a dictionary (self.containers) mapping container-name to
        container object.
        """
        container_impl_dict = self.collect_container_info(platforms)

        for cname, impls in container_impl_dict.items():
            # It would be best if the AssetFactory used _get_or_create for
            # containers, but it cannot because it uses class type and directory
            # to determine uniqueness, and multiple containers share the same assembly dir
            self.containers[cname] = AssetFactory.factory("hdl-container", self.directory,
                                                          name=cname, container_impls=impls)

    def get_utilization(self):
        """
        Get utilization report for this ApplicationAssembly.
        Get the utilization for each of container this assembly references.

        The returned Report contains a data-point (dict) for each Container, stored in the
        Report instance's data_points array. Each data-point maps dimension/header to value for
        that container.
        """
        self.init_containers(self.hdl_platforms)
        # Sort based on container name
        util_report = ocpiutil.Report()
        for cname in sorted(self.containers):
            # Get the dictionaries of utilization report items for this container.
            # Each dictionary returned corresponds to one implementation of this
            # container, and serves as a single data-point/row.
            # Add all data-points for this container to the running list
            util_report += self.containers[cname].get_utilization()
        return util_report

class HdlAssembliesCollection(HDLBuildableAsset, ReportableAsset):
    """
    This class represents an OpenCPI Project's hdl/assemblies directory
    Instances of this class can contain a list of contained assemblies in "assembly_list"
    """

    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes assemblies collection member data and calls the super class __init__. Throws
        an exception if the directory passed in is not a valid  hdl-assemblies directory.
        valid kwargs handled at this level are:
            init_hdl_assembs (T/F) - Instructs the method whether to construct all
                                     HdlApplicationAssembly objects contained in the project
        """
        self.check_dirtype("hdl-assemblies", directory)
        super().__init__(directory, name, **kwargs)

        self.assembly_list = []
        if kwargs.get("init_hdlassembs", False):
            logging.debug("HdlAssembliesCollection constructor creating HdlApplicationAssembly " +
                          "Objects")
            for assemb_directory in self.get_valid_assemblies():
                self.assembly_list.append(AssetFactory.factory("hdl-assembly", assemb_directory,
                                                               **kwargs))

    def get_valid_assemblies(self):
        """
        Probes make in order to determine the list of active assemblies in the
        assemblies collection
        """
        assembs_list = []
        ocpiutil.logging.debug("Getting valid assemblies from: " + self.directory + "/Makefile")
        make_assembs = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                   mk_arg="ShellAssembliesVars=1 showassemblies",
                                                   verbose=True)["Assemblies"]

        # Collect list of assembly directories
        for name in make_assembs:
            assembs_list.append(self.directory + "/" + name)
        return assembs_list

    def show_utilization(self):
        """
        Show utilization separately for each assembly in this collection
        """
        for assembly in self.assembly_list:
            assembly.show_utilization()


    #placeholder function
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("HdlAssembliesCollection.build() is not implemented")


class HdlContainer(HdlAssembly, ReportableAsset):
    """
    Instances of this class represent HDL Containers, which are specified in XML
    and are a special type of HDL Assembly that includes a platform configuration
    and an application assembly as well as other workers.

    An HDL Container can also be implied (e.g. the base/default container)

    Containers have various implementations which are bound to platform configurations.
    These implementations are storded in an instance field "implementations" that maps
    HDL Container Implementation name to HdlContainerImplementation instance.
    """

    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HdlContainer member data and calls the super class __init__.
        valid kwargs handled at this level are:
            container_impls (dict) - Dictionary mapping container implementation name
                                     to the HdlPlatform that it maps to.
        """
        super().__init__(directory, name, **kwargs)

        # Maps container implementation names to their corresponding
        # HdlContainerImplementation instances
        self.implementations = {}
        # We expect to see a 'container_impls' argument that is a dictionary mapping
        # full container implementation names to the platforms they support
        for impl_name, plat in kwargs.get("container_impls", {}).items():
            impl_dir = self.directory + "/container-" + impl_name
            self.implementations[impl_name] = HdlContainerImplementation(directory=impl_dir,
                                                                         name=impl_name,
                                                                         platform=plat)

    def get_utilization(self):
        """
        Get the utilization Report for this Container. It will be combination of reports for
        each container implementation with a leading "Container" column

        The returned Report contains a data-point (dict) for each container implementation,
        stored in the Report instance's data_points array.
        Each data-point maps dimension/header to value for that implementation.
        """
        # Add to the default list of reportable synthesis items to report on
        ordered_headers = ["Container"] + hdltargets.HdlReportableToolSet.get_ordered_items("impl")
        sort_priority = hdltargets.HdlReportableToolSet.get_sort_priority() + ["Container"]
        # Initialize an empty data-set with these default headers
        util_report = ocpiutil.Report(ordered_headers=ordered_headers, sort_priority=sort_priority)
        for impl in self.implementations.values():
            # Get the dictionary that maps utilization report "headers" to discovered "values"
            # Do this for the current container implementation
            sub_report = impl.get_utilization()

            # If the container implementation returns a dict of utilization information,
            # add it to the list
            if sub_report:
                # We want to add the container name as a report element
                sub_report.assign_for_all_points(key="Container", value=self.name)
                # Add this dataset to the list of utilization dictionaries. It will serve
                # as a single data-point/row in the report
                util_report += sub_report
        return util_report

class HdlContainerImplementation(HdlAssembly):
    """
    Instances of this class represent specific implementations of an HDL Container
    A Container Implementation is bound to a single Platform and even a single
    Platform Configuration
    """
    #TODO map to a platform configuration? At least get name from make
    def __init__(self, directory, name, **kwargs):
        """
        Initializes HdlContainerImplementation member data and calls the super class __init__.
        valid kwargs handled at this level are:
            platform (HdlPlatform) - The HdlPlatform object that is bound to this implementation.
        """
        super().__init__(directory, name, **kwargs)
        self.platform = kwargs.get("platform", None)
        # The subdirectory where target-specific artifacts will be made for this
        # container implementation
        self.target_subdir = self.directory + "/target-" + self.platform.target.name

    def get_utilization(self):
        """
        Generate a report/dataset containing the utilization for this implementation
        This will basically just be a report with a single datapoint in it

        The returned Report contains a single data-point (dict) for this container implementation,
        stored in the Report instance's data_points array.
        The data-point maps dimension/header to value for that implementation.

        Since a Container Implementation is an implementation asset, the utilization report will
        be generated with mode=impl
        """
        if isinstance(self.platform.target.toolset, hdltargets.HdlReportableToolSet):
            return self.platform.target.toolset.construct_report_item(directory=self.target_subdir,
                                                                      platform=self.platform,
                                                                      mode="impl")

class Test(RunnableAsset, HDLBuildableAsset, RCCBuildableAsset):
    """
    This class represents an OpenCPI Component Unit test.  Contains build/run settings that are
    specific to Tests.
    """
    valid_settings = ["keep_sims", "acc_errors", "case", "verbose", "remote_test_sys", "view"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes Test member data  and calls the super class __init__. Throws an exception if
        the directory passed in is not a valid test directory.
        valid kwargs handled at this level are:
            keep_sims (T/F) - Keep HDL simulation files for any simulation platforms
            acc_errors (T/F) - Causes errors to accumulate and tests to continue on
            case (list) - Specify Which test cases that will be run/verified
            mode (list) - Specify which phases of the unit test to run
            remote_test_sys (list) - Specify remote systems to run the test(s)
        """
        self.check_dirtype("test", directory)
        super().__init__(directory, name, **kwargs)

        self.keep_sims = kwargs.get("keep_sims", False)
        self.view = kwargs.get("view", False)
        self.acc_errors = kwargs.get("acc_errors", False)
        self.case = kwargs.get("case", None)
        self.mode = kwargs.get("mode", "all")
        self.remote_test_sys = kwargs.get("remote_test_sys", None)

        # using all instead of build so that old style unit tests wont blow up
        # all and build will evaluate to the same make target
        self.mode_dict = {}
        # pylint:disable=bad-whitespace
        self.mode_dict['gen_build']       = ["all"]
        self.mode_dict['prep_run_verify'] = ["run"]
        self.mode_dict['clean_all']       = ["clean"]
        self.mode_dict['prep']            = ["prepare"]
        self.mode_dict['run']             = ["runnoprepare"]
        self.mode_dict['prep_run']        = ["runonly"]
        self.mode_dict['verify']          = ["verify"]
        self.mode_dict['view']            = ["view"]
        self.mode_dict['gen']             = ["generate"]
        self.mode_dict['clean_run']       = ["cleanrun"]
        self.mode_dict['clean_sim']       = ["cleansim"]
        self.mode_dict['all']             = ["all", "run"]
        # pylint:enable=bad-whitespace

    def run(self):
        """
        Runs the Test with the settings specified in the object
        """
        return ocpiutil.execute_cmd(self.get_settings(),
                                    self.directory,
                                    self.mode_dict[self.mode])

    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("Test.build() is not implemented")

class Application(RunnableAsset, RCCBuildableAsset):
    """
    This class represents an OpenCPI ACI Application.
    """
    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes Application member data  and calls the super class __init__.  Throws an
        exception if the directory passed in is not a valid application directory.
        valid kwargs handled at this level are:
            None
        """
        self.check_dirtype("application", directory)
        super().__init__(directory, name, **kwargs)

    def run(self):
        """
        Runs the Application with the settings specified in the object
        """
        return ocpiutil.execute_cmd(self.get_settings(),
                                    self.directory, ["run"])

    #placeholder function
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("Application.build() is not implemented")

class ApplicationsCollection(RunnableAsset, RCCBuildableAsset):
    """
    This class represents an OpenCPI applications directory.  Ability act on multiple applications
    with a single instance are located in this class.
    """
    valid_settings = ["run_before", "run_after", "run_arg"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes ApplicationsCoclletion member data  and calls the super class __init__.
        Throws an exception if the directory passed in is not a valid applications directory.
        valid kwargs handled at this level are:
            run_before (list) - Arguments to insert before the ACI executable or ocpirun
            run_after (list) - Arguments to insert at the end of the execution command line A
            run_arg (list) - Arguments to insert immediately after the ACI executable or ocpirun
        """
        self.check_dirtype("applications", directory)
        super().__init__(directory, name, **kwargs)

        self.run_before = kwargs.get("run_before", None)
        self.run_after = kwargs.get("run_after", None)
        self.run_arg = kwargs.get("run_arg", None)

    def run(self):
        """
        Runs the ApplicationsCollection with the settings specified in the object.  Running a
        ApplicationsCollection will run all the applications that are contained in the
        ApplicationsCollection
        """
        return ocpiutil.execute_cmd(self.get_settings(),
                                    self.directory, ["run"])

    #placeholder function
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("ApplicationsCollection.build() is not implemented")

class Library(RunnableAsset, RCCBuildableAsset, HDLBuildableAsset, ReportableAsset):
    """
    This class represents an OpenCPI Library.  Contains a list of the tests that are in this
    library and can be initialized or left as None if not needed
    """
    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes Library member data  and calls the super class __init__.  Throws an
        exception if the directory passed in is not a valid library directory.
        valid kwargs handled at this level are:
            init_tests   (T/F) - Instructs the method weather to construct all test objects contained
                                 in the library
            init_workers (T/F) - Instructs the method weather to construct all worker objects contained
                                 in the library
        """
        self.check_dirtype("library", directory)
        super().__init__(directory, name, **kwargs)
        self.test_list = None
        if kwargs.get("init_tests", False):
            self.test_list = []
            logging.debug("Library constructor creating Test Objects")
            for test_directory in self.get_valid_tests():
                self.test_list.append(AssetFactory.factory("test", test_directory, **kwargs))

        self.worker_list = None
        if kwargs.get("init_workers", False):
            # Collect the list of workers and initialize Worker objects for each worker
            # of a supported authoring model
            self.worker_list = []
            logging.debug("Library constructor creating Worker Objects")
            for worker_directory in self.get_valid_workers():
                auth = Worker.get_authoring_model(worker_directory)
                if auth not in Asset.valid_authoring_models:
                    logging.debug("Skipping worker \"" + directory +
                                  "\" with unsupported authoring model \"" + auth + "\"")
                else:
                    wkr_name = os.path.splitext(os.path.basename(worker_directory))[0]
                    self.worker_list.append(AssetFactory.factory("worker", worker_directory,
                                                                 name=wkr_name,
                                                                 **kwargs))

        self.package_id = Library.get_package_id(self.directory)

    @staticmethod
    def get_package_id(directory='.'):
        lib_vars = ocpiutil.set_vars_from_make(mk_file=directory + "/Makefile",
                                               mk_arg="ShellLibraryVars=1 showpackage",
                                               verbose=True)
        return "".join(lib_vars['Package'])

    def get_valid_workers(self):
        """
        Probes make in order to determine the list of active tests in the library
        """
        workers_list = []
        ocpiutil.logging.debug("Getting valid workers from: " + self.directory + "/Makefile")
        # Ask Make for list of Workers in this library
        make_workers = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                   mk_arg="ShellLibraryVars=1 showworkers",
                                                   verbose=True)["Workers"]

        # For each worker in the list, append the worker's directory to the list and return it
        for name in make_workers:
            workers_list.append(self.directory + "/" + name)
        return workers_list

    def get_valid_tests(self):
        """
        Probes make in order to determine the list of active tests in the library
        """
        ret_val = []
        ocpiutil.logging.debug("Getting valid tests from: " + self.directory + "/Makefile")
        make_tests = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                 mk_arg="ShellLibraryVars=1 showtests",
                                                 verbose=True)["Tests"]

        for name in make_tests:
            ret_val.append(self.directory + "/" + name)
        return ret_val

    def run(self):
        """
        Runs the Library with the settings specified in the object.  Throws an exception if the
        tests were not initialized by using the init_tests variable at initialization.  Running a
        Library will run all the component unit tests that are contained in the Library
        """
        ret_val = 0
        if self.test_list is None:
            raise ocpiutil.OCPIException("For a Library to be run \"init_tests\" must be set to " +
                                         "True when the object is constructed")
        for test in self.test_list:
            run_val = test.run()
            ret_val = ret_val + run_val
        return ret_val

    def show_utilization(self):
        """
        Show utilization separately for each HdlWorker in this library
        """
        for worker in self.worker_list:
            if isinstance(worker, HdlWorker):
                worker.show_utilization()

    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("Library.build() is not implemented")

# TODO: Should also extend CreatableAsset, ShowableAsset
class Registry(Asset):
    """
    The Registry class represents an OpenCPI project registry. As an OpenCPI
    registry contains project-package-ID named symlinks to project directories,
    registry instances contain dictionaries mapping package-ID to project instances.
    Projects can be added or removed from a registry
    """

    def __init__(self, directory, name=None, **kwargs):
        super().__init__(directory, name, **kwargs)

        # Each registry instance has a list of projects registered within it.
        # Initialize this list by probing the filesystem for links that exist
        # in the registry dir.
        # __projects maps package-ID --> project instance
        self.__projects = {}
        for proj in glob(self.directory + '/*'):
            pid = os.path.basename(proj)
            self.__projects[pid] = AssetFactory.factory("project", proj) if os.path.exists(proj) \
                                   else None

    def contains(self, package_id=None, directory=None):
        """
        Given a project's package-ID or directory, determine if the project is present
        in this registry.
        """
        # If neither package_id nor directory are provided, exception
        if package_id is None:
            if directory is None:
                raise ocpiutil.OCPIException("Could determine whether project exists because " +
                                             "the package_id and directory provided were both " +
                                             "None.\nProvide one or both of these arguments.")
            # Project's package-ID is determined from its directory if not provided
            package_id = ocpiutil.get_project_package(directory)
            if package_id is None:
                raise ocpiutil.OCPIException("Could not determine package-ID of project located " +
                                             "at \"" + directory + "\".\nDouble check this " +
                                             "configurations.")

        # If the project is registered here by package-ID, return True.
        # If not, or if a different project is registered here with that pacakge-ID, return False
        if package_id in self.__projects:
            # pylint:disable=bad-continuation
            if (directory is not None and
                os.path.realpath(directory) != self.__projects[package_id].directory):
                logging.warning("Registry at \"" + self.directory + "\" contains a project with " +
                                "package-ID \"" + package_id + "\", but it is not the same " +
                                "project as \"" + directory + "\".")
                return False
            # pylint:enable=bad-continuation
            return True
        return False

    def add(self, directory="."):
        """
        Given a project, get its package-ID, create the corresponding link in the project registry,
        and add it to this Registry instance's __projects dictionary.
        If a project with the same package-ID already exists in the registry, fail.
        """
        if not ocpiutil.is_path_in_project(directory):
            raise ocpiutil.OCPIException("Failure to register project.  Project \"" + directory +
                                         "\" in location: " + os.getcwd() + " is not in a " +
                                         "project or does not exist.")

        project = AssetFactory.factory("project", directory)
        pid = project.package_id

        if pid == "local":
            raise ocpiutil.OCPIException("Failure to register project. Cannot register a " +
                                         "project with package-ID 'local'.\nSet the  " +
                                         "PackageName, PackagePrefix and/or Package " +
                                         "variables in your Project.mk.")
        if pid in self.__projects and self.__projects[pid] is not None:
            # If the project is already registered and is the same
            if self.__projects[pid].directory == project.directory:
                logging.debug("Project link is already in the registry. Proceeding...")
                return
            raise ocpiutil.OCPIException("Failure to register project with package '" + pid +
                                         "'.\nA project/link with that package qualifier " +
                                         "already exists and is registered in '" + self.directory +
                                         "'.\nThe old project is not being overwitten to" +
                                         " unregister the original project, call: 'ocpidev " +
                                         "unregister project " + pid +"'.\nThen, run the " +
                                         "command: 'ocpidev -d " + project.directory +
                                         " register project'")

        # link will be created at <registry>/<package-ID>
        project_link = self.directory + "/" + pid

        # if this statement is reached and the link exists, it is a broken link
        if os.path.lexists(project_link):
            # remove the broken link that would conflict
            self.remove_link(pid)

        # Perform the actual registration: create the symlink to the project in this registry dir
        self.create_link(project)
        # Add the project to this registry's projects dictionary
        self.__projects[project.package_id] = project

    def remove(self, package_id=None, directory=None):
        """
        Given a project's package-ID or directory, determine if the project is present
        in this registry. If so, remove it from this registry's __projects dictionary
        and remove the registered symlink.
        """
        if package_id is None:
            package_id = ocpiutil.get_project_package(directory)
            if package_id is None:
                raise ocpiutil.OCPIException("Could not unregister project located at \"" +
                                             directory + "\" because the project's package-ID " +
                                             "could not be determined.\nIs it really a project?")

        if package_id not in self.__projects:
            link_path = self.directory + "/" + package_id
            if os.path.exists(link_path) and not os.path.exists(os.readlink(link_path)):
                logging.debug("Removing the following broken link from the registry:\n" +
                              link_path + " -> " + os.readlink(link_path))
                self.remove_link(package_id)
                return
            raise ocpiutil.OCPIException("Could not unregister project with package-ID \"" +
                                         package_id + "\" because the project is not in the " +
                                         "registry.\n Run 'ocpidev show registry --table' for " +
                                         "information about the currently registered projects.\n")

        project_link = self.__projects[package_id].directory
        if directory is not None and os.path.realpath(directory) != project_link:
            raise ocpiutil.OCPIException("Failure to unregister project with package '" +
                                         package_id + "'.\nThe registered project with link '" +
                                         package_id + " --> " + project_link + "' does not " +
                                         "point to the specified project '" +
                                         os.path.realpath(directory) + "'." + "\nThis project " +
                                         "does not appear to be registered.")

        # Remove the symlink registry/package-ID --> project
        self.remove_link(package_id)
        # Remove the project from this registry's dict
        self.__projects.pop(package_id)

    def create_link(self, project):
        """
        Create a link to the provided project in this registry
        """
        # Try to make the path relative. This helps with environments involving mounted directories
        # Find the path that is common to the project and registry
        common_prefix = os.path.commonprefix([project.directory, self.directory])
        # If the two paths contain no common directory except root,
        #     use the path as-is
        # Otherwise, use the relative path from the registry to the project
        if common_prefix == '/' or common_prefix == '':
            project_to_reg = os.path.normpath(project.directory)
        else:
            project_to_reg = os.path.relpath(os.path.normpath(project.directory), self.directory)

        project_link = self.directory + "/" + project.package_id
        try:
            os.symlink(project_to_reg, project_link)
        except OSError:
            raise ocpiutil.OCPIException("Failure to register project link: " +  project_link +
                                         " --> " + project_to_reg + "\nCommand attempted: " +
                                         "'ln -s " + project_to_reg + " " + project_link +
                                         "'.\nTo (un)register projects in " +
                                         "/opt/opencpi/project-registry, you need to be a " +
                                         "member of the opencpi group.")

    def remove_link(self, package_id):
        """
        Remove link with name=package-ID from this registry
        """
        link_path = self.directory + "/" + package_id
        try:
            os.unlink(link_path)
        except OSError:
            raise ocpiutil.OCPIException("Failure to unregister link to project: " + package_id +
                                         " --> " + os.readlink(link_path) + "\nCommand " +
                                         "attempted: 'unlink " + link_path + "'\nTo " +
                                         "(un)register projects in " +
                                         "/opt/opencpi/project-registry, you need to be a " +
                                         "member of the opencpi group.")

    def get_project(self, package_id):
        """
        Return the project with the specified package-id that is registered in this registry
        """
        if package_id not in self.__projects:
            raise ocpiutil.OCPIException("\"" + package_id + "\" is not a valid package-id or " +
                                         "project directory")
        return self.__projects[package_id]

    @staticmethod
    def create(asset_dir="."):
        print("making: " + asset_dir)
        os.mkdir(asset_dir)
        return AssetFactory.factory("registry", asset_dir)

    @staticmethod
    def get_default_registry_dir():
        """
        Get the default registry from the environment setup. Check in the following order:
        OCPI_PROJECT_REGISTRY_DIR, OCPI_CDK_DIR/../project-registry or /opt/opencpi/project-registry
        """
        project_registry_dir = os.environ.get('OCPI_PROJECT_REGISTRY_DIR')
        if project_registry_dir is None:
            cdkdir = os.environ.get('OCPI_CDK_DIR')
            if cdkdir:
                project_registry_dir = cdkdir + "/../project-registry"
            else:
                project_registry_dir = "/opt/opencpi/project-registry"
        return project_registry_dir

    @classmethod
    def get_registry_dir(cls, directory="."):
        """
        Determine the project registry directory. If in a project, check for the imports link.
        Otherwise, get the default registry from the environment setup:
            OCPI_PROJECT_REGISTRY_DIR, OCPI_CDK_DIR/../project-registry or
            /opt/opencpi/project-registry

        Determine whether the resulting path exists.

        Return the exists boolean and the path to the project registry directory.
        """
        if ocpiutil.is_path_in_project(directory) and \
           os.path.isdir(ocpiutil.get_path_to_project_top(directory) + "/imports"):
            # allow imports to be a link OR a dir (needed for deep copies of exported projects)
            project_registry_dir = os.path.realpath(ocpiutil.get_path_to_project_top(directory) +
                                                    "/imports")
        else:
            project_registry_dir = cls.get_default_registry_dir()

        exists = os.path.exists(project_registry_dir)
        if not exists:
            raise ocpiutil.OCPIException("The project registry directory '" + project_registry_dir +
                                         "' does not exist.\nCorrect " +
                                         "'OCPI_PROJECT_REGISTRY_DIR' or run: " +
                                         "'ocpidev create registry " + project_registry_dir + "'")
        elif not os.path.isdir(project_registry_dir):
            raise ocpiutil.OCPIException("The current project registry '" + project_registry_dir +
                                         "' exists but is not a directory.\nCorrect " +
                                         "'OCPI_PROJECT_REGISTRY_DIR'")
        return project_registry_dir


# TODO: Should also extend CreatableAsset, ShowableAsset
class Project(RunnableAsset, RCCBuildableAsset, HDLBuildableAsset, ShowableAsset, ReportableAsset):
    """
    The Project class represents an OpenCPI project. Only one Project class should
    exist per OpenCPI project. Projects can be built, run, registered, shown....
    """
    valid_settings = []
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes Project member data  and calls the super class __init__.  Throws an
        exception if the directory passed in is not a valid project directory.
        valid kwargs handled at this level are:
            init_libs        (T/F) - Instructs the method whether to construct all library objects
                                     contained in the project
            init_apps        (T/F) - Instructs the method whether to construct all application
                                     objects contained in the project
            init_hdl_plats   (T/F) - Instructs the method whether to construct all HdlPlatformWorker
                                     objects contained in the project (at least those with a
                                     corresponding build platform listed in self.hdl_platforms)
            init_hdl_assembs (T/F) - Instructs the method whether to construct all
                                     HdlApplicationAssembly objects contained in the project
        """
        self.check_dirtype("project", directory)
        super().__init__(directory, name, **kwargs)
        self.lib_list = None
        self.apps_list = None

        # Boolean for whether or the current directory is within this project
        # TODO: is current_project needed as a field, or can it be a function?
        #self.current_project = ocpiutil.get_path_to_project_top() == self.directory
        self.__registry = None

        # flag to determine if a project is exported
        # __is_exported is set to true in __init_package_id() if this is a non-source exported
        # project
        self.__is_exported = False
        # Determine the package-ID for this project and set self.package_id
        self.__init_package_id()

        # NOTE: imports link to registry is NOT initialized in this constructor.
        #       Neither is the __registry Registry object.
        if kwargs.get("init_libs", False):
            self.lib_list = []
            logging.debug("Project constructor creating Library Objects")
            for lib_directory in self.get_valid_libraries():
                self.lib_list.append(AssetFactory.factory("library", lib_directory, **kwargs))

        if kwargs.get("init_apps", False):
            self.apps_list = []
            logging.debug("Project constructor creating Applications Objects")
            for app_directory in self.get_valid_apps():
                self.apps_list.append(AssetFactory.factory("applications", app_directory, **kwargs))

        if kwargs.get("init_hdlplats", False):
            logging.debug("Project constructor creating HdlPlatformsCollection Object")
            plats_directory = self.directory + "/hdl/platforms"
            # If hdl/platforms exists for this project, construct the HdlPlatformsCollection
            # instance
            if os.path.exists(plats_directory):
                self.hdlplatforms = AssetFactory.factory("hdl-platforms", plats_directory,
                                                         **kwargs)
            else:
                self.hdlplatforms = None
        else:
            self.hdlplatforms = None

        if kwargs.get("init_hdlassembs", False):
            logging.debug("Project constructor creating HdlAssembliesCollection Object")
            assemb_directory = self.directory + "/hdl/assemblies"
            # If hdl/assemblies exists for this project, construct the HdlAssembliesCollection
            # instance
            if os.path.exists(assemb_directory):
                self.hdlassemblies = AssetFactory.factory("hdl-assemblies", assemb_directory,
                                                          **kwargs)
            else:
                self.hdlassemblies = None
        else:
            self.hdlassemblies = None

    def __eq__(self, other):
        """
        Two projects are equivalent iff their directories match
        """
        #TODO: do we need realpath too? remove the abs/realpaths if we instead call
        # them in the Asset constructor
        return other is not None and \
                os.path.realpath(self.directory) == os.path.realpath(other.directory)

    def delete(self, force=False):
        """
        Remove the project from the registry if it is registered anywhere and remove the project
        from disk
        """
        try:
            self.registry().remove(package_id=self.package_id)
        except ocpiutil.OCPIException:
            # do nothing it's ok if the unregistering fails
            pass
        super().delete(force)

    def __init_package_id(self):
        """
        Get the Package Name of the project containing 'self.directory'.
        """
        # From the project top, probe the Makefile for the projectpackage
        # which is printed in cdk/include/project.mk in the projectpackage rule
        # if ShellProjectVars is defined
        project_package = None
        # If the project-package-id file exists, set package-id to its contents
        if os.path.isfile(self.directory + "/project-package-id"):
            with open(self.directory + "/project-package-id", "r") as package_id_file:
                self.__is_exported = True
                project_package = package_id_file.read().strip()
                logging.debug("Read Project-ID '" + project_package + "' from file: " +
                              self.directory + "/project-package-id")

        # Otherwise, ask Makefile at the project top for the ProjectPackage
        if project_package is None or project_package == "":
            project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                       mk_arg="projectpackage ShellProjectVars=1",
                                                       verbose=True)
            if not project_vars is None and 'ProjectPackage' in project_vars and \
               len(project_vars['ProjectPackage']) > 0:
                # There is only one value associated with ProjectPackage, so get element 0
                project_package = project_vars['ProjectPackage'][0]
            else:
                raise ocpiutil.OCPIException("Could not determine Package-ID of project \"" +
                                             self.directory + "\".")
        self.package_id = project_package

    def get_valid_apps(self):
        """
        Gets a list of all directories of type applications in the project and puts that
        applications directory and the basename of that directory into a dictionary to return
        """
        return ocpiutil.get_subdirs_of_type("applications", self.directory)

    def get_valid_libraries(self):
        """
        Gets a list of all directories of type library in the project and puts that
        library directory and the basename of that directory into a dictionary to return
        """
        return ocpiutil.get_subdirs_of_type("library", self.directory)

    def run(self):
        """
        Runs the Project with the settings specified in the object Throws an exception if no
        applications or libraries are initialized using the init_apps or init_libs variables at
        initialization time
        """
        ret_val = 0
        if (self.apps_list is None) and (self.lib_list is None):
            raise ocpiutil.OCPIException("For a Project to be run \"init_libs\" and " +
                                         "\"init_tests\" or \"init_apps\" must be set to " +
                                         "True when the object is constructed")
        if self.apps_list is not None:
            for apps in self.apps_list:
                run_val = apps.run()
                ret_val = ret_val + run_val
        if self.lib_list is not None:
            for lib in self.lib_list:
                run_val = lib.run()
                ret_val = ret_val + run_val
        return ret_val
    #placeholder function
    def build(self):
        """
        This is a placeholder function will be the function that builds this Asset
        """
        raise NotImplementedError("Project.build() is not implemented")

    def show_tests(self, **kwargs):
        if self.lib_list is None:
            raise ocpiutil.OCPIException("For a Project to show tests \"init_libs\" "
                                         "must be set to True when the object is constructed")

        details = kwargs.get("details", "")
        if details == "simple":
            for lib in self.lib_list:
                valid_tests = lib.get_valid_tests()
                if  valid_tests:
                    print("Library: " + lib.directory)
                for test_dir in valid_tests:
                    print("    Test: " + os.path.basename(test_dir.rstrip('/')))
        elif details == "table":
            row_1 = ["Library Directory", "Test"]
            row_2 = ["----------------------------------------", "-------------------"]
            rows = [row_1, row_2]
            for lib in self.lib_list:
                valid_tests = lib.get_valid_tests()
                for test_dir in valid_tests:
                    row_n = [lib.directory, os.path.basename(test_dir.rstrip('/'))]
                    rows.append(row_n)
            ocpiutil.print_table(rows)
        else:
            '''
            JSON format:
            {project:{
              name: proj_name
              directory: proj_directory
              libraries:{
                lib_name:{
                  name: lib_name
                  directory:lib_directory
                  tests:{
                    test_name : test_directory
                    ...
                  }
                }
              }
            }
            '''
            json_dict = {}
            project_dict = {}
            libraries_dict = {}
            for lib in self.lib_list:
                valid_tests = lib.get_valid_tests()
                if  valid_tests:
                    lib_dict = {}
                    lib_package = lib.package_id
                    lib_dict["package"] = lib_package
                    lib_dict["directory"] = lib.directory
                    lib_dict["tests"] = {os.path.basename(test.rstrip('/')):test
                                         for test in valid_tests}
                    libraries_dict[lib_package] = lib_dict
            project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                       mk_arg="projectdeps ShellProjectVars=1",
                                                       verbose=True)
            project_dict["dependencies"] = project_vars['ProjectDependencies']
            project_dict["directory"] = self.directory
            project_dict["libraries"] = libraries_dict
            project_dict["package"] = self.package_id
            json_dict["project"] = project_dict
            json.dump(json_dict, sys.stdout)
            print()

    def show_libraries(self, **kwargs):
        details = kwargs.get("details", "")
        if details == "simple":
            for lib_directory in self.get_valid_libraries():
                print("Library: " + lib_directory)
        elif details == "table":
            row_1 = ["Library Directories"]
            row_2 = ["----------------------------------------"]
            rows = [row_1, row_2]
            for lib_directory in self.get_valid_libraries():
                row_n = [lib_directory]
                rows.append(row_n)
            ocpiutil.print_table(rows)
        else:
            json_dict = {}
            project_dict = {}
            libraries_dict = {}
            for lib_directory in self.get_valid_libraries():
                lib_dict = {}
                lib_package = Library.get_package_id(lib_directory)
                lib_dict["package"] = lib_package
                lib_dict["directory"] = lib_directory
                libraries_dict[lib_package] = lib_dict
            project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                       mk_arg="projectdeps ShellProjectVars=1",
                                                       verbose=True)
            project_dict["dependencies"] = project_vars['ProjectDependencies']
            project_dict["directory"] = self.directory
            project_dict["libraries"] = libraries_dict
            project_dict["package"] = self.package_id
            json_dict["project"] = project_dict
            json.dump(json_dict, sys.stdout)
            print()

    def print_project_table(self):
        project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                   mk_arg="projectdeps ShellProjectVars=1",
                                                   verbose=True)

        row_1 = ["Project Directory", "Package-ID", "Project Dependencies"]
        row_2 = ["---------------------------", "---------------", "---------------------"]
        row_3 = [self.directory, self.package_id, ", ".join(project_vars['ProjectDependencies'])]
        rows = [row_1, row_2, row_3]
        ocpiutil.print_table(rows)

    def show(self, **kwargs):
        """
        This method prints out information about the project based on the options passed in as
        kwargs
        valid kwargs handled by this method are:
            json (T/F) - Instructs the method whether to output information in json format or
                         human readable format
            tests (T/F) - Instructs the method whether print out the tests that that exist in
                          this project
        """
        details = kwargs.get("details", "")
        if kwargs.get("tests", False):
            self.show_tests(**kwargs)
        elif kwargs.get("libraries", False):
            self.show_libraries(**kwargs)
        elif not kwargs.get("verbose", False):
            project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                       mk_arg="projectdeps ShellProjectVars=1",
                                                       verbose=True)
            if details == "simple":
                print("Project Directory: " + self.directory)
                print("Package-ID: " + self.package_id)
                print("Project Dependencies: " + ", ".join(project_vars['ProjectDependencies']))
            elif details == "table":
                self.print_project_table()
            else:
                json_dict = {'project': {'directory': self.directory,
                                         'package': self.package_id,
                                         'dependencies': project_vars['ProjectDependencies']}}
                json.dump(json_dict, sys.stdout)
                print()
        else:
            if self.lib_list is None:
                raise ocpiutil.OCPIException("For a Project to show verbosely \"init_libs\" "
                                             "must be set to True when the object is constructed")
            if details == "simple":
                project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                           mk_arg="projectdeps ShellProjectVars=1",
                                                           verbose=True)
                print("Project Directory: " + self.directory)
                print("Package-ID: " + self.package_id)
                print("Project Dependencies: " + ", ".join(project_vars['ProjectDependencies']))
                # this will likely change when we have more to show under libraries but for now
                # all we have is tests
                prim_dir = self.directory + "/hdl/primitives"
                if os.path.isdir(prim_dir):
                    print("  HDL Primitives: " + prim_dir)
                    prims = [dir for dir in os.listdir(prim_dir)
                             if not os.path.isfile(os.path.join(prim_dir, dir))]
                    prims = [x for x in prims if x != "lib"]
                    for prim in prims:
                        print("    Primitive: " + prim)
                for lib in self.lib_list:
                    print("  Library: " + lib.directory)
                    valid_tests = lib.get_valid_tests()
                    for test_dir in valid_tests:
                        print("    Test: " + os.path.basename(test_dir.rstrip('/')))
            elif details == "table":
                print("Overview:")
                self.print_project_table()
                print("Libraries:")
                self.show_libraries(details="table")
                print("Tests:")
                self.show_tests(details="table")
                print("Primitives:")
                row_1 = ["Primitive Directory", "Primitive"]
                row_2 = ["---------------------------", "---------------"]
                rows = [row_1, row_2]
                prim_dir = self.directory + "/hdl/primitives"
                if os.path.isdir(prim_dir):
                    prims = [dir for dir in os.listdir(prim_dir)
                             if not os.path.isfile(os.path.join(prim_dir, dir))]
                    prims = [x for x in prims if x != "lib"]
                    for prim in prims:
                        row_n = [prim_dir, prim]
                        rows.append(row_n)
                    ocpiutil.print_table(rows)
            else:
                json_dict = {}
                project_dict = {}
                libraries_dict = {}
                for lib in self.lib_list:
                    lib_dict = {}
                    lib_package = lib.package_id
                    lib_dict["package"] = lib_package
                    lib_dict["directory"] = lib.directory
                    valid_tests = lib.get_valid_tests()
                    if  valid_tests:
                        lib_dict["tests"] = {os.path.basename(test.rstrip('/')):test
                                             for test in valid_tests}
                    libraries_dict[lib_package] = lib_dict
                prim_dir = self.directory + "/hdl/primitives"
                if os.path.isdir(prim_dir):
                    prims_dict = {}
                    prims = [dir for dir in os.listdir(prim_dir)
                             if not os.path.isfile(os.path.join(prim_dir, dir))]
                    prims = [x for x in prims if x != "lib"]
                    prim_vars = ocpiutil.set_vars_from_make(mk_file=prim_dir + "/Makefile",
                                                            mk_arg="ShellTestVars=1 showpackage")
                    prim_pkg = "".join(prim_vars['Package'])
                    for prim in prims:
                        prims_dict[prim] = prim_dir + "/" + prim
                    project_dict["primitives"] = prims_dict
                project_vars = ocpiutil.set_vars_from_make(mk_file=self.directory + "/Makefile",
                                                           mk_arg="projectdeps ShellProjectVars=1",
                                                           verbose=True)
                project_dict["dependencies"] = project_vars['ProjectDependencies']
                project_dict["directory"] = self.directory
                project_dict["libraries"] = libraries_dict
                project_dict["package"] = self.package_id
                json_dict["project"] = project_dict
                json.dump(json_dict, sys.stdout)
                print()

    def show_utilization(self):
        """
        Show utilization separately for each library in this project and for all assemblies
        """
        for library in self.lib_list:
            library.show_utilization()

        # if this project has an hdl/platforms dir, show its utilization
        if self.hdlplatforms:
            self.hdlplatforms.show_utilization()

        # if this project has an hdl/assemblies dir, show its utilization
        if self.hdlassemblies:
            self.hdlassemblies.show_utilization()

    def initialize_registry_link(self):
        """
        If the imports link for the project does not exist, set it to the default project registry.
        Basically, make sure the imports link exists for this project.
        """
        imports_link = self.directory + "/imports"
        if os.path.exists(imports_link):
            logging.info("Imports link exists for project " + self.directory +
                         ". No registry initialization needed")
        else:
            # Get the default project registry set by the environment state
            self.set_registry(Registry.get_default_registry_dir())

    def set_registry(self, registry_path=None):
        """
        Set the project registry link for this project. If a registry path is provided,
        set the link to that path. Otherwise, set it to the default registry based on the
        current environment.
        I.e. Create the 'imports' link at the top-level of the project to point to the project
             registry
        """
        # If registry path is not provided, get the default
        if registry_path is None or registry_path == "":
            # Get the default project registry set by the environment state
            registry_path = Registry.get_default_registry_dir()
        reg = self.registry()
        # If we are about to set a new registry, and this project is registered in the current one,
        # raise an exception; user needs to unregister the project first.
        # pylint:disable=bad-continuation
        if ((os.path.realpath(reg.directory) != os.path.realpath(registry_path)) and
            reg.contains(directory=self.directory)):
            raise ocpiutil.OCPIException("Before (un)setting the registry for the project at \"" +
                                         self.directory + "\", you must unregister the project.\n" +
                                         "This can be done by running 'ocpidev unregister project" +
                                         " " + self.package_id + "'.")
        # pylint:enable=bad-continuation

        ocpiutil.logging.warning("To use this registry, run the following command and add it " +
                                 "to your ~/.bashrc:\nexport OCPI_PROJECT_REGISTRY_DIR=" +
                                 os.path.realpath(registry_path))

        #self.__registry = AssetFactory.factory("registry", registry_path)
        # TODO: pull this relative link functionality into a helper function
        # Try to make the path relative. This helps with environments involving mounted directories
        # Find the path that is common to the registry and project-top
        common_prefix = os.path.commonprefix([os.path.normpath(registry_path), self.directory])
        # If the two paths contain no common directory except root,
        #     use the path as-is
        # Otherwise, use the relative path from project to registry_path
        # actual_registry_path is the actual path that can be checked for existence of the registry
        #     it is either an absolute path or a relative path
        if common_prefix == '/' or common_prefix == '':
            rel_registry_path = os.path.normpath(registry_path)
            actual_registry_path = rel_registry_path
        else:
            rel_registry_path = os.path.relpath(os.path.normpath(registry_path), self.directory)
            actual_registry_path = self.directory + "/" + rel_registry_path
        # Registry must exist and must be a directory
        if os.path.isdir(actual_registry_path):
            imports_link = self.directory + "/imports"
            # If it exists and IS NOT a link, tell the user to manually remove it.
            # If 'imports' exists and is a link, remove the link.
            if os.path.exists(imports_link) or os.path.islink(imports_link):
                if os.path.realpath(actual_registry_path) == os.path.realpath(imports_link):
                    # Imports already point to this registry
                    return
                else:
                    self.unset_registry()
            # ln -s registry_path imports_link
            try:
                os.symlink(rel_registry_path, imports_link)
            except OSError:
                # Symlink creation failed....
                # User probably does not have write permissions in the project
                raise ocpiutil.OCPIException("Failure to set project link: " + imports_link +
                                             " --> " + rel_registry_path + "\nCommand " +
                                             "attempted: " + "'ln -s " + rel_registry_path + " " +
                                             imports_link + "'.\nMake sure you have correct "+
                                             "permissions in this project.")
        else:
            raise ocpiutil.OCPIException("Failure to set project registry to: '" + registry_path +
                                         "'.  Tried to use relative path: " + rel_registry_path +
                                         " in project: " + self.directory + "'.\nPath does not " +
                                         "exist or is not a directory.")

    def unset_registry(self):
        """
        Unset the project registry link for this project.
        I.e. remove the 'imports' link at the top-level of the project.
        """
        # If the 'imports' link exists at the project-top, and it is a link, remove it.
        # If it is not a link, let the user remove it manually.
        reg = self.registry()
        # If we are about to unset the, but this project is registered in the current one,
        # raise an exception; user needs to unregister the project first.
        if reg.contains(directory=self.directory):
            raise ocpiutil.OCPIException("Before (un)setting the registry for the project at \"" +
                                         self.directory + "\", you must unregister the project.\n" +
                                         "This can be done by running 'ocpidev unregister project" +
                                         " " + self.package_id + "'.")

        imports_link = self.directory + "/imports"
        if os.path.islink(imports_link):
            os.unlink(imports_link)
            # Set this project's registry reference to the default
            self.__registry = None
        else:
            if os.path.exists(imports_link):
                raise ocpiutil.OCPIException("The 'imports' for the current project ('" +
                                             imports_link + "') is not a symbolic link.\nThe " +
                                             "file will need to be removed manually.")
            else:
                logging.debug("Unset project registry has succeeded, but nothing was done.\n" +
                              "Registry was not set in the first place for this project.")

    def registry(self):
        """
        This function will return the registry object for this Project instance.
        If the registry is None, it will try to find/construct it first
        """
        if self.__registry is None:
            self.__registry = AssetFactory.factory("registry",
                                                   Registry.get_registry_dir(self.directory))
            if self.__registry is None:
                raise ocpiutil.OCPIException("The registry for the current project ('" +
                                             self.directory + "') could not be determined.")
        return self.__registry
