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
definitions for utility functions that have to do with the filesystem
"""

import subprocess
import os
import os.path
from contextlib import contextmanager
import logging
import re
from _opencpi.util import OCPIException

###############################################################################
# Utility functions for extracting variables and information from and calling
# Makefiles
###############################################################################
def execute_cmd(settings, directory, action=None):
    """
    This command is a wrapper around any calls to make in order to encapsulate the use of make to a
    minimal number of places.  The function contains a hard-coded dictionary of generic settings to
    make variables that it uses to construct the call to make at that given directory
    """
    settings_dict = {'rcc_plats'       : "RccPlatforms",
                     'hdl_plat_strs'   : "HdlPlatforms",
                     'hdl_tgt_strs'    : "HdlTargets",
                     'only_plats'      : "OnlyPlatforms",
                     'ex_plats'        : "ExcludePlatforms",
                     'keep_sims'       : "KeepSimulations",
                     'acc_errors'      : "TestAccumulateErrors",
                     'view'            : "View",
                     'cases'           : "Cases",
                     'run_before'      : "OcpiRunBefore",
                     'run_after'       : "OcpiRunAfter",
                     'run_arg'         : "OcpiRunArgs",
                     'remote_test_sys' : "OCPI_REMOTE_TEST_SYSTEMS",
                     'verbose'         : "TestVerbose"}
    make_list = []

    make_list.append("make")
    make_list.append("-C")
    make_list.append(directory)
    debug_string = "make -C " + directory

    if action is not None:
        make_list.extend(action)
        debug_string += " " + ' '.join(action) + " "

    logging.debug("settings for make command: " + str(settings.items()))
    for setting, value in settings.items():
        if isinstance(value, bool):
            make_list.append(settings_dict[setting] + '=1')
            debug_string += " " + settings_dict[setting] + '=1'
        elif isinstance(value, list) or isinstance(value, set):
            make_list.append(settings_dict[setting] + '='  + ' '.join(value))
            if len(value) > 1:
                debug_string += " " + settings_dict[setting] + '="'  + ' '.join(value) + '"'
            else:
                debug_string += " " + settings_dict[setting] + '='  + ' '.join(value)
        else:
            # pylint:disable=undefined-variable
            raise OCPIException("Invalid setting data-type passed to execute_cmd().  Valid data-" +
                                "types are bool and list")
            # pylint:enable=undefined-variable

    logging.debug("running make command: " + debug_string)
    #shell=True is bad dont set it here running the following command was able to execute
    # arbitary code
    #ocpidev run test --hdl-platform \$\(./script.temp\)
    # all the script would need to do is cat isim then go on its merry way doing whatever it wanted
    try:
        child = subprocess.Popen(make_list)
        child.wait()
    except KeyboardInterrupt:
        child.kill()
        # pylint:disable=undefined-variable
        raise OCPIException("Received Keyboard Interrupt - Exiting")
        # pylint:enable=undefined-variable
    return child.returncode

#TODO fix these problems with the function instead of just disabling them
# pylint:disable=too-many-locals
# pylint:disable=too-many-branches
def set_vars_from_make(mk_file, mk_arg="", verbose=None):
    """
    Collect a dictionary of variables from a makefile
    --------------------------------------------------
    First arg is .mk file to use
    Second arg is make arguments needed to invoke correct output
        The output can be an assignment or a target
    Third arg is a verbosity flag
    Return a dictionary of variable names mapped to values from make

    OCPI_LOG_LEVEL>=6  will print stderr from make for user to see
    OCPI_LOG_LEVEL>=10 will pass OCPI_DEBUG_MAKE to make command and will
                       print both stdout and stderr for user to see
    """
    with open(os.devnull, 'w') as fnull:
        make_exists = subprocess.Popen(["which", "make"],
                      stdout=subprocess.PIPE, stderr=fnull).communicate()[0]
        if make_exists is None or make_exists == "":
            if verbose != None and verbose != "":
                logging.error("The '\"make\"' command is not available.")
            return 1

        # If log level >= 10 set OCPI_DEBUG_MAKE=1 (max debug level)
        ocpi_log_level = int(os.environ.get('OCPI_LOG_LEVEL', 0))
        if ocpi_log_level >= 10:
            mk_dbg = "OCPI_DEBUG_MAKE=1"
        else:
            mk_dbg = ""

        # If mk_file is a "Makefile" then we use the -C option on the directory containing
        # the makefile else (is a .mk) use the -f option on the file
        if mk_file.endswith("/Makefile"):
            make_cmd = "make " + mk_dbg + " -n -r -s -C " + os.path.dirname(mk_file) + " " + mk_arg
        else:
            make_cmd = "make " + mk_dbg + " -n -r -s -f " + mk_file + " " + mk_arg

        logging.debug("Calling make via:" + str(make_cmd.split()))

        child = subprocess.Popen(make_cmd.split(), stderr=subprocess.PIPE, stdout=subprocess.PIPE,
                                 universal_newlines=True)
        mk_output, mk_err = child.communicate()

        # Print out output from make if log level is high
        logging.debug("STDOUT output from Make (set_vars_from_make):\n" + str(mk_output))

        # Print out stderr from make if log level is medium/high or if make returned error
        if child.returncode != 0 or ocpi_log_level >= 6:
            if mk_err and verbose:
                logging.error("STDERR output from Make (set_vars_from_make):\n" + str(mk_err))
            if child.returncode != 0:
                # pylint:disable=undefined-variable
                raise OCPIException("The following make command returned an error:\n" + make_cmd)
                # pylint:enable=undefined-variable

        try:
            grep_str = re.search(r'(^|\n)[a-zA-Z_][a-zA-Z_]*=.*',
                                 str(mk_output.strip())).group()
        except AttributeError:
            logging.warning("No variables are set from \"" + mk_file + "\"")
            return None

        assignment_strs = [x.strip() for x in grep_str.split(';') if len(x.strip()) > 0]
        make_vars = {}
        for var_assignment in assignment_strs:
            var_name, var_val = var_assignment.split('=')
            # If the value is an empty string or just matching quotes, assign [] as the value
            if var_val == "\"\"" or var_val == "\'\'" or var_val == "":
                assignment_value = []
            else:
                assignment_value = var_val.strip('"').strip().split(' ')
            make_vars[var_name] = assignment_value
        return make_vars
# pylint:enable=too-many-locals
# pylint:enable=too-many-branches

###############################################################################
# String, number and dictionary manipulation utility functions
###############################################################################

# https://stackoverflow.com/questions/38987/how-to-merge-two-dictionaries-in-a-single-expression
# when code is moved to python 3.5+ this function goes away and becomes: z = {**x, **y}
def merge_two_dicts(dict1, dict2):
    """
    This function combines two dictionaries into a single dictionary, if both have the same key
    the value in dict2 overwrites the value in dict1
    >>> dict1 = {'a': 'avalue'}
    >>> dict2 = {'b': 'bvalue'}
    >>> dict3 = merge_two_dicts(dict1, dict2)
    >>> sorted(dict3.items())
    [('a', 'avalue'), ('b', 'bvalue')]
    """
    merged = dict1.copy()   # start with dict's keys and values
    merged.update(dict2)    # modifies merged with dict2's keys and values & returns None
    return merged

def isfloat(value):
    """
    Can the parameter be represented as an float?

    For example (doctest):
    >>> isfloat("1")
    True
    >>> isfloat("1.0")
    True
    >>> isfloat("")
    False
    >>> isfloat("string")
    False
    """
    try:
        float(value)
        return True
    except ValueError:
        return False

def isint(value):
    """
    Can the parameter be represented as an int?

    For example (doctest):
    >>> isint(1)
    True
    >>> isint("1")
    True
    >>> isint("1.0")
    False
    >>> isint("")
    False
    >>> isint("string")
    False
    """
    try:
        if isinstance(value, str):
            return value.replace(",", "").isdigit()
        return isinstance(value, int)
    except ValueError:
        return False

def str_to_num(num_str):
    """
    converts a sting to a numerical value of type int or float based on the string value
    """
    num_str_stripped = num_str.replace(",", "")
    return int(num_str_stripped) if isint(num_str_stripped) else float(num_str_stripped)

def freq_from_str_period(prd_string):
    """
    Convert a string representing period in ps, ns, us, ms, s (default)
    to a string representing frequency in Hz
    Return the string or "" on failure

    For example (doctest):
    >>> freq_from_str_period("250ps")
    4000000000.0
    >>> freq_from_str_period("250ns")
    4000000.0
    >>> freq_from_str_period("250us")
    4000.0
    >>> freq_from_str_period("250ms")
    4.0
    >>> freq_from_str_period("250s")
    0.004

    No units provided, 's' is assumed
    >>> freq_from_str_period("250")
    0.004

    >>> freq_from_str_period("")
    >>> freq_from_str_period("string")
    """
    if prd_string is None:
        return None
    period = 0.0
    ps_prd = re.sub('ps', '', prd_string)
    ns_prd = re.sub('ns', '', prd_string)
    ms_prd = re.sub('ms', '', prd_string)
    us_prd = re.sub('us', '', prd_string)
    # Make sure 's' is only detected if 's' directly follows a digit
    s_prd = re.sub(r'([0-9])s', r'\g<1>', prd_string)
    # Choose the numerator based on which units were detected
    if ps_prd != prd_string:
        prd = ps_prd
        numerator = 1E12
    elif ns_prd != prd_string:
        prd = ns_prd
        numerator = 1E9
    elif us_prd != prd_string:
        prd = us_prd
        numerator = 1E6
    elif ms_prd != prd_string:
        prd = ms_prd
        numerator = 1E3
    else:
        prd = s_prd
        numerator = 1.0
    if isfloat(prd):
        period = float(prd)
    if period != 0:
        freq = numerator/float(period)
        return freq
    return None

def first_num_in_str(parse_string):
    """
    Return the first number in a list of strings. Allow ',' or '.'  and trailing text
    (e.g. MHz, ns). Return "" if no number is found.

    For example (doctest):
    >>> first_num_in_str("Here is my number: 12345.12345 (here is a second number 222.222)")
    '12345.12345'
    >>> first_num_in_str("Another number: 12,345,123")
    '12,345,123'
    >>> first_num_in_str("A frequency: 123,000MHz")
    '123,000'
    >>> first_num_in_str("A period: 123,000ns")
    '123,000'
    >>> first_num_in_str("abcd1234efgh5678")
    '1234'
    >>> first_num_in_str("There is no string here!")
    """
    first_num_regex = re.compile(r"([-+]?([0-9]{1,3}\,?)+(\.[0-9]*)?)")
    result = re.search(first_num_regex, parse_string)
    if result:
        return result.group(0)
    else:
        return None

def match_regex(target_file, regex):
    # pylint:disable=anomalous-backslash-in-string
    """
    Parse the target file for a regex and return the first group/element
    of the first match

    For example (doctest):
    Here, we have a line of text below and we parse this current file
    using a regular expression to grab the 'ns' number from the string.
    # This text right here contains a number (2.000ns)!!!! It should match the regex
    >>> match_regex(os.path.realpath(__file__), "#.*This text right.*\((.*)ns\)!!!!.*")
    '2.000'
    """
    # pylint:anomalous-backslash-in-string
    if isinstance(regex, str):
        regex = re.compile(regex)
    elif not isinstance(regex, type(re.compile(''))):
        # pylint:disable=undefined-variable
        raise OCPIException("Error: regular expression invalid")
        # pylint:enable=undefined-variable

    # Have to use encoding/errors flags here to avoid failure when file has non-ASCII
    with open(target_file, encoding="utf8", errors="ignore") as tgt_file:
        for line in tgt_file:
            # List of all matches on the line
            matches = re.findall(regex, line)
            # Were any matches found?
            if matches and matches[0]:
                # Get the first match on the line
                match = matches[0]
                if isinstance(match, tuple):
                    # Match might be a tuple (if 1+ groups are in the pattern). In that case
                    # return the first group in the match tuple
                    return match[0]
                return match
    # no match found
    return None

def match_regex_get_first_num(target_file, regex):
    """
    Match the given regex in the given file (get the first match),
    and return the first number in the matched string.
    """
    # TODO add doctest
    if isinstance(regex, str):
        # if regex is a string, compile to regex
        regex = re.compile(regex)
    elif not isinstance(regex, type(re.compile(''))):
        # pylint:disable=undefined-variable
        raise OCPIException("Error: regular expression invalid")
        # pylint:enable=undefined-variable

    # Have to use encoding/errors flags here to avoid failure when file has non-ASCII
    with open(target_file, encoding="utf8", errors='ignore') as tgt_file:
        for line in tgt_file:
            # List of all matches on the line
            matches = re.findall(regex, line)
            # Were any matches found?
            if matches and matches[0]:
                # Get the first number in the first match on the line
                first_num = first_num_in_str(matches[0])
                if first_num != None:
                    return first_num
    # no number/match found
    return None

def write_file_from_string(file_name, string):
    """
    Generates a file on the filesystem from the string that is passed in the file of file_name
    """
    out_file = open(file_name, "w")
    out_file.write(string)
    out_file.close()

###############################################################################
# Functions to ease filesystem navigation
###############################################################################
def name_of_dir(directory="."):
    """
    Return the name of the directory provided. This will return the actual
    Directory name even if the argument is something like "."
    For example (doctest):
    >>> name_of_dir("/etc/.")
    'etc'
    >>> name_of_dir("/etc/../etc")
    'etc'
    """
    return os.path.basename(os.path.realpath(directory))

# Disabling a pylint check so we use the name 'cd' even though it is so short
# pylint:disable=invalid-name
@contextmanager
def cd(target):
    """
    Change directory to 'target'. To be used with 'with' so that origin directory
    is automatically returned to on completion of 'with' block
    """
    origin = os.getcwd()
    os.chdir(os.path.expanduser(target))
    try:
        yield
    finally:
        os.chdir(origin)
# pylint:enable=invalid-name

###############################################################################
# Functions for prompting the user for input
###############################################################################
def get_ok(prompt="", default=False):
    """
    Prompt the user to say okay
    """
    print(prompt, end=' ')
    while True:
        ok_input = input(" [y/n]? ")
        if ok_input.lower() in ['y', 'yes', 'ok']:
            return True
        if ok_input.lower() in ['n', 'no', 'nope']:
            return False
        if ok_input.lower() == '':
            return default
def rchop(thestring, ending):
    """
    chop off the substring ending from thestring if it ends with the substring ending and returns
    the result
    """
    if thestring.endswith(ending):
        return thestring[:-len(ending)]
    return thestring

if __name__ == "__main__":
    import doctest
    import sys
    __LOG_LEVEL = os.environ.get('OCPI_LOG_LEVEL')
    __VERBOSITY = False
    if __LOG_LEVEL:
        try:
            if int(__LOG_LEVEL) >= 8:
                __VERBOSITY = True
        except ValueError:
            pass
    doctest.testmod(verbose=__VERBOSITY, optionflags=doctest.ELLIPSIS)
    sys.exit(doctest.testmod()[0])
