#!/usr/bin/env python

"""This is a python script that takes user input from the command line and
creates a far (Framework Archive Manifest) file for distribution."""

import os, sys, getopt, string, xml.dom.minidom, zipfile, md5
from XmlRoutines import *
from WorkspaceRoutines import *

class Far:
  """This class is used to collect arbitrarty data from the template file."""
  def __init__(far):
    """Assign the default values for the far fields."""
    far.FileName = "output.far"
    far.FarName=""
    far.Version=""
    far.License=""
    far.Abstract=""
    far.Description=""
    far.Copyright=""
    far.SpdFiles=[]
    far.FpdFiles=[]
    far.ExtraFiles=[]

far = Far()
"""The far object is constructed from the template file the user passed in."""

def AddToZip(zip, infile):

  """Add a file to a zip file, provided it is not already there."""

  if not infile in zip.namelist():
    zip.write(inWorkspace(infile), infile)

def parseMsa(msaFile, spdDir):

  """Parse an msa file and return a list of all the files that this msa
  includes."""

  filelist = [msaFile]

  msaDir = os.path.dirname(msaFile)

  msa = xml.dom.minidom.parse(inWorkspace(os.path.join(spdDir, msaFile)))

  xmlPaths = [
    "/ModuleSurfaceArea/SourceFiles/Filename",
    "/ModuleSurfaceArea/NonProcessedFiles/Filename" ]

  for xmlPath in xmlPaths:
    for f in XmlList(msa, xmlPath):
      filelist.append(str(os.path.join(msaDir, XmlElementData(f))))

  return filelist

def parseSpd(spdFile):

  """Parse an spd file and return a list of all the files that this spd
  includes."""

  files = []

  spdDir = os.path.dirname(spdFile)

  spd = xml.dom.minidom.parse(inWorkspace(spdFile))

  # We are currently ignoring these hints.
  readonly = XmlElement(spd, "/PackageSurfaceArea/PackageDefinitions/ReadOnly") != "false"
  repackage = XmlElement(spd, "/PackageSurfaceArea/PackageDefinitions/RePackage") != "false"

  xmlPaths = [
    "/PackageSurfaceArea/LibraryClassDeclarations/LibraryClass/IncludeHeader",
    "/PackageSurfaceArea/IndustryStdIncludes/IndustryStdHeader/IncludeHeader" ]

    # These are covered by the Industry Standard Includes.
    # "/PackageSurfaceArea/PackageHeaders/IncludePkgHeader"

  for xmlPath in xmlPaths:
    for f in XmlList(spd, xmlPath):
      files.append(str(XmlElementData(f)))

  for f in XmlList(spd, "/PackageSurfaceArea/MsaFiles/Filename"):
    msaFile = str(XmlElementData(f))
    files += parseMsa(msaFile, spdDir)

  cwd = os.getcwd()
  os.chdir(inWorkspace(spdDir))
  for root, dirs, entries in os.walk("Include"):
    # Some files need to be skipped.
    for r in ["CVS", ".svn"]:
      if r in dirs:
        dirs.remove(r)
    for entry in entries:
      files.append(os.path.join(os.path.normpath(root), entry))
  os.chdir(cwd)

  return files

def makeFarHeader(doc):

  """Create a dom tree for the Far Header. It will use information from the
  template file passed on the command line, if present."""
 
  header = XmlAppendChildElement(doc.documentElement, "FarHeader")

  XmlAppendChildElement(header, "FarName", far.FarName)
  XmlAppendChildElement(header, "GuidValue", genguid())
  XmlAppendChildElement(header, "Version", far.Version)
  XmlAppendChildElement(header, "Abstract", far.Abstract)
  XmlAppendChildElement(header, "Description", far.Description)
  XmlAppendChildElement(header, "Copyright", far.Copyright)
  XmlAppendChildElement(header, "License", far.License)
  XmlAppendChildElement(header, "Specification", "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION 0x00000052")

  return header

def getSpdGuidVersion(spdFile):

  """Returns a tuple (guid, version) which is read from the given spdFile."""

  spd = xml.dom.minidom.parse(inWorkspace(spdFile))

  return (XmlElement(spd, "/PackageSurfaceArea/SpdHeader/GuidValue"),
          XmlElement(spd, "/PackageSurfaceArea/SpdHeader/Version"))

