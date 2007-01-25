#!/usr/bin/env python

"""This is a python script that takes user input from the command line and
installs a far (Framework Archive Manifest) file into the workspace."""

import os, sys, getopt, string, xml.dom.minidom, zipfile, md5
from XmlRoutines import *
from WorkspaceRoutines import *

class Flags:
  """Keep track of some command line flags and operating modes."""
  def __init__(self):
    self.verbose = False
    self.force = False
    self.reinstall = False
    self.dir = ''

class Database:

  """This class encapsulates the FrameworkDatabase file for the workspace we
  are operating on."""

  def __init__(self, filename="Tools/Conf/FrameworkDatabase.db"): 

    # First try to get a lock file.
    self.DBFile = inWorkspace(filename)
    self.lockfile = inWorkspace("Tools/Conf/FrameworkDatabase.lock")
    if os.path.exists(self.lockfile):
      self.itsMyLockFile = False
      print "Error: The database file is locked by ", self.lockfile
      raise OSError("The Database is locked.")
    else:
      self.lock = open(self.lockfile, 'w')
      self.lock.write("pid "+str(os.getpid()))
      self.itsMyLockFile = True

    self.dom = XmlParseFile(inWorkspace(filename))

    self.installedPackages = {}
    self.installedPlatforms = {}
    self.installedFars = {}

    for spdfile in XmlList(self.dom, "/FrameworkDatabase/PackageList/Filename"):
      filename = str(XmlElementData(spdfile))
      spd = XmlParseFileSection(inWorkspace(filename), "SpdHeader")
      self.installedPackages[GetSpdGuidVersion(spd, 1)] = \
        XmlElement(spd, "/SpdHeader/PackageName")

    for fpdfile in XmlList(self.dom, "/FrameworkDatabase/PlatformList/Filename"):
      filename = str(XmlElementData(fpdfile))
      fpd = XmlParseFileSection(inWorkspace(filename), "PlatformHeader")
      self.installedPlatforms[GetFpdGuidVersion(fpd, 1)] = \
        XmlElement(fpd, "/PlatformHeader/PlatformName")

    for farfile in  XmlList(self.dom, "/FrameworkDatabase/FarList/Filename"):
      farGuid = Guid(farfile.getAttribute("FarGuid"))
      self.installedFars[farGuid] = XmlElementData(farfile)

    self.packageList = XmlNode(self.dom, "/FrameworkDatabase/PackageList")
    self.platformList = XmlNode(self.dom, "/FrameworkDatabase/PlatformList")
    self.farList = XmlNode(self.dom, "/FrameworkDatabase/FarList")

  def __del__(self):
    if self.itsMyLockFile:
      self.lock.close()
      os.unlink(self.lockfile)

  def HasPackage(self, (guid, version)):
    """Return true iff this package is already installed."""
    if version == "":
      # Look for the guid.
      for (g, v) in self.installedPackages.keys():
        if g == guid:
          return True
    return self.installedPackages.has_key((guid, version))

  def HasPlatform(self, (guid, version)):
    """Return true iff this platform is already installed."""
    if version == "":
      # Look for the guid.
      for (g, v) in self.installedPlatforms.keys():
        if g == guid:
          return True
    return self.installedPlatforms.has_key((guid, version))

  def HasFar(self, farguid):
    """Return true iff this far is already installed."""
    return self.installedFars.has_key(farguid)

  def AddPackage(self, f):
    """Put this package in the database"""
    XmlAppendChildElement(self.packageList, "Filename", f)
    
  def AddPlatform(self, f):
    """Put this platform in the database"""
    XmlAppendChildElement(self.platformList, "Filename", f)

  def AddFar(self, f, guid=""):
    """Put this far in the database"""
    XmlAppendChildElement(self.farList, "Filename", f, {"FarGuid":guid} )

  def Write(self):
    """Save the Xml tree out to the file."""
    if True:
      XmlSaveFile(self.dom, self.DBFile)
    else:
      f=open(self.DBFile, 'w')
      f.write(self.dom.toprettyxml(2*" "))
      f.close()

