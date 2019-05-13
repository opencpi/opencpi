#!/usr/bin/env python2
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

# TODO: pylint and clean warnings

from __future__ import print_function
import sys
import re
import textwrap
#import xml.etree.ElementTree as ET
from lxml import etree
from enum import Enum
import os

lcName = ''
ucName = ''

def getIsPropAccessibilityKey(key):
   # TODO: re-write and test: return key.lower() in ("volatile", "readback", ...)
   ret = key == "Volatile"
   ret |= key == "Readable"
   ret |= key == "Readback"
   ret |= key == "Writable"
   ret |= key == "Initial"
   return ret

def getBoolFromXmlBool(val):
   # TODO: re-write and test: see rewrite notes for getIsPropAccessibilityKey
   if val == "0":
      val == False
   elif val == "1":
      val == True
   elif val == "False":
      val == False
   elif val == "True":
      val == True
   elif val == "false":
      val == False
   elif val == "true":
      val == True
   return val

def getIsPropAttributeWhoseValIsBool(key):
   return getIsPropAccessibilityKey(key)

def getListFromParsedAttributeKeys(root, keys):
   ret = []
   for key in keys:
      for attr in root.iter(key):
         ret.append(attr)
      for attr in root.iter(key.lower()):
         ret.append(attr)
   return ret

def getDictFromParsedAttributeKeys(attrs, keys):
   ret = dict()
   for key in keys:
      tmp = attrs.get(key)
      if (type(tmp) is not str):
         tmp = attrs.get(key.lower())
      ret[key] = tmp
   return ret

def getPropDictFromParsedAttributeKey(attrs, key):
  ret = attrs.get(key)
  if (type(ret) is not str):
     ret = attrs.get(key.lower())
  return ret

def getPropDictFromParsedAttributeKeys(attrs):

   keys = ["Name", "Type", "SequenceLength",
           "Volatile", "Readable", "Readback", "Writable",
           "Initial", "Enums", "Default", "Description"]

   ret = dict()
   for key in keys:
      ret[key] = getPropDictFromParsedAttributeKey(attrs, key)

   arrayDimensionsKey = "ArrayDimensions"
   tmp = attrs.get(arrayDimensionsKey)
   if (type(tmp) is not str):
      tmp = attrs.get(arrayDimensionsKey.lower())
      if (type(tmp) is not str):
         tmp = getPropDictFromParsedAttributeKey(attrs, "ArrayLength")
   # put either ArrayDimensions or ArrayLength in ArrayDimensions dict entry
   ret[arrayDimensionsKey] = tmp

   return ret

def parsePorts(root):
   keys = ["DataInterfaceSpec", "Port", "StreamInterface"]
   return getListFromParsedAttributeKeys(root, keys)

def parseProps(root):
   return getListFromParsedAttributeKeys(root, ["Property"])

def parseHdlDevice(dev):
   return parseProps(dev)

def parseRoot(root):
   isHDL = None
   isOCS = True
   cProps = []
   wProps = []
   cPorts = []
   wPorts = []
   for elem in ["RccWorker", "rccworker"]:
      for worker in root.iter(elem):
         isOCS = False
         isHDL = False
         #print("DEBUG: found RccWorker")
         workerProperties = parseProps(worker)
         wPorts = parsePorts(worker)
         wProps = parseProps(worker)
   for elem in ["HdlWorker", "hdlworker"]:
      for worker in root.iter(elem):
         isOCS = False
         isHDL = True
         #print("DEBUG: found HdlWorker")
         workerProperties = parseProps(worker)
         wPorts = parsePorts(worker)
         wProps = parseProps(worker)
   for elem in ["HdlDevice", "hdldevice"]:
      for worker in root.iter(elem):
         isOCS = False
         isHDL = True
         #print("DEBUG: found HdlDevice")
         workerProperties = parseProps(worker)
         wPorts = parsePorts(worker)
         wProps = parseProps(worker)
   for elem in ["ComponentSpec", "componentspec"]:
      for comp in root.iter(elem):
         #print("DEBUG: found ComponentSpec")
         cPorts = parsePorts(comp)
         if isOCS:
            cProps = parseProps(comp)
         else:
            wProps.append(parseProps(comp))
   return [cProps, wProps, cPorts, wPorts, isOCS, isHDL]

