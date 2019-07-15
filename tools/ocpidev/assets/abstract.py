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
Abstract classes that are used in other places within the assets module are defined in this file
"""

from abc import ABCMeta, abstractmethod
import os
import logging
import copy
import shutil
import _opencpi.hdltargets as hdltargets
import _opencpi.util as ocpiutil

class Asset(metaclass=ABCMeta):
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

    def get_valid_components(self):
        """
        this is function is used by both projects and libraries  to find the component specs that
        are owned by that asset.
        TODO move this function to a separate class from asset and have project and library inherit
             from that class???
        """
        ret_val = []
        if os.path.isdir(self.directory + "/specs"):
            files = [dir for dir in os.listdir(self.directory + "/specs")
                     if os.path.isfile(os.path.join(self.directory + "/specs", dir))]
            for comp in files:
                if Component.is_component_spec_file(self.directory + "/specs/" + comp):
                    ret_val.append(self.directory + "/specs/" + comp)
        return ret_val

class BuildableAsset(Asset):
    """
    Virtual class that requires that any child classes implement a build method.  Contains settings
    that are specific to all assets that can be run
    """
    valid_settings = ["only_plats", "ex_plats"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes BuildableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            ex_plats (list) - list of platforms(strings) to exclude from a build
            only_plats (list) - list of the only platforms(strings) to build for
        """
        super().__init__(directory, name, **kwargs)
        self.ex_plats = kwargs.get("ex_plats", None)
        self.only_plats = kwargs.get("only_plats", None)

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
    valid_settings = ["hdl_plat_strs", "hdl_tgt_strs"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HDLBuildableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            hdl_plat_strs (list) - list of hdl platforms(strings) to build for
            hdl_tgt_strs  (list) - list of hdl targets(strings) to build for
        """
        super().__init__(directory, name, **kwargs)

        # Collect the string lists of HDL Targets and Platforms from kwargs
        self.hdl_tgt_strs = kwargs.get("hdl_tgts", None)
        self.hdl_plat_strs = kwargs.get("hdl_plats", None)

        # Initialize the lists of hdl targets/platforms to empty sets
        # Note that they are sets instead of lists to easily avoid duplication
        #    Also note that there is no literal for the empty set in python3.4
        #    because {} is for dicts, so set() must be used.
        self.hdl_targets = set()
        self.hdl_platforms = set()

        import _opencpi.hdltargets as hdltargets
        # If there were HDL Targets provided, construct HdlTarget object for each
        # and add to the hdl_targets set
        if self.hdl_tgt_strs is not None:
            if "all" in self.hdl_tgt_strs:
                self.hdl_targets = set(hdltargets.HdlToolFactory.get_or_create_all("hdltarget"))
            else:
                for tgt in self.hdl_tgt_strs:
                    self.hdl_targets.add(hdltargets.HdlToolFactory.factory("hdltarget", tgt))
        # If there were HDL Platforms provided, construct HdlPlatform object for each
        # and add to the hdl_platforms set.
        # Also get the corresponding HdlTarget and add to the hdl_targets set
        if self.hdl_plat_strs is not None:
            if "all" in self.hdl_plat_strs:
                self.hdl_platforms = set(hdltargets.HdlToolFactory.get_or_create_all("hdlplatform"))
                self.hdl_targets = set(hdltargets.HdlToolFactory.get_or_create_all("hdltarget"))
            elif "local" not in self.hdl_plat_strs:
                for plat in self.hdl_plat_strs:
                    print("HDLbuildable plat: "+ plat)
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
    valid_settings = ["rcc_plats"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes HDLBuildableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            rcc_plat (list) - list of rcc platforms(strings) to build for
        """
        super().__init__(directory, name, **kwargs)
        self.rcc_plats = kwargs.get("rcc_plats", None)

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
    def show(self, details, verbose, **kwargs):
        """
        This function will show this asset must be implemented by the child class
        """
        raise NotImplementedError("ShowableAsset.show() is not implemented")

class ReportableAsset(Asset):
    """
    Skeleton class providing get/show_utilization functions
    for reporting utilization of an asset.

    get_utilization is generally overridden by sub-classes, but
    show_utilization is usually only overridden for sub-classes that are collections
    of OpenCPI assets (e.g. ones where show_utilization is called for children assets).
    """
    valid_formats = ["table", "latex"]
    def __init__(self, directory, name=None, **kwargs):
        """
        Initializes ReportableAsset member data  and calls the super class __init__
        valid kwargs handled at this level are:
            output_format (str) - mode to output utilization info (table, latex)
                                  output_formats not yet implemented: simple, json, csv
        """
        super().__init__(directory, name, **kwargs)

        self.output_format = kwargs.get("output_format", "table")

    def get_utilization(self):
        """
        This is a placeholder function will be the function that returns ocpiutil.Report instance
        for this asset. Sub-classes should override this function to collect utilization information
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

        This default behavior is likely sufficient, but sub-classes that are collections of OpenCPI
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
            if dirtype == "hdl-platform":
                plat_obj = hdltargets.HdlToolFactory.factory("hdlplatform", self.name)
                if not plat_obj.get_toolset().is_simtool:
                    logging.warning("Skipping " + caption + " because the report is empty")
            else:
                logging.warning("Skipping " + caption + " because the report is empty")
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
                    logging.info("  LaTeX Utilization Table was written to: " + util_file_path +
                                 "\n")
# TODO is this required ?
# pylint:disable=wrong-import-position
from .component import Component
# pylint:disable=wrong-import-position
