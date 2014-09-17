## @file
# Override built in module os.path to provide support for long file path
#
# Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

import os
from Common.LongFilePathSupport import LongFilePath

def isfile(path):
    return os.path.isfile(LongFilePath(path))

def isdir(path):
    return os.path.isdir(LongFilePath(path))

def exists(path):
    return os.path.exists(LongFilePath(path))

def getsize(filename):
    return os.path.getsize(LongFilePath(filename))

def getmtime(filename):
    return os.path.getmtime(LongFilePath(filename))

def getatime(filename):
    return os.path.getatime(LongFilePath(filename))

def getctime(filename):
    return os.path.getctime(LongFilePath(filename))

join = os.path.join
splitext = os.path.splitext
splitdrive = os.path.splitdrive
split = os.path.split
abspath = os.path.abspath
basename = os.path.basename
commonprefix = os.path.commonprefix
sep = os.path.sep
normpath = os.path.normpath
normcase = os.path.normcase
dirname = os.path.dirname
islink = os.path.islink
isabs = os.path.isabs
realpath = os.path.realpath