def getCompName(ocsFilePath):
   parsedList = ocsFilePath.split("/")
   parsedName = parsedList[-1]
   parsedName = parsedName.split("spec.xml")
   name = parsedName[0]
   return name[:-1]

def parseCompName(myName):
   global ucName
   global lcName
   lcName = getCompName(myName)
   lcName = lcName.replace('_','\_')
   parsedList = re.split('_|-', lcName)
   ucName = ''
   for i in parsedList:
     ucName += i + ' '
   ucName = ucName.title()

def parseWorkerName(myName):
   global ucName
   global lcName
   parsedList = myName.split("/")
   parsedName = parsedList[-1]
   parsedName = parsedName.split(".xml")
   lcName = parsedName[0]
   lcName = lcName.replace('_','\_')
   parsedList = re.split('_|-', lcName)
   ucName = ''
   for i in parsedList:
     ucName += i + ' '
   ucName = ucName.title()

def latexify(string):
   string = string.replace('_','\_')
   string = string.replace('&','\&')
   string = string.replace('%','\%')
   string = string.replace('^','\^{}')
   return string

class PropertyType(Enum):
    ULONG=0
    LONG=1
    ULONGLONG=2
    LONGLONG=3
    USHORT=4
    SHORT=5
    UCHAR=6

class Property():

   def __init__(self, name, ptype=PropertyType.ULONG,
                sequenceLength=None, arrayDimensions=None,
                volatile=None, readable=None, readback=None, writable=None,
                initial=False, enums=None, default=None, description=None):
      self.name = name
      self.ptype = ptype
      self.sequenceLength = sequenceLength
      self.arrayDimensions = arrayDimensions
      self.volatile = False if volatile is None else volatile
      self.readable = False if readable is None else readable
      self.readback = False if readback is None else readback
      self.writable = False if writable is None else writable
      self.initial = False if initial is None else initial
      self.enums = enums
      self.default = 0 if default is None else default
      self.description = description

   def __str__(self):
      ret = "name=" + str(self.name)
      ret += ", ptype=" + str(self.ptype)
      ret += ", sequenceLength=" + str(self.sequenceLength)
      ret += ", arrayDimensions=" + str(self.arrayDimensions)
      ret += ", volatile=" + str(self.volatile)
      ret += ", readable=" + str(self.readable)
      ret += ", readback=" + str(self.readback)
      ret += ", writable=" + str(self.writable)
      ret += ", initial=" + str(self.initial)
      ret += ", enums=" + str(self.enums)
      ret += ", default=" + str(self.default)
      ret += ", description=" + str(self.description)
      return ret

   def getLatexTableRow(self):
      return str(PropertyLatexTableRow(self))

class PropertyLatexTableRow():

   def __init__(self, prop):
      self.prop = prop

   def __str__(self):
      # TODO: Re-write with " & ".join()
      ret = self.getColumnStrName()
      ret += " & "
      ret += self.getColumnStrType()
      ret += " & "
      ret += self.getColumnStrSequenceLength()
      ret += " & "
      ret += self.getColumnStrArrayDimensions()
      ret += " & "
      ret += self.getColumnStrAccessibility()
      ret += " & "
      ret += self.getColumnStrValidRange()
      ret += " & "
      ret += self.getColumnStrDefault()
      ret += " & "
      ret += self.getColumnStrDescription()
      return ret

   def getColumnStrName(self):
      return latexify(str(self.prop.name))

   def getColumnStrType(self):
      return str(self.prop.ptype)

   def getColumnStrSequenceLength(self):
      ret = ""
      if self.prop.sequenceLength is None:
         ret += "-"
      else:
         ret += latexify(str(self.prop.sequenceLength))
      return ret

   def getColumnStrArrayDimensions(self):
      ret = ""
      if self.prop.arrayDimensions is None:
         ret += "-"
      else:
         ret += latexify(str(self.prop.arrayDimensions))
      return ret

   def getColumnStrValidRange(self):
      ret = ""
      if self.prop.enums is None:
         ret += "Standard"
      else:
         # insert spaces after commas to aid in LaTeX table cell wrapping
         ret += latexify(self.prop.enums).replace(",", ", ")
      return ret

   def getColumnStrAccessibility(self):
      # TODO: Create array of strings then use join
      ret = ""
      insertComma = False
      if self.prop.volatile:
         ret += "Volatile"
         insertComma = True
      if self.prop.readable:
         ret += ", Readable" if insertComma else "Readable"
         insertComma = True
      if self.prop.readback:
         ret += ", Readback" if insertComma else "Readback"
         insertComma = True
      if self.prop.writable:
         ret += ", Writable" if insertComma else "Writable"
         insertComma = True
      if self.prop.initial:
         ret += ", Initial" if insertComma else "Initial"
         insertComma = True
      return ret

   def getColumnStrDefault(self):
      return latexify(str(self.prop.default))

   def getColumnStrDescription(self):
      ret = ""
      if self.prop.description is None:
         ret += "-"
      else:
         ret += latexify(self.prop.description)
      return ret

