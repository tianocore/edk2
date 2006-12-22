#!/usr/bin/env python

"""List the contents of the Framework Database to the screen in a readble
form."""

import os, sys, getopt, string, xml.dom.minidom, zipfile, md5
from XmlRoutines import *
from WorkspaceRoutines import *

def openDatabase(f):

  print "Dumping the contents of %s workspace database file." % f

  db = xml.dom.minidom.parse(inWorkspace(f))

  return db

def showSpds(db):

  print "--------\nPackages\n--------"

  for spdFile in XmlList(db, "/FrameworkDatabase/PackageList/Filename"):
    spdFileName = XmlElementData(spdFile)
    spd = xml.dom.minidom.parse(inWorkspace(spdFileName))
    spdName = XmlElement(spd, "/PackageSurfaceArea/SpdHeader/PackageName")

    print "  %-24s %-10s" % (spdName, spdFileName)

def showFpds(db):

  print "--------\nPlatforms\n--------"

  for fpdFile in XmlList(db, "/FrameworkDatabase/PlatformList/Filename"):
    fpdFileName = XmlElementData(fpdFile)
    fpd = xml.dom.minidom.parse(inWorkspace(fpdFileName))
    fpdName = XmlElement(fpd, "/PlatformSurfaceArea/PlatformHeader/PlatformName")

    print "  %-24s %-10s" % (fpdName, fpdFileName)

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':

  db = openDatabase("Tools/Conf/FrameworkDatabase.db")

  showSpds(db)
  showFpds(db)
