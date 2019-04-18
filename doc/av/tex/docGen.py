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

from __future__ import print_function
import sys
import re
import textwrap
#import xml.etree.ElementTree as ET
from lxml import etree
import os

lcName = ''
ucName = ''
properties = []
ports = []

def parseComponentSpec (root):
   global properties
   global port
   for port in root.iter("DataInterfaceSpec"):
      ports.append(port)
   for port in root.iter("datainterfacespec"):
      ports.append(port)
   for port in root.iter("Port"):
      ports.append(port)
   for port in root.iter("port"):
      ports.append(port)
   for prop in root.iter("Property"):
      properties.append(prop)
   for prop in root.iter("property"):
      properties.append(prop)

def raiseExceptionMultipleEmbeddedComponentSpec(elem):
   raise Exception("invalid XML: multiple embedded " + elem + " elements found")

def parseEmbeddedComponentSpec (root):
   numComps = 0
   for elem in ["ComponentSpec", "componentspec"]:
      for comp in root.iter(elem):
         print("DEBUG: found embedded component spec")
         numComps += 1
         if(numComps > 1):
            raiseExceptionMultipleEmbeddedComponentSpec(elem)
         parseComponentSpec(comp)

def parseName (myName):
   global ucName
   global lcName
   parsedList = myName.split("/")
   parsedName = parsedList[-1]
   parsedName = parsedName.split("spec.xml")
   name = parsedName[0]
   lcName = name[:-1]
   lcName = lcName.replace('_','\_')
   parsedList = re.split('_|-', lcName)
   ucName = ''
   for i in parsedList:
     ucName += i + ' '
   ucName = ucName.title()
def genAccess (prop):
   volStr = prop.get("Volatile")
   prop.get("Name")
   if (type(volStr) is not str):
      volStr = prop.get("volatile")
   readableStr = prop.get("Readable")
   if (type(readableStr) is not str):
      readableStr = prop.get("readable")
   initStr = prop.get("Initial")
   if (type(initStr) is not str):
      initStr = prop.get("initial")
   writeableStr = prop.get("Writable")
   if (type(writeableStr) is not str):
      writeableStr = prop.get("writable")

   if type(volStr) is str:
      readStr = "Volatile"
   elif type(readableStr) is str:
      readStr = "Readable"
   else:
      readStr = ""
   if type(initStr) is str:
      writeStr = "Initial"
   elif type(writeableStr) is str:
      writeStr = "Writeable"
   else:
      writeStr = ""
   if (readStr is not "") and (writeStr is not ""):
     commaStr = ", "
   else:
     commaStr = ""
   return readStr + commaStr + writeStr

def latexify(string):
   return string.replace('_','\_').replace('&','\&')

def getDescriptionLatexString(prop):
   descrStr = prop.get("Description")
   if (type(descrStr) is not str):
      descrStr = prop.get("description")
   if (descrStr is None):
      descrStr = "-"
   return latexify(descrStr)

def printSpecTable (outFile):
   outFile.write("\\begin{longtable}{|p{4cm}|p{1.5cm}|c|c|c|p{1.5cm}|p{1cm}|p{5cm}|}\n")
   outFile.write("\\hline\n")
   outFile.write("\\rowcolor{blue}\n")
   outFile.write("Name                 & Type   & SequenceLength & ArrayDimensions & Accessibility       & Valid Range & Default & Description\n")
   outFile.write("\\\\\n")
   for prop in properties:
      nameStr = prop.get("Name")
      if (type(nameStr) is str):
         nameStr = latexify(nameStr)
      else:
         nameStr = prop.get("name")
         if (type(nameStr) is str):
            nameStr = latexify(nameStr)
      typeStr = prop.get("Type")
      if (type(typeStr) is not str):
         typeStr = prop.get("type")
      defaultStr = prop.get("Default")
      if (type(defaultStr) is not str):
         defaultStr = prop.get("default")
      if (type(defaultStr) is not str):
         defaultStr = "-"
      descrStr = getDescriptionLatexString(prop)
      accsessStr = genAccess(prop)
      outFile.write("\\hline\n")
      outFile.write(str(nameStr) + " & " + typeStr + "  & - & - & " + accsessStr + " & -  &"+ defaultStr + " & " + descrStr + "\n")
      outFile.write("\\\\\n")
   outFile.write("\\hline\n")
   outFile.write("\\end{longtable}\n")

