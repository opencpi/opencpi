#!/usr/bin/python3
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

import sys
import os
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/scripts/')  # for ocpishow and genProjMetaData
import genProjMetaData
sys.path.append(os.getenv('OCPI_CDK_DIR') + '/' + os.getenv('OCPI_TOOL_PLATFORM') + '/lib/')
import _opencpi.util as ocpiutil
import _opencpi.assets.factory as ocpifactory
import _opencpi.assets.registry as ocpiregistry
import grp
import getpass
import shutil
import tarfile
import subprocess

def print_help(err):
    print("""
usage: ocpi-copy-projects [proj_dir registry_dir]

Utility for making user copies of globally-installed projects.  Can be run in
both interactive mode or given arguments where no user interaction is needed.

Optional Positional Arguments:
proj_dir           Destination location for copied projects.
                   e.g. ~/ocpi_projects
registry_dir       Registry location where copied projects are registered
                   default: /opt/opencpi/project-registry or value of
                            OCPI_PROJECT_REGISTRY_DIR

If no arguments are given the script is interactive and the user is required to
input required information.  In interactive mode the user is able to do the
following:
            1) Unregister any projects from the destination registry if they
               choose.
            2) update .bashrc to set the environment variable
               OCPI_PROJECT_REGISTRY_DIR to the destination registry

""")
    sys.exit(err)

def extract_all_tar(cur_dir, delete=False):
    tarfiles = [os.path.join(cur_dir, fname) for fname in os.listdir(cur_dir)
                if os.path.isfile(os.path.join(cur_dir, fname))
                and tarfile.is_tarfile(os.path.join(cur_dir, fname))]

    for tfile in tarfiles:
        tarball = tarfile.open(tfile)
        tarball.extractall(os.path.dirname(tfile))
        if delete:
            os.remove(tfile)

def dir_exists_fail(in_dir):
    print("\"" + in_dir + "\" already exists and is not empty, cannot make new projects there. " +
           "Pick a different location or remove the directory or the contents")
    sys.exit(1)

# main
projects_dir = os.getenv('OCPI_CDK_DIR') + "/../projects/"
projects = [dir for dir in os.listdir(projects_dir)
            if not os.path.isfile(os.path.join(projects_dir, dir))]
while "bsps" in projects:
    projects.remove("bsps")
new_user_dir = None
new_reg_dir = None
force = False
if len(sys.argv) == 3:
    new_user_dir = sys.argv[1]
    new_reg_dir = sys.argv[2]
    force = True
    # if dir exists and is not empty
    if os.path.isdir(new_user_dir) and os.listdir(new_user_dir):
        dir_exists_fail(new_user_dir)
elif len(sys.argv) == 2 and sys.argv[1] in {"--help", "-h"}:
    print_help(0)
elif len(sys.argv) == 2:
    new_user_dir = sys.argv[1]
    new_reg_dir = ocpiregistry.Registry.get_registry_dir()
    force = True
    # if dir exists and is not empty
    if os.path.isdir(new_user_dir) and os.listdir(new_user_dir):
        dir_exists_fail(new_user_dir)
elif len(sys.argv) != 1:
    print("Incorrect number of arguments")
    print_help(1)

if not len(projects):
    print("Could not find any projects in '"+projects_dir+"'!")
    sys.exit(1)

print("Available projects are: " + ", ".join(projects))

proj_dir_dict = {}
while not new_user_dir:
    default_dir = "~/ocpi_projects"
    user_dir = input("Location to copy projects (default: " +
                      default_dir + "): ")
    if user_dir == "":
        user_dir = default_dir
    user_dir = os.path.abspath(os.path.expanduser(user_dir))
    print(user_dir)
    # if dir exists and is not empty
    if os.path.isdir(user_dir) and os.listdir(user_dir):
        dir_exists_fail(user_dir)
    else:
        new_user_dir = user_dir

for proj in projects:
    dest_dir = os.path.join(new_user_dir, proj)
    print("Copying " + proj)
    shutil.copytree(os.path.join(projects_dir, proj), dest_dir, symlinks=True)
    extract_all_tar(dest_dir, True)
    proj_dir_dict[proj] = dest_dir
