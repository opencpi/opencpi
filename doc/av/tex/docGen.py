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

# TODO / FIXME: Handle xinclude properly

import os
import itertools
import re
import shutil
import sys
import textwrap
from xml.etree import ElementTree as etree
from enum import Enum
try:
    import jinja2
    from jinja2 import Template
except ImportError:
    print("ERROR : Could not import jinja2; try 'sudo yum install python34-jinja2'",
          file=sys.stderr)
    sys.exit(1)

sys.path.append(os.path.dirname(os.path.realpath(__file__)) + '/../../../tools/cdk/python/')
import _opencpi.util  as ocpi
# Set OCPI_LOG_LEVEL to a desired number to see warnings or debug statements

# User setup:
USE_SECTION_NUMBERS = False
NEWLINE_PER_TABLE_CELL = False

# End of user-configurable data (if you change these, some code or string constants need fixes)
LC_NAME = ''
UC_NAME = ''
PROP_LIST = ['volatile', 'readable', 'readback', 'writable', 'initial']  # Unused?
SECTION_HEADER = r"\section"+(('*', '')[USE_SECTION_NUMBERS])+"{{{}}}\n"
COMPONENT_PROPS_INC_FILENAME = 'component_spec_properties.inc'
WORKER_PROPS_INC_FILENAME = 'worker_properties.inc'
COMPONENT_PORTS_INC_FILENAME = 'component_ports.inc'
WORKER_PORTS_INC_FILENAME = 'worker_interfaces.inc'
DEVELOPER_DOC_INC_FILENAME = 'developer_doc.inc'
mywrapper = textwrap.TextWrapper(width=shutil.get_terminal_size()[0])


# http://eosrei.net/articles/2015/11/latex-templates-python-and-jinja2-generate-pdfs
LATEX_JINJA_ENV = jinja2.Environment(
    block_start_string=r'\BLOCK{',
    block_end_string='}',
    variable_start_string=r'\VAR{',
    variable_end_string='}',
    comment_start_string=r'\#{',
    comment_end_string='}',
    line_statement_prefix='%%',
    line_comment_prefix='%#',
    trim_blocks=True,
    autoescape=False,
    loader=jinja2.FileSystemLoader(os.path.dirname(os.path.realpath(__file__))+"/snippets/jinja2/")
)


def scramble_case(val):
    """ Scrambles all permutations of a string's case
    >>> scramble_case('ab')
    ['AB', 'Ab', 'aB', 'ab']
    >>> scramble_case('ab3')
    ['AB3', 'Ab3', 'aB3', 'ab3']
    """
    # https://stackoverflow.com/a/11144539/836748
    # Want unique, otherwise numbers make repeats because lower = upper
    temp_set = set(map(''.join, itertools.product(*list(zip(val.upper(), val.lower())))))
    temp_list = list(temp_set)
    temp_list.sort()
    return temp_list


def get_bool_from_xml_val(val, default=False):
    # TODO: This seems to be unused?
    """ Converts val to boolean with a default of 'default' if missing
    >>> get_bool_from_xml_val(None)
    False
    >>> get_bool_from_xml_val(None, 'nooope')
    'nooope'
    >>> get_bool_from_xml_val('TrUe')
    True
    >>> get_bool_from_xml_val('1')
    True
    >>> get_bool_from_xml_val('0')
    False
    >>> get_bool_from_xml_val('false')
    False
    >>> get_bool_from_xml_val('falsssssse')
    Traceback (most recent call last):
        ...
    Exception: get_bool_from_xml_val() cannot handle input: falsssssse
    """
    if val is None:
        return default
    if val.lower() in ['0', 'false']:
        return False
    if val.lower() in ['1', 'true']:
        return True
    raise Exception("get_bool_from_xml_val() cannot handle input: " + val)


def check_is_property_access_valid(key):
    # TODO: This seems to be unused?
    """ Confirms property accessibility is valid option """
    return key.lower() in PROP_LIST


