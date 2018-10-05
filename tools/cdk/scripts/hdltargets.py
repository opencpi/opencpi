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
HdlTarget, HdlPlatform, and HdlToolSet
are classes which contain information and functions for collecting OpenCPI
supported HDL platforms and their corresponding target-parts and vendor
toolsets.
"""
import os
import ocpiutil

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
        >>> HdlToolSet.get("mytool1").name
        'mytool1'
        >>> [str(tool) for tool in HdlToolSet.all()]
        ['mytool1', 'mytool2']
    """
    __all_toolsets = {}
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
        HdlToolSet.__all_toolsets[name] = self

    def __str__(self):
        return self.name

    def __eq__(self, other):
        if isinstance(other, str):
            return self.name == other
        return isinstance(other, HdlToolSet) and self.name == other.name

    def __lt__(self, other):
        return str(self) < str(other)

    @staticmethod
    def get(name):
        """
        Get an existing instance of HdlToolSet class by name.
        """
        if name in HdlToolSet.__all_toolsets:
            return HdlToolSet.__all_toolsets[name]
        return None

    @staticmethod
    def all():
        """
        Get all instances of the HdlToolSet class
        """
        return sorted(HdlToolSet.__all_toolsets.values())

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
        >>> HdlTarget.get("mytgt1").name
        'mytgt1'
        >>> HdlTarget.get("mytgt1").parts
        ['part1.1', 'part1.2']
        >>> HdlTarget.get("mytgt2").vendor
        'vend2'
        >>> [tgt.name for tgt in HdlTarget.all()]
        ['mytgt0', 'mytgt1', 'mytgt2']
    """
    __all_targets = {}
    def __init__(self, name, vendor, parts, toolset):
        """
        Create an instance of HdlTarget.
        Give it a name and associate it with a vendor, a list of parts, and an HdlToolSet.
        """
        self.name = name
        self.vendor = vendor
        self.parts = parts
        self.toolset = toolset
        HdlTarget.__all_targets[name] = self

    def __str__(self):
        return self.name

    def __lt__(self, other):
        if self.vendor < other.vendor:
            return True
        elif self.vendor == other.vendor:
            return str(self) < str(other)
        else:
            return False

    @staticmethod
    def get(name):
        """
        Get an existing instance of HdlTarget class by name.
        """
        if name in HdlTarget.__all_targets:
            return HdlTarget.__all_targets[name]
        return None

    @staticmethod
    def all():
        """
        Get all instances of the HdlTarget class
        """
        return sorted(HdlTarget.__all_targets.values())

    @staticmethod
    def all_except_sims():
        """
        Get all instances of the HdlTarget class except those that have
        simulator toolsets.
        Example (doctest):
        >>> [tgt.name for tgt in HdlTarget.all_except_sims()]
        ['mytgt0', 'mytgt1']
        """
        return sorted([target for target in list(HdlTarget.__all_targets.values())
                       if not target.toolset.is_simtool])

    @staticmethod
    def get_all_vendors():
        return sorted(set([target.vendor for target in list(HdlTarget.__all_targets.values())]))

    @staticmethod
    def get_all_targets_for_vendor(vendor):
        """
        Get all instances of the HdlTarget class that have the specified vendor.
        Example (doctest):
        >>> [tgt.name for tgt in HdlTarget.get_all_targets_for_vendor('vend1')]
        ['mytgt0', 'mytgt1']
        """
        return sorted([target for target in list(HdlTarget.__all_targets.values())
                       if target.vendor == vendor])

    @staticmethod
    def get_all_targets_for_toolset(toolset):
        """
        Get all instances of the HdlTarget class that have the specified toolset.
        Example (doctest):
        >>> [tgt.name for tgt in HdlTarget.get_all_targets_for_toolset('mytool1')]
        ['mytgt0', 'mytgt1']
        """
        return sorted([target for target in list(HdlTarget.__all_targets.values())
                       if target.toolset == toolset])

    @staticmethod
    def get_target_for_part(part):
        """
        Example (doctest):
        Get an instance of the HdlTarget class that has the specified part.
        >>> HdlTarget.get_target_for_part('part2').name
        'mytgt2'
        """
        targets = [target for target in list(HdlTarget.__all_targets.values()) if part in target.parts]
        if len(targets) > 0:
            return targets[0]
        return None

    # Collect a dictionary of variable names -> values
    # from make-land
    # NOTE: Internal method
    @staticmethod
    def __get_make_vars():
        return ocpiutil.set_vars_from_make(os.environ["OCPI_CDK_DIR"] +
                                           "/include/hdl/hdl-targets.mk",
                                           "ShellHdlTargetsVars=1 ShellGlobalProjectsVars=1",
                                           "verbose")

    @staticmethod
    def __initialize_from_dict(mk_dict):
        if mk_dict and 'HdlTopTargets' in mk_dict:
            for vendor in mk_dict['HdlTopTargets']:
                if 'HdlTargets_' + vendor in mk_dict:
                    families = mk_dict['HdlTargets_' + vendor]
                else:
                    families = [vendor]
                for family in families:
                    if 'HdlTargets_' + family in mk_dict:
                        parts = mk_dict['HdlTargets_' + family]
                    else:
                        parts = [family]
                    if not HdlTarget.get(family):
                        toolname = mk_dict['HdlToolSet_' + family][0]
                        tooltitle = mk_dict['HdlToolName_' + toolname][0]
                        HdlTarget(family, vendor, parts,
                                  HdlToolSet(toolname, tooltitle,
                                             'HdlSimTools' in mk_dict and\
                                             family in mk_dict['HdlSimTools']))

    # Collect HDL target/platform variables from make-land
    # and initialize a collection of HdlTarget/ToolSet objects
    # accordingly.
    # NOTE: This is called from the HdlPlatform class' cousin
    # method and should not be called elsewhere!
    @staticmethod
    def _initialize_from_make_variables():
        mk_dict = HdlTarget.__get_make_vars()
        HdlTarget.__initialize_from_dict(mk_dict)
        return mk_dict

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
        >>> HdlPlatform.get("myplat0").name
        'myplat0'
        >>> HdlPlatform.get("myplat0").exactpart
        'exactpart0'
        >>> [plat.name for plat in HdlPlatform.all()]
        ['myplat0', 'myplat1', 'myplat2']
    """
    __all_platforms = {}
    def __init__(self, name, target, exactpart, built=False):
        self.name = name
        self.target = target
        self.exactpart = exactpart
        HdlPlatform.__all_platforms[name] = self
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

    @staticmethod
    def get(name):
        """
        Get an existing instance of HdlPlatform class by name.
        """
        if name in HdlPlatform.__all_platforms:
            return HdlPlatform.__all_platforms[name]
        return None

    @staticmethod
    def all():
        """
        Get all instances of the HdlPlatform class
        """
        return sorted(HdlPlatform.__all_platforms.values())

    @staticmethod
    def all_built():
        """
        Get all instances of the HdlPlatform class
        """
        return sorted([plat for plat in list(HdlPlatform.__all_platforms.values())
                       if plat.built])

    @staticmethod
    def all_except_sims():
        """
        Get all instances of the HdlPlatform class except those that have
        simulator toolsets.
        Example (doctest):
        >>> [plat.name for plat in HdlPlatform.all_except_sims()]
        ['myplat0', 'myplat1']
        """
        return sorted([plat for plat in list(HdlPlatform.__all_platforms.values())
                       if not plat.get_toolset().is_simtool])

    def get_toolset(self):
        return self.target.toolset

    @staticmethod
    def get_all_platforms_for_toolset(toolset):
        """
        Get all instances of the HdlPlatform class that have the specified toolset.
        Example (doctest):
        >>> [plat.name for plat in HdlPlatform.get_all_platforms_for_toolset('mytool1')]
        ['myplat0', 'myplat1']
        """
        return sorted([platform for platform in HdlPlatform.all()
                       if platform.get_toolset() == toolset])

