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

#Simport ocpiutil
import os
import logging
import datetime
from functools import reduce
import operator
from .file import *
from _opencpi.util import OCPIException

# This function first finds the max-length of each 'column' and then iterates
# through each list and reformats each element to match the length corresponding
# to its 'column'
def normalize_column_lengths(lists):
    """
    This function takes a list of lists and treats each list like a row in a table.
    It then space-expands the length of each string element so that all elements in
    the same column have the same length.

    For example (doctest):
    >>> list1, list2 = normalize_column_lengths([["15 chr long str",\
                                                  "this is a longgg string"],\
                                                 ["< 15", "pretty short"]])
    >>> print (str(list1))
    ['15 chr long str', 'this is a longgg string']
    >>> print (str(list2))
    ['< 15           ', 'pretty short           ']
    """
    lists = [[str(elem) for elem in lst] for lst in lists]
    format_function = lambda length, string_elem: ("{0:<" + str(length) + "}").format(string_elem)
    newlens = []
    for column in zip(*lists):
        newlens.append(len(max(column, key=len)))
    newlists = []
    for oldlist in lists:
        newlists.append([format_function(length, elem) for elem, length in zip(oldlist, newlens)])
    return newlists

def format_table(rows, col_delim='|', row_delim=None, surr_cols_delim='|', surr_rows_delim='-',
                 underline=None):
    """
    Return a table specified by the list of rows in the 'rows' parameter. Optionally specify
    col_delim and row_delim which are each a single character that will be repeated to separate
    cols and rows. Note that if row_delim is a string with len>1, it will be used a single time
    for each row instead of being repeated to form a row. surr_rows_delim and surr_cols_delim
    determine the border of the table.
    """

    rows_norm = normalize_column_lengths(rows)

    # If an underline character was provided, insert a row containing this character repeated
    # at position 1 (right below the header)
    if underline:
        rows_norm.insert(1, [underline * len(elem) for elem in rows_norm[0]])

    # The start/end column delimeters should have a space after/before them IFF they are nonempty
    start_col_delim = surr_cols_delim + ' ' if surr_cols_delim else ''
    end_col_delim = ' ' + surr_cols_delim if surr_cols_delim else ''

    row_strs = []
    # Separate the elements in each row by spaces and column-delimeters
    for row in rows_norm:
        row_strs.append(start_col_delim
                        + reduce(lambda x, y: x + ' ' + col_delim + ' ' + y, row)
                        + end_col_delim)
    max_row_len = len(max(row_strs, key=len))
    if row_delim:
        # row_delim can be a single char that is repeated to form a row, or can be multiple
        # characters which would be used as-is as the row-delimeter
        if len(row_delim) == 1:
            row_line = row_delim * max_row_len
        else:
            row_line = row_delim

    table_str = ""
    if surr_rows_delim:
        # surr_row_delim is a single character that will be repeated to form the rows that form
        # the borders of the table
        table_str += surr_rows_delim * max_row_len + "\n"

    # Print each row of the table followed by a delimeter row (e.g. a line)
    for line in row_strs:
        table_str += line.rstrip() + "\n"
        if row_delim:
            table_str += row_line.rstrip() + "\n"

    if surr_rows_delim:
        # surr_row_delim is a single character that will be repeated to form the rows that form
        # the borders of the table
        table_str += surr_rows_delim * max_row_len + "\n"

    return table_str

def print_table(rows, col_delim='|', row_delim=None, surr_cols_delim='|', surr_rows_delim='-',
                underline=None):
    """
    Wrapper for "format_table()" that prints the returned table
    """
    print(format_table(rows, col_delim, row_delim, surr_cols_delim, surr_rows_delim, underline))


