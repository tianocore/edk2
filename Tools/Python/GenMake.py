#!/usr/bin/env python

"""Create Makefiles for the MdePkg."""

import os, sys, getopt, string, xml.dom.minidom, shutil
from XmlRoutines import *
from WorkspaceRoutines import *

ARCH = "X64"

Makefile = """MAKEROOT ?= ..

LIBNAME = %s

OBJECTS = %s 

include $(MAKEROOT)/lib.makefile
"""

def openMdeSpd():

  """Open the MdePkg.spd and process the msa files."""

  db = xml.dom.minidom.parse(inWorkspace("MdePkg/MdePkg.spd"))

  for msaFile in XmlList(db, "/PackageSurfaceArea/MsaFiles/Filename"):
    msaFileName = XmlElementData(msaFile)
    DoLib(msaFileName)

  return db

def inMde(f):
  """Make a path relative to the Mde Pkg root dir."""
  return inWorkspace(os.path.join("MdePkg", f))

def DoLib(msafile):

  """Create a directory with the sources, AutoGen.h and a makefile."""

  sources = []

  msa = xml.dom.minidom.parse(inMde(msafile))
  libName = str(XmlElement(msa, "/ModuleSurfaceArea/MsaHeader/ModuleName"))
  base, _ = os.path.splitext(msafile)
  msabase = os.path.basename(base)

  suppArch = str(XmlElement(msa, "/ModuleSurfaceArea/ModuleDefinitions/SupportedArchitectures"))
  if not ARCH in string.split(suppArch, " "):
    return

  try:
    os.path.isdir(libName) or os.mkdir(libName);
  except:
    print "Error: file %s exists" % libName
    sys.exit()
    

  for msaFile in XmlList(msa, "/ModuleSurfaceArea/SourceFiles/Filename"):

    msaFileName = str(XmlElementData(msaFile))
    arch = msaFile.getAttribute("SupArchList")
    toolchain = msaFile.getAttribute("ToolChainFamily")
    base, ext = os.path.splitext(msaFileName)

    if arch in ["", ARCH] and (ext in [".c", ".h"] or toolchain in ["GCC"]):
      if ext in [".c", ".S"]:
        sources.append(str(base+".o"))
      targetDir = os.path.join(libName, os.path.dirname(msaFileName))
      try:
        os.makedirs(targetDir)
      except:
        pass
      shutil.copy(inMde(os.path.join(os.path.dirname(msafile), msaFileName)), 
        targetDir)

    # Write a Makefile for this module
    f = open(os.path.join(libName, "Makefile"), "w")
    f.write(Makefile % (libName, string.join(sources, " ")))
    f.close()

    # Right now we are getting the AutoGen.h file from a previous build. We
    # could create it from scratch also.
    shutil.copy(inWorkspace("Build/Mde/DEBUG_UNIXGCC/%s/MdePkg/Library/%s/%s/DEBUG/AutoGen.h") % (ARCH, libName, msabase), libName)

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':

  openMdeSpd();
