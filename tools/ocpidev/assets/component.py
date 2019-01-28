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
import os
import sys
import logging
import subprocess
import json
from xml.etree import ElementTree as ET

# Skeleton class
class Component(Asset):
    def __init__(self, directory, name=None, **kwargs):
        if not name:
            name= os.path.basename(directory)
        directory= os.path.dirname(directory)
        super().__init__(directory, name, **kwargs)

        self.package_id =  self.__init_package_id()
        if kwargs.get("init_comp_details", False):
            self.get_ocpigen_metadata()

    @classmethod
    def is_component_spec_file(cls, file):
        #TODO do we actually want to open files to make sure and not just rely on the nameing
        #     convention???
        return file.endswith(("_spec.xml", "-spec.xml"))

    def __init_package_id(self):
        parent_dir = self.directory + "/../"
        if ocpiutil.get_dirtype(parent_dir) == "library":
            ret_val = ocpiutil.set_vars_from_make(mk_file=parent_dir + "/Makefile",
                                                  mk_arg="showpackage ShellLibraryVars=1",
                                                  verbose=True)["Package"][0]
        elif ocpiutil.get_dirtype(parent_dir) == "project":
            ret_val = ocpiutil.set_vars_from_make(mk_file=parent_dir + "/Makefile",
                                                  mk_arg="projectpackage ShellProjectVars=1",
                                                  verbose=True)["ProjectPackage"][0]
        else:
            raise ocpiutil.OCPIException("Could not determine Package-ID of component dirtype of " +
                                         "parent directory: " + parent_dir + " dirtype: " +
                                         str(ocpiutil.get_dirtype(parent_dir)))
        return ret_val

    def get_ocpigen_metadata(self):
        #get list of locations to look for include xmls from make
        parent_dir = self.directory + "/../"
        if ocpiutil.get_dirtype(parent_dir) == "library":
            make_vars = ocpiutil.set_vars_from_make(mk_file=parent_dir + "/Makefile",
                                                    mk_arg="showincludes ShellLibraryVars=1",
                                                    verbose=True)
        elif ocpiutil.get_dirtype(parent_dir) == "project":
            make_vars = ocpiutil.set_vars_from_make(mk_file=parent_dir + "/Makefile",
                                                    mk_arg="projectincludes ShellProjectVars=1",
                                                    verbose=True)
        xml_include_dirs = make_vars["XmlIncludeDirsInternal"]
        #call ocpigen -G
        ocpigen_cmd = ["ocpigen", "-G", "-O", "none", "-V", "none", "-H", "none"]
        for inc_dir in xml_include_dirs:
            ocpigen_cmd.append("-I")
            ocpigen_cmd.append(inc_dir)
        ocpigen_cmd.append(self.directory + "/" + self.name)
        ocpiutil.logging.debug("running ocpigen cmd: " + str(ocpigen_cmd))
        old_log_level=os.environ.get("OCPI_LOG_LEVEL", "0")
        os.environ["OCPI_LOG_LEVEL"] = "0"
        comp_xml = subprocess.Popen(ocpigen_cmd, stdout=subprocess.PIPE).communicate()[0]
        os.environ["OCPI_LOG_LEVEL"]= old_log_level


        #put xml ouput file into an ElementTree object
        ocpiutil.logging.debug("Component Artifact XML from ocpigen: \n" + str(comp_xml))
        parsed_xml= ET.fromstring(comp_xml)

        self.property_list= []
        self.port_list= []
        #set up self.property_list from the xml
        for props in parsed_xml.findall("property"):
            temp_dict = props.attrib
            enum=temp_dict.get("enums")
            if enum:
                temp_dict["enums"]=enum.split(",")
            if props.attrib.get("type") == "Struct":
                temp_dict["Struct"] = self.get_struct_dict_from_xml(props.findall("member"))
            self.property_list.append(temp_dict)

        #set up self.port_list from this dict
        for port in parsed_xml.findall("port"):
            port_details = port.attrib
            for child in port:
                if child.tag == "protocol":
                    port_details["protocol"] = child.attrib["name"]
            self.port_list.append(port_details)

    def get_struct_dict_from_xml(self, struct):
        ret_dict= {}
        for member in struct:
            name= member.attrib["name"]
            ret_dict[name]={}
            if member.attrib["type"] == "Struct":
                ret_dict[name]["Struct"]= self.get_struct_dict_from_xml(member.findall("member"))
            ret_dict[name]["type"]= member.attrib.get("type", "ULong")
            ret_dict[name]["name"]= name
            enum= member.attrib.get("enums")
            if enum:
                ret_dict[name]["enums"]=enum.split(",")
            other_details = [detail for detail in member.attrib
                             if detail not in ["type", "name", "enums"]]
            for detail in other_details:
                ret_dict[name][detail]= member.attrib[detail]
        return ret_dict

    #TODO move to ocpiutil or asset class
    @classmethod
    def get_type_from_dict(cls, my_dict):
        base_type = my_dict.get("type", "ULong")
        is_seq = my_dict.get("sequenceLength")
        is_array = my_dict.get("arrayLength")
        is_enum = my_dict.get("enums")
        is_string = my_dict.get("stringLength")
        #change the base tyoe string for enums or strings
        if is_enum:
            base_type = "Enum " + str(is_enum)
        elif is_string:
            base_type = "String[" + is_string + "]"
        #add sequence tor array information on to the front of the output string where applicable
        if is_seq and is_array:
            prop_type = "Sequence{" + is_seq + "} of Array[" + is_array +"] of " + base_type
        elif is_seq:
            prop_type = "Sequence{" + is_seq + "} of " + base_type
        elif is_array:
            prop_type = "Array[" + is_array + "] of " + base_type
        else:
            prop_type= base_type
        return prop_type

    def show(self, details, verbose, **kwargs):
        json_dict = {}
        port_dict = {}
        prop_dict = {}
        for prop in self.property_list:
            prop_detatils = {"accessibility": {"initial" : prop.get("initial", "0"),
                                               "readable" : prop.get("readable", "0"),
                                               "writable" : prop.get("writable", "0"),
                                               "volatile" : prop.get("volatile", "0"),
                                               "parameter" : prop.get("parameter", "0"),
                                               "padding" : prop.get("padding", "0") },
                             "name" : prop["name"],
                             "type" : prop.get("type", "ULong") }

            #prop_detatils["type"]= self.get_type_from_dict(prop)
            required_details = ["initial", "readable", "writable", "volatile", "parameter",
                                "padding", "type", "name"]
            if verbose <= 0:
                #Adding this here causes struct to not be put into the dictonary
                required_details.append("Struct")
            other_details = [detail for detail in prop if detail not in required_details]
            for prop_attr in other_details:
                prop_detatils[prop_attr] = prop[prop_attr]
            prop_dict[prop["name"]] = prop_detatils
        for port  in self.port_list:
            port_detatils = {"protocol": port.get("protocol", None),
                             "producer": port.get("producer", "0") }
            port_dict[port["name"]] = port_detatils
        json_dict["ports"] = port_dict
        json_dict["properties"] = prop_dict
        json_dict["name"] = self.name
        json_dict["package_id"] = self.package_id
        json_dict["directory"] = os.path.dirname(self.directory)

        if details == "simple":
            print("Component: " + json_dict["name"] + " Package ID: " + json_dict["package_id"])
            print("Directory: " + json_dict["directory"])
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
        elif details == "table":
            print("Component: " + json_dict["name"] + " Package ID: " + json_dict["package_id"])
            print("Directory: " + json_dict["directory"])
            if json_dict.get("properties"):
                rows = [["Property Name", "Type", "Accessability"]]
                for prop in json_dict["properties"]:
                    access_str = ""
                    for access in json_dict["properties"][prop]["accessibility"]:
                        if json_dict["properties"][prop]["accessibility"][access] == "1":
                            access_str += access + " "
                    rows.append([prop,
                                self.get_type_from_dict(json_dict["properties"][prop]),
                                access_str])
                ocpiutil.print_table(rows, underline="-")
                if verbose >= 0:  #output any structs\
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
        else:
            json.dump(json_dict, sys.stdout)
            print()