def get_xml_attributes_as_list(root, keys):
    """ Extracts XML attributes based on given keys in any case """
    ret = []
    for key in [scramble_case(k) for k in keys]:
        for i in key:
            for attr in root.iter():
                if attr.tag == i:
                    ret.append(attr)
    return ret


def get_anycase(attrs, key):
    """ Helper function to call attrs.get() based on given keys in any case permutation.
        Calls "items" on attrs because lxml element does not act as a pythonic list
        (it doesn't support "in")
    """
    attr_keys = [v[0] for v in list(attrs.items())]
    valid_keys = [k for k in scramble_case(key) if k in attr_keys]
    if valid_keys:
        assert len(valid_keys) == 1  # There should never be more than one permutation
        return attrs.get(valid_keys[0])
    return None


def get_dict_from_attributes(attrs, keys):
    """ Creates dict based on given keys in any case permutation
    >>> test = {'key1': 'A', 'key2': 'B', 'KeY3': 'C'}
    >>> get_dict_from_attributes(test, ['KEY3']) ==  {'KEY3': 'C'}
    True
    >>> get_dict_from_attributes(test, ['kEy1', 'KEY3']) == {'KEY3': 'C', 'kEy1': 'A'}
    True
    >>> get_dict_from_attributes(test, [])
    {}
    >>> get_dict_from_attributes(test, ['Nope'])
    {'Nope': None}

    """
    return {key: get_anycase(attrs, key) for key in keys}


def get_prop_dict_from_attributes(attrs):
    """ Creates dict based on a property's attributes we care about """
    keys = ["Name", "Type", "SequenceLength",
            "Volatile", "Readable", "Readback", "Writable",
            "Initial", "Parameter", "Enums", "Default", "ReadSync", "WriteSync",
            "Description", "ArrayDimensions", "Value"]
    ret = get_dict_from_attributes(attrs, keys)

    # Check for ArrayLength as a fallback if ArrayDimensions missing
    if not ret["ArrayDimensions"]:
        ret["ArrayDimensions"] = get_anycase(attrs, "ArrayLength")

    # Enforce defaults
    if not ret["Type"]:
        ret["Type"] = "ulong"
    if not ret["Default"]:
        ret["Default"] = ret["Value"]
    if not ret["Value"]:
        if ((ret["Initial"] == "true") or (ret["Writable"] == "true") or
                (ret["Parameter"] == "true")):
            ret["Default"] = 0

    return ret


def get_ports_from_xml(root):
    """ Creates list based on port keywords in XML """
    keys = ["DataInterfaceSpec", "Port", "StreamInterface"]
    return get_xml_attributes_as_list(root, keys)


def get_properties_from_xml(root, isOCS):
    """ Creates list based on property keywords in XML """
    ret = get_xml_attributes_as_list(root, ["Property"])
    if not isOCS:
        ret += get_xml_attributes_as_list(root, ["SpecProperty"])
    return ret


def parse_root_xml(root):
    """ Main XML parsing routine.
        Creates dict with various flags as well as component/worker ports and properties.
        Checks any case permutation of various known flags, e.g. RccWorker and HdlWORker.
    """
    res = {'isHDL': False,
           'isOCS': True,
           'cProps': [],
           'wProps': [],
           'cPorts': [],
           'wPorts': []}
    for elem in scramble_case("RccWorker"):
        for worker in root.iter(elem):
            res['isOCS'] = False
            res['isHDL'] = False
            res['wPorts'] = get_ports_from_xml(worker)
            res['wProps'] = get_properties_from_xml(worker, isOCS=False)
    for elem in scramble_case("HdlWorker"):
        for worker in root.iter(elem):
            res['isOCS'] = False
            res['isHDL'] = True
            res['wPorts'] = get_ports_from_xml(worker)
            res['wProps'] = get_properties_from_xml(worker, isOCS=False)
    for elem in scramble_case("HdlDevice"):
        for worker in root.iter(elem):
            res['isOCS'] = False
            res['isHDL'] = True
            res['wPorts'] = get_ports_from_xml(worker)
            res['wProps'] = get_properties_from_xml(worker, isOCS=False)
    for elem in scramble_case("ComponentSpec"):
        for comp in root.iter(elem):
            res['cPorts'] = get_ports_from_xml(comp)
            if res['isOCS']:
                res['cProps'] = get_properties_from_xml(comp, isOCS=True)
            else:
                res['wProps'].append(get_properties_from_xml(comp, isOCS=True))
    return res


