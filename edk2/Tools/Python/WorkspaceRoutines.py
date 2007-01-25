#!/usr/bin/env python

# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
# 
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

import os, sys, re, getopt, string, glob, xml.dom.minidom, pprint, md5, socket, getpass, time, random

def inWorkspace(rel_path=""):
  """Treat the given path as relative to the workspace."""

  # Make sure the user has set the workspace variable:
  try:
    return os.path.join(os.environ["WORKSPACE"], rel_path )
  except:
    print "Oops! You must set the WORKSPACE environment variable to run this script."
    sys.exit()

def genguid():
  g = md5.md5(
        str(random.random()) +
        getpass.getuser() + 
        str(time.time()) + 
        socket.gethostbyname(socket.gethostname())).hexdigest()
  return Guid("%s-%s-%s-%s-%s" % (g[0:8], g[8:12], g[12:16], g[16:20], g[20:]))

def lean(path):
  """Lean the slashes forward"""

  return os.path.normpath(path).replace("\\", "/")

def mkdir(path):
  """Make a directory if it is not there already."""

  try:
    os.makedirs(path)
  except:
    pass

def Md5(filename):

  sum = ""

  try:
    f=open(filename, "rb")
    sum = md5.md5(f.read()).hexdigest()
    f.close()
  except IOError:
    print "Error: Unable to open file: %s" % filename
    sys.exit()

  return sum

def Guid(guidString):
  """Convert the guid string into a canonical form suitable for comparison."""
  return string.lower(guidString)
