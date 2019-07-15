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
Definition of Componnet and ShowableComponent classes
"""

import os
import sys
import subprocess
import json
from xml.etree import ElementTree as ET
import _opencpi.util as ocpiutil
from .abstract import ShowableAsset

class ShowableComponent(ShowableAsset):
    """
    Any OpenCPI Worker or Component.  Intended to hold all the common functionality of workers and
    components.  Expected to be a virtual class an no real objects will get created of this class
    but nothing prevents it.
    """
    def __init__(self, directory, name=None, **kwargs):
        super().__init__(directory, name, **kwargs)
        # should be set in child classes
        self.ocpigen_xml = self.ocpigen_xml if self.ocpigen_xml else ""
        if kwargs.get("init_ocpigen_details", False):
            self.get_ocpigen_metadata(self.ocpigen_xml)
        package_id = kwargs.get("package_id", None)
        self.package_id = package_id if package_id else self.__init_package_id()

    def show(self, details, verbose, **kwargs):
        """
        Not implemented and not intended to be implemented
        """
        raise NotImplementedError("show() is not implemented")

    def get_ocpigen_metadata(self, xml_file):
        """
        Ask ocpigen (the code generator)  to parse the worker(OWD) or component(OCS) xml file and
        spit out an artifact xml that this function parses into class variables.
          property_list - list of every property, each property in this list will be a dictionary of
                          all the xml attributes associated with it from the artifact xml
          port_list     - list of every port each port in this list will be a dictionary of
                          all the xml attributes associated with it from the artifact xml some of
                          which are unused
          slave_list    - list of every slave worker's name expected to be blank for an OCS

        Function attributes:
          xml_file - the file to have ocpigen parse
        """
        #get list of locations to look for include xml files from make
        if ocpiutil.get_dirtype(self.directory + "/../") == "library":
            xml_dirs = ocpiutil.set_vars_from_make(mk_file=self.directory + "/../" + "/Makefile",
                                                   mk_arg="showincludes ShellLibraryVars=1",
                                                   verbose=True)["XmlIncludeDirsInternal"]
        elif ocpiutil.get_dirtype(self.directory + "/../") == "project":
            xml_dirs = ocpiutil.set_vars_from_make(mk_file=self.directory + "/../" + "/Makefile",
                                                   mk_arg="projectincludes ShellProjectVars=1",
                                                   verbose=True)["XmlIncludeDirsInternal"]
        #call ocpigen -G
        ocpigen_cmd = ["ocpigen", "-G", "-O", "none", "-V", "none", "-H", "none"]
        for inc_dir in xml_dirs:
            ocpigen_cmd.append("-I")
            ocpigen_cmd.append(inc_dir)

        ocpigen_cmd.append(xml_file)
        ocpiutil.logging.debug("running ocpigen cmd: " + str(ocpigen_cmd))
        old_log_level = os.environ.get("OCPI_LOG_LEVEL", "0")
        os.environ["OCPI_LOG_LEVEL"] = "0"
        comp_xml = subprocess.Popen(ocpigen_cmd, stdout=subprocess.PIPE).communicate()[0]
        os.environ["OCPI_LOG_LEVEL"] = old_log_level

        #put xml output file into an ElementTree object
        ocpiutil.logging.debug("Component Artifact XML from ocpigen: \n" + str(comp_xml))
        try:
            parsed_xml = ET.fromstring(comp_xml)
        except ET.ParseError:
            raise ocpiutil.OCPIException("Error with xml file from ocpigen.\n\nocpigen command: " +
                                         str(ocpigen_cmd) + "\n\nxml output: \n" + str(comp_xml))

        self.property_list = []
        self.port_list = []
        self.slave_list = []
        #set up self.property_list from the xml
        for props in parsed_xml.findall("property"):
            temp_dict = props.attrib
            enum = temp_dict.get("enums")
            if enum:
                temp_dict["enums"] = enum.split(",")
            if props.attrib.get("type") == "Struct":
                temp_dict["Struct"] = self.get_struct_dict_from_xml(props.findall("member"))
            self.property_list.append(temp_dict)
        #set up self.port_list from this dict
        for port in parsed_xml.findall("port"):
            port_details = port.attrib
            for child in port:
                if child.tag == "protocol":
                    port_details["protocol"] = child.attrib.get("padding", "N/A")
            self.port_list.append(port_details)
        #set up self.slave_list from the xml
        for slave in parsed_xml.findall("slave"):
            self.slave_list.append(slave.attrib["worker"])

    def __init_package_id(self):
        """
        Determine the Package id based on the library or project that the Worker resides in.  only
        a component will reside at the top level of a project.
        """
        parent_dir = self.directory + "/../"
        if ocpiutil.get_dirtype(parent_dir) == "library":
            ret_val = ocpiutil.set_vars_from_make(mk_file=parent_dir + "/Makefile",
                                                  mk_arg="showpackage ShellLibraryVars=1",
                                                  verbose=True)["Package"][0]
        elif ocpiutil.get_dirtype(parent_dir) == "project":
            ret_val = ocpiutil.set_vars_from_make(mk_file=parent_dir + "/Makefile",
                                                  mk_arg="projectpackage ShellProjectVars=1",
                                                  verbose=True)["ProjectPackage"][0]
        elif ocpiutil.get_dirtype(parent_dir) == "hdl-platforms":
            ret_val = "N/A"
        else:
            raise ocpiutil.OCPIException("Could not determine Package-ID of component dirtype of " +
                                         "parent directory: " + parent_dir + " dirtype: " +
                                         str(ocpiutil.get_dirtype(parent_dir)))
        return ret_val

    def __show_table_ports_props(self, json_dict, verbose, is_worker):
        """
        print out the ports and properties of the component in table format
        """
        if json_dict.get("properties"):
            rows = ([["Property Name", "Spec Property", "Type", "Accessability"]] if is_worker else
                    [["Property Name", "Type", "Accessability"]])
            for prop in json_dict["properties"]:
                access_str = ""
                for access in json_dict["properties"][prop]["accessibility"]:
                    if json_dict["properties"][prop]["accessibility"][access] == "1":
                        access_str += access + " "
                if is_worker:
                    rows.append([prop,
                                 json_dict["properties"][prop]["isImpl"] == 1,
                                 self.get_type_from_dict(json_dict["properties"][prop]),
                                 access_str])
                else:
                    rows.append([prop,
                                 self.get_type_from_dict(json_dict["properties"][prop]),
                                 access_str])
            ocpiutil.print_table(rows, underline="-")
            if verbose > 0:  #output any structs
                for prop in json_dict["properties"]:
                    if json_dict["properties"][prop]["type"] == "Struct":
                        print("Struct for " + prop)
                        rows = [["Member Name", "Type"]]
                        for member in json_dict["properties"][prop]["Struct"]:
                            member_dict = json_dict["properties"][prop]["Struct"][member]
                            rows.append([member,
                                         self.get_type_from_dict(member_dict)])
                        ocpiutil.print_table(rows, underline="-")
        if json_dict.get("ports"):
            rows = [["Port Name", "Protocol", "Producer"]]
            for port in json_dict["ports"]:
                rows.append([port,
                             json_dict["ports"][port]["protocol"],
                             json_dict["ports"][port]["producer"]])
            ocpiutil.print_table(rows, underline="-")

    @staticmethod
    def __show_simple_ports_props(json_dict):
        """
        print out the ports and properties of the component in simple format 
        """
        if json_dict.get("properties"):
            print("Properties:")
            for prop in json_dict["properties"]:
                print(prop + " ", end="")
            print()
        if json_dict.get("ports"):
            print("ports:")
            for port in json_dict["ports"]:
                print(port + " ", end="")
            print()

    def _show_ports_props(self, json_dict, details, verbose, is_worker):
        """
        Print out the ports and properties of a given component/worker given the dictionary that is
        passed in with this information in it

        Function attributes:
          json_dict  - the constructed dictionary to output the information for
          details    - the mode to print out the information in table or simple are the only valid
                       options
          verbose    - integer for verbosity level 0 is default and lowest and anything above 1
                       shows struct internals and hidden properties
          is_ worker - switch for component vs worker is intended for limited use otherwise there
                       should be 2 separate functions rather then using this Boolean
        """
        if details == "simple":
            self.__show_simple_ports_props(json_dict)
        elif details == "table":
            self.__show_table_ports_props(json_dict, verbose, is_worker)


    def get_struct_dict_from_xml(self, struct):
        """
        Static and recursive function that will generate the dictionary for Struct of Stuct of
        Struct ... etc. data-types.
        """
        ret_dict = {}
        for member in struct:
            name = member.attrib["name"]
            ret_dict[name] = {}
            if member.attrib.get("type", "ULong") == "Struct":
                ret_dict[name]["Struct"] = self.get_struct_dict_from_xml(member.findall("member"))
            ret_dict[name]["type"] = member.attrib.get("type", "ULong")
            ret_dict[name]["name"] = name
            enum = member.attrib.get("enums")
            if enum:
                ret_dict[name]["enums"] = enum.split(",")
            other_details = [detail for detail in member.attrib
                             if detail not in ["type", "name", "enums"]]
            for detail in other_details:
                ret_dict[name][detail] = member.attrib[detail]
        return ret_dict

    @classmethod
    def get_type_from_dict(cls, my_dict):
        """
        For use with printing in table mode will output a more informational data-type for more
        complex data-types like arrays, structs, enums etc.  returns a string of the data-type that
        caller prints out to the screen
        """
        base_type = my_dict.get("type", "ULong")
        is_seq = my_dict.get("sequenceLength")
        is_array = my_dict.get("arrayLength")
        if not is_array:
            is_array = my_dict.get("arrayDimensions")
        is_enum = my_dict.get("enums")
        is_string = my_dict.get("stringLength")
        #change the base-type string for enums or strings
        if is_enum:
            base_type = "Enum " + str(is_enum)
        elif is_string:
            base_type = "String[" + is_string + "]"
        #add sequence or array information on to the front of the output string where applicable
        if is_seq and is_array:
            prop_type = "Sequence{" + is_seq + "} of Array[" + is_array +"] of " + base_type
        elif is_seq:
            prop_type = "Sequence{" + is_seq + "} of " + base_type
        elif is_array:
            prop_type = "Array[" + is_array + "] of " + base_type
        else:
            prop_type = base_type
        return prop_type

    def _get_show_dict(self, verbose):
        """
        compose and return the dictionary that the show verb uses to output information about this
        worker/component.
        Function attributes:
          verbose - data-type of integer if number is non zero hidden properties are added to the
                    dictionary as well as more information about struct data-types
        """
        json_dict = {}
        port_dict = {}
        prop_dict = {}
        for prop in self.property_list:
            if verbose > 0 or prop.get("hidden", "0") == "0":
                combined_reads = prop.get("readable", "0") +  prop.get("readback", "0")
                # doing an or on string values is set to "1" if either readable or readback are "1"
                readback = "0" if combined_reads == "00" else "1"
                prop_detatils = {"accessibility": {"initial" : prop.get("initial", "0"),
                                                   "readback" : readback,
                                                   "writable" : prop.get("writable", "0"),
                                                   "volatile" : prop.get("volatile", "0"),
                                                   "parameter" : prop.get("parameter", "0"),
                                                   "padding" : prop.get("padding", "0")},
                                 "name" : prop.get("name", "N/A"),
                                 "type" : prop.get("type", "ULong")}

                required_details = ["initial", "readback", "writable", "volatile", "parameter",
                                    "padding", "type", "name", "specparameter", "specinitial",
                                    "specwritable", "specreadable", "specvolitile"]
                if verbose <= 0:
                    #Adding this here causes struct to not be put into the dictionary
                    required_details.append("Struct")
                other_details = [detail for detail in prop if detail not in required_details]
                for prop_attr in other_details:
                    prop_detatils[prop_attr] = prop[prop_attr]
                prop_dict[prop["name"]] = prop_detatils
        for port  in self.port_list:
            port_detatils = {"protocol": port.get("protocol", None),
                             "producer": port.get("producer", "0")}
            port_dict[port["name"]] = port_detatils
        json_dict["ports"] = port_dict
        json_dict["properties"] = prop_dict
        json_dict["name"] = self.name
        json_dict["package_id"] = self.package_id
        json_dict["directory"] = self.directory
        return json_dict

class Component(ShowableComponent):
    """
    Any OpenCPI Component.
    """
    def __init__(self, directory, name=None, **kwargs):
        if not name:
            name = os.path.basename(directory)
        directory = os.path.dirname(directory)
        self.ocpigen_xml = directory + "/" + name
        super().__init__(directory, name, **kwargs)

    @classmethod
    def is_component_spec_file(cls, file):
        """
        Determines if a provided xml file contains a component spec.

        TODO do we actually want to open files to make sure and not just rely on the naming
             convention???
        """

        return file.endswith(("_spec.xml", "-spec.xml"))

    def show(self, details, verbose, **kwargs):
        """
        Print out the ports and properties of a given component in the format that is provided by
        the caller

        Function attributes:
          details    - the mode to print out the information in table or simple are the only valid
                       options
          verbose    - integer for verbosity level 0 is default and lowest and anything above 1
                       shows struct internals and hidden properties
          kwargs     - no extra kwargs arguments expected
        """
        json_dict = self._get_show_dict(verbose)

        if details == "simple" or details == "table":
            print("Component: " + json_dict["name"] + " Package ID: " + json_dict["package_id"])
            print("Directory: " + json_dict["directory"])
            self._show_ports_props(json_dict, details, verbose, False)
        else:
            json.dump(json_dict, sys.stdout)
            print()

    @staticmethod
    def get_working_dir(name, library, hdl_library, hdl_platform):
        """
        return the directory of a Component given the name (name) and
        library specifiers (library, hdl_library, hdl_platform)
        """
        # if more then one of the library location variable are not None it is an error.
        # a length of 0 assumes default location of <project>/specs

        if len(list(filter(None, [library, hdl_library, hdl_platform]))) > 1:
            ocpiutil.throw_invalid_libs_e()
        cur_dirtype = ocpiutil.get_dirtype()
        valid_dirtypes = ["project", "libraries", "library", "hdl-platform"]
        if cur_dirtype not in valid_dirtypes:
            ocpiutil.throw_not_valid_dirtype_e(valid_dirtypes)
        if library:
            if not library.startswith("components"):
                library = "components/" + library
            specs_loc = ocpiutil.get_path_to_project_top() + "/" + library + "/specs/"
        elif hdl_library:
            specs_loc = ocpiutil.get_path_to_project_top() + "/hdl/" + hdl_library + "/specs/"
        elif hdl_platform:
            specs_loc = (ocpiutil.get_path_to_project_top() + "/hdl/platforms/" + hdl_platform +
                         "/devices/specs/")
        elif name:
            if cur_dirtype == "hdl-platform":
                specs_loc = "devices/specs/"
            else:
                specs_loc = "specs/"
        else: ocpiutil.throw_not_blank_e("component", "name", True)
        return ocpiutil.get_component_filename(specs_loc, name)
