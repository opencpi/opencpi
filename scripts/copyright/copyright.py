#!/usr/bin/env python3
"""Inserts copyright into files, replacing old ones found (AV-2759, AV-2912)"""
# For copyright information concerning THIS script; see build_copyright below.

from __future__ import print_function
import argparse
import fnmatch
import os.path
import re
import string
import subprocess
import sys
import textwrap
import logging
logging.basicConfig(filename='copyright.log',
                    format='%(asctime)s:%(levelname)s: %(message)s',
                    level=logging.INFO)

# Some global variables
bailing = False
filename = 'unknown'
gbuff = []
scan_mode = os.getenv('OCPI_SCAN_COPYRIGHTS') is not None  # Hidden mode to scan the repo

# Query rows
try:
    with open(os.devnull, 'w') as devnull:
        rows = int(subprocess.check_output(['tput', 'lines'], stderr=devnull))-20
except OSError:
    rows = 40
except subprocess.CalledProcessError:
    rows = 40

# import opencpi.colors as color - don't want dependency here
class color(object):
    """ Look-up Table for colors """
    CLS = BLUE = GREEN = YELLOW = RED = BOLD = UNDERLINE = END = ''
    try:
        with open(os.devnull, 'w') as devnull:
            dont_care = int(subprocess.check_output(['tput', 'lines'], stderr=devnull))
            if os.getenv('TERM', 'dumb') != "dumb":
                CLS = subprocess.check_output(['tput', 'clear'], stderr=devnull).decode('ascii')
                BLUE = subprocess.check_output(['tput', 'setaf', '4'], stderr=devnull).decode('ascii')
                GREEN = subprocess.check_output(['tput', 'setaf', '2'], stderr=devnull).decode('ascii')
                YELLOW = subprocess.check_output(['tput', 'setaf', '3'], stderr=devnull).decode('ascii')
                RED = subprocess.check_output(['tput', 'setaf', '1'], stderr=devnull).decode('ascii')
                BOLD = subprocess.check_output(['tput', 'bold'], stderr=devnull).decode('ascii')
                UNDERLINE = subprocess.check_output(['tput', 'smul'], stderr=devnull).decode('ascii')
                END = subprocess.check_output(['tput', 'sgr0'], stderr=devnull).decode('ascii')
    except OSError:
        pass
    except subprocess.CalledProcessError:
        pass


def build_copyright(comment, pre='', post=''):
    """
    Function that builds the copyright notice when given single line comment
    prefix, block prefix, and block postfix.
    """
    if len(comment):
        comment += ' '
    if len(pre):
        pre += '\n'
    res = pre
    for st in """This file is protected by Copyright. Please refer to the COPYRIGHT file distributed with this source distribution.

This file is part of OpenCPI <http://www.opencpi.org>

OpenCPI is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.""".splitlines():
        if len(st):
            res += textwrap.fill(st, 80, initial_indent=comment, subsequent_indent=comment)
            res += '\n'
        else:
            res += comment
            if len(comment):
                res = res.rstrip() + '\n'  # Remove trailing whitespace
            else:
                res += '\n'
    if len(post):
        res += post + '\n'
    return res


ocpi_copyright = dict()
ocpi_copyright['script'] = build_copyright('#')  # First for re-use
ocpi_copyright['c'] = build_copyright(' *', pre='/*', post=' */')
ocpi_copyright['groovy'] = ocpi_copyright['c']
ocpi_copyright['java'] = ocpi_copyright['c']
ocpi_copyright['latex'] = build_copyright('', pre=r'\iffalse', post=r'\fi')
ocpi_copyright['m4'] = build_copyright('dnl')
ocpi_copyright['make'] = ocpi_copyright['script']
ocpi_copyright['matlab'] = build_copyright('%')
ocpi_copyright['rpmspec'] = ocpi_copyright['script']
ocpi_copyright['text'] = build_copyright('')
ocpi_copyright['verilog'] = ocpi_copyright['c']
ocpi_copyright['vhdl'] = build_copyright('--')
ocpi_copyright['xml'] = build_copyright('   -', pre=r'<!--', post=r'-->')