def initialize_from_make_variables():
    """
    Set the project path to cover all projects available,
    initialize all available platforms and targets from from make variables.
    """
    mk_dict = HdlTarget._initialize_from_make_variables()
    if mk_dict and 'HdlAllPlatforms' in mk_dict:
        for platform_name in mk_dict['HdlAllPlatforms']:
            exactpart = mk_dict['HdlPart_' + platform_name][0]
            target_name = mk_dict['HdlFamily_' + exactpart][0]
            built = platform_name in mk_dict['HdlBuiltPlatforms']
            target = HdlTarget.get(target_name)
            if not HdlPlatform.get(platform_name):
                HdlPlatform(platform_name, target, exactpart, built)

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
    os.environ['OCPI_CDK_DIR'] = os.path.realpath('.')
    doctest.testmod(verbose=__VERBOSITY, optionflags=doctest.ELLIPSIS,
                    extraglobs={'tool1': HdlToolSet("mytool1"),
                                'tool2': HdlToolSet("mytool2", "MyTool1", True),
                                'target0': HdlTarget("mytgt0", "vend1",
                                                     ["part0.1", "part0.2"],
                                                     HdlToolSet.get("mytool1")),
                                'target1': HdlTarget("mytgt1", "vend1",
                                                     "part1", HdlToolSet.get("mytool1")),
                                'target2': HdlTarget("mytgt2", "vend2",
                                                     "part2", HdlToolSet.get("mytool2"))})
else:
    # On run or import, initialize objects from make variables
    initialize_from_make_variables()