def getPropertyFromXmlProp(xmlProp):

   attributes = getPropDictFromParsedAttributeKeys(xmlProp)

   ret = Property(name            = attributes["Name"],
                  ptype           = attributes["Type"],
                  sequenceLength  = attributes["SequenceLength"],
                  arrayDimensions = attributes["ArrayDimensions"],
                  volatile        = attributes["Volatile"],
                  readable        = attributes["Readable"],
                  readback        = attributes["Readback"],
                  writable        = attributes["Writable"],
                  initial         = attributes["Initial"],
                  enums           = attributes["Enums"],
                  default         = attributes["Default"],
                  description     = attributes["Description"])
   #print("DEBUG:", str(ret))
   return ret

def emitPropertyLatexTableTitleRow(outFile):
   # TODO: Move to multi-line string (triple-double-quoted)
   outFile.write("\\hline\n")
   outFile.write("\\rowcolor{blue}\n")
   outFile.write("Name                 & Type   & SequenceLength & ArrayDimensions & Accessibility       & Valid Range & Default & Description\n")
   outFile.write("\\\\\n")

def emitPropertyLatexTableRow(outFile, xmlProp):
   outFile.write("\\hline\n")
   outFile.write(getPropertyFromXmlProp(xmlProp).getLatexTableRow())
   outFile.write("\\\\\n")

def emitPropTable(outFile, props, isHDL):
   outFile.write("\\begin{scriptsize}\n")
   outFile.write("\\begin{longtable}{|p{\\dimexpr0.2\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.05\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.125\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.125\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.1\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.1\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.1\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.2\\linewidth-2\\tabcolsep\\relax}|}\n")
   emitPropertyLatexTableTitleRow(outFile)
   for xmlProp in props:
      emitPropertyLatexTableRow(outFile, xmlProp)
   outFile.write("\\hline\n")
   outFile.write("\\end{longtable}\n")
   outFile.write("\\end{scriptsize}\n")

def deleteFile(fileName):
   print("INFO : deleting file", fileName)
   os.remove(fileName)

def emitPortTable(outFile, ports):
   # TODO: Move to multi-line string (triple-double-quoted)
   outFile.write("\\begin{scriptsize}\n")
   outFile.write("\\begin{longtable}{|p{\\dimexpr0.3\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.2\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.2\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.3\\linewidth-2\\tabcolsep\\relax}|}\n")
   outFile.write("\\hline\n")
   outFile.write("\\rowcolor{blue}\n")
   outFile.write("Name & Producer & Protocol & Optional \n")
   outFile.write("\\\\\n")
   for port in ports:
      nameStr = port.get("Name")
      protStr = port.get("Protocol")
      if (type(protStr) is str):
         protStr = protStr.replace('_','\_')
      else:
         protStr = port.get("protocol")
         if (type(protStr) is str):
            protStr = protStr.replace('_','\_')
      if (type(nameStr) is str):
         aameStr = nameStr.replace('_','\_')
      else:
         nameStr = port.get("name")
         if (type(nameStr) is str):
            nameStr = nameStr.replace('_','\_')
      prodStr = port.get("Producer")
      if (type(prodStr) is not str):
          prodStr = port.get("producer")
      if (type(prodStr) is not str):
          prodStr = "False"
      optStr = port.get("Optional")
      if (type(optStr) is not str):
          optStr = port.get("optional")
      if (type(optStr) is not str):
          optStr = "False"
      outFile.write("\\hline\n")
      outFile.write(nameStr + " & " + prodStr + " & " + str(protStr) + "& " + optStr)
      outFile.write("\\\\\n")
   outFile.write("\\hline\n")
   outFile.write("\\end{longtable}\n")
   outFile.write("\\end{scriptsize}\n")