# LUT for extension <=> file type
extensions = dict()
extensions['c'] = ('c', 'cc', 'cl', 'cpp', 'cxx', 'h', 'hh', 'hpp', 'hxx')
extensions['groovy'] = ('groovy',)
extensions['java'] = ('java',)
extensions['latex'] = ('tex',)
extensions['m4'] = ('m4',)
extensions['make'] = ('mk',)
extensions['matlab'] = ('m',)
extensions['rpmspec'] = ('spec',)
extensions['script'] = ('csh', 'js', 'pl', 'py', 'qsf', 'sdc', 'sh', 'tcl', 'ucf', 'xcf', 'xdc')
extensions['text'] = ('md', 'txt',)  # Includes Markdown text
extensions['verilog'] = ('v', 'vh')
extensions['vhdl'] = ('vhd', 'vhi')
extensions['xml'] = ('sdef',)
extensions['skip'] = ('a', 'la', 'lai',  # Static libraries
                      'asm',  # Future? Don't think we have any of our own now.
                      'aux',
                      'bat',  # Future?
                      'bak',
                      'bin',
                      'bit',
                      'bitz',
                      'build',
                      'bz2', 'tbz2',
                      'dat',
                      'doc', 'docx',
                      'edf', 'map', 'ngc', 'par',  # Xilinx files
                      'fdb_latexmk', 'fls', 'toc',  # LaTeX temporary files
                      'gch',
                      'gif',
                      'git',
                      'golden',
                      'gz', 'tgz',
                      'hold',
                      'htm', 'html',
                      'ini',
                      'jar',  # Java
                      'jpeg', 'jpg',
                      'json',
                      'log',
                      'md5',
                      'o', 'obj', 'ko', 'so',  # Shared libraries
                      'odg', 'odp', 'ods', 'odt', 'ott',
                      'out',
                      'patch',
                      'png',
                      'pptx',
                      'pdf',
                      'prefs',
                      'props',
                      'properties',  # Java
                      'pyc',  # Pre-compiled python1/2
                      'rpm',
                      'tar',
                      'vsd', 'vsdx',
                      'xls', 'xlsx', 'xlsm',
                      'xml', 'grc',
                      'zip')  # Autoskip these extensions

# LUT for basename <=> file type
basenames = dict()
basenames['make'] = ('Makefile', 'Makefile.*')
basenames['script'] = ('Project.exports',)
basenames['skip'] = ('.gitattributes', '.gitignore', '.project', 'target', '.tags*', 'test.*', '.DS_Store')
basenames['text'] = ('COPYRIGHT', 'LICENSE', 'README', 'README*')

# Custom scanning functions (run before anything else)
other_regexs = (
    re.compile(r'Copyright.*Altera Corporation'),
    re.compile(r'\(C\)\s*\d+.*Altera Corporation'),
    re.compile(r'Copyright.*Bluespec, Inc'),
    re.compile(r'Vendor\s*:\s*Xilinx'),
    re.compile(r'Copyright.*Xilinx, Inc'),
    re.compile(r'Xilinx Core Generator'),
    re.compile(r'Downloaded.*opencores'),
    re.compile(r'OpenSplice\s+DDS'),
    re.compile(r'This file is part of CommPy'),
    re.compile(r'The Khronos Group Inc.'),
    re.compile(r'Copyright \(C\)[\s\d ,-]+Massachusetts Institute of Technology', re.IGNORECASE),
    re.compile(r'Copyright.*Intel Corp'),
    re.compile(r'Copyright \(c\) Internet2'),  # Fasttime
    re.compile(r'Copyright[\s\d ,-]+Aaron Voisine'),  # ezxml
    re.compile(r'^#\s*Doxyfile\s+[.\d]+$'),  # Doxygen config files
    re.compile(r'Copyright \(c\).*Google,?\s+Inc'),  # Some Eclipse things
)
bad_paths = (
    '/.git/',
    '/build/autotools/imported-macros/',
    '/chipscope/',
    '/doc/av/internal/',
    '/doc/av/tex/doc_gen_test/ocs/golden/',  # docGen test golden files
    '/doc/av/tex/doc_gen_test/ocs2/golden/',  # docGen test golden files
    '/doc/av/tex/doc_gen_test/ocs_portsinout/golden/',  # docGen test golden files
    '/docs/IDE_Guide_Shots/',  # IDE
    '/fixed_float',
    '/gen/',
    '/kernel-headers',
    '/opencpi-zynq-linux-release-',
    '/prerequisites/',
    '/prerequisites-build/',
    '/projects/assets/components/util_comps/socket_write.rcc/asio/',  # Boost
    '/projects/assets/components/util_comps/socket_write.rcc/ext_src/',  # Boost
    '/release-2013.4/',
    '/releng/blacklist/blacklist_metadata/',
    '/releng/blacklist/sanitize/',
    '/releng/config_files/',
    '/releng/oss_release/',
    '/runtime/foreign/',
    '/target-',
    '/tests/pytests/utilization_proj/',
    'vendor',
    '/vm_support/',
    '/xilinx-zynq-binary-release-',
)
bad_path_globs = (
    '*/component_ports.inc', '*/component_properties.inc',  # LaTeX include files for generated documentation
    '*/component_spec_properties.inc',                      # "
    '*/developer_doc.inc', '*/properties.inc',              # "
    '*/worker_interfaces.inc', '*/worker_properties.inc',   # "
    '*/configurations*.inc', '*/utilization*.inc',  # LaTeX include files for utilization
    '*/idata/*',  # Unit test data source
    '*/ip_user_files/sim_scripts/*',  # Vivado simulation scripts
    '*/MANIFEST.MF',  # Java packaging
    '*/managed_ip_project/managed_ip_project.cache/*',  # Vivado core cruft
    '*/notes',  # Misc notes
    '*/odata/*',  # Unit test data destination
    '*/package-name',
    '*/package-id',
    '*/project-registry',  # Don't scan registered projects outside this source tree
    '*/projects/core/hdl/primitives/sync/xsim/*/xsim/*',  # Non-copyrighted Xilinx cores
    '*/__pycache__/*',
    '*/*.sh.example',
    '*/*.json_example',
    '*/snippets/*',
    '*.test/test*/description',
    '*.test/test*/portmap',
    '*.test/test*/*.input',
    '*.test/golden*',
    '*.test/test*/golden*',
    '*/xilinx*/release/*',  # Symlinked versions of things in bad_paths above
    '*/vivado_ila/*',
)


