#!/usr/bin/env python

# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
# 
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""Calculate the dependencies a given module has by looking through the source
code to see what guids and functions are referenced to see which Packages and
Library Classes need to be referenced. """

import os, sys, re, getopt, string, glob, xml.dom.minidom, pprint
from XmlRoutines import *

# Map each function name back to the lib class that declares it.
function_table = {}

# Map each guid name to a package name.
cname_table = {}

def inWorkspace(rel_path):
  """Treat the given path as relative to the workspace."""

  # Make sure the user has set the workspace variable:
  try:
    return os.path.join(os.environ["WORKSPACE"], rel_path )
  except:
    print "Oops! You must set the WORKSPACE environment variable to run this script."
    sys.exit()

def getIdentifiers(infiles):

  """Build a set of all the identifiers in this file."""

  # Start with an empty set.
  ids = set()

  for infile in infiles:

    # Open the file
    f = open(infile)

    # Create some lexical categories that we will use to filter out
    strings=re.compile('L?"[^"]*"')
    chars=re.compile("'[^']*'")
    hex=re.compile("0[Xx][0-9a-fA-F]*")
    keywords = re.compile('for|do|while|if|else|break|int|unsigned|switch|volatile|goto|case|char|long|struct|return|extern')
    common = re.compile('VOID|UINTN|UINT32|UINT8|UINT64')

    # Compile a Regular expression to grab all the identifers from the input.
    identifier = re.compile('[_a-zA-Z][0-9_a-zA-Z]{3,}')

    for line in f.readlines():

      # Filter some lexical categories out.
      # for filter in [strings, chars, hex, keywords, common]:
      for filter in [strings, chars, hex]:
        line = re.sub(filter, '', line)
      
      # Add all the identifiers that we found on this line.
      ids = ids.union(set(identifier.findall(line)))

    # Close the file
    f.close()    

  # Return the set of identifiers.
  return ids


def search_classes(ids):

  """ Search the set of classes for functions."""

  # Start with an empty set.
  classes = set()

  for id in ids:
    try:
      # If it is not a "hit" in the table add it to the set.
      classes.add(function_table[id])
    except:
      # If it is not a "hit" in the table, ignore it.
      pass

  return classes

def search_cnames(ids):

  """Search all the Packages to see if this code uses a Guid from one of them.
  Return a set of matching packages."""

  packages = set()

  for id in ids:
    try:
      # If it is not a "hit" in the table add it to the set.
      packages.add(cname_table[id])
    except:
      # If it is not a "hit" in the table, ignore it.
      pass

  return packages

def getSpds():

  """Open the database and get all the spd files out."""

  # Open the database
  database = xml.dom.minidom.parse(inWorkspace("Tools/Conf/FrameworkDatabase.db"))

  # Get a list of all the packages
  for filename in XmlList(database, "/FrameworkDatabase/PackageList/Filename"):
    spdFile = XmlElementData(filename)

    # Now open the spd file and build the database of guids.
    getCNames(inWorkspace(spdFile))
    getLibClasses(inWorkspace(spdFile))

def getCNames(spdFile):

  """Extract all the C_Names from an spd file."""

  # Begin to parse the XML of the .spd
  spd = xml.dom.minidom.parse(spdFile)

  # Get the name of the package
  packageName = XmlElement(spd, "PackageSurfaceArea/SpdHeader/PackageName")
  packageVersion = XmlElement(spd, "PackageSurfaceArea/SpdHeader/Version")
  packageGuid = XmlElement(spd, "PackageSurfaceArea/SpdHeader/GuidValue")

  # Find the C_Name
  for cname in XmlList(spd, "/PackageSurfaceArea/GuidDeclarations/Entry/C_Name") + \
               XmlList(spd, "/PackageSurfaceArea/PcdDeclarations/PcdEntry/C_Name") + \
               XmlList(spd, "/PackageSurfaceArea/PpiDeclarations/Entry/C_Name") + \
               XmlList(spd, "/PackageSurfaceArea/ProtocolDeclarations/Entry/C_Name"):

    # Get the text of the <C_Name> tag.
    cname_text = XmlElementData(cname)

    # Map the <C_Name> to the <PackageName>. We will use this to lookup every 
    # identifier in the Input Code.
    cname_table[cname_text] = {"name": packageName, "version": packageVersion, "guid": packageGuid}


  return

def getLibClasses(spdFile):

  """Extract all the Lib Classes from an spd file."""

  # Begin to parse the XML of the .spd
  spd = xml.dom.minidom.parse(spdFile)

  # Get the guid of the package
  packageGuid = XmlElement(spd, "/PackageSurfaceArea/SpdHeader/GuidValue")

  for libClass in XmlList(spd, "/PackageSurfaceArea/LibraryClassDeclarations/LibraryClass"):
    className = XmlAttribute(libClass, "Name")
    headerfile = XmlElementData(libClass.getElementsByTagName("IncludeHeader")[0])

    packageRoot=os.path.dirname(spdFile)

    headerfile = os.path.join(packageRoot, headerfile)

    f = open(headerfile)

    # This pattern can pick out function names if the EFI coding
    # standard is followed. We could also use dumpbin on library
    # instances to get a list of symbols.
    functionPattern = re.compile("([_a-zA-Z][_a-zA-Z0-9]*) *\( *");

    for line in f.readlines():
      m = functionPattern.match(line)
      if m:
        functionName = m.group(1)
        # Map it!
        function_table[functionName] = (className, packageGuid)

    f.close()

def guid(strVal):
  """Make a guid number out of a guid hex string."""
  return long(strVal.replace('-',''), 16)

# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
if __name__ == '__main__':

  # Create a pretty printer for dumping data structures in a readable form.
  pp = pprint.PrettyPrinter(indent=2)

  # Process the command line args.
  optlist, args = getopt.getopt(sys.argv[1:], 'h', [ 'example-long-arg=', 'testing'])

  """You should pass a file name as a paramter. It should be preprocessed text
of all the .c and .h files in your module, which is cat'ed together into one
large file."""

  # Scrape out all the things that look like identifiers.
  ids = getIdentifiers(args)

  # Read in the spds from the workspace to find the Guids.
  getSpds()

  # Debug stuff.
  print "Function Table = "
  pp.pprint(function_table)
  print "CName Table = "
  pp.pprint(cname_table)
  print "Classes = "
  pp.pprint(list(search_classes(ids)))
  print "C_Names = "
  pp.pprint(list(search_cnames(ids)))
