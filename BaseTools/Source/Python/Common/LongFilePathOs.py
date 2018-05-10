## @file
# Override built in module os to provide support for long file path
#
# Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

import os
import LongFilePathOsPath
from Common.LongFilePathSupport import LongFilePath
from Common.LongFilePathSupport import UniToStr
import time

path = LongFilePathOsPath

def access(path, mode):
    return os.access(LongFilePath(path), mode)

def remove(path):
   Timeout = 0.0
   while Timeout < 5.0:
       try:
           return os.remove(LongFilePath(path))
       except:
           time.sleep(0.1)
           Timeout = Timeout + 0.1
   return os.remove(LongFilePath(path))

def removedirs(name):
    return os.removedirs(LongFilePath(name))

def rmdir(path):
    return os.rmdir(LongFilePath(path))

def mkdir(path):
    return os.mkdir(LongFilePath(path))

def makedirs(name, mode=0777):
    return os.makedirs(LongFilePath(name), mode)

def rename(old, new):
    return os.rename(LongFilePath(old), LongFilePath(new))

def chdir(path):
    return os.chdir(LongFilePath(path))

def chmod(path, mode):
    return os.chmod(LongFilePath(path), mode)

def stat(path):
    return os.stat(LongFilePath(path))

def utime(path, times):
    return os.utime(LongFilePath(path), times)

def listdir(path):
    List = []
    uList = os.listdir(u"%s" % LongFilePath(path))
    for Item in uList:
        List.append(UniToStr(Item))
    return List

environ = os.environ
getcwd = os.getcwd
chdir = os.chdir
walk = os.walk
W_OK = os.W_OK
F_OK = os.F_OK
sep = os.sep
linesep = os.linesep
getenv = os.getenv
pathsep = os.pathsep
name = os.name
SEEK_SET = os.SEEK_SET
SEEK_END = os.SEEK_END