def ExtractFile(zip, file, defaultDir="", workspaceLocation="", md5sum=""):
  """Unzip a file."""
  if flags.verbose:
    print "Extracting ", file

  destFile = os.path.join(inWorkspace(workspaceLocation), str(file))
  destDir = os.path.dirname(destFile)

  mkdir(destDir)

  f = open(destFile, "w")
  contents = zip.read(os.path.join(defaultDir,file))
  if md5sum and (md5.md5(contents).hexdigest() != md5sum):
    print "Error: The md5 sum does not match on file %s." % file
  f.write(contents)
  f.close()

def GetFpdGuidVersion(Dom, strip=0):

  """Get the Guid and version of the fpd from a dom object."""

  gpath = ["PlatformSurfaceArea", "PlatformHeader", "GuidValue"]
  vpath = ["PlatformSurfaceArea", "PlatformHeader", "Version"]

  return Guid(XmlElement(Dom, "/".join(gpath[strip:]))), \
              XmlElement(Dom, "/".join(vpath[strip:]))

def GetSpdGuidVersion(Dom, strip=0):

  """Get the Guid and version of the spd from a dom object."""

  gpath = ["PackageSurfaceArea", "SpdHeader", "GuidValue"]
  vpath = ["PackageSurfaceArea", "SpdHeader", "Version"]

  return Guid(XmlElement(Dom, "/".join(gpath[strip:]))), \
              XmlElement(Dom, "/".join(vpath[strip:]))

