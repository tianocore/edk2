#!/usr/bin/env python
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
#
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

#
# ARMCC tools do not support cygwin paths. Ths script converts cygwin paths to DOS paths
# in any arguments. 
#
# armcc_wrapper.py ToolToExec [command line to convert]
#
# anthing with the / will be converted via cygpath cygwin call or manually. 
# -I/cygpath/c/example is a special case as you can not pass -I to cygpath
#
# ExceptionList if a tool takes an argument with a / add it to the exception list
#
import sys
import os
import subprocess
import pipes

#
# Convert using cygpath command line tool
# Currently not used, but just in case we need it in the future
#
def ConvertCygPathToDosViacygpath(CygPath):
  p = subprocess.Popen("cygpath -m " + pipes.quote(CygPath), shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, close_fds=True)
  return p.stdout.read().strip()

#
#
#
def ConvertCygPathToDos(CygPath):
  if CygPath.find("/cygdrive/") == 0:
    # convert /cygdrive/c/Xyz to c:/Xyz
    DosPath = CygPath[10] + ':' + CygPath[11:]
  else:
    DosPath = CygPath
  
  # pipes.quote will add the extra \\ for us.
  return DosPath.replace('/','\\')


# we receive our options as a list, but we will be passing them to the shell as a line
# this means we have to requote things as they will get one round of unquoting.
# we can't set "shell=False" because we are running commands from the PATH and
# if you don't use the shell you don't get a PATH search.
def main(argv):

  # use 1st argument as name of tool to call
  Command = pipes.quote(sys.argv[1]);
  
  ExceptionList = ["/interwork"]
  
  for arg in argv:
    if arg.find('/') == -1:
      # if we don't need to convert just add to the command line
      Command = Command + ' ' + pipes.quote(arg)
    elif arg in ExceptionList:
      # if it is in the list, then don't do a cygpath
      # assembler stuff after --apcs has the /.
      Command = Command + ' ' + pipes.quote(arg)
    else:
      if ((arg[0] == '-') and (arg[1] == 'I' or arg[1] == 'i')):
        CygPath = arg[0] + arg[1] + ConvertCygPathToDos(arg[2:])
      else:
        CygPath = ConvertCygPathToDos(arg)

      Command = Command + ' ' + pipes.quote(CygPath)
    
  # call the real tool with the converted paths
  return subprocess.call(Command, shell=True)

 
if __name__ == "__main__":
  try:
     ret = main(sys.argv[2:])

  except:
    print "exiting: exception from " + sys.argv[0]
    ret = 2

  sys.exit(ret)