def scanfunc_skip():
    """Scans entire buffer for certain keywords, pathnames, etc."""
    for badp in bad_paths:
        if badp in filename:
            logging.debug('%s: Found bad path "%s". Skipping.', filename, badp)
            return True
    if any(fnmatch.fnmatch(filename, p) for p in bad_path_globs):
        logging.debug('%s: Found bad path glob. Skipping.', filename)
        return True
    if len(gbuff):
        for line in gbuff:
            for reg in other_regexs:
                if reg.search(line) is not None:
                    logging.debug('%s: Found copyrighted regex "%s". Skipping.', filename, str(reg.pattern))
                    return True
        ext = os.path.splitext(filename)[1][1:]
        if gbuff[0].strip().startswith('<?xml') and ext not in extensions['xml']:
            return True
    return False


scanfuncs = dict()
scanfuncs['script'] = lambda: len(gbuff) > 0 and any([x in gbuff[0] for x in ('/sh', '/bash', '/csh', '/env python', '/env bash')])
scanfuncs['skip'] = scanfunc_skip


def file_to_type(fname):
    """Finds a file type using file name or data in gbuff (empty data OK)"""
    # First, custom functions to filter
    for key, func in scanfuncs.items():
        if func():
            return key
    # Then check extensions
    ext = os.path.splitext(fname)[1][1:]
    for key, val in extensions.items():
        if any(ext == x for x in val):
            return key
    # Then try file basename (using file matching rules)
    bname = os.path.basename(fname)
    for key, val in basenames.items():
        if any(fnmatch.fnmatch(bname, x) for x in val):
            return key
    return ''


def choose_ftype():
    """Prompt the user for the file type"""
    i = 0
    ftypes = ['skip']  # Want skip to always be zero (along with blank)
    ftypes.extend(sorted([x for x in extensions.keys() if x != 'skip']))
    print(color.RED + "\nCould not determine file type. Please choose one (use 'script' for generic '#' comments):")
    for t in ftypes:
        print('%d:%s ' % (i, t), end='')
        i += 1
    print(':: ' + color.END, end='')
    inp = input()
    if not len(inp):
        return 'skip'
    return ftypes[int(inp)]


def get_ok():
    """Prompt the user to say okay"""
    while True:
        ok = input(' [y/N]? ')
        if ok.lower() in ('y', 'yes', 'ok'):
            return True
        if ok.lower() in ('', 'n', 'no', 'nope'):
            return False


