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
Utility for reporting information on HDL assets. Contains the ReportableItem class.

Documentation and testing:
    Documentation can be viewed by running:
        $ pydoc3 hdlreportableitem
Note on testing:
    When adding functions to this file, add unit tests to
    opencpi/tests/pytests/*_test.py
"""

import _opencpi.util as ocpiutil


class ReportableItem(object):
    #pylint:disable=anomalous-backslash-in-string
    """
    ReportableItem

    A key/name of an item to be reported with an associated regular expression(s).
    This regex(s) is used to search files for a value or some information corresponding to the item.
    Regex(s) are split up into those for synthesis reporting (synth_regexs), and those for
    implementation reporting (impl_regexs). Each one (synth/impl_regexs) can either be a single
    regular expression, a list of regular expressions, or a dictionary mapping keys to regular
    expressions. This description is expanded on in the match_* functions of this class.

    Calling <thisreportableitem>.match_and_transform_synth(target_file) will search for the
    synth regex(s) in the target_file and return the results. The method for performing this search
    can be specified (for synth and/or impl) via the match_and_transform_*_function
    arguments, or the following default function will be used:
        ocpiutil.match_regex - find regex in file and return the first match group
    Other options:
        ocpiutil.match_regex_get_first_num - find regex in file and return the first number in
                                             that string
        Other options: ocpiutil.match_regex_get_first_num_freq - find regex in file, return the
                                                                 first number in the string
                                                                 converted from period in ns to
                                                                 freq in MHz
                       custom functions - user defined named or anonymous functions for parsing
                                          a file for a regular expression and returning the result

    Example (doctest):
        # Here, we create a reportable item named "Item0" with a regex that matches
        # the following line:
        # This text right here contains a number (2.000ns)!!!! It should match item0's regex
        # We then call match_and_transform_synth() and give it this current file as the argument.
        # It should find that line and parse out the first number it finds:
        >>> item0 = ReportableItem("Item0", "#.*This text right.*\((.*)ns\)!!!!.*")
        >>> item0.match_and_transform_synth(os.path.realpath(__file__))
        '2.000'

        # Next, we override the default match_and_transform_synth_function with a function
        # from a period to a frequency.
        >>> item1 = ReportableItem("Item0", "#.*This text right.*!!!!.*",
                                   match_and_transform_synth_function=
                                       ocpiutil.match_regex_get_first_num_freq)
        >>> item1.match_and_transform_synth(os.path.realpath(__file__))
        '500.000'
    """
    #pylint:enable=anomalous-backslash-in-string

    #TODO change to use kwargs too many arguments
    def __init__(self, key, synth_regexs, impl_regexs=None,
                 match_and_transform_synth_function=None,
                 match_and_transform_impl_function=None):
        """
        synth_regexs can be a single, list or dictionary of regular expressions
        impl_regexs is the same form as synth_regexs and defaults to synth_regexs
        match_*synth_function is the function to apply on files to match the synth_regexs
                              and format the result. Defaults to ocpiutil.match_regex
        match_*impl_function is the same form as the synth version, and defaults to the
                             match_*synth_function
        """
        self.key = key
        # regexes used to capture strings that will be reported
        self.synth_regexs = synth_regexs
        self.impl_regexs = synth_regexs if impl_regexs is None else impl_regexs

        # This is the function that will be used to parse a file for each regex and
        # potentially transform the result in some way. By default the functionality is:
        #     Return the first match for the regex in the target file
        self.__synth_function = (ocpiutil.match_regex if match_and_transform_synth_function is None
                                                      else match_and_transform_synth_function)
        self.__impl_function = (self.__synth_function if match_and_transform_impl_function is None
                                                      else match_and_transform_impl_function)

    @staticmethod
    def do_matching_multi_regexs(target_file, regexs, matching_funct):
        """
        Given a regular expression or list of regexs, apply the matching function to using the
        target_file and each regex until a match is found. Return the match.
        """
        if isinstance(regexs, list):
            regex_list = regexs
        else:
            # If regexs is singular, wrap it in a list
            regex_list = [regexs]
        for rgx in regex_list:
            # Try each regex on the target file until a match is found
            match = matching_funct(target_file, rgx)
            if match is not None and match != "" and match != "0":
                return match
        # No match is found, return None
        return None

    def match_and_transform_synth(self, target_file):
        """
        Call the function used to find this item's regex in a synth file

        self.synth_regexs can be a single regular expression, a list of regular expressions,
        or a dictionary mapping string keys to regular expressions.

        If synth_regexs is a:
            single regular expression, this regex is matched on the target_file and the results
                                       are returned
            list of regular expressions, each regex is used to make an attempt at matching in the
                                         target_file, and the first result is returned
            dict of regular expressions, each regex is used to match the target_file, and results
                                         are returned as a list of form ["<key>": <ret-val, ...]
        """
        # If synth_regex is not a single regex or a list of regexs, but a dictionary of regexs,
        # iterate through each and accumulate an array of the results.
        # Call the synth's function for matching the regular
        # expression(s).
        if isinstance(self.synth_regexs, dict):
            results = []
            for key, rgx in self.synth_regexs.items():
                match = ReportableItem.do_matching_multi_regexs(target_file,
                                                                rgx,
                                                                self.__synth_function)
                if match is not None:
                    results.append(key + ": " + match)

            if len(results) <= 0:
                return None
            else:
                return results
        else:
            # synth_regexs is a single regex or a list of regexs
            return ReportableItem.do_matching_multi_regexs(target_file, self.synth_regexs,
                                                           self.__synth_function)

    def match_and_transform_impl(self, target_file):
        """
        Call the function used to find this item's regex in a impl file

        For more information on the various modes of impl_regexs, see the match_and_transform_synth
        comments for synth_regexs
        """
        # If impl_regexs is not a single regex or list of regexs, but a dictionary of regexs,
        # iterate through each and accumulate an array of the results
        # Call the impl's function for matching the regular
        # expression(s).
        if isinstance(self.impl_regexs, dict):
            results = []
            for key, rgx in self.impl_regexs.items():
                match = ReportableItem.do_matching_multi_regexs(target_file, rgx,
                                                                self.__impl_function)
                if match is not None:
                    results.append(key + ": " + match)
            if len(results) <= 0:
                return None
            else:
                return results
        else:
            # impl_regexs is a single regexc or list of regexs
            return ReportableItem.do_matching_multi_regexs(target_file,
                                                           self.impl_regexs,
                                                           self.__impl_function)

if __name__ == "__main__":
    import os
    import doctest
    __LOG_LEVEL = os.environ.get('OCPI_LOG_LEVEL')
    __VERBOSITY = False
    if __LOG_LEVEL:
        try:
            if int(__LOG_LEVEL) >= 8:
                __VERBOSITY = True
        except ValueError:
            pass
    doctest.testmod(verbose=__VERBOSITY, optionflags=doctest.ELLIPSIS)