def component_name_from_path(ocs_file_path):
    """ Computes base component name from full path
    >>> component_name_from_path('../../specs/fifo-spec.xml')
    'fifo'
    >>> component_name_from_path('../../fifo.hdl/fifo.xml')
    'fifo'
    >>> component_name_from_path('fifo-spec.xml')
    'fifo'
    >>> component_name_from_path('fifo.xml')
    'fifo'
    """
    file_name = os.path.split(ocs_file_path)[-1]  # Last element of path
    file_basename = os.path.splitext(file_name)[0]
    return re.sub('[_-]spec', '', file_basename)  # Strip off -spec or _spec


def update_global_names(my_name):
    """ Updates globals UC_NAME and LC_NAME after normalizing.
        UC_NAME will convert _ and - to spaces and get Title Case.
        LC_NAME will be LaTeX-safe.
        (Cannot get globals to work with doctest)
    """
    global UC_NAME
    global LC_NAME
    # my_name = "This_is_my_really_long-name-to-test-with"
    # Convert underscores and hyphens to spaces and then title case:
    UC_NAME = (" ".join(re.split('[-_]', my_name))).title()
    LC_NAME = latexify(my_name)


def update_global_names_from_path(my_path):
    """ Updates globals UC_NAME and LC_NAME based on an XML file's path """
    update_global_names(component_name_from_path(my_path))


def latexify(string):
    """ Escapes TeX-reserved characters
    >>> latexify("this is a test")
    'this is a test'
    >>> latexify("this is a test & so was this") == r"this is a test \& so was this"
    True
    >>> latexify("this_is_a_test") == r"this\_is\_a\_test"
    True
    >>> latexify("Please don't % comment ^^this^^ out") == r"Please don't \% comment \^{}\^{}this\^{}\^{} out"
    True
    """
    string = string.replace('_', r'\_')
    string = string.replace('&', r'\&')
    string = string.replace('%', r'\%')
    string = string.replace('^', r'\^{}')
    return string


def get_tex_table_row(data):
    """ This will generate a table row based on list given to it """
    return r"""\hline
{}\\
""".format(" &{}".format((' ', '\n')[NEWLINE_PER_TABLE_CELL]).join(data))  # Note: The \n gives line per cell vs row


class PropertyType(Enum):
    """ Enumerated list that gives us str() methods for output later """
    ULONG = 0
    LONG = 1
    ULONGLONG = 2
    LONGLONG = 3
    USHORT = 4
    SHORT = 5
    UCHAR = 6


class Property(object):
    """ Property class that self-generates from XML attributes dict """
    # TODO: Use the Property class from ocpidev stuff?
    # NOTE: not using _name because pylint barfs w/ PropertyLatexTableRow
    def __init__(self, xml_attributes):
        self.name = xml_attributes["Name"]
        self.ptype = xml_attributes.get("Type", PropertyType.ULONG)
        self.sequence_length = xml_attributes.get("SequenceLength")
        self.array_dimensions = xml_attributes.get("ArrayDimensions")
        self.volatile = xml_attributes.get("Volatile", False)
        self.readable = xml_attributes.get("Readable", False)
        self.readback = xml_attributes.get("Readback", False)
        self.writable = xml_attributes.get("Writable", False)
        self.initial = xml_attributes.get("Initial", False)
        self.parameter = xml_attributes.get("Parameter", False)
        self.enums = xml_attributes["Enums"]
        self.default = xml_attributes.get("Default", 0)
        self.readsync = xml_attributes.get("ReadSync", False)
        self.writesync = xml_attributes.get("WriteSync", False)
        self.description = xml_attributes.get("Description", '')
        # TODO / FIXME - is this really a warning?
        if not self.description:
            msg = " does not have a description in the XML"
            ocpi.logging.warning("WARN :" + self.name + msg)

    def __str__(self):
        ret = "name=" + str(self.name)
        ret += ", ptype=" + str(self.ptype)
        ret += ", sequence_length=" + str(self.sequence_length)
        ret += ", array_dimensions=" + str(self.array_dimensions)
        ret += ", volatile=" + str(self.volatile)
        ret += ", readable=" + str(self.readable)
        ret += ", readback=" + str(self.readback)
        ret += ", writable=" + str(self.writable)
        ret += ", initial=" + str(self.initial)
        ret += ", parameter=" + str(self.parameter)
        ret += ", enums=" + str(self.enums)
        ret += ", default=" + str(self.default)
        ret += ", readsync=" + str(self.readsync)
        ret += ", writesync=" + str(self.writesync)
        ret += ", description=" + str(self.description)
        return ret

    def as_latex_table_row(self):
        """ Creates a PropertyLatexTableRow and then returns string version """
        return str(PropertyLatexTableRow(self))