def print_highlighted(start=0, end=0, clear=False, inserted=False):
    """Prints reasonable length of file, with highlighting options."""
    if not len(gbuff):
        print("<Empty file>")
        return
    # Scan for unprintables in first line
    for x in [y.strip() for y in gbuff[0]]:
        if not all(z in string.printable for z in x):
            print("<File contains unprintable characters>")
            return
    if clear:
        print(color.CLS, end='')
    if end == 0:
        mycolor = color.BOLD
        end = start
    else:
        mycolor = color.RED
    endl = '\n'
    if inserted:
        mycolor += color.GREEN
        endl = ''  # when not in "inserted" mode, we put newlines around block
    i = 1
    print(color.END, end='')
    for line in gbuff:
        if i == start:
            print(mycolor, end=endl)
        if not inserted:
            print('%02d: %s' % (i, line), end='')
        else:  # line numbers are messed up because the license is multiline
            print(line, end='')
        if i == end:
            print(color.END, end=endl)
        i += 1
        if i == rows:
            break
    print(color.END, end='')  # Just in case.


def smart_scan():
    """Tries to scan the file and find the existing copyright block."""
    start = 1
    # Skip shell declaration
    if gbuff[0].startswith("#!"):
        start = 2
    if gbuff[0].startswith("<?xml"):
        start = 2
    # Skip preprocessor commands and comments
    while any([x in gbuff[start-1] for x in ('not used by RPM-based',
                                             '#ifdef',
                                             '#ifndef',
                                             '#define',
                                             '#endif',
                                             '#include',
                                             '#warning',
                                             '#error',
                                            )]):
        start += 1
    # Check the first 40 lines
    end = start
    while end < 40:
        if "along with OpenCPI." in gbuff[end]:
            break
        end += 1
    if end >= 40:
        return (False,)
    # end either has the last line or the second-to-last, depending on the language
    while any([x in gbuff[end+1] for x in ('*/', '#')]):
        end += 1
    # bash comments might have conflicted with preprocessor
    while any([x in gbuff[end] for x in ('#define', '#ifdef', '#ifndef', '#endif', '#include')]):
        end -= 1
    # slurp up blank lines
    while len(gbuff[end+1].strip()) == 0:
        end += 1
    logging.warning('%s: smart_scan removing lines %d to %d.', filename, start, end+1)
    return (True, start, end+1) # 0-based to 1-based


def process():
    """Does the "real" work."""
    # Check if UPDATED exists (if so, skip file).
    if any('<http://www.opencpi' in x for x in gbuff):
        logging.info('%s: Seems to already have new copyright clause.', filename)
        return False
    ftype = file_to_type(filename)  # Yes, re-run now that gbuff is populated
    if scan_mode:
        if ftype not in ('skip', 'text'):
            logging.error('%s: Not skipped and not updated.', filename)
        return False
    global bailing
    # Check if copyright clause exists already.
    already = any("part of OpenCPI" in x for x in gbuff)
    if already:
        logging.warning('%s: Seems to have older copyright clause.', filename)
    # Present original:
    print(color.CLS + color.BOLD + 'Original file ({0}):'.format(filename) + color.END)
    if len(ftype):
        print(color.BOLD + 'Detected file type: {0}'.format(ftype) + color.END)
        logging.info('%s: Detected file type: %s.', filename, ftype)
    if not len(ftype):
        print_highlighted()
        ftype = choose_ftype()
        if ftype != 'skip':
            logging.warning('%s: User-specified file type: %s.', filename, ftype)
    try:
        # We use ValueError to skip because that is what a newline into int()
        # gives, allowing a single "enter" key to skip to next file.
        fast_insert = False
        if ftype in ('skip', '') or bailing:
            raise ValueError()
        if already:
            smart_scan_res = smart_scan()
            while True:
                if not smart_scan_res[0]:
                    print_highlighted()
                    print(color.BOLD + '\nLine to ' + color.RED + 'begin replacing? ' + color.END, end='')
                    start = int(input())
                    print(color.BOLD + 'Line to ' + color.RED + 'end replacing? ' + color.END, end='')
                    end = int(input())
                    logging.warning('%s: User-specified removing lines %d to %d.', filename, start, end)
                    print(color.CLS)
                else:
                    start = smart_scan_res[1]
                    end = smart_scan_res[2]
                print_highlighted(start, end)
                print(color.BOLD + 'Replace this block with copyright clause followed by a single break', end='')
                if get_ok():
                    break
                else:  # They said no...
                    if smart_scan_res[0]:
                        # Want to re-process
                        logging.warning('%s: Smart scanning failed; user rejected.', filename)
                        smart_scan_res = (False,)
                        print("Switching to user-input mode.")
                        continue
                    raise ValueError()
            # Delete the old
            del gbuff[start-1:end]
            fast_insert = True
        else:  # Not "already"
            print_highlighted()
            print(color.BOLD + "\nLine to insert block (or '.' for auto)? " + color.END, end='')
            start = input()
            if start == '.':  # "fast insert" which means 1,n,y,save
                start = 1
                # Check for shell shebang, LaTeX document class, or XML header
                if (gbuff[0].startswith("#!") or
                    gbuff[0].strip().startswith(r'\documentclass') or
                    gbuff[0].startswith("<?xml")):
                    start = 2
                fast_insert = True
                logging.warning('%s: User-requested fast insert at line %d.', filename, start)
            else:
                start = int(start)
        # Insert the new
        gbuff.insert(start-1, ocpi_copyright[ftype])
        # Show them the new stuff
        print_highlighted(start, clear=True, inserted=True)
        print(color.BOLD + 'Insert newline before', end='')
        inserted_before = 0
        if fast_insert:
            print('? No (automatic)')
        elif get_ok():
            gbuff.insert(start-1, '\n')
            inserted_before = 1
        print('Insert newline after', end='')
        if fast_insert:
            print('? Yes (automatic)')
        if fast_insert or get_ok():
            gbuff.insert(start+inserted_before, '\n')
        if not fast_insert:
            print_highlighted(clear=True, inserted=True)
            print(color.BOLD + 'Save it', end='')
            if not get_ok():
                raise ValueError()
        else:
            print(color.BOLD + 'Saving (automatic)')
    except (KeyboardInterrupt, ValueError) as err:
        print(color.END, end='')
        if isinstance(err, KeyboardInterrupt):
            bailing = True
        return False
    return True


