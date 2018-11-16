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
HdlToolFactory is a factory for collecting information from elsewhere in
OpenCPI regarding HDL targets, platforms and tools and then constructing
instances for them.

HdlTarget, HdlPlatform, HdlToolSet and HdlReportableToolSet
are classes which contain information and functions for collecting OpenCPI
supported HDL platforms and their corresponding target-parts and vendor
toolsets.

Documentation and testing:
    Documentation can be viewed by running:
        $ pydoc3 hdltargets
Note on testing:
    When adding functions to this file, add unit tests to
    opencpi/tests/pytests/*_test.py
"""

import os
import sys
import glob
import importlib
from functools import partial
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util as ocpiutil
import _opencpi.hdltools as hdltools

class HdlToolFactory(object):
    """
    This class contains logic for collecting Tool/Target/Platform information
    and constructing classes for each. It also contains logic for importing
    tool modules and absorbing their metadata.
    """

    __mk_dict = {}
    __tgt_dict = {}
    __plat_dict = {}
    __tool_dict = {}
    __tool_assets = {}

    # __tool_reporting is an internal dictionary that is initialized to contain
    # metadata associated with tools. It is initialized by importing the tools'
    # modules and populating the dict as follows
    #     {
    #      "vivado": {
    #                 "synth_files": [],
    #                 "impl_files": [],
    #                 "reportable_items": []
    #                },
    #      ...
    #     }
    __tool_reporting = {}

    # Initialization for this module. Collect ReportableItems and files for each tool
    # Iterate through all tools, import each, and each tool's initialize reporting info
    for tool_str in hdltools.__all__:
        try:
            # import the tool submodule of "hdltools"
            tool_module = importlib.import_module("_opencpi.hdltools." + tool_str)
            # Initialize metadata for the tool
            __tool_reporting[tool_str] = {}
            # pylint:disable=bad-whitespace
            __tool_reporting[tool_str]["synth_files"]      = tool_module.synth_files
            __tool_reporting[tool_str]["impl_files"]       = tool_module.impl_files
            __tool_reporting[tool_str]["reportable_items"] = tool_module.reportable_items
            # pylint:enable=bad-whitespace
        except ImportError as ex:
            ocpiutil.logging.error(str(ex))
            ocpiutil.logging.error("Could not import HDL ToolSet module \"" + tool_str + "\"")

    @classmethod
    def factory(cls, asset_type, name=None):
        """
        Class method that is the intended wrapper to create all instances of any Asset subclass.
        Returns a constructed object of the type specified by asset_type. Throws an exception for
        an invalid type.

        Every asset must have a directory, and may provide a name and other args.
        Some assets will be created via the corresponding subclass constructor.
        Others will use an auxiliary function to first check if a matching instance
        already exists.
        """
        # actions maps asset_type string to the function that creates objects of that type
        # Some types will use plain constructors,
        # and some will use __get_or_create with asset_cls set accordingly
        actions = {"hdltoolset":  partial(cls.__get_or_create, HdlToolSet),
                   "hdltarget":   partial(cls.__get_or_create, HdlTarget),
                   "hdlplatform": partial(cls.__get_or_create, HdlPlatform)}

        if asset_type not in actions.keys():
            raise ocpiutil.OCPIException("Bad asset creation, \"" + asset_type + "\" not supported")

        # Call the action for this type and hand it the arguments provided
        return actions[asset_type](name)

    @classmethod
    def __get_or_create(cls, asset_cls, name):
        """
        Given an asset subclass type, check whether an instance of the
        subclass already exists for the provided directory. If so, return
        that instance. Otherwise, call the subclass constructor and return
        the new instance.
        """
        # Determine the sub-dictionary in __assets corresponding to the provided class (asset_cls)
        if asset_cls not in cls.__tool_assets:
            cls.__tool_assets[asset_cls] = {}
        asset_inst_dict = cls.__tool_assets[asset_cls]

        # Does an instance of the asset_cls subclass exist with a directory matching the provided
        # "dictionary" parameter? If so, just return that instance.
        if name not in asset_inst_dict:
            # If not, construct a new one, add it to the dictionary, and return it
            asset_inst_dict[name] = cls.__init_hdltool_asset_from_dict(asset_cls, name)
        return asset_inst_dict[name]

    @classmethod
    def get_or_create_all(cls, asset_type):
        """
        Create all assets of the provided type (target platform or tool)
        """
        if cls.__tgt_dict == {}:
            cls.__parse_hdltargets_from_make()
        asset_list = []
        if asset_type == "hdltarget":
            # iterate through all target names, construct, and add to return list for tgt in cls.__tgt_dict:
            for tgt in cls.__tgt_dict:
                asset_list.append(cls.__get_or_create(HdlTarget, tgt))
            return asset_list
        if asset_type == "hdlplatform":
            # iterate through all platform names, construct, and add to return list
            for plat in cls.__plat_dict:
                asset_list.append(cls.__get_or_create(HdlPlatform, plat))
            return asset_list
        if asset_type == "hdltoolset":
            # iterate through all tool names, construct, and add to return list
            for tool in cls.__tool_dict:
                asset_list.append(cls.__get_or_create(HdlToolSet, tool))
            return asset_list
        raise ocpiutil.OCPIException("Bad asset creation, \"" + asset_type + "\" not supported")

    @classmethod
    def get_all_vendors(cls):
        """
        Get list of vendors (e.g. unique list of vendors covering all targets)
        """
        return sorted(set([target.vendor for target in list(cls.get_or_create_all("hdltarget"))]))

    @classmethod
    def get_all_targets_for_vendor(cls, vendor):
        """
        Get all instances of the HdlTarget class that have the specified vendor.
        """
        return sorted([target for target in list(cls.get_or_create_all("hdltarget"))
                       if target.vendor == vendor])

    @classmethod
    def __init_hdltool_asset_from_dict(cls, asset_cls, name):
        """
        Construct an asset instance using information from the static internal
        dictionaries that store hdltarget/tool/platform info.
        """
        if cls.__tgt_dict == {}:
            # if the dicts have not yet been initialized, initialize them using
            # information from make
            cls.__parse_hdltargets_from_make()

        if asset_cls is HdlToolSet:
            # if tool, name should be a key in tool_dict
            if name not in cls.__tool_dict:
                raise ocpiutil.OCPIException("Tool with name \"" + name + "\" does not exist. " +
                                             "Run 'ocpidev show hdl targets --table' for more " +
                                             "information on tools and HDL targets.")

            if name in cls.__tool_reporting.keys():
                # if this tool is found in the reporting dictionary,
                # get this tools reporting metadata to pass to constructor
                report_dict = cls.__tool_reporting[name]
                # and create a reportable tool
                return HdlReportableToolSet(name=name, title=cls.__tool_dict[name]["title"],
                                            is_simtool=cls.__tool_dict[name]["is_simtool"],
                                            synth_files=report_dict["synth_files"],
                                            impl_files=report_dict["impl_files"],
                                            reportable_items=report_dict["reportable_items"])

            else:
                # otherwise, this is not a reportable tool, so just go ahead and call the
                # HdlToolSet constructor
                return asset_cls(name=name, title=cls.__tool_dict[name]["title"],
                                 is_simtool=cls.__tool_dict[name]["is_simtool"])

        if asset_cls is HdlTarget:
            # if tgt, name should be a key in tgt_dict
            if name not in cls.__tgt_dict:
                raise ocpiutil.OCPIException("Target with name \"" + name + "\" does not exist. " +
                                             "Valid targets are:\n" +
                                             str(list(cls.__tgt_dict.keys())))
            # construct and return the target
            return asset_cls(name=name, vendor=cls.__tgt_dict[name]["vendor"],
                             parts=cls.__tgt_dict[name]["parts"],
                             toolset=cls.__tgt_dict[name]["toolset"])
        if asset_cls is HdlPlatform:
            # if plat, name should be a key in plat_dict
            if name not in cls.__plat_dict:
                raise ocpiutil.OCPIException("Platform with name \"" + name + "\" does not exist" +
                                             " or its project needs to be registered. Valid "
                                             "platforms are:\n" +
                                             str(list(cls.__plat_dict.keys())) + "\n"
                                             "To make sure the platform's project is registered " +
                                             "run 'ocpidev register project'" + " in the " +
                                             "project containing the platform.")
            # ensure this platform's target exists and get it
            tgt_for_plat = cls.factory("hdltarget", cls.__plat_dict[name]["targetname"])
            # construct and return the target
            return asset_cls(name=name, target=tgt_for_plat,
                             exactpart=cls.__plat_dict[name]["exactpart"],
                             built=cls.__plat_dict[name]["built"])

        asset_cls_name = "None" if asset_class is none else asset_cls.__name__
        raise ocpiutil.OCPIException("Bad initializion. Asset Class \"" + asset_cls_name +
                                     "\" not supported")

    @classmethod
    def __parse_hdltargets_from_make(cls):
        """
        Ask make for the HDL target/platform/toolset information, and parse it into the
        __tool_dict, __tgt_dict and __plat_dict dictionaries. Other functions in this
        class will use those dictionaries to construct HdlTarget/Platform/ToolSet intances
        """
        if cls.__mk_dict == {}:
            # Ask make for tool/tgt/plat info which will be reorganized/parsed below
            cls.__mk_dict = ocpiutil.set_vars_from_make(os.environ["OCPI_CDK_DIR"] +
                                                        "/include/hdl/hdl-targets.mk",
                                                        "ShellHdlTargetsVars=1 " +
                                                        "ShellGlobalProjectsVars=1",
                                                        "verbose")
        # Top targets are general groups that likely contain multiple child targets/families
        if 'HdlTopTargets' in cls.__mk_dict:
            # we call TopTargets "vendors" because that is a more readable term
            for vendor in cls.__mk_dict['HdlTopTargets']:
                # Each vendor may have a list of associated targets. We assign those
                # to the "families" list
                if 'HdlTargets_' + vendor in cls.__mk_dict:
                    families = cls.__mk_dict['HdlTargets_' + vendor]
                else:
                    # if there is no list of targets for the vendor, then the vendor name is
                    # itself the target to use
                    families = [vendor]
                # for each family, create an entry in the class' tgt_dict. set the vendor, list
                # of associated parts, the toolset, and some other metadata
                for family in families:
                    cls.__tgt_dict[family] = {}
                    cls.__tgt_dict[family]["vendor"] = vendor
                    # A family will have a list of associated parts, so get them and
                    # add them to the tgt_dict under the "parts" entry for this family
                    if 'HdlTargets_' + family in cls.__mk_dict:
                        cls.__tgt_dict[family]["parts"] = cls.__mk_dict['HdlTargets_' + family]
                    else:
                        # if there is no parts list, then the family itself is the part
                        cls.__tgt_dict[family]["parts"] = [family]

                    # There must be a toolset associated with each family, so get it and add
                    # it to the tgt_dict under the family's "toolset" entry
                    toolname = cls.__mk_dict['HdlToolSet_' + family][0]
                    cls.__tgt_dict[family]["toolset"] = toolname

                    # Add this tool to the separate tool_dict along with its associated title,
                    # and whether or not it is a simulator tool
                    if toolname not in cls.__tool_dict:
                        cls.__tool_dict[toolname] = {}
                        # Get the title for this tool via HdlToolName
                        if 'HdlToolName_' + toolname in cls.__mk_dict:
                            cls.__tool_dict[toolname]["title"] = \
                                    cls.__mk_dict['HdlToolName_' + toolname][0]

                        # Determine if this tool is one of the HdlSimTools. Set the is_simtool
                        # entry in __tool_dict accordingly
                        is_simtool = ('HdlSimTools' in cls.__mk_dict and
                                      family in cls.__mk_dict['HdlSimTools'])
                        cls.__tool_dict[toolname]["is_simtool"] = is_simtool

        # For each HdlPlatform, add an entry to __plat_dict including the platform's exact part,
        # its HDL target name, and whether it is built
        if 'HdlAllPlatforms' in cls.__mk_dict:
            for platname in cls.__mk_dict['HdlAllPlatforms']:
                cls.__plat_dict[platname] = {}
                exactpart = cls.__mk_dict['HdlPart_' + platname][0]
                cls.__plat_dict[platname]['exactpart'] = exactpart
                cls.__plat_dict[platname]['targetname'] = cls.__mk_dict['HdlFamily_' + exactpart][0]
                cls.__plat_dict[platname]['built'] = platname in cls.__mk_dict['HdlBuiltPlatforms']

class HdlToolSet(object):
    """
    HdlToolSet
    A HDL compilation toolset (e.g. vivado, quartus, xsim, modelsim).
    A toolset may or may not be a simulation tool.

    Example (doctest):
        >>> tool1 = HdlToolSet("mytool1")
        >>> tool2 = HdlToolSet("mytool2", "MyTool2", True)
        >>> tool1.name
        'mytool1'
        >>> tool1.title
        'mytool1'
        >>> tool1.is_simtool
        False
        >>> tool2.title
        'MyTool2'
        >>> tool2.is_simtool
        True
    """
    def __init__(self, name, title=None, is_simtool=False):
        """
        Create an instance of HdlToolSet.
        Give it a name, a title (e.g. capitalized, abbreviated...),
        give it a boolean indication of whether or not it is a
        simulation tool, and add to the static list of tools.
        """
        self.name = name
        self.title = title if title else name
        self.is_simtool = is_simtool

    def __str__(self):
        return self.name

    def __eq__(self, other):
        if isinstance(other, str):
            return self.name == other
        return isinstance(other, HdlToolSet) and self.name == other.name

    def __lt__(self, other):
        return str(self) < str(other)

class HdlReportableToolSet(HdlToolSet):
    """
    HdlReportableToolSet
    Subclass of HdlToolSet that supports utilization reporting
    """
    __valid_modes = ["synth", "impl"]

    # Synth items that should precede the common items in a synth report:
    __default_synth_items = []

    __default_synth_sort_priority = ["Tool", "OCPI Target"]
    __default_impl_sort_priority = ["Tool", "OCPI Platform"]

    # Implementation items that should precede the common items in an impl report:
    #
    # Container should be appended to each element dictionary prior to reporting on that
    # dictionary in the "impl" fasion. OpenCPI Platform is determined for each data-point
    # based on the platform arguments passed in.
    __default_impl_items = ["OCPI Platform"]
    # TODO add ReportableItem w/ function for Timing pass/failure for impl

    # OpenCPI Target is determined for each data-point based on the target arguments passed in
    # The remaining items here are truly ReportableItems that are calculated from each ToolSets's
    # output files
    __default_common_items = ["OCPI Target", "Tool", "Version", "Device", "Registers (Typ)",
                              "LUTs (Typ)", "Fmax (MHz) (Typ)", "Memory/Special Functions"]
    __ordered_items = []

    def __init__(self, name, title=None, is_simtool=False,
                 synth_files=[], impl_files=[], reportable_items=[]):
        super().__init__(name, title, is_simtool)
        self.files = {}
        self.files["synth"] = synth_files
        self.files["impl"] = impl_files
        self.reportable_items = reportable_items
        for item in reportable_items:
            if item.key not in HdlReportableToolSet.get_ordered_items():
                raise ocpiutil.OCPIException("Item \"" + item.key + "\" is not recognized." +
                                             "Be sure to use " +
                                             "HdlReportableToolSet.set_ordered_items([<items>])" +
                                             "to set the list of acceptable report items.")

    def get_files(self, directory, mode="synth"):
        """
        Return the list of files that are usable for reporting utilization given the directory
        and mode.
        """
        if mode not in self.__valid_modes:
            raise ocpiutil.OCPIException("Cannot provide reportable files for mode \"" +
                                         mode + "\". Valid modes are: " + str(self.__valid_modes))

        file_options = [directory + "/" + fname for fname in self.files[mode]]
        report_files = []
        for fil in file_options:
            globbed_files = glob.glob(fil)
            if globbed_files == []:
                ocpiutil.logging.info("Skipping non-existent report file \"" + fil + "\"")
            else:
                report_files += glob.glob(fil)
        return report_files

    @classmethod
    def get_ordered_items(cls, mode="synth"):
        """
        Return the default ordering for the given mode
        """
        if mode == "synth":
            return cls.__default_synth_items + cls.__default_common_items + cls.__ordered_items
        if mode == "impl":
            return cls.__default_impl_items + cls.__default_common_items + cls.__ordered_items
        raise ocpiutil.OCPIException("Valid modes for reporting utilization are \"synth\" " +
                                     "and \"impl\". Mode specified was \"" + mode + "\".")

    @classmethod
    def get_sort_priority(cls, mode="synth"):
        """
        Return the default sort priority for the given mode
        """
        if mode == "synth":
            return cls.__default_synth_sort_priority
        elif mode == "impl":
            return cls.__default_impl_sort_priority
        else:
            raise ocpiutil.OCPIException("Valid modes for reporting utilization are \"synth\" " +
                                         "and \"impl\". Mode specified was \"" + mode + "\".")

    def collect_report_items(self, files, target=None, platform=None, mode="synth"):
        """
        For a given list of files and a target OR platform, iterate through this
        tool's ReportableItems and apply them to each file to collect a dictionary
        mapping element keys to the values where each value was determined
        by applying a ReportableItem to a file.
        """
        ocpiutil.logging.debug("Reporting on files: " + str(files))
        if files is None or files == []:
            ocpiutil.logging.info("No files to report on")
            return {}
        if mode != "synth" and mode != "impl":
            raise ocpiutil.OCPIException("Valid modes for reporting utilization are \"synth\" " +
                                         "and \"impl\". Mode specified was \"" + mode + "\".")

        # pylint:disable=bad-continuation
        if ((mode == "synth" and target is None) or (mode == "impl" and platform is None) or
            (target is not None  and platform is not None)):
            raise ocpiutil.OCPIException("Synthesis reporting operates only on HDL targets.\n" +
                                         "Implementation reporting operates only on HDL platforms.")
        # pylint:enable=bad-continuation

        elem_dict = {}
        if mode == "impl":
            elem_dict["OCPI Platform"] = platform.name
            elem_dict["OCPI Target"] = platform.target.name
        else:
            elem_dict["OCPI Target"] = target.name
        elem_dict["Tool"] = self.title

        # If not a single value is found for this tool's ReportableItems,
        # this data-point is empty
        non_empty = False
        # Iterate through this tool's ReportableItems and match each item (regex) with on the files
        # provided. Each value found is added to this data_point or filled in with None
        for item in self.reportable_items:
            elem = None
            for fil in files:
                if mode == "synth":
                    elem = item.match_and_transform_synth(fil)
                else:
                    elem = item.match_and_transform_impl(fil)
                if elem:
                    elem_dict[item.key] = elem
                    # An element's value has been found, so this item is non-empty
                    non_empty = True
                    break
            if elem is None:
                # No value was found/matched in the provided files for this item
                # Fill in None
                elem_dict[item.key] = None
        return elem_dict if non_empty else {}

    def construct_report_item(self, directory, target=None, platform=None, mode="synth",
                              init_report=None):
        """
        For a single directory and single target OR platform, construct (or add to) a Report with
        a new data-point for the directory and target/platform provided. The data-point will
        contain a key/value pair for each ReportableItem of this toolset.
        """
        if mode != "synth" and mode != "impl":
            raise ocpiutil.OCPIException("Valid modes for reporting utilization are \"synth\" " +
                                         "and \"impl\". Mode specified was \"" + mode + "\".")

        # pylint:disable=bad-continuation
        if ((mode == "synth" and target is None) or (mode == "impl" and platform is None) or
            (target is not None  and platform is not None)):
            raise ocpiutil.OCPIException("Synthesis reporting operates only on HDL targets.\n" +
                                         "Implementation reporting operates only on HDL platforms.")
        # pylint:enable=bad-continuation

        # Initialize a report object with the default ordered headers for this mode
        if init_report is None:
            init_report = ocpiutil.Report(ordered_headers=self.get_ordered_items(mode))

        # Determine the toolset to collect these report items
        if mode == "synth":
            toolset = target.toolset
        elif mode == "impl":
            toolset = platform.target.toolset

        # For the tool in question, get the files for reporting on this directory/mode
        files = toolset.get_files(directory, mode)
        # Collect the actual report items dict of itemkey=>value and add as a data-point to the
        # report for returning
        new_report = toolset.collect_report_items(files, target=target, platform=platform,
                                                  mode=mode)
        # If the new_report is non-empty, add it to the initial one
        if new_report:
            init_report.append(new_report)
        return init_report

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

class HdlPlatform(object):
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

# Set log level and setup for doctest
if __name__ == "__main__":
    import doctest
    __LOG_LEVEL = os.environ.get('OCPI_LOG_LEVEL')
    __VERBOSITY = False
    if __LOG_LEVEL:
        try:
            if int(__LOG_LEVEL) >= 8:
                __VERBOSITY = True
        except ValueError:
            pass
    # for testing, set an invalid projects dir
    #os.environ['OCPI_CDK_DIR'] = os.path.realpath('.')
    init_instances = {'tool1': HdlToolSet("mytool1"),
                      'tool2': HdlToolSet("mytool2", "MyTool1", True)}

    init_instances['target0'] = HdlTarget("mytgt0", "vend1",
                                          ["part0.1", "part0.2"],
                                          init_instances['tool1'])
    init_instances['target1'] = HdlTarget("mytgt1", "vend1",
                                           "part1", init_instances['tool1'])
    init_instances['target2'] = HdlTarget("mytgt2", "vend2",
                                           "part2", init_instances['tool2'])
    doctest.testmod(verbose=__VERBOSITY, optionflags=doctest.ELLIPSIS, extraglobs=init_instances)