class PropertyLatexTableRow(object):
    """ Helper class that creates a LaTeX table row based on a Property """
    def __init__(self, prop):
        self.prop = prop

    def __str__(self):
        return get_tex_table_row([
            latexify(str(self.prop.name)),
            latexify(str(self.prop.ptype)),
            latexify(str(self.prop.sequence_length or '-')),
            latexify(str(self.prop.array_dimensions or '-')),
            latexify(self.get_accessibility()),
            latexify(self.get_enums()),
            self.get_default_str(),
            self.get_description_str(),
        ])

    def get_description_str(self):
        ret = ""
        if self.prop.description is None:
            ret += "-"
        else:
            ret += latexify(str(self.prop.description))
        return ret

    def get_default_str(self):
        ret = ""
        if self.prop.default is None:
            ret += "-"
        else:
            ret += latexify(str(self.prop.default))
        return ret

    def get_accessibility(self):
        """ Iterates over accessibility entries and returns single string """
        ret = []
        if self.prop.volatile:
            ret.append("Volatile")
        if self.prop.readable:
            ret.append("Readable")
        if self.prop.readback:
            ret.append("Readback")
        if self.prop.writable:
            ret.append("Writable")
        if self.prop.initial:
            ret.append("Initial")
        if self.prop.parameter:
            ret.append("Parameter")
        if self.prop.readsync:
            ret.append("ReadSync")
        if self.prop.writesync:
            ret.append("WriteSync")
        return ", ".join(ret)

    def get_enums(self):
        """ Returns enums as a string, cleaned up with some extra whitespace """
        if self.prop.enums is None:
            return "Standard"
        # insert spaces after commas to aid in LaTeX table cell wrapping
        return latexify(self.prop.enums).replace(",", ", ")


def append_property_table_row(res, xml_prop):
    """ LaTeX property table row from a property """
    props = Property(get_prop_dict_from_attributes(xml_prop))
    res.append(props.as_latex_table_row())


def append_port_table_row(res, port):
    """ LaTeX port table row from a port """
    res.append(
        get_tex_table_row(
            [latexify(get_anycase(port, "Name") or "(unnamed?)"),
             latexify(get_anycase(port, "Producer") or "false"),
             latexify(get_anycase(port, "Protocol") or "(none)"),
             latexify(get_anycase(port, "Optional") or "false")]))


def append_interface_table_row(res, port, worker_root):
    """ LaTeX interface table row from a port """
    attrs = get_dict_from_attributes(port, ["Name", "DataWidth"])

    if not attrs["DataWidth"]:
        tmp = get_dict_from_attributes(worker_root, ["DataWidth"])
        attrs["DataWidth"] = tmp["DataWidth"]
    if not attrs["DataWidth"]:
        # TODO / FIXME ..........................................................
        attrs["DataWidth"] = "#the width of the smallest element in the message protocol indicated in the OCS"

    res.append(
        get_tex_table_row(
            ["StreamInterface",
             latexify(attrs["Name"]),
             latexify(attrs["DataWidth"])]))


