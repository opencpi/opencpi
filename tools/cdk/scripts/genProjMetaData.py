#!/usr/bin/python
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


from lxml import etree as ET
import os
import sys
import subprocess


def addLibs(curRoot, libs):
   for a in libs:
     worker = ET.SubElement(curRoot, "library")
     worker.set('name', a)

def addSpecs(curRoot, curDir):
  for dirName, subdirList, fileList in os.walk(curDir):
    if (dirName.endswith("/specs")):
      for a in fileList:
        worker = ET.SubElement(curRoot, "spec")
        worker.set('name', a)

def addWorkers(curRoot, workers, dirName):
   addSpecs(curRoot, dirName)
   for a in workers:
     if(not a.endswith(".test") and (a not in ["lib","specs","gen","doc"])):
       worker = ET.SubElement(curRoot, "worker")
       worker.set('name', a)

def dirIsLib(dirName, comps):
  for name in comps.findall("library"):
    if (dirName.endswith(name.get("name"))):
      return (True, name)
  return (False, [])

def addApplications (root, apps):
  for a in apps:
    app = ET.SubElement(root, "application")
    app.set('name', a)

def addPlatforms (root, plats):
  for a in plats:
    if(a not in ["lib"]):
      app = ET.SubElement(root, "platforms")
      app.set('name', a)

def addAssemblies (root, assys):
  for a in assys:
    app = ET.SubElement(root, "assembly")
    app.set('name', a)

def isStale (myDir, force):
  retVal = True
  find_output = ""
  # find /home/chinkey/git_stuff/ocpiassets -anewer ~/git_stuff/ocpiassets/project.xml -name "*.xml"
  print force
  if (force == False):
    find_output = subprocess.Popen(['find', myDir, "-name", "*.xml", '-newer', myDir + "/project.xml", "-quit"], stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]

  if (find_output != ""):
    retVal = False
  else:
    print "project.xml does not yet exist. It will be created."

  return retVal

def getProjDir(myDir, force ):
  loopVal=True
  currentDir = myDir
  if (force == True):
    return currentDir
  loopNum = 0;
  while (loopNum < 10):
    if (os.path.isfile(currentDir + "/Makefile")):
      with open(currentDir + "/Makefile", 'r') as myfile:
        data=myfile.read().replace('\n', '')
      if (data.find("include $(OCPI_CDK_DIR)/include/project.mk") > -1) :
        return currentDir
    currentDir = currentDir + "/../"
    loopNum = loopNum + 1
  print "Not in a Project Directory, Check the directory passed to the script"
  sys.exit(0)


# main
if len(sys.argv) < 2 :
  print("ERROR: need to specify the path to the project")
  sys.exit(1)

if ((len(sys.argv) == 3) and (sys.argv[2] == "force")):
  force = True
else:
  force = False

mydir = sys.argv[1]
mydir = getProjDir(mydir, force)

if (isStale(mydir, force)):
  root = ET.Element("project")
  comps = ET.SubElement(root, "components")
  hdl = ET.SubElement(root, "hdl")
  assys = ET.SubElement(hdl, "assemblies")
  prims = ET.SubElement(hdl, "primatives")

  for dirName, subdirList, fileList in os.walk(mydir):
    if "exports" in dirName:
      continue
    elif dirName.endswith("/components"):
      addLibs(comps, subdirList)
    elif dirName.endswith("/applications"):
      addApplications(root, subdirList)
    elif dirName.endswith("/platforms"):
      addPlatforms(hdl, subdirList)
    elif dirName.endswith("/cards"):
      cards = ET.SubElement(hdl, "library")
      cards.set('name', "cards")
      addWorkers(cards, subdirList, dirName)
    elif dirName.endswith("/devices"):
      if "platforms" not in dirName:
        devs = ET.SubElement(hdl, "library")
        devs.set('name', "devices")
        addWorkers(devs, subdirList, dirName)
      else:
        dirSplit = dirName.split('/')
        platName = [];
        index = range(0, len(dirSplit)-1)
        for i, sub in zip(index, dirSplit):
          if (sub == "platforms"):
            platName = dirSplit[i+1]
        plat = hdl.findall("platforms[@name='"+platName+"']")
        devs = ET.SubElement(plat[0], "library")
        devs.set('name',"devices")
        addWorkers(devs, subdirList, dirName)
    elif dirName.endswith("hdl/assemblies"):
      addAssemblies(assys,subdirList)
    retVal = dirIsLib(dirName, comps)
    if (retVal[0]):
      addWorkers(retVal[1], subdirList, dirName)

  print "Updated: "+mydir+"/project.xml"
  myFile = open(mydir+"/project.xml", 'w')
  myFile.write(ET.tostring(root,pretty_print=True))
else:
  print("meatadata is not stale, not regenerating")
sys.exit(0)