def emitIntTable(outFile, ports):
   # TODO: Move to multi-line string (triple-double-quoted)
   outFile.write("\\begin{longtable}{|p{\\dimexpr0.33\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.34\\linewidth-2\\tabcolsep\\relax}\n")
   outFile.write("                  |p{\\dimexpr0.33\\linewidth-2\\tabcolsep\\relax}|}\n")
   outFile.write("\\hline\n")
   outFile.write("\\rowcolor{blue}\n")
   outFile.write("Type & Name & DataWidth \n")
   outFile.write("\\\\\n")

   for port in ports:
      attrs = getDictFromParsedAttributeKeys(port, ["Name", "DataWidth"])
      outFile.write("\\hline\n")
      nn = latexify(attrs["Name"])
      dd = latexify(attrs["DataWidth"])
      outFile.write("StreamInterface & " + nn + " & " + dd)
      outFile.write("\\\\\n")
   outFile.write("\\hline\n")
   outFile.write("\\end{longtable}\n")

def shouldExitBecauseFileOverwriteUnallowed(fileName):
   res = None
   if(os.path.isfile(fileName)):
      msg = "WARN : file " + fileName + " already exists, overwrite (y or n)? "
      isYesOrNo = False
      while not isYesOrNo:
         res = raw_input(msg)
         isYesOrNo |= (res == "y")
         isYesOrNo |= (res == "n")
   return (res != None) and (res != "y")

def emitHeader(fp, editingIntended):
   # TODO: Move to multi-line string (triple-double-quoted)
   fp.write("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n")
   fp.write("% this file was generated by docGen.py\n");
   if editingIntended:
      fp.write("% this file is intended to be edited\n")
   else:
      fp.write("% editing this file is NOT recommended\n");
   fp.write("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n")

def parseOCSOrOWD(filePathToOCSOrOWD):
   print("INFO : parsing", filePathToOCSOrOWD)
   tree = etree.parse(filePathToOCSOrOWD)
   tree.xinclude()

   return parseRoot(tree.getroot())

def printPreEmitMessage(fileName, append = None):
   if shouldExitBecauseFileOverwriteUnallowed(fileName):
      exit(0)#raise Exception('exit_to_prevent_file_overwrite')

   msg = "INFO : emitting " + fileName
   if append is not None:
      msg += append
   print(msg)

def emitDataSheetTexFile(OCSorOWDFileName):
   parseCompName(OCSorOWDFileName)

   dataSheetFileName = getCompName(OCSorOWDFileName) + ".tex"
   dataSheetFile = None

   skeletonFileName = os.path.dirname(os.path.realpath(__file__))
   skeletonFileName += "/snippets/Component_Template.tex"
   inFile = None

   msg = " (compile using rubber -d " + dataSheetFileName + ")"
   printPreEmitMessage(dataSheetFileName, append=msg)
   inFile = open(skeletonFileName, 'r')
   dataSheetFile = open(dataSheetFileName, 'wr')

   emitHeader(dataSheetFile, editingIntended=False)

   currentLine = 'a thing'
   while currentLine != '':
      currentLine = inFile.readline()
      if currentLine == '%GEN_COMPLC_NAME\n':
         dataSheetFile.write("\def\comp{"+ lcName +"}\n")
         dataSheetFile.write(currentLine)
      elif currentLine == '%GEN_COMPUC_NAME\n':
         dataSheetFile.write("\def\Comp{"+ ucName +"}\n")
         dataSheetFile.write(currentLine)
      else:
         dataSheetFile.write (currentLine)

   dataSheetFile.close()

def emitNoComponent(ff, thing):
   msg = "This component has no " + thing + "."
   ff.write(msg)

