#!/usr/bin/env python

import os, sys, re, getopt, string, glob, xml.dom.minidom, pprint, zipfile, tempfile
from XmlRoutines import *
from WorkspaceRoutines import *

def parseMsa(msaFile, spdDir):

  filelist = []

  msaDir = os.path.dirname(msaFile)

  msa = xml.dom.minidom.parse(inWorkspace(msaFile))

  xmlPaths = [
    "/ModuleSurfaceArea/SourceFiles/Filename" ]

  for xmlPath in xmlPaths:
    for f in XmlList(msa, xmlPath):
      filelist.append(str(os.path.join(msaDir, XmlElementData(f))))

  return filelist

def parseSpd(spdFile):

  filelist = [spdFile]
  msaFileList = []

  spdDir = os.path.dirname(spdFile)

  spd = xml.dom.minidom.parse(inWorkspace(spdFile))

  xmlPaths = [
    "/PackageSurfaceArea/LibraryClassDeclarations/LibraryClass/IncludeHeader",
    "/PackageSurfaceArea/IndustryStdIncludes/IndustryStdHeader/IncludeHeader",
    "/PackageSurfaceArea/<PackageHeaders/IncludePkgHeader" ]

  for xmlPath in xmlPaths:
    for f in XmlList(spd, xmlPath):
      filelist.append(str(os.path.join(spdDir, XmlElementData(f))))

  for xmlPath in ["/PackageSurfaceArea/MsaFiles/Filename"]:
    for f in XmlList(spd, xmlPath):
      msaFile = str(os.path.join(spdDir, XmlElementData(f)))
      filelist.append(msaFile)

      filelist += parseMsa(msaFile, spdDir)

  return filelist

def makeFar(filelist, farname):

  man = \
"""<?xml version="1.0" encoding="UTF-8"?>
<FrameworkArchiveManifest>
</FrameworkArchiveManifest>
"""
  zip = zipfile.ZipFile(farname, "w")
  for file in args:
    if not os.path.exists(inWorkspace(file)):
      print "Skipping non-existent file '%s'." % file
    (_, extension) = os.path.splitext(file)
    if extension == ".spd":
      filelist = parseSpd(file)
    elif extension == ".fpd":
      filelist = [file]
    else:
      filelist = []
    for f in set(filelist):
      zip.write(inWorkspace(f), f)
  zip.writestr("FrameworkArchiveManifest.xml", man)
  zip.close()
  return

# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
if __name__ == '__main__':

  # Create a pretty printer for dumping data structures in a readable form.
  # pp = pprint.PrettyPrinter(indent=2)

  # Process the command line args.
  optlist, args = getopt.getopt(sys.argv[1:], 'h', [ 'example-long-arg=', 'testing'])

  makeFar(args, "test.far")