def deleteFile(fileName):
   print("INFO : deleting file", fileName)
   os.remove(fileName)

def printWorkerTable(outFile):
   raise Exception("no worker file parsing options yet")

def printPortTable (outFile):
   outFile.write("\\begin{tabular}{|M{2cm}|M{1.5cm}|M{4cm}|c|c|M{9cm}|}\n")
   outFile.write("\\hline\n")
   outFile.write("\\rowcolor{blue}\n")
   outFile.write("Name & Producer & Protocol & Optional & Advanced & Usage\n")
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
      outFile.write(nameStr + " & " + prodStr + " & " + str(protStr) + "& " + optStr + " & - & -")
      outFile.write("\\\\\n")
   outFile.write("\\hline\n")
   outFile.write("\\end{tabular}\n")

def printIntTable (outFile):
   outFile.write("\\begin{tabular}{|M{2cm}|M{1.5cm}|M{4cm}|c|M{12cm}|}\n")
   outFile.write("\\hline\n")
   outFile.write("\\rowcolor{blue}\n")
   outFile.write("Type & Name & DataWidth & Advanced & Usage\n")
   outFile.write("\\\\\n")
   for port in ports:
      nameStr = port.get("Name")
      if (type(nameStr) is str):
         nameStr.replace('_','\_')
      else:
         nameStr = port.get("name").replace('_','\_')
      outFile.write("\\hline\n")
      outFile.write("StreamInterface & " + nameStr + " & - & - & -")
      outFile.write("\\\\\n")
   outFile.write("\\hline\n")
   outFile.write("\\end{tabular}\n")

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

def usage():
    print("Usage: docGen.py SPECFILE")
    print(textwrap.fill("""Generate an OpenCPI Worker data sheet called 'newFile.tex' based on a skeleton LaTeX file (snippets/Component_Template.tex) and the Worker specification file that is passed into the script"""))

def main():
   outFile = None
   inFile = None
   try:
      if (len(sys.argv) < 2) or (sys.argv[1] == "--help") or (sys.argv[1] == "-help"):
         usage()
         sys.exit(1)

      specFileName =  sys.argv[1]

      outFileName = "newFile.tex"
      if shouldExitBecauseFileOverwriteUnallowed(outFileName):
         print("INFO : exiting WITHOUT generating documentation")
         exit(0)
      outFile = open(outFileName, 'wr')

      skeletonFileName = os.path.dirname(os.path.realpath(__file__))
      skeletonFileName += "/snippets/Component_Template.tex"
      inFile = open(skeletonFileName, 'r')

      print("INFO : parsing", specFileName)
      tree = etree.parse(specFileName)
      #print(tree)
      tree.xinclude()
      currentLine = 'a thing'
      parseName(specFileName)
      parseComponentSpec(tree.getroot())
      parseEmbeddedComponentSpec(tree.getroot())
      while currentLine != '':
         currentLine = inFile.readline()
         if currentLine == '%GEN_COMPLC_NAME\n':
            outFile.write ("\def\comp{"+ lcName +"}\n")
            outFile.write (currentLine)
         elif currentLine == '%GEN_COMPUC_NAME\n':
            outFile.write ("\def\Comp{"+ ucName +"}\n")
            outFile.write (currentLine)
         elif currentLine == '%GEN_SPEC_TABLE\n':
            if (properties != []):
                printSpecTable(outFile)
            outFile.write (currentLine)
         elif currentLine == '%GEN_WORKER_TABLE\n':
            #if (properties != []):
            #   printWorkerTable(outFile)
            outFile.write (currentLine)
         elif currentLine == '%GEN_PORT_TABLE\n':
            if (ports != []):
               printPortTable(outFile)
            outFile.write (currentLine)
         elif currentLine == '%GEN_INTERFACE_TABLE\n':
            if (ports != []):
               printIntTable(outFile)
            outFile.write (currentLine)
         else:
            outFile.write (currentLine)
      msg = "INFO : documentation has been generated in Latex source file:"
      print(msg, outFile.name)
   
      inFile.close()
      outFile.close()
   except Exception as err:
      print("ERROR: caught exception:", err)
      if inFile != None:
         inFile.close()
      if outFile != None:
         outFile.close()
         deleteFile(outFile.name)
      exit(1)

if __name__ == '__main__':
   main()