def emitPropsTableTexFile(props, doSectionNumbers, isOCS, isHDL, fileName, titleText):

   sectionText = "section*"
   if doSectionNumbers:
      sectionText = "section"

   printPreEmitMessage(fileName)

   ff = open(fileName, 'wr')
   emitHeader(ff, editingIntended=False)

   if not (isOCS and titleText == "Worker Properties"):
      if props == []:
         if isOCS:
            emitNoComponent(ff, "properties")
      else:
         ff.write("\\" + sectionText + "{" + titleText + "}\n")
         if not isOCS: # i.e. is OWD
            if isHDL:
               ff.write("\subsection*{\comp.hdl}\n")
            else:
               ff.write("\subsection*{\comp.rcc}\n")
         emitPropTable(ff, props, isHDL)

   ff.close()

def emitPortsTableTexFile(ports, doSectionNumbers, isOCS, isHDL, fileName, titleText):

   sectionText = "section*"
   if doSectionNumbers:
      sectionText = "section"

   printPreEmitMessage(fileName)

   ff = open(fileName, 'wr')

   emitHeader(ff, editingIntended=False)

   if not (isOCS and titleText == "Worker Interfaces"):
      ff.write("\\" + sectionText + "{" + titleText + "}\n")
      if ports == []:
         if isOCS:
            emitNoComponent(ff, "ports")
      else:
         if isOCS:
            emitPortTable(ff, ports)
         else:
            if isHDL:
               ff.write("\subsection*{\comp.hdl}\n")
            else:
               ff.write("\subsection*{\comp.rcc}\n")
            emitIntTable(ff, ports)

   ff.close()

def emitTablesTexFiles(cProps, wProps, cPorts, wPorts, isOCS, isHDL):
   compPropsFileName  = 'component_spec_properties.tex'
   workerPropsFileName = 'worker_properties.tex'
   compPortsFileName   = 'component_ports.tex'
   workerPortsFileName = 'worker_interfaces.tex'

   doSectionNumbers = False
   if isOCS:
      emitPropsTableTexFile(cProps, doSectionNumbers, isOCS, isHDL,
                            fileName=compPropsFileName,
                            titleText="Component Properties")
      emitPortsTableTexFile(cPorts, doSectionNumbers, isOCS, isHDL,
                            fileName=compPortsFileName,
                            titleText="Component Ports")
   if not (isOCS and os.path.isfile(workerPropsFileName)):
      emitPropsTableTexFile(wProps, doSectionNumbers, isOCS, isHDL,
                            fileName=workerPropsFileName,
                            titleText="Worker Properties")
   if not (isOCS and os.path.isfile(workerPortsFileName)):
      emitPortsTableTexFile(wPorts, doSectionNumbers, isOCS, isHDL,
                            fileName=workerPortsFileName,
                            titleText="Worker Interfaces")

