#!/usr/bin/env python

"""Create GNU Makefiles for the Libraries of the MdePkg."""

import os, sys, getopt, string, xml.dom.minidom, shutil
from XmlRoutines import *
from WorkspaceRoutines import *

copyingSources = 1

Makefile = string.Template("""ARCH = $ARCH

MAKEROOT ?= ../..

VPATH = ..

LIBNAME = $LIBNAME

OBJECTS = $OBJECTS

include $$(MAKEROOT)/lib.makefile
""")

def mkdir(path):
  """Make a directory if it is not there already."""

  try:
    os.makedirs(path)
  except:
    pass
  
def openMdeSpd(arch):

  """Open the MdePkg.spd and process the msa files."""

  db = xml.dom.minidom.parse(inWorkspace("MdePkg/MdePkg.spd"))

  for msaFile in XmlList(db, "/PackageSurfaceArea/MsaFiles/Filename"):
    msaFileName = XmlElementData(msaFile)
    doLib(msaFileName, arch)

  return db

def inMde(f):
  """Make a path relative to the Mde Pkg root dir."""
  return inWorkspace(os.path.join("MdePkg", f))

def doLib(msafile, arch):

  """Create a directory with the sources, AutoGen.h and a makefile."""

  sources = []

  msa = xml.dom.minidom.parse(inMde(msafile))
  libName = str(XmlElement(msa, "/ModuleSurfaceArea/MsaHeader/ModuleName"))
  base, _ = os.path.splitext(msafile)
  msabase = os.path.basename(base)

  suppArch = str(XmlElement(msa, "/ModuleSurfaceArea/ModuleDefinitions/SupportedArchitectures"))
  if not arch in string.split(suppArch, " "):
    return

  mkdir(libName);

  buildDir = os.path.join(libName, "build-%s" % arch )
  mkdir(buildDir)

  for sourceFile in XmlList(msa, "/ModuleSurfaceArea/SourceFiles/Filename"):

    sourceFileName = str(XmlElementData(sourceFile))
    suppArchs = sourceFile.getAttribute("SupArchList").split(" ")
    toolchain = sourceFile.getAttribute("ToolChainFamily")
    base, ext = os.path.splitext(sourceFileName)

    if ( suppArchs == [""] or arch in suppArchs) and (ext in [".c", ".h", ".S"] or toolchain in ["GCC"]):
      if ext in [".c", ".S"]:
        sources.append(str(base+".o"))
      sourceDir = os.path.join(libName, os.path.dirname(sourceFileName))
      mkdir(sourceDir)
      mkdir(os.path.join(buildDir, os.path.dirname(sourceFileName)))
      if copyingSources :
        shutil.copy(inMde(os.path.join(os.path.dirname(msafile), sourceFileName)), 
          sourceDir)

    # Write a Makefile for this module
    f = open(os.path.join(buildDir, "Makefile"), "w")
    f.write(Makefile.substitute(ARCH=arch, LIBNAME=libName, OBJECTS=string.join(sources, " ")))
    f.close()

    # Right now we are getting the AutoGen.h file from a previous build. We
    # could create it from scratch also.
    shutil.copy(inWorkspace("Build/Mde/DEBUG_UNIXGCC/%s/MdePkg/Library/%s/%s/DEBUG/AutoGen.h") % (arch, libName, msabase), buildDir)

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':

  for arch in ["IA32", "X64"]:
    openMdeSpd(arch);