def makeFar(files, farname):

  """Make a far out of the given filelist and writes it to the file farname."""

  domImpl = xml.dom.minidom.getDOMImplementation()
  man = domImpl.createDocument(None, "FrameworkArchiveManifest", None)
  top_element = man.documentElement

  top_element.appendChild(makeFarHeader(man))

  packList = XmlAppendChildElement(top_element, "FarPackageList")
  platList = XmlAppendChildElement(top_element, "FarPlatformList")
  contents = XmlAppendChildElement(top_element, "Contents")
  XmlAppendChildElement(top_element, "UserExtensions")

  try:
    zip = zipfile.ZipFile(farname, "w", zipfile.ZIP_DEFLATED)
  except:
    zip = zipfile.ZipFile(farname, "w", zipfile.ZIP_STORED)
  for infile in set(files):
    if not os.path.exists(inWorkspace(infile)):
      print "Error: Non-existent file '%s'." % infile
      sys.exit()
    (_, extension) = os.path.splitext(infile)
    if extension == ".spd":
      filelist = parseSpd(infile)
      spdDir = os.path.dirname(infile)

      (spdGuid, spdVersion) = getSpdGuidVersion(infile)

      package = XmlAppendChildElement(packList, "FarPackage")
      XmlAppendChildElement(package, "FarFilename", lean(infile), {"Md5Sum": Md5(inWorkspace(infile))})
      AddToZip(zip, infile)
      XmlAppendChildElement(package, "GuidValue", spdGuid)
      XmlAppendChildElement(package, "Version", spdVersion)
      XmlAppendChildElement(package, "DefaultPath", spdDir)
      XmlAppendChildElement(package, "FarPlatformList")
      packContents = XmlAppendChildElement(package, "Contents")
      XmlAppendChildElement(package, "UserExtensions")

      for spdfile in filelist:
        XmlAppendChildElement(packContents, "FarFilename", lean(spdfile), {"Md5Sum": Md5(inWorkspace(os.path.join(spdDir, spdfile)))})
        AddToZip(zip, os.path.join(spdDir,spdfile))

    elif extension == ".fpd":

      platform = XmlAppendChildElement(platList, "FarPlatform")
      XmlAppendChildElement(platform, "FarFilename", lean(infile), {"Md5Sum": Md5(inWorkspace(infile))})
      AddToZip(zip, infile)

    else:
      XmlAppendChildElement(contents, "FarFilename", lean(infile), {"Md5Sum": Md5(inWorkspace(infile))})
      AddToZip(zip, infile)

  zip.writestr("FrameworkArchiveManifest.xml", man.toxml('UTF-8'))
  zip.close()
  return

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':

  # Create a pretty printer for dumping data structures in a readable form.
  # pp = pprint.PrettyPrinter(indent=2)

  # Process the command line args.
  optlist, args = getopt.getopt(sys.argv[1:], 'ho:t:v', [ 'template=', 'output=', 'far=', 'help', 'debug', 'verbose', 'version'])

  # First pass through the options list.
  for o, a in optlist:
    if o in ["-h", "--help"]:
      print """
Pass a list of .spd and .fpd files to be placed into a far for distribution.
You may give the name of the far with a -f or --far option. For example:

  %s --template far-template --far library.far MdePkg/MdePkg.spd

The file paths of .spd and .fpd are treated as relative to the WORKSPACE
environment variable which must be set to a valid workspace root directory.

A template file may be passed in with the --template option. This template file
is a text file that allows more contol over the contents of the far.
""" % os.path.basename(sys.argv[0])

      sys.exit()
      optlist.remove((o,a))
    if o in ["-t", "--template"]:
      # The template file is processed first, so that command line options can
      # override it.
      templateName = a
      execfile(templateName)
      optlist.remove((o,a))

  # Second pass through the options list. These can override the first pass.
  for o, a in optlist:
    if o in ["-o", "--far", "--output"]:
      far.FileName = a

  # Let's err on the side of caution and not let people blow away data 
  # accidentally.
  if os.path.exists(far.FileName):
    print "Error: File %s exists. Not overwriting." % far.FileName
    sys.exit()

  makeFar(far.SpdFiles + far.FpdFiles + far.ExtraFiles + args, far.FileName)
