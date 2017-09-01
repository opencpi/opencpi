import os
import os.path
import re
import subprocess

# Print an error message and raise an exception
def bad(message, error=RuntimeError):
  print "Error: " + message
  raise error

# Given an argument list, join the args in a single
# string and print them. Print nothing if any arg
# is empty or None
def print_if_exists(*args):
  for a in args:
    if a == None or a == "":
      return
  print "".join(args)

###############################################################################
# Collect a dictionary of variables from a makefile
###
# First arg is .mk_ file to use
# second arg is Make arg to invoke the right output
#   The output can be an assignment or a target
# third arg = verbose
# Return a dictionary of variable names mapped to values
# from Make land
###############################################################################
def set_vars_from_make(mk_file, mk_arg="", verbose=None):
  FNULL = open(os.devnull, 'w')
  make_exists = subprocess.Popen(["which", "make"], stdout=subprocess.PIPE, stderr=FNULL).communicate()[0]
  if (make_exists == None or make_exists == ""):
    if (verbose != None and verbose != ""):
      print "The '\"make\"' command is not available."
    return 1

  make_cmd = "make -n -r -s -f " + mk_file + " " + mk_arg
  if (verbose == None or verbose == ""):
    mk_process = subprocess.Popen(make_cmd.split(), stdout=subprocess.PIPE)
  else:
    mk_process = subprocess.Popen(make_cmd.split(), stdout=subprocess.PIPE, stderr=FNULL)
  mk_output =  mk_process.communicate()[0]

  try:
    grep_str = re.match(r'^[a-zA-Z_][a-zA-Z_]*=.*', mk_output).group()
  except AttributeError:
    print "No variables are set from \"" + mk_file + "\""
    return -1

  assignment_strs = filter(lambda s: len(s) > 0, [ x.strip() for x in grep_str.split(';')])
  make_vars = {}
  for x in assignment_strs:
    make_vars[x.split('=')[0]] = x.split('=')[1]
    #print make_vars[x.split('=')[0]]
  return make_vars

###############################################################################
# Utility functions for extracting variables and information from
# hdl-targets.mk
###############################################################################

# Get make variables from hdl-targets.mk
# Dictionary key examples are:
#   HdlAllTargets, HdlTargets (filtered), HdlAllPlatforms, HdlAllFamilies,
#   HdlTargets_<family> (HdlTargets_zynq), HdlPart_<platform> (HdlPart_zed)
def get_make_vars_hdl_targets():
  return set_vars_from_make(os.environ["OCPI_CDK_DIR"] + "/include/hdl/hdl-targets.mk", "ShellHdlTargetsVars=1", "verbose")

def get_part_for_platform(platform, make_vars):
  return make_vars['HdlPart_' + platform]

def get_target_for_part(part, make_vars):
  return make_vars['HdlFamily_' + part]

def get_toolset_for_target(target, make_vars):
  return make_vars['HdlToolSet_' + target]

def get_toolset_for_platform(platform, make_vars):
  return get_toolset_for_target(get_target_for_part(get_part_for_platform(platform, make_vars), make_vars), make_vars)

###############################################################################
# Utility functions for extracting variables and information from
# rcc-make.mk
###############################################################################

# Get make variables from rcc-make.mk
# Dictionary key examples are:
#   RccAllPlatforms, RccPlatforms, RccAllTargets, RccTargets
def get_make_vars_rcc_targets():
  return set_vars_from_make(os.environ["OCPI_CDK_DIR"] + "/include/rcc/rcc-make.mk", "ShellRccTargetsVars=1", "verbose")

###############################################################################
# Utility functions for collecting information about the directories
# in a project
###############################################################################

# Determine a directoy's type by parsing it for the last 'include ... *.mk' line
def get_dirtype(directory=".",noerror=False):
  match = None
  if os.path.isfile(directory + "/Makefile"):
    for line in open(directory + "/Makefile"):
      result = re.match("^\s*include\s*.*OCPI_CDK_DIR.*/include/(hdl/)?(.*)\.mk.*", line)
      match = result.group(2) if result != None else match
  if match == None:
    if noerror:
      return None
    else:
      bad("directory \"" + directory + "\" does not have a dirtype.")
  return match

# Return a list of directories underneath the given directory
# that have a certain type (library, worker, hdl-assembly...)
def get_subdirs_of_type(dirtype,directory="."):
  subdir_list = []
  for subdir,_,_  in os.walk(directory):
    if get_dirtype(subdir, noerror=True) == dirtype:
      subdir_list.append(subdir)
  return subdir_list

###############################################################################
# String and number manipulation utility functions
###############################################################################

def isfloat(value):
  try:
    float(value)
    return True
  except ValueError:
    return False

def isint(value):
  try:
    int(value)
    return True
  except ValueError:
    return False

# Convert a string representing period in ns
# to a string representing frequency in MHz
# Return the string or "" on failure
def freq_from_period(prd_string):
  if prd_string == None:
    return ""
  period=0
  prd=re.sub('ns','',prd_string)
  if isfloat(prd):
    period=float(prd)
  if period != 0:
    freq=1000/float(period)
    return '{:.3f}'.format(freq)
  return ""

# Return the first number in a list
# of strings. Ignore commas and 'ns'
# Return "" if no number is found
def first_num_in_str_list(strings):
  for i in strings.split(' '):
    num=re.sub(',','',i)
    num=re.sub('ns','',i)
    if isfloat(num):
       if isint(num):
         return str(num)
       else:
         return '{:.3f}'.format(float(num))
  return ""