def InstallFar(farfile, workspaceLocation=""):

  """Unpack the far an install it in the workspace. We need to adhere to the
  rules of far handling."""

  far = zipfile.ZipFile(farfile, "r")

  # Use this list to make sure we get everything from the far.
  zipContents = far.namelist()

  manifest = xml.dom.minidom.parseString(far.read("FrameworkArchiveManifest.xml"))
  zipContents.remove("FrameworkArchiveManifest.xml")
  fdb = Database()

  # First we need to make sure that the far will install cleanly.

  installError = False # Let's hope for the best.
  spdDoms = []
  farSpds = []

  # Check the packages
  for farPackage in XmlList(manifest, "/FrameworkArchiveManifest/FarPackageList/FarPackage/FarFilename"):
    spdfile = str(XmlElementData(farPackage))
    spd = XmlParseStringSection(far.read(spdfile), "SpdHeader")
    packageGV = GetSpdGuidVersion(spd, 1)
    if fdb.HasPackage(packageGV):
      if not flags.reinstall:
        print "Error: This package is already installed: ", spdfile
        installError = True

    # Build up a list of the package guid versions that this far is bringing in.
    # This is needed to satisfy dependencies of msas that are in the other packages of
    # this far.
    farSpds.append(packageGV)

    spdDoms.append((spd, spdfile))

  for spd, spdfile in spdDoms:
    # Now we need to get a list of every msa in this spd and check the package dependencies.
    for msafile in XmlList(spd, "/PackageSurfaceArea/MsaFiles/Filename"):
      msafilePath = str(os.path.join(os.path.dirname(spdfile), XmlElementData(msafile)))

      msa = XmlParseString(far.read(msafilePath))

      for package in XmlList(msa, "/ModuleSurfaceArea/PackageDependencies/Package"):
        guid = Guid(package.getAttribute("PackageGuid"))
        version = package.getAttribute("PackageVersion")

        # Does anyone provide this package?
        if not fdb.HasPackage((guid, version)) and not (guid, version) in farSpds:
          print ("Error: The module %s depends on the package guid %s version %s, which " + \
                "is not installed in the workspace, nor is it provided by this far.") \
                % (msafilePath, guid, version)
          installError = True

  # Check the platforms
  for farPlatform in XmlList(manifest, "/FrameworkArchiveManifest/FarPlatformList/FarPlatform/FarFilename"):
    fpdfile = str(XmlElementData(farPlatform))
    fpd = XmlParseString(far.read(fpdfile))
    if fdb.HasPlatform(GetFpdGuidVersion(fpd, 0)):
      if not flags.reinstall:
        print "Error: This platform is already installed: ", fpdfile
        installError = True

    # Now we need to check that all the Platforms (and modules?) that are
    # referenced by this fpd are installed in the workspace or are in this far.
    packagesNeeded = set()

    # Go through the dependencies
    for dependency in XmlList(fpd, "/PlatformSurfaceArea/FrameworkModules/ModuleSA") + \
                      XmlList(fpd, "/PlatformSurfaceArea/FrameworkModules/ModuleSA/Libraries/Instance"):
      packagesNeeded.add((Guid(dependency.getAttribute("PackageGuid")), 
                               dependency.getAttribute("PackageVersion")))

    # Let's see if all the packages are in the workspace 
    for guid, version in packagesNeeded:
      # Does anyone provide this package?
      if not fdb.HasPackage((guid, version)) and not (guid, version) in farSpds:
        print ("Error: The fpd %s depends on the package guid %s version %s, which " + \
              "is not installed in the workspace, nor is it provided by this far.") \
              % (fpdfile, guid, version)
        installError = True

  # Check the fars
  thisFarGuid = Guid(XmlElement(manifest, "/FrameworkArchiveManifest/FarHeader/GuidValue"))
  if fdb.HasFar(thisFarGuid):
    if not flags.reinstall:
      print "Error: There is a far with this guid already installed."
      installError = True

  # We can not do the install
  if installError:
    if flags.force:
      print "Warning: Ignoring previous errors as you requested."
    else:
      return False

  # Install the packages
  for farPackage in XmlList(manifest, "/FrameworkArchiveManifest/FarPackageList/FarPackage"):

    filename = XmlElement(farPackage, "FarPackage/FarFilename")
    if not flags.reinstall:
      fdb.AddPackage(filename)
    ExtractFile(far, filename, workspaceLocation)
    zipContents.remove(filename)

    DefaultPath = XmlElement(farPackage, "FarPackage/DefaultPath") 

    for content in XmlList(farPackage, "FarPackage/Contents/FarFilename"):

      filename = XmlElementData(content)
      ExtractFile(far, filename, DefaultPath, workspaceLocation, md5sum=content.getAttribute("Md5Sum"))
      zipContents.remove(os.path.join(DefaultPath, filename))

  # Install the platforms
  for farPlatform in XmlList(manifest, "/FrameworkArchiveManifest/FarPlatformList/FarPlatform"):
    
    filename = XmlElement(farPlatform, "FarPlatform/FarFilename")
    if not flags.reinstall:
      fdb.AddPlatform(filename)
    ExtractFile(far, filename, "", workspaceLocation)
    zipContents.remove(filename)

  # Install the Contents
  for content in XmlList(manifest, "/FrameworkArchiveManifest/Contents/FarFilename"):

    filename = XmlElementData(content)
    ExtractFile(far, filename, "", workspaceLocation)
    zipContents.remove(filename)

  # What if there are more files in the far?
  if not zipContents == []:
    print "Warning: There are files in the far that were not expected: ", zipContents

  if not flags.reinstall:
    fdb.AddFar(farfile, thisFarGuid)

  # If everything has gone well, we can put the manifest file in a safe place...
  farDir = inWorkspace("Tools/Conf/InstalledFars/")
  mkdir(farDir)
  f=open(os.path.join(farDir, thisFarGuid), 'w')
  f.write(far.read("FrameworkArchiveManifest.xml"))
  f.close()

  # Write out the new database
  if not flags.reinstall:
    fdb.Write()
  
  far.close()

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':

  flags = Flags()

  # Process the command line args.
  optlist, args = getopt.getopt(sys.argv[1:], '?hvfd:', ['directory=', 'help', 'verbose', 'force', 'reinstall'])

  # First pass through the options list.
  for o, a in optlist:
    if o in ["-h", "-?", "--help"]:
      print """
%s: Install a far (Framework Archive) into the current workspace.
""" % os.path.basename(sys.argv[0])

      sys.exit()
      optlist.remove((o,a))
    if o in ["-v", "--verbose"]:
      flags.verbose = True
    if o in ["-d", "--directory"]:
      flags.dir = a
    if o in ["-f", "--force"]:
      flags.force = True
    if o in ["--reinstall"]:
      flags.reinstall = True

  for f in args:
    InstallFar(f)
  if args == []:
    print "Please pass a far filename on the command line."
