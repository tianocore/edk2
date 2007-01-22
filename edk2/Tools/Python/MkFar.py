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

def parseMsa(msaFile, spdDir):

  """ XXX Parse an msa file and return a list of all the files that this msa
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

  xmlPaths = [
    "/PackageSurfaceArea/LibraryClassDeclarations/LibraryClass/IncludeHeader",
    "/PackageSurfaceArea/IndustryStdIncludes/IndustryStdHeader/IncludeHeader",
    "/PackageSurfaceArea/PackageHeaders/IncludePkgHeader" ]

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

  header = doc.createElement("FarHeader")
  name = doc.createElement("FarName")
  name.appendChild(doc.createTextNode(far.FarName))
  header.appendChild(name)
  guidVal = doc.createElement("GuidValue")
  guidVal.appendChild(doc.createTextNode(genguid()))
  header.appendChild(guidVal)
  ver = doc.createElement("Version")
  ver.appendChild(doc.createTextNode(far.Version))
  header.appendChild(ver)
  abstract = doc.createElement("Abstract")
  abstract.appendChild(doc.createTextNode(far.Abstract))
  header.appendChild(abstract)
  desc = doc.createElement("Description")
  desc.appendChild(doc.createTextNode(far.Description))
  header.appendChild(desc)
  copy = doc.createElement("Copyright")
  copy.appendChild(doc.createTextNode(far.Copyright))
  header.appendChild(copy)
  lic = doc.createElement("License")
  lic.appendChild(doc.createTextNode(far.License))
  header.appendChild(lic)
  spec = doc.createElement("Specification")
  spec.appendChild(doc.createTextNode("FRAMEWORK_BUILD_PACKAGING_SPECIFICATION 0x00000052"))
  header.appendChild(spec)

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

  packList = man.createElement("FarPackageList")
  top_element.appendChild(packList)

  platList = man.createElement("FarPlatformList")
  top_element.appendChild(platList)

  contents = man.createElement("Contents")
  top_element.appendChild(contents)

  exts = man.createElement("UserExtensions")
  top_element.appendChild(exts)

  zip = zipfile.ZipFile(farname, "w")
  for infile in set(files):
    if not os.path.exists(inWorkspace(infile)):
      print "Error: Non-existent file '%s'." % infile
      sys.exit()
    (_, extension) = os.path.splitext(infile)
    if extension == ".spd":
      filelist = parseSpd(infile)
      spdDir = os.path.dirname(infile)

      (spdGuid, spdVersion) = getSpdGuidVersion(infile)

      package = man.createElement("FarPackage")
      packList.appendChild(package)

      spdfilename = farFileNode(man, inWorkspace(infile))
      zip.write(inWorkspace(infile), infile)
      spdfilename.appendChild(man.createTextNode(lean(infile)))
      package.appendChild(spdfilename)

      guidValue = man.createElement("GuidValue")
      guidValue.appendChild(man.createTextNode(spdGuid))
      package.appendChild(guidValue)

      version = man.createElement("Version")
      version.appendChild(man.createTextNode(spdVersion))
      package.appendChild(version)

      defaultPath = man.createElement("DefaultPath")
      defaultPath.appendChild(man.createTextNode(spdDir))
      package.appendChild(defaultPath)

      farPlatformList = man.createElement("FarPlatformList")
      package.appendChild(farPlatformList)

      packContents = man.createElement("Contents")
      package.appendChild(packContents)

      ue = man.createElement("UserExtensions")
      package.appendChild(ue)

      for spdfile in filelist:
        content = farFileNode(man, inWorkspace(os.path.join(spdDir, spdfile))) 
        zip.write(inWorkspace(os.path.join(spdDir, spdfile)), os.path.join(spdDir,spdfile))
        content.appendChild(man.createTextNode(lean(spdfile)))
        packContents.appendChild(content)

    elif extension == ".fpd":

      platform = man.createElement("FarPlatform")
      platList.appendChild(platform)

      fpdfilename = farFileNode(man, inWorkspace(infile))
      zip.write(inWorkspace(infile), infile)
      platform.appendChild(fpdfilename)
      fpdfilename.appendChild(man.createTextNode(lean(infile)))

    else:
      content = farFileNode(man, inWorkspace(infile))
      zip.write(inWorkspace(infile), infile)
      content.appendChild(man.createTextNode(lean(infile)))
      contents.appendChild(content)

  zip.writestr("FrameworkArchiveManifest.xml", man.toprettyxml(2*" "))
  zip.close()
  return

def farFileNode(doc, filename):

  """This is a function that returns a dom tree for a given file that is
  included in the far. An md5sum is calculated for that file."""

  content = doc.createElement("FarFilename")
  try:
    f=open(filename, "rb")
    content.setAttribute("Md5sum", md5.md5(f.read()).hexdigest())
    f.close()
  except IOError:
    print "Error: Unable to open file: %s" % filename
    sys.exit()

  return content

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':

  # Create a pretty printer for dumping data structures in a readable form.
  # pp = pprint.PrettyPrinter(indent=2)

  # Process the command line args.
  optlist, args = getopt.getopt(sys.argv[1:], 'hf:t:', [ 'template=', 'far=', 'help'])

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
    print o, a
    if o in ["-f", "--far"]:
      far.FileName = a

  # Let's err on the side of caution and not let people blow away data 
  # accidentally.
  if os.path.exists(far.FileName):
    print "Error: File %s exists. Not overwriting." % far.FileName
    sys.exit()

  makeFar(far.SpdFiles + far.FpdFiles + far.ExtraFiles + args, far.FileName)