def create_default_filelist():
    """ Runs "find" to create default file list """
    logging.debug("Creating default file list using find.")
    find_cmd = ['find',
                '-L',  # follow symlinks
                '.',  # from here
                '(',
                    '-name', 'project-registry', '-prune',  # stop file system loops
                    '-o',  # or
                    '-name', 'imports', '-prune',  # more loops
                    '-o',  # or
                    '-name', 'exports', '-prune',  # more loops
                ')', '-false',  # don't want them either
                '-o',  # or
                '-xtype', 'f',  # is, or points to, a file
               ]
    ret_list = subprocess.check_output(find_cmd).decode('ascii').rstrip().split('\n')
    logging.debug("create_default_filelist returning: " + str(ret_list))
    return ret_list


def recurse_directories_in_list(file_list):
    """ Recurses directories from a file list """
    ret_list = []
    for name in file_list:
        if os.path.isdir(name):
            logging.debug(name + " is a directory")
            for root, _, files in os.walk(name):
                ret_list.extend([os.path.join(root, fname) for fname in files])
        elif os.path.isfile(name):
            logging.debug(name + " is a file")
            ret_list.append(name)
        else:
            logging.error(name + " is neither file nor directory!")
            raise FileNotFoundError(name)
    logging.debug("recurse_directories_in_list returning: " + str(ret_list))
    return ret_list


# "main"
my_parser = argparse.ArgumentParser(description="""
Inserts copyright into files, replacing old ones found.

To scan for copyright violations, see the accompanying check_copyright.sh script.
""", formatter_class=argparse.RawTextHelpFormatter)
my_parser.add_argument('-v', '--verify',
                       help="silently verifies files; see copyright.log output for results",
                       action='store_true')
my_parser.add_argument('file_or_dir', nargs='*', default=create_default_filelist())
parsed_args = my_parser.parse_args()
if parsed_args.verify:
    logging.info("Verify given on command line")
    scan_mode = True
if len(sys.argv) < 2:
    if scan_mode:
        print("OCPI_SCAN_COPYRIGHTS detected; running in verification-only mode.")
    else:
        print("No file(s) given on command line. Using auto-scan.")
        logging.warning("No file(s) given on command line. Using auto-scan.")
for filename in sorted(recurse_directories_in_list(parsed_args.file_or_dir)):
    try:
        gbuff = []
        # Read in the file and do the manipulations if file name doesn't skip
        run = False
        if file_to_type(filename) != 'skip':
            with open(filename, 'r', errors='ignore') as f:
                gbuff = f.readlines()
            run = process()
        if bailing:
            sys.exit(1)
        # Do the write if we got this far and process() said to do something
        if run:
            logging.warning('%s: Saving.', filename)
            with open(filename, 'w') as f:
                print(''.join(gbuff), file=f, end='')
    except IOError as err:
        print('Could not process {0}: {1}'.format(filename, str(err)))
        logging.error('%s: Could not open: %s.', filename, str(err))
print(color.BOLD + color.GREEN + 'Done. See "copyright.log" for details.' + color.END)