def normalize_file_name(filename):
    """ This function will normalize a filename by returning a matching
    file if it already exists, e.g. (if the latter already exists)
    iqstream_max_calculator.tex => IQStream_Max_Calculator.tex
    * The function name implies more may be done later (e.g. Title Case) """
    files = [f for f in os.listdir('.') if f.lower() == filename.lower()]
    if files:
        return files[0]
    return filename


def emit_latex_header(out_file, user_edits):
    """ Writes standardized LaTeX header indicating if user is expected to edit file """
    edit_str = "this file is intended to be edited" if user_edits \
        else "editing this file is NOT recommended"
    out_file.write(
r"""%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% this file was generated by docGen.py
% {}
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
""".format(edit_str))


def prompt_to_overwrite(filename, warn_existing=False):
    """ Will prompt use to ensure 'filename' shouldn't be overwritten.
        Will return True if the file should be overwritten.
        If "warn_existing" set, it will imply they should NOT answer yes.
        """
    if not os.path.isfile(filename):
        return True
    if warn_existing:
        msg = "exists and you PROBABLY DON'T want to rewrite it!"
        ocpi.logging.warning(filename + msg)
    msg = "WARN : file " + filename + " already exists, overwrite (y or n)? "
    res = ''
    while res not in ['y', 'n']:
        res = input(msg)
    return res == "y"


def emit_datasheet_tex_file(xml_file):
    """ Main routine that creates the LaTeX files after parsing the XML.
        Will replace some keywords (e.g. GEN_COMPLC_NAME).
    """
    update_global_names_from_path(xml_file)

    datasheet_filename = normalize_file_name(component_name_from_path(xml_file) + ".tex")

    if not prompt_to_overwrite(datasheet_filename):
        print("INFO : skipping", datasheet_filename)
        return
    print("INFO : emitting {0} (compile using rubber -d {0})".format(datasheet_filename))
    j_template = LATEX_JINJA_ENV.get_template("Component_Template.tex")
    with open(datasheet_filename, 'w') as datasheet_file:
        emit_latex_header(datasheet_file, user_edits=False)
        print(j_template.render(LC_NAME=LC_NAME, UC_NAME=UC_NAME), file=datasheet_file)
    return


def emit_property_table_file(data, prop_key, filename, title_text):
    """ Main routine for property tables to a file """
    if not prompt_to_overwrite(filename):
        print("INFO : skipping", filename)
        return
    print("INFO : emitting", filename)
    is_OCS = data.get('isOCS', False)
    is_HDL = data.get('isHDL', False)
    props = data[prop_key]
    print("INFO : is_OCS", is_OCS, "is_HDL", is_HDL, "props", props)

    j_template = LATEX_JINJA_ENV.get_template("Properties.tex")
    latex_rows = []
    for xml_prop in props:
        append_property_table_row(latex_rows, xml_prop)
    with open(filename, 'w') as outfile:
        emit_latex_header(outfile, user_edits=False)
        if not (is_OCS and prop_key == 'wProps'):
            print(j_template.render(title_text=title_text,
                                    latex_rows=latex_rows,
                                    is_OCS=is_OCS,
                                    is_HDL=is_HDL,
                                    comp_name=r"\comp.{}".format(('rcc', 'hdl')[is_HDL]),
                                    sec_star=('*', '')[USE_SECTION_NUMBERS],
                                   ), file=outfile)


