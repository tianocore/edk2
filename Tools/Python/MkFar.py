#!/usr/bin/env python

import os, sys, re, getopt, string, glob, xml.dom.minidom, pprint, zipfile, tempfile
from XmlRoutines import *
from WorkspaceRoutines import *

def parseMsa(msaFile, spdDir):

  filelist = [msaFile]

  msaDir = os.path.dirname(msaFile)

  msa = xml.dom.minidom.parse(inWorkspace(msaFile))

  xmlPaths = [
    "/ModuleSurfaceArea/SourceFiles/Filename" ]

  for xmlPath in xmlPaths:
    for f in XmlList(msa, xmlPath):
      filelist.append(str(os.path.join(msaDir, XmlElementData(f))))

  return filelist

def parseSpd(spdFile):

  filelist = []

  spdDir = os.path.dirname(spdFile)

  spd = xml.dom.minidom.parse(inWorkspace(spdFile))

  xmlPaths = [
    "/PackageSurfaceArea/LibraryClassDeclarations/LibraryClass/IncludeHeader",
    "/PackageSurfaceArea/IndustryStdIncludes/IndustryStdHeader/IncludeHeader",
    "/PackageSurfaceArea/PackageHeaders/IncludePkgHeader" ]

  for xmlPath in xmlPaths:
    for f in XmlList(spd, xmlPath):
      filelist.append(str(os.path.join(spdDir, XmlElementData(f))))

  for f in XmlList(spd, "/PackageSurfaceArea/MsaFiles/Filename"):
    msaFile = str(os.path.join(spdDir, XmlElementData(f)))
    filelist += parseMsa(msaFile, spdDir)

  return filelist

def makeFar(filelist, farname):

  domImpl = xml.dom.minidom.getDOMImplementation()
  man = domImpl.createDocument(None, "FrameworkArchiveManifest", None)
  top_element = man.documentElement

  header = man.createElement("FarHeader")
  top_element.appendChild(header)

  packList = man.createElement("FarPackageList")
  top_element.appendChild(packList)

  platList = man.createElement("FarPlatformList")
  top_element.appendChild(platList)

  contents = man.createElement("Contents")
  top_element.appendChild(contents)

  zip = zipfile.ZipFile(farname, "w")
  for infile in filelist:
    if not os.path.exists(inWorkspace(infile)):
      print "Skipping non-existent file '%s'." % infile
    (_, extension) = os.path.splitext(infile)
    if extension == ".spd":
      filelist = parseSpd(infile)

      package = man.createElement("FarPackage")
      packList.appendChild(package)

      spdfilename = man.createElement("FarFilename")
      package.appendChild(spdfilename)

      spdfilename.appendChild( man.createTextNode(infile) )
      zip.write(inWorkspace(infile), infile)

      for spdfile in filelist:
        content = man.createElement("FarFilename")
        content.appendChild( man.createTextNode(spdfile))
        contents.appendChild(content)
        zip.write(inWorkspace(spdfile), spdfile)

    elif extension == ".fpd":

      platform = man.createElement("FarPlatform")
      platList.appendChild(platform)

      fpdfilename = man.createElement("FarFilename")
      platform.appendChild(fpdfilename)

      fpdfilename.appendChild( man.createTextNode(infile) )
      zip.write(inWorkspace(infile), infile)

    else:
      print "Skipping file '%s' since is is not a .spd or .fpd." % infile

  zip.writestr("FrameworkArchiveManifest.xml", man.toprettyxml(2*" "))
  zip.close()
  return

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':

  # Create a pretty printer for dumping data structures in a readable form.
  # pp = pprint.PrettyPrinter(indent=2)

  # Default name for far file.
  farName = "output.far"

  # Process the command line args.
  optlist, args = getopt.getopt(sys.argv[1:], 'hf:', [ 'far=', 'help'])

  for o, a in optlist:
    if o in ["-h", "--help"]:
      print """
Pass a list of .spd and .fpd files to be placed into a far for distribution.
You may give the name of the far with a -f or --far option. For example:

  %s --far library.far MdePkg/MdePkg.spd

The file paths of .spd and .fpd are relative to the WORKSPACE envirnonment
which must be set to a valid workspace root directory.
""" % os.path.basename(sys.argv[0])

      sys.exit()
    if o in ["-f", "--far"]:
      farName = a

  makeFar(args, farName)