def emitDeveloperDocTexFileIfDoesNotExist():
   fileName = 'developer_doc.tex'
   if(os.path.isfile(fileName)):
      print("INFO : skipping emission of", fileName, "because it already exists")
   if(not os.path.isfile(fileName)):
      ff = None
      printPreEmitMessage(fileName, append=" (developers should edit this)")
      # TODO: Move to 'with' syntax to catch file open/close junk
      # TODO: Move to multi-line string (triple-double-quoted)
      try:
         ff = open(fileName, 'wr')
         emitHeader(ff, editingIntended=True)
         ff.write("\n")
         ff.write("\\section*{Summary - \\Comp}\n")
         ff.write("% Make table whose width is equal to which will be used for text\n")
         ff.write("% wrapping, split into 2 equal columns\n")
         ff.write("\\begin{longtable}{|p{\\dimexpr0.5\\textwidth-2\\tabcolsep\\relax}\n")
         ff.write("                  |p{\\dimexpr0.5\\textwidth-2\\tabcolsep\\relax}|}\n")
         ff.write("  \\hline\n")
         ff.write("  \\rowcolor{blue}\n")
         ff.write("  & \\\\\n")
         ff.write("  \\hline\n")
         ff.write("  Name              & \\comp \\\\\n")
         ff.write("  \\hline\n")
         ff.write("  Latest Version    &  v1.4 (release date 9/2018) \\\\\n")
         ff.write("  \\hline\n")
         ff.write("  Worker Type       &  \\\\\n")
         ff.write("  \\hline\n")
         ff.write("  Component Library &  \\\\\n")
         ff.write("  \\hline\n")
         ff.write("  Workers           &  \\\\\n")
         ff.write("  \\hline\n")
         ff.write("  Tested Platforms  &  \\\\\n")
         ff.write("  \\hline\n")
         ff.write("\\end{longtable}\n")
         ff.write("\n")
         ff.write("\\section*{Functionality}\n")
         ff.write("\\begin{flushleft}\n")
         ff.write("\\end{flushleft}\n")
         ff.write("\n")
         ff.write("\\section*{Worker Implementation Details}\n")
         ff.write("\\begin{flushleft}\n")
         ff.write("\\end{flushleft}\n")
         ff.write("\n")
         ff.write("\\section*{Theory}\n")
         ff.write("\\begin{flushleft}\n")
         ff.write("\\end{flushleft}\n")
         ff.write("\n")
         ff.write("\\section*{Block Diagrams}\n")
         ff.write("\\subsection*{Top level}\n")
         ff.write("\\begin{center}\n")
         ff.write("\\end{center}\\pagebreak\n")
         ff.write("\n")
         ff.write("%\\subsection*{State Machine}\n")
         ff.write("%\\begin{flushleft}\n")
         ff.write("%\\end{flushleft}\n")
         ff.write("\n")
         ff.write("\\section*{Source Dependencies}\n")
         ff.write("%\\subsection*{\\comp.rcc}\n")
         ff.write("%\\subsection*{\\comp.hdl}\n")
         ff.write("\n")
         ff.write("\\begin{landscape}\n")
         ff.write("  \\input{component_spec_properties.tex} % it is recommended to NOT remove this line\n")
         ff.write("\n")
         ff.write("  \\input{worker_properties.tex} % it is recommended to NOT remove this line\n")
         ff.write("\n")
         ff.write("  \\input{component_ports.tex} % it is recommended to NOT remove this line\n")
         ff.write("\n")
         ff.write("  \\input{worker_interfaces.tex} % it is recommended to NOT remove this line\n")
         ff.write("\\end{landscape}\n")
         ff.write("\n")
         ff.write("\\section*{Control Timing and Signals}\n")
         ff.write("%\\subsection*{\\comp.hdl}\n")
         ff.write("\\begin{flushleft}\n")
         ff.write("\\end{flushleft}\n")
         ff.write("\n")
         ff.write("\\section*{Performance and Resource Utilization}\n")
         ff.write("%\\subsubsection*{\\comp.rcc}\n")
         ff.write("%\\subsubsection*{\\comp.hdl}\n")
         ff.write("\n")
         ff.write("\\section*{Test and Verification}\n")
         ff.write("\\begin{flushleft}\n")
         ff.write("\\end{flushleft}\n")
         ff.close()
      except Exception as err:
         if ff != None:
            ff.close()
         raise err

def usage():
    print("Usage: docGen.py <path to OCS> # run this first")
    print("       docGen.py <path to OWD> # run this second")
    print(textwrap.fill("""Generates LaTex source files suitable for compiling a Component DataSheet. Recommended to run on both OCS and OWD in the recommended order."""))

def main():
   if (len(sys.argv) < 2) or (sys.argv[1] == "--help") or (sys.argv[1] == "-help"):
      usage()
      sys.exit(1)

   [cProps, wProps, cPorts, wPorts, isOCS, isHDL] = parseOCSOrOWD(sys.argv[1])

   emitTablesTexFiles(cProps, wProps, cPorts, wPorts, isOCS, isHDL)

   if isOCS:
      emitDeveloperDocTexFileIfDoesNotExist()
      emitDataSheetTexFile(OCSorOWDFileName = sys.argv[1])
   else: # is OWD
      parseWorkerName(sys.argv[1])
      print("WARN : because the argument", sys.argv[1], "is an OWD, this script generates table .tex files. An OCS argument must be used in order to generate a compilable component datasheet.")

if __name__ == '__main__':
   main()
