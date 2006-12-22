#!/usr/bin/env python

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
  return "%s-%s-%s-%s-%s" % (g[0:8], g[8:12], g[12:16], g[16:20], g[20:])

def lean(path):
  """Lean the slashes forward"""

  return os.path.normpath(path).replace("\\", "/")