class Report(object):
    """
    A Report consists of multiple data points. Each data point is a dictionary of key=>value
    pairs. The headers (which correspond to the keys for each data-point), should be represented
    via the "ordered_headers" list which determines the ordering each data-point's items when
    presented.
    """
    def __init__(self, ordered_headers=[], sort_priority=[]):
        # User can initialize with the list of ordered headers
        self.ordered_headers = ordered_headers
        # User can initialize with the list of headers to sort by
        self.sort_priority = sort_priority
        self.data_points = []

    def __bool__(self):
        """
        If data_points = [], then 'if self' should return False
        """
        return bool(self.data_points)

    def append(self, data_point):
        """
        Appending a data-point to this Report just appends the data-point to
        this Report's list of data_points
        """
        self.data_points.append(data_point)

    def __iadd__(self, other):
        """
        Adding a Report to this one is the equivalent of adding a Report's data_points
        to this Reports data-point list. If this Report's ordered_headers is empty,
        use the other Report's ordered_headers.
        if not isinstance(other, Report):
        """
        if not isinstance(other, Report):
            raise OCPIException("Can only add another Report object to a Report.")
        # Headers are unchanged on addition unless headers are empty in which case,
        # user other's headers
        if not self.ordered_headers:
            self.ordered_headers = other.ordered_headers
        self.data_points += other.data_points
        return self

    def assign_for_all_points(self, key, value):
        """
        Add a key=>value pair to every data-point in this report
        Essentially, add a dimension to the data-set, and initialize every data-point's value
        """
        for point in self.data_points:
            point[key] = value

    def print_table(self, none_str="N/A"):
        """
        Print a table of this Report's data-points. Lead with the headers and present
        each data-point's elements as per the order in "ordered_headers".
        Let the caller specify a string to use for empty/None entries (Default: N/A)
        """
        if self.data_points:
            # Fill in empty values in data-points with none_str
            for header in self.ordered_headers:
                for point in self.data_points:
                    if header not in point:
                        point[header] = none_str
            # Construct the list of rows. Do so following the ordered_headers order
            rows = []
            for point in self.data_points:
                row = []
                for header in self.ordered_headers:
                    elem = point[header]
                    if elem is None:
                        row.append(none_str)
                    elif isinstance(elem, str):
                        # element is a string, just append to row
                        row.append(elem)
                    elif isinstance(elem, list):
                        # element is a list, join its contents with comma and append to row
                        row.append(", ".join(elem))
                    else:
                        # Unsupported element type (not a string or list)
                        try:
                            row.append(str(elem))
                        except Exception as ex:
                            logging.error(ex)
                            raise OCPIException("Element in Report is not a string and could not" +
                                                " be cast to a string.")

                rows.append(row)
            # Sort the rows by each header listed in sort_priority. Do this in reverse order
            # so the last sort key (and therefore the most significant) is the first element
            # in sort_priority
            for sort_header in reversed(self.sort_priority):
                rows.sort(key=operator.itemgetter(self.ordered_headers.index(sort_header)))
            # insert the headers as row 0
            rows.insert(0, self.ordered_headers)
            # format the table using a generic function and print
            print(format_table(rows, underline="-"))
        else:
            logging.warning("Not printing table for empty report.")

    def get_latex_table(self, caption="Utilization Table", none_str="N/A",
                        gen_stamp="Generated on %s" % datetime.datetime.now().ctime()):
        """
        Get a table of this Report's data-points. Lead with the headers and present each
        data-point's elements as per the order in "ordered_headers".
        Do this in a LaTeX parseable table format, and allow for a caption to be provided.
        Let the caller specify a string to use for empty/None entries (Default: N/A)
        Let the caller specify a generation-stamp, which will be placed at the top
            of the output. Defaults to "Generated on <date-and-time>"
        """
        # LaTeX strings being generated have many backslashes as is common in LaTeX.
        # So, disable the pylint checker for this:
        # pylint:disable=anomalous-backslash-in-string
        if self.data_points:
            # Fill in empty values in data-points with none_str
            for header in self.ordered_headers:
                for point in self.data_points:
                    if header not in point:
                        point[header] = none_str
            # Set the first row to a copy of our ordered_headers list
            # Sort the rows by each header listed in sort_priority. Do this in reverse order
            rows = [[header.replace("_", "\_") for header in self.ordered_headers.copy()]]
            # so the last sort key (and therefore the most significant) is the first element
            # in sort_priority
            for sort_header in reversed(self.sort_priority):
                rows.sort(key=operator.itemgetter(self.ordered_headers.index(sort_header)))
            # Add an indent to the first element for pretty alignment in LaTeX
            rows[0][0] = " " * 12 + rows[0][0]
            # End the line in a LaTeX newline
            rows[0][-1] += " \\\\"

            # The string below is the LaTeX table template. It is filled out before being returned
            latex_table_tmplt = \
"""%% %s

%% It is best to wrap this table in \\begin{landscape} and \\end{landscape} in its including doc
\\begin{tiny}
    \\begin{longtable}[l]{* {%d}{|c}|}
    \captionsetup{justification=raggedright,singlelinecheck=false}
    \caption{%s}\\\\
        \hline
        \\rowcolor{blue}
%s        \end{longtable}
\end{tiny}
"""

            for point in self.data_points:
                latex_row = []
                for header in self.ordered_headers:
                    # Even if str() is removed here, must make sure we copy the point
                    # and do not change it in-place
                    elem = point[header]
                    if elem is None:
                        elem_str = none_str
                    elif isinstance(elem, list):
                        # If the value for this cell is a list, create a sub-table
                        # with a row for each entry
                        elem_str = "\\begin{tabular}{@{}l@{}}" + \
                                   " \\\\ ".join(elem) + \
                                   "\end{tabular}"
                    elif isint(elem) or isfloat(elem):
                        if isinstance(elem, str):
                            elem_str = str_to_num(elem)
                        else:
                            elem_str = elem
                    else:
                        # loop above ensures that all elements are lists or strings,
                        # so this is a str
                        elem_str = elem
                    # LaTeX requires that plain-text underscores by escaped
                    latex_row.append(str(elem_str).replace("_", "\_"))
                # indent the row
                latex_row[0] = " " * 12 + latex_row[0]
                # add a LaTeX newline
                latex_row[-1] += " \\\\"
                rows.append(latex_row)

            # Format the table so that columns align in the LaTeX source. Separate lines
            # with "\hline", and separate columns with "&".
            latex_table = format_table(rows, col_delim="&", row_delim=" " * 12 + "\\hline",
                                       surr_cols_delim="", surr_rows_delim="", underline="")
            # Fill in the LaTeX table template with the current date/time, the provided caption,
            # the column descriptor string, and the actual table content
            return latex_table_tmplt % (gen_stamp, len(self.ordered_headers),
                                        caption.replace("_", "\_"), latex_table)
        else:
            logging.warning("Not printing table for empty report.")
            return ""
        # pylint:enable=anomalous-backslash-in-string

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
    doctest.testmod(verbose=__VERBOSITY, optionflags=doctest.ELLIPSIS)
