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
Show HDL utilization for an OpenCPI HDL Asset
"""
import argparse
import os
import sys
import pydoc
import types
import _opencpi.util as ocpiutil
import _opencpi.assets.factory as ocpifactory
from _opencpi.assets.abstract import ReportableAsset

# There is a top-level parser that parses FIRST_NOUNS
# If the noun is a PLAIN_NOUN, it proceeds normally,
# but if the noun is a SUB_PARSER_NOUN, then it
# looks for a second noun from the SUBNOUNS list.
PLAIN_NOUNS = ["worker", "workers", "library", "project"]
SUB_PARSER_NOUNS = ["hdl"]
FIRST_NOUNS = PLAIN_NOUNS + SUB_PARSER_NOUNS
SUBNOUNS = {}
SUBNOUNS['hdl'] = ["platform", "platforms", "assembly", "assemblies"]

# NOUNS is the list of all nouns, where noun-subparser nouns
# are listed as <noun>-<subnoun>
NOUNS = PLAIN_NOUNS
for noun in SUB_PARSER_NOUNS:
    for subnoun in SUBNOUNS[noun]:
        NOUNS.append(noun + "-" + subnoun)

# Most nouns correspond to dirtypes, except nouns that represent 'all' or 'multiple' assets of a
# certain type. For example 'workers' or 'tests' do not correspond to dirtypes, but to multiple
# assets of the singular type (worker/test).
#
# 'hdl-assemblies' on the other hand is a noun AND a dirtype, because there is actually a directory
# of that type (hdl/assemblies).
#
# Likewise, applications is a dirtype because there is a directory of that type.
NOUN_PLURALS = ["workers"]
DIR_TYPES = [noun for noun in NOUNS if noun not in NOUN_PLURALS]

def parse_cl_vars():
    """
    Construct the argparse object and parse all the command line arguments into a dictionary to
    return
    """
    description = ("""
    Utility for displaying/recording FPGA resource utilization for HDL OpenCPI assets.\n
    Usage Examples: \n
        show utilization for a single worker (using build results from all platforms):
            ocpidev utilization worker <worker-name>

        show utilization for a single worker (using build results from a single platform):
            ocpidev utilization worker <worker-name> --hdl-platform <hdl-platform>

        show utilization for a single worker (using build results from a single target):
            ocpidev utilization worker <worker-name> --hdl-target <hdl-target>

        show utilization for all workers (in the current project/library/etc):
            ocpidev utilization workers

        show utilization for a single worker in a named library:
            ocpidev utilization worker <worker-name> -l <library>

        show utilization for a single HDL Platform:
            ocpidev utilization hdl platform <platform-name>

        show utilization for all HDL Platforms (in the current project):
            ocpidev utilization hdl platforms

        show utilization for a single HDL Assembly:
            ocpidev utilization hdl assembly <assembly-name>

        show utilization for all HDL Assemblies (in the current project):
            ocpidev utilization hdl assemblies

        record utilization for a single HDL Assembly in LaTeX format:
            ocpidev utilization hdl assembly <assembly-name> --format=latex

        show utilization for all supported assets in a project:
            ocpidev utilization project

        record utilization for all supported assets in a project in LaTeX format:
            ocpidev utilization project --format=latex
    """)

    parser = argparse.ArgumentParser(description=description,
                                     formatter_class=argparse.RawTextHelpFormatter,
                                     prog="ocpidev utilization")
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
    parser.add_argument("noun", type=str, nargs='?', help="This is either the noun to show or the" +
                        " authoring model to operate on. If choosing an authoring model " +
                        "(hdl), there are secondary nouns that can follow.\nValid " +
                        "nouns are: " + ", ".join(FIRST_NOUNS) + "\nValid secondary " +
                        "nouns for 'hdl' are: " + ", ".join(SUBNOUNS['hdl']),
                        choices=FIRST_NOUNS)
    parser.add_argument("-v", "--verbose", action="store_true", help="Be verbose with output.")

    parser.add_argument("--format", dest="output_format", default="table",
                        choices=ReportableAsset.valid_formats,
                        help='Format to output utilization information. "latex" results in ' +
                        'silent stdout, and all output goes to "utilization.inc" files ' +
                        'in the directories for the assets acted on.')
    parser.add_argument("--hdl-platform", metavar="HDL_PLAT", dest="hdl_plats", action="append",
                        help="Specify which HDL platform from the list of buildable " +
                        "platforms to show utilization for.")
    parser.add_argument("--hdl-target", metavar="HDL_TGT", dest="hdl_tgts", action="append",
                        help="Specify which HDL target from the list of buildable " +
                        "targets to show utilization for. Only valid for workers (not assemblies)")
    parser.add_argument("-d", dest="cur_dir", default=os.path.curdir,
                        help="Change directory to the specified path before proceeding. " +
                        "Changing directory may have no effect for some commands.")
    parser.add_argument("-l", "--library", dest="library", default=None,
                        help="Specify the component library in which this operation will be " +
                        "performed.")
    parser.add_argument("--hdl-library", dest="hdl_library", default=None,
                        help="Specify the hdl library in which this operation will be " +
                        "performed.")
    parser.add_argument("-P", dest="hdl_plat_dir", default=None,
                        help="Specify the hdl platform subdirectory to operate in.")

    first_pass_args, remaining_args0 = parser.parse_known_args()

    # Create a subparser with similar initialization as the top-level parser
    # This will parse sub-nouns (e.g. after authoring model) and the actual name/asset
    # to act on
    subparser = argparse.ArgumentParser(description=description)
    subparser.print_help = parser.print_help
    subparser.print_usage = parser.print_usage
    subparser.error = parser.error

    # If this is a sub-parser-noun, add an argument to handle the list of subnouns
    first_noun = first_pass_args.noun
    if first_noun in SUB_PARSER_NOUNS:
        subnouns = SUBNOUNS[first_pass_args.noun]
        subparser.add_argument("subnoun", type=str, help='The sub-noun to operate on (after ' +
                               'the specified first noun "' + first_noun + '"). Options are ' +
                               '"' + ", ".join(subnouns) + '".',
                               choices=subnouns)

    # finally, parse the actual name of the asset to act on
    subparser.add_argument("name", default="", type=str, action="store", nargs='?',
                           help="This is the name of the asset to show utilization for.")

    args, remaining_args1 = subparser.parse_known_args(remaining_args0, namespace=first_pass_args)
    vars_args = vars(args)
    if "subnoun" in vars_args:
        vars_args["noun"] = first_pass_args.noun + "-" + vars_args["subnoun"]
    if vars_args["noun"] is None:
        vars_args["noun"] = ""

    if remaining_args1:
        ocpiutil.logging.error("invalid options were used: " + " ".join(remaining_args1))
        sys.exit(1)

    return vars_args

def set_init_values(args, dir_type):
    """
    Determine which contents of library and project objects need to be initialized and set the
    corresponding kwargs values
    """
    my_noun = dir_type if args['noun'] is None else args['noun']
    container_nouns = ["library", "libraries", "project"]
    if (my_noun in ["workers", "library", "libraries", "project"] or dir_type in container_nouns):
        args['init_workers'] = True
    if my_noun in ["hdl-assemblies", "project"] or dir_type in ["hdl-assemblies", "project"]:
        args['init_hdlassembs'] = True
    if my_noun in ["workers", "libraries", "project"] or dir_type in ["libraries", "project"]:
        args['init_libs'] = True
    if my_noun in ["hdl-platforms", "project"] or dir_type in ["hdl-platforms", "project"]:
        args['init_hdlplats'] = True
    if my_noun in ["library", "workers", ""] and dir_type == "libraries":
        args['init_libs_col'] = True
    if my_noun == "workers":
        if dir_type not in ["library", "libraries", "project"]:
            raise ocpiutil.OCPIException('Use of workers noun in an invalid directory type: "' +
                                         dir_type + '". Valid types are library and project')
    # This driver defaults hdl_plats to all
    if not args['hdl_plats'] and not args['hdl_tgts']:
        args['hdl_plats'] = ["all"]

def main():
    """
    Function that is called if this module is called as a main function
    """
    args = parse_cl_vars()
    try:
        cur_dir = args['cur_dir']
        with ocpiutil.cd(cur_dir):
            cur_dir_basename = os.path.basename(os.path.realpath("."))

            name = args['name']
            # Now that we have grabbed name, delete it from the args that will be passed into the
            # AssetFactory because name is explicitly passed to AssetFactory as a separate argument
            del args['name']

            # From the current directory and directory-modifier options, determine
            # the directory to operate from for this ocpidev command
            directory = ocpiutil.get_ocpidev_working_dir(noun=args.get("noun", ""),
                                                         name=name,
                                                         library=args['library'],
                                                         hdl_library=args['hdl_library'],
                                                         hdl_platform=args['hdl_plat_dir'])

            ocpiutil.logging.debug('Choose directory "' + directory + '" to operate in')

            dir_type = ocpiutil.get_dirtype(directory)

            # Check dir_type for errors:
            # If there is no noun, and the working directory is not a supported type
            if args['noun'] is None and dir_type not in DIR_TYPES:
                raise ocpiutil.OCPIException('Invalid directory type "' + str(dir_type) +
                                             '" Valid directory types are: ' +
                                             ", ".join(DIR_TYPES))

            # If name is not set, set it to the current directory's basename
            if not name and dir_type in DIR_TYPES:
                name = cur_dir_basename

            # Initialize settings to be used by Asset classes
            set_init_values(args, dir_type)

            # Check worker authoring model and strip authoring model for Worker construction
            if args['noun'] == "worker" or (args['noun'] is None and dir_type == "worker"):
                if not name.endswith(".hdl"):
                    ocpiutil.logging.warning("Can only show utilization for workers of authoring " +
                                             "model 'hdl'.")
                    sys.exit()
                name = name.rsplit('.', 1)[0]

            ocpiutil.logging.debug("Creating asset object with the following \nname: " +
                                   str(name) + "\n" + "directory: " + str(directory) +
                                   "\nargs: " + str(args))
            my_asset = ocpifactory.AssetFactory.factory(dir_type, directory, name, **args)
            my_asset.show_utilization()

    except ocpiutil.OCPIException as ex:
        ocpiutil.logging.error(ex)
        # Construct error message and exit
        if args['noun'] is not None:
            my_noun = '' if args['noun'] is None else ' "' + args['noun'] + '"'
            my_name = '' if not name else ' named "' + name + '"'
            my_dir = ' in directory "' + args['cur_dir'] + '"'
            ocpiutil.logging.error("Unable to show utilization for " + my_noun +  my_name + my_dir +
                                   " due to previous errors")
        else:
            ocpiutil.logging.error("Unable to show utilization due to previous errors.")
        sys.exit(1)

if __name__ == '__main__':
    main()
