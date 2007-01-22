#!/usr/bin/env python

"""This is a python script that takes user input from the command line and
installs a far (Framework Archive Manifest) file into the workspace."""

import os, sys, getopt, string, xml.dom.minidom, zipfile, md5
from XmlRoutines import *
from WorkspaceRoutines import *

verbose = False
force = False

class Database:

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
      self.installedPackages[XmlElement(spd, "/SpdHeader/GuidValue"), XmlElement(spd, "/SpdHeader/Version")] = \
        XmlElement(spd, "/SpdHeader/PackageName")

    for fpdfile in XmlList(self.dom, "/FrameworkDatabase/PlatformList/Filename"):
      filename = str(XmlElementData(fpdfile))
      fpd = XmlParseFileSection(inWorkspace(filename), "PlatformHeader")
      self.installedPlatforms[XmlElement(fpd, "/PlatformHeader/GuidValue"), XmlElement(fpd, "/PlatformHeader/Version") ] = \
        XmlElement(fpd, "/PlatformHeader/PlatformName")

    for farfile in  XmlList(self.dom, "/FrameworkDatabase/FarList/Filename"):
      farGuid = farfile.getAttribute("FarGuid")
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
    filename = self.dom.createElement("Filename")
    filename.appendChild(self.dom.createTextNode(f))
    self.packageList.appendChild(filename)
    
  def AddPlatform(self, f):
    filename = self.dom.createElement("Filename")
    filename.appendChild(self.dom.createTextNode(f))
    self.platformList.appendChild(filename)

  def AddFar(self, f, guid=""):
    filename = self.dom.createElement("Filename")
    filename.setAttribute("FarGuid", guid)
    filename.appendChild(self.dom.createTextNode(f))
    self.farList.appendChild(filename)

  def Write(self):
    if True:
      XmlSaveFile(self.dom, self.DBFile)
    else:
      f=open(self.DBFile, 'w')
      f.write(self.dom.toprettyxml(2*" "))
      f.close()

def ExtractFile(zip, file, workspaceLocation=""):

  if verbose:
    print "Extracting ", file

  destFile = os.path.join(inWorkspace(workspaceLocation), str(file))
  destDir = os.path.dirname(destFile)

  mkdir(destDir)

  f = open(destFile, "w")
  f.write(zip.read(file))
  f.close()

def GetFpdGuidVersion(Dom):

  """Get the Guid and version of the fpd from a dom object."""

  return XmlElement(Dom, "/PlatformSurfaceArea/PlatformHeader/GuidValue"), \
         XmlElement(Dom, "/PlatformSurfaceArea/PlatformHeader/Version")

def GetSpdGuidVersion(Dom):

  """Get the Guid and version of the spd from a dom object."""

  return XmlElement(Dom, "/PackageSurfaceArea/SpdHeader/GuidValue"), \
         XmlElement(Dom, "/PackageSurfaceArea/SpdHeader/Version")

def InstallFar(farfile, workspaceLocation=""):

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
    spd = XmlParseString(far.read(spdfile))
    packageGV = GetSpdGuidVersion(spd)
    if fdb.HasPackage(packageGV):
      print "Error: This package is already installed: ", spdfile
      installError = True

    # Build up a list of the package guid versions that this far is bringing in.
    # This is needed to satisfy dependencies of msas that are in the other packages of
    # this far.

    farSpds.append(packageGV)

    spdDoms.append(spd)

  for spd in spdDoms:
    # Now we need to get a list of every msa in this spd and check the package dependencies.
    for msafile in XmlList(spd, "/PackageSurfaceArea/MsaFiles/Filename"):
      msafilePath = str(os.path.join(os.path.dirname(spdfile), XmlElementData(msafile)))

      msa = XmlParseString(far.read(msafilePath))

      for package in XmlList(msa, "/ModuleSurfaceArea/PackageDependencies/Package"):
        guid = package.getAttribute("PackageGuid")
        version = package.getAttribute("PackageVersion")

        if not fdb.HasPackage((guid, version)) and not (guid, version) in farSpds:
          print "The module %s depends on the package guid % version %s, which is not installed in the workspace." \
            % (msafilePath, guid, version)
          installError = True

  # Check the platforms
  for farPlatform in XmlList(manifest, "/FrameworkArchiveManifest/FarPlatformList/FarPlatform/FarFilename"):
    fpdfile = str(XmlElementData(farPlatform))
    fpd = XmlParseString(far.read(fpdfile))
    if fdb.HasPlatform(GetFpdGuidVersion(fpd)):
      print "Error: This platform is already installed: ", fpdfile
      installError = True

  # Check the fars
  thisFarGuid = XmlElement(manifest, "/FrameworkArchiveManifest/FarHeader/GuidValue")
  if fdb.HasFar(thisFarGuid):
    print "Error: There is a far with this guid already installed."
    installError = True

  # We can not do the install
  if installError:
    if force:
      print "Ignoring previous errors as you requested."
    else:
      return False

  # Install the packages
  for farPackage in XmlList(manifest, "/FrameworkArchiveManifest/FarPackageList/FarPackage"):

    filename = XmlElement(farPackage, "FarPackage/FarFilename")
    fdb.AddPackage(filename)
    ExtractFile(far, filename, workspaceLocation)
    zipContents.remove(filename)

    for content in XmlList(farPackage, "FarPackage/Contents/FarFilename"):

      filename = XmlElementData(content)
      ExtractFile(far, filename, workspaceLocation)
      zipContents.remove(filename)

  # Install the platforms
  for farPlatform in XmlList(manifest, "/FrameworkArchiveManifest/FarPlatformList/FarPlatform"):
    
    filename = XmlElement(farPlatform, "FarPlatform/FarFilename")
    fdb.AddPlatform(filename)
    ExtractFile(far, filename, workspaceLocation)
    zipContents.remove(filename)

  # Install the Contents
  for content in XmlList(manifest, "/FrameworkArchiveManifest/Contents/FarFilename"):

    filename = XmlElementData(content)
    ExtractFile(far, filename, workspaceLocation)
    zipContents.remove(filename)

  # What if there are more files in the far?
  if not zipContents == []:
    print "There are still files in the far:", zipContents

  fdb.AddFar(farfile, thisFarGuid)

  # If everything has gone well, we can put the manifest file in a safe place...
  farDir = inWorkspace("Tools/Conf/InstalledFars/")
  mkdir(farDir)
  f=open(os.path.join(farDir, thisFarGuid), 'w')
  f.write(far.read("FrameworkArchiveManifest.xml"))
  f.close()

  # Write out the new database
  fdb.Write()
  
  far.close()

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':

  # Process the command line args.
  optlist, args = getopt.getopt(sys.argv[1:], '?hvf', ['help', 'verbose', 'force'])

  # First pass through the options list.
  for o, a in optlist:
    if o in ["-h", "--help"]:
      print """
Install a far (Framework Archive) into the current workspace.
""" % os.path.basename(sys.argv[0])

      sys.exit()
      optlist.remove((o,a))
    if o in ["-v", "--verbose"]:
      verbose = True
    if o in ["-f", "--force"]:
      force = True

  for f in args:
    InstallFar(f)
  if args == []:
    print "Please pass a far filename on the command line."
