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
Defines the AssetFactory class
"""


from functools import partial
import os
import _opencpi.util as ocpiutil

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
        import _opencpi.assets.application
        import _opencpi.assets.test
        import _opencpi.assets.worker
        import _opencpi.assets.platform
        import _opencpi.assets.assembly
        import _opencpi.assets.library
        import _opencpi.assets.project
        import _opencpi.assets.registry
        import _opencpi.assets.component
        actions = {"worker":         cls.__worker_with_model,
                   "hdl-assemblies":_opencpi.assets.assembly.HdlAssembliesCollection,
                   "hdl-assembly":  _opencpi.assets.assembly.HdlApplicationAssembly,
                   "hdl-platforms": _opencpi.assets.platform.HdlPlatformsCollection,
                   "hdl-platform":  _opencpi.assets.platform.HdlPlatformWorker,
                   "hdl-container": _opencpi.assets.assembly.HdlContainer,
                   "rcc-platforms": _opencpi.assets.platform.RccPlatformsCollection,
                   "rcc-platform":  _opencpi.assets.platform.RccPlatform,
                   "test":          _opencpi.assets.test.Test,
                   "component":     _opencpi.assets.component.Component,
                   "application":   _opencpi.assets.application.Application,
                   "applications":  _opencpi.assets.application.ApplicationsCollection,
                   "library":       _opencpi.assets.library.Library,
                   "libraries":     _opencpi.assets.library.LibraryCollection,
                   "multi-lib":     _opencpi.assets.library.LibraryCollection,
                   "project":       partial(cls.__get_or_create, _opencpi.assets.project.Project),
                   "registry":      partial(cls.__get_or_create, _opencpi.assets.registry.Registry)}

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
        import _opencpi.assets.worker
        if os.path.basename(os.path.realpath(ocpiutil.rchop(directory, '/'))).endswith(".hdl"):
            return _opencpi.assets.worker.HdlLibraryWorker(directory, name, **kwargs)
        elif os.path.basename(os.path.realpath(ocpiutil.rchop(directory, '/'))).endswith(".rcc"):
            return _opencpi.assets.worker.RccWorker(directory, name, **kwargs)
        else:
            raise ocpiutil.OCPIException("Unsupported authoring model for worker located at '" +
                                         directory + "'")
    @classmethod
    def remove_all(cls):
        """
        Removes all instances from the static class variable __assets
        """
        cls.__assets = {}

    @classmethod
    def remove(cls, directory=None, instance=None):
        """
        Removes an instance from the static class variable __assets by directory or the instance
        itself.  Throws an exception if neither optional argument is provided.
        """
        if directory is not None:
            real_dir = os.path.realpath(directory)
            import _opencpi.assets.project
            import _opencpi.assets.registry
            dirtype_dict = {"project": _opencpi.assets.project.Project,
                            "registry": _opencpi.assets.registry.Registry}
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
