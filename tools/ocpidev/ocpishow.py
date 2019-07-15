#!/usr/bin/env python3
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
Main program that processes the show verb for ocpidev
"""
import argparse
import os
import sys
import types
import pydoc
import _opencpi.util as ocpiutil
import _opencpi.assets.factory as ocpifactory
import _opencpi.assets.registry as ocpiregistry
import _opencpi.assets.project as ocpiproj
import _opencpi.assets.platform as ocpiplat

# There is a top-level parser that parses FIRST_NOUNS
# If the noun is a PLAIN_NOUN, it proceeds normally,
# but if the noun is a SUB_PARSER_NOUN, then it
# looks for a second noun from the SUBNOUNS list.
PLAIN_NOUNS = ["registry", "projects", "workers", "components", "platforms", "targets", "tests",
               "libraries", "project", "component", "worker"]
SUB_PARSER_NOUNS = ["hdl", "rcc"]
FIRST_NOUNS = PLAIN_NOUNS + SUB_PARSER_NOUNS
SUBNOUNS = {}
SUBNOUNS['hdl'] = ["targets", "platforms"]
SUBNOUNS['rcc'] = ["targets", "platforms"]
NOUN_NON_PLURALS = ["component", "project", "worker"]

# NOUNS is the list of all nouns, where noun-subparser nouns
# are listed as <noun>-<subnoun>
NOUNS = PLAIN_NOUNS
for loop_noun in SUB_PARSER_NOUNS:
    for subnoun in SUBNOUNS[loop_noun]:
        NOUNS.append(loop_noun + "-" + subnoun)

# Most nouns correspond to dirtypes, except nouns that represent 'all' or 'multiple' assets of a
# certain type. For example 'workers' or 'tests' do not correspond to dirtypes, but to multiple
# assets of the singular type (worker/test).
#
# 'hdl-assemblies' on the other hand is a noun AND a dirtype, because there is actually a directory
# of that type (hdl/assemblies).
#
# Likewise, applications is a dirtype because there is a directory of that type.
NOUN_PLURALS = ["projects", "workers", "components", "platforms", "targets", "tests", "libraries"]
DIR_TYPES = [noun for noun in NOUNS if noun not in NOUN_PLURALS]

def parse_cl_vars():
    """
    Construct the argparse object and parse all the command line arguments into a dictionary to
    return
    """
    description = ("Utility for showing the project registry, any " +
                   "available projects (registered or in path), workers, " +
                   "components, HDL/RCC Platforms and Targets available at built time.")
    parser = argparse.ArgumentParser(description=description, prog="ocpidev show")
    # Print help screen and usage to pager. Code block adapted and inlined from from:
    # https://github.com/fmenabe/python-clg/blob/master/LICENSE#L6
    #     under MIT license
    parser.print_help = types.MethodType(lambda self,
                                                _=None: pydoc.pager("\n" + self.format_help()),
                                         parser)
    parser.print_usage = types.MethodType(lambda self,
                                                 _=None: pydoc.pager("\n" + self.format_usage()),
                                          parser)
    # This displays the error AND help screen when there is a usage error or no arguments provided
    parser.error = types.MethodType(
        lambda self, error_message: (
            pydoc.pager(error_message + "\n\n" + self.format_help()),
            exit(1)
            ), parser)

    parser.add_argument("noun", type=str, nargs='?', help="This is either the noun to show or " +
                        "the authoring model to operate on. If choosing an authoring model " +
                        "(hdl/rcc), the platforms or targets nouns can follow.",
                        choices=FIRST_NOUNS)
    parser.add_argument("-v", "--verbose", action="store_const", dest="verbose", default=0, const=1,
                        help="Be verbose with output.")
    parser.add_argument("-vv", "--very-verbose", action="store_const", dest="verbose", default=0,
                        const=2, help="Be very verbose with output.")
    parser.add_argument("-vvv", "--very--very-verbose", action="store_const", default=0,
                        dest="verbose", const=3, help="Be very very verbose with output.")
    parser.add_argument("-d", dest="cur_dir", default=os.path.curdir,
                        help="Change directory to the specified path " + "before proceeding. " +
                        "Changing directory may have no effect for some commands.")
    parser.add_argument("-l", "--library", dest="library", default=None,
                        help="Specify the component library in which this operation will be " +
                        "performed.")
    parser.add_argument("--hdl-library", dest="hdl_library", default=None,
                        help="Specify the hdl library in which this operation will be " +
                        "performed.")
    parser.add_argument("-P", dest="hdl_plat_dir", default=None,
                        help="Specify the hdl platform subdirectory to operate in.")
    details_group_top = parser.add_argument_group("details arguments",
                                                  "mutually exclusive arguments to specify how " +
                                                  "the output is formated, the default is " +
                                                  "--table if nothing is specified.")
    details_group = details_group_top.add_mutually_exclusive_group()
    details_group.add_argument("--table", action="store_const", dest="details", const="table",
                               default="table",
                               help="Pretty-print details in a table associated with the chosen " +
                               "noun.")
    details_group.add_argument("--json", action="store_const", dest="details", const="json",
                               default="table",
                               help="Print information in a json format")
    details_group.add_argument("--simple", action="store_const", dest="details", const="simple",
                               default="table",
                               help="Print information in a simple format ")
    scope_group_top = parser.add_argument_group("scope arguments",
                                                "mutually exclusive arguments to specify how to " +
                                                "search for assets to be output, the default is " +
                                                "--global-scope if nothing is specified.")
    scope_group = scope_group_top.add_mutually_exclusive_group()
    scope_group.add_argument("--local-scope", action="store_const", dest="scope", const="local",
                             default="global",
                             help="Only show assets in the local project")
    scope_group.add_argument("--global-scope", action="store_const", dest="scope", const="global",
                             default="global",
                             help="show assets in all projects.")

    first_pass_args, remaining_args = parser.parse_known_args()
    if not remaining_args:
        args = vars(first_pass_args)
        args["name"] = None
        noun = first_pass_args.noun
    elif first_pass_args.noun in SUB_PARSER_NOUNS:

        subparser = argparse.ArgumentParser(description=description)

        subparser.add_argument("subnoun", type=str, help="This is either the noun to show or the " +
                               "authoring model to operate on. If choosing an authoring model " +
                               "(hdl/rcc), the platforms or targets noun can follow.",
                               choices=SUBNOUNS[first_pass_args.noun])
        args = vars(subparser.parse_args(remaining_args, namespace=first_pass_args))
        args["name"] = None
        noun = first_pass_args.noun + args["subnoun"]
    else:
        subparser = argparse.ArgumentParser(description=description)
        # finally, parse the actual name of the asset to act on
        subparser.add_argument("name", default=".", type=str, action="store", nargs='?',
                               help="This is the name of the asset to show utilization for.")
        args = vars(subparser.parse_args(remaining_args, namespace=first_pass_args))
        noun = first_pass_args.noun

    return args, noun

def check_scope_options(scope, noun):
    """
    Verify that the scope options passed in are valid given the noun
    """
    if noun in NOUN_NON_PLURALS:
        scope = "local"

    valid_scope_dict = {
        "registry":["global"],
        "projects":["global"],
        "workers":["global"],
        "components":["global"],
        "component":["local"],
        "worker":["local"],
        "tests":["local"],
        "libraries":["local"],
        "project":["local"],
        "platforms":["global"],
        "targets":["global"],
        "rccplatforms":["global"],
        "rcctargets":["global"],
        "hdlplatforms":["global"],
        "hdltargets":["global"],
    }
    if scope not in valid_scope_dict[noun]:
        raise ocpiutil.OCPIException("Invalid scope option '" + scope + "' for " + noun +
                                     ".  Valid options are: " + " ,".join(valid_scope_dict[noun]))

def set_init_values(args, noun):
    """
    based on the noun and verbocity level set the init variabes up for the classes so that the
    right portions of the classes are constructed
    """
    if noun == "project" and args["verbose"] > 0:
        args["init_apps_col"] = True
        args["init_hdlplats"] = True
        args["init_rccplats"] = True
        args["hdl_plats"] = ["local"]
        args["init_comps"] = True
    if noun == "project" and args["verbose"] > 1:
        args["init_libs"] = True
        args["init_hdlassembs"] = True
    elif noun in ["tests", "workers", "components"]:
        args["init_libs"] = True
    elif noun in ["component", "worker"]:
        args["init_ocpigen_details"] = True

def get_noun_from_plural(args, noun, scope):
    """
    deterimne what object type to create for a plural noun based on the noun and the scope
    """
    action = ""
    class_dict = {
        None:           "ocpiproj.Project",
        "projects":     "ocpiproj.Project",
        "hdltargets":   "ocpiplat.HdlTarget",
        "rcctargets":   "ocpiplat.RccTarget",
        "targets":      "ocpiplat.Target",
        "hdlplatforms": "ocpiplat.HdlPlatform",
        "rccplatforms": "ocpiplat.RccPlatform",
        "platforms":    "ocpiplat.Platform",
        }
    if args["noun"] in ["tests", "libraries", "workers", "components"]:
        action = "show_" + args["noun"]
        if scope == "global":
            noun = "registry"
        elif scope == "local":
            noun = "project"
    else:
        action = class_dict[noun] + '.show_all("' + args["details"] + '")'
        noun = None
    return noun, action

def main():
    """
    Function that is called if this module is called as a main function
    """
    (args, noun) = parse_cl_vars()
    action = "show"
    try:
        cur_dir = args['cur_dir']
        with ocpiutil.cd(cur_dir):
            dir_type = ocpiutil.get_dirtype()
            # args['name'] could be None if no name is provided at the command line
            name = args.get("name", None)
            if not name:
                name = ""
            if not noun:
                noun = ocpiutil.get_dirtype()
            # Now that we have grabbed name, delete it from the args that will be passed into the
            # AssetFactory because name is explicitly passed to AssetFactory as a separate argument
            del args['name']
            check_scope_options(args.get("scope", None), noun)
            if noun in ["registry", "projects"]:
                directory = ocpiregistry.Registry.get_registry_dir()
            elif noun not in ["libraries", "hdlplatforms", "hdltargets", "rccplatforms",
                              "rcctargets", "platforms", "targets", "workers", "components"]:
                directory = ocpiutil.get_ocpidev_working_dir(noun=noun,
                                                             name=name,
                                                             library=args['library'],
                                                             hdl_library=args['hdl_library'],
                                                             hdl_platform=args['hdl_plat_dir'])
            elif noun in ["libraries", "tests", "workers", "components"]:
                if args.get("scope", None) == "local":
                    directory = ocpiutil.get_path_to_project_top()
                elif args.get("scope", None) == "global":
                    directory = ocpiregistry.Registry.get_registry_dir()
            else:
                directory = ""

            ocpiutil.logging.debug('Choose directory "' + directory + '" to operate in')
            if noun is None:
                noun = dir_type
            #TODO the noun libraries could be an asset or a plural need to add logic to deal with
            #     determining which one it is.  for now we are going to assume that it is always a
            #     plural

            # If name is not set, set it to the current directory's basename
            if name == "." and dir_type in DIR_TYPES:
                name = os.path.basename(os.path.realpath("."))
            set_init_values(args, noun, )
            plural_noun_list = NOUN_PLURALS + ["hdlplatforms", "hdltargets", "rccplatforms",
                                               "rcctargets", "platforms", "targets"]
            if noun in plural_noun_list:
                (noun, action) = get_noun_from_plural(args, noun, args.get("scope", None))
            ocpiutil.logging.debug('Choose noun: "' + str(noun) + '" and action: "' + action +'"')
            if noun not in [None, "hdlplatforms", "hdltargets", "rccplatforms", "rcctargets",
                            "platforms", "targets", "projects"]:
                # pylint:disable=unused-variable
                ocpiutil.logging.debug("constructing asset noun: " + noun + " directory: " +
                                       directory + "args : " + str(args))
                my_asset = ocpifactory.AssetFactory.factory(noun, directory, "", **args)
                action = ("my_asset." + action + '("' + args["details"] + '", ' +
                          str(args["verbose"]) + ')')
                # pylint:enable=unused-variable

            ocpiutil.logging.debug("running show action: '" + action + "'")
            # not using anything that is user defined so eval is fine
            # pylint:disable=eval-used
            eval(action)
            # pylint:enable=eval-used
    except ocpiutil.OCPIException as ex:
        ocpiutil.logging.error(ex)
        sys.exit(1)


if __name__ == '__main__':
    main()