try:
    is_ocpi_grp = getpass.getuser() in grp.getgrnam("opencpi").gr_mem
except:
    is_ocpi_grp = False

# TODO: Should look in .bashrc and see if OCPI_PROJECT_REGISTRY_DIR set and import it into
# "new_reg_dir".  Otherwise, we will fill up their .bashrc on multiple runs
if not new_reg_dir:
    prompt_str = ("Use default registry location (" +
                  os.path.abspath(ocpiregistry.Registry.get_registry_dir()) + "):")
    if not ocpiutil.get_ok(prompt=prompt_str, default=True):
        while not new_reg_dir:
            reg_dir = input("Registry location (default: ~/project-registry): ")
            if reg_dir == "":
                reg_dir = "~/project-registry"
            reg_dir = os.path.expanduser(reg_dir)
            if not os.path.isdir(reg_dir):
                # got a valid directory otherwise keep trying
                new_reg_dir = reg_dir
                my_registry = ocpiregistry.Registry.create(new_reg_dir)
            else:
                new_reg_dir = reg_dir
                my_registry = ocpifactory.AssetFactory.factory("registry", new_reg_dir)
        if ocpiutil.get_ok(prompt="~/.bashrc must be updated now to reflect new default " +
                           "registry. Do so automatically now:", default=True):
            bashrc = open(os.path.expanduser("~/.bashrc"), 'a')
            bashrc.write("export OCPI_PROJECT_REGISTRY_DIR=" + new_reg_dir + "\n")

    elif (is_ocpi_grp or
         ocpiregistry.Registry.get_registry_dir() != "/opt/opencpi/project-registry"):
        my_registry = ocpifactory.AssetFactory.factory("registry",
                                                       ocpiregistry.Registry.get_registry_dir())
    else:
        print("User not in the opencpi user group. This is required to register projects in the " +
              "default location")
        print("\nProject creation successful but projects were not registered automatically")
        sys.exit(0)

elif (not is_ocpi_grp and new_reg_dir == "/opt/opencpi/project-registry"):
    print("User not in the opencpi user group. This is required to register projects in the " +
          "default location")
    print("\nProject creation successful but projects were not registered automatically")
    sys.exit(0)

else:
    if os.path.isdir(new_reg_dir):
        my_registry = ocpifactory.AssetFactory.factory("registry", new_reg_dir)
    else:
        my_registry = ocpiregistry.Registry.create(new_reg_dir)

os.environ['OCPI_PROJECT_REGISTRY_DIR'] = my_registry.directory
user_yes = False
if not force:
    my_registry.show("table", False)
    user_yes = ocpiutil.get_ok(prompt="Unregister old copies of the projects from the registry",
                               default=True)

for proj in projects:
    proj_pack_id = ocpifactory.AssetFactory.factory("project", new_user_dir + "/" + proj).package_id
    link_name = my_registry.directory + "/" + proj_pack_id
    # remove the project from the registry if it's there and one of the following:
    # 1. the user wants us to
    # 2. it's the project from the rpm
    # 3. the link to the project is broken
    if (os.path.exists(link_name) and
        (my_registry.contains(directory="/opt/opencpi/projects/core") or
         user_yes or
         not os.path.exists(os.readlink(link_name)))):
        print("removing: " + proj_pack_id + " from registry")
        my_registry.remove(package_id=proj_pack_id)

for proj in projects:
    # call ./genProjMetadata for this project
    sys.argv = ["genProjMetadata.py", new_user_dir + '/' + proj]
    genProjMetaData.main()

num_exceptions = 0
for proj in projects:
    try:
        proj_obj = ocpifactory.AssetFactory.factory("project", new_user_dir + '/' + proj)
        proj_obj.set_registry(new_reg_dir)
        my_registry.add(proj_dir_dict[proj])
        make_call = ["make", "-C", proj_dir_dict[proj], "exports"]
        child = subprocess.Popen(make_call, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        child.wait()

    except ocpiutil.OCPIException as ex:
        print("\nunable to register project " + proj + " due to error: \n" + str(ex))
        num_exceptions += 1

my_registry.show("table", False)
if num_exceptions:
    print("\nProject creation successful, but some or all projects were not registered " +
          "automatically")
else:
    print("\nProject creation successful")
sys.exit(0)