def emit_ports_table_file(data, prop_key, filename, title_text, worker_root):
    """ Main routine for port tables to a file """
    if not prompt_to_overwrite(filename):
        print("INFO : skipping", filename)
        return
    print("INFO : emitting", filename)
    is_OCS = data.get('isOCS', False)
    is_HDL = data.get('isHDL', False)
    ports = data[prop_key]

    j_template = LATEX_JINJA_ENV.get_template("Ports.tex")
    latex_rows = []
    for xml_port in ports:
        if is_HDL:
            append_interface_table_row(latex_rows, xml_port, worker_root)
        else:
            append_port_table_row(latex_rows, xml_port)
    with open(filename, 'w') as outfile:
        emit_latex_header(outfile, user_edits=False)
        if not (is_OCS and prop_key == 'wPorts'):
            print(j_template.render(title_text=title_text,
                                    latex_rows=latex_rows,
                                    is_OCS=is_OCS,
                                    is_HDL=is_HDL,
                                    comp_name=r"\comp.{}".format(('rcc', 'hdl')[is_HDL]),
                                    sec_star=('*', '')[USE_SECTION_NUMBERS],
                                   ), file=outfile)


def emit_latex_inc_files(data, worker_root):
    """ Creates the various .inc snippet files based on parsed data """
    if data.get('isOCS', False):
        emit_property_table_file(data, prop_key='cProps',
                                 filename=COMPONENT_PROPS_INC_FILENAME,
                                 title_text="Component Properties")
        emit_ports_table_file(data, prop_key='cPorts',
                              filename=COMPONENT_PORTS_INC_FILENAME,
                              title_text="Component Ports",
                              worker_root=worker_root)
    # This is non-intuitive, but for now, *always* emit worker .inc files so
    # that the emitted <OCS-name>.tex will successfully compile in all cases
    # (including when the only argument to this script is the OCS)
    emit_property_table_file(data, prop_key='wProps',
                             filename=WORKER_PROPS_INC_FILENAME,
                             title_text="Worker Properties")
    emit_ports_table_file(data, prop_key='wPorts',
                          filename=WORKER_PORTS_INC_FILENAME,
                          title_text="Worker Interfaces",
                          worker_root=worker_root)


def emit_developer_doc_file():
    """ Creates "main" developer doc that they probably DON'T want to edit """
    if not prompt_to_overwrite(DEVELOPER_DOC_INC_FILENAME, warn_existing=True):
        print("INFO : skipping", DEVELOPER_DOC_INC_FILENAME)
        return
    print("INFO : emitting", DEVELOPER_DOC_INC_FILENAME, "(developers should edit this)")
    j_template = LATEX_JINJA_ENV.get_template("Developer_Doc.tex")
    with open(DEVELOPER_DOC_INC_FILENAME, 'w') as out_file:
        emit_latex_header(out_file, user_edits=True)
        print(j_template.render(), file=out_file)


def input_file_contains_xi_include(filename):
    ret = False
    with open(filename, "r") as input_file:
        for line in input_file:
            for case in scramble_case("xi:include"):
                if case in line:
                    ret = True
                    break
    return ret


def usage():
    """ Prints our help """
    print("Usage: docGen.py <path to OCS> # run this first (expected to compile on its own)")
    print("       docGen.py <path to OWD> # run this second")
    print(mywrapper.fill("Generates LaTeX source files suitable for compiling a"
                         " component datasheet. Recommended to run on OCS"
                         " first, and OWD second (if available)."))


def main():
    """ Some day somebody might want to use this as a library... """
    if len(sys.argv) != 2 or sys.argv[1].lower() in ['--help', '-help', '-h']:
        usage()
        sys.exit(1)

    if input_file_contains_xi_include(sys.argv[1]):
        print("File contains xi:include, which is not yet supported. Exiting now.")
        sys.exit(1)

    tree = etree.parse(sys.argv[1])
    # TODO: Figure out how to parse the xi:include statements; Write a custom parser or wait for fix maybe in python 3.8
    root = tree.getroot()
    parsed_data = parse_root_xml(root)
    emit_latex_inc_files(parsed_data, root)
    if parsed_data['isOCS']:
        emit_developer_doc_file()
        emit_datasheet_tex_file(xml_file=sys.argv[1])
    else:  # is OWD
        update_global_names_from_path(sys.argv[1])
        ocpi.logging.warning(
            mywrapper.fill(" because the argument " + sys.argv[1] + " is an OWD, this script "
                           "generates table .inc files. An OCS argument must be used in order to "
                           "generate a compilable component datasheet."))

if __name__ == '__main__':
    main()
