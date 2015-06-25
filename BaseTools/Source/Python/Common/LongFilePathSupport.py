## @file
# Override built in function file.open to provide support for long file path
#
# Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

import os
import platform
import shutil
import codecs

##
# OpenLongPath
# Convert a file path to a long file path
#
def LongFilePath(FileName):
    FileName = os.path.normpath(FileName)
    if platform.system() == 'Windows':
        if FileName.startswith('\\\\?\\'):
            return FileName
        if FileName.startswith('\\\\'):
            return '\\\\?\\UNC\\' + FileName[2:]
        if os.path.isabs(FileName):
            return '\\\\?\\' + FileName
    return FileName

##
# OpenLongFilePath
# wrap open to support opening a long file path
#
def OpenLongFilePath(FileName, Mode='r', Buffer= -1):
    return open(LongFilePath(FileName), Mode, Buffer)

def CodecOpenLongFilePath(Filename, Mode='rb', Encoding=None, Errors='strict', Buffering=1):
    return codecs.open(LongFilePath(Filename), Mode, Encoding, Errors, Buffering)

##
# CopyLongFilePath
# wrap copyfile to support copy a long file path
#
def CopyLongFilePath(src, dst):
    with open(LongFilePath(src), 'rb') as fsrc:
        with open(LongFilePath(dst), 'wb') as fdst:
            shutil.copyfileobj(fsrc, fdst)

## Convert a python unicode string to a normal string
#
# Convert a python unicode string to a normal string
# UniToStr(u'I am a string') is 'I am a string'
#
# @param Uni:  The python unicode string
#
# @retval:     The formatted normal string
#
def UniToStr(Uni):
    return repr(Uni)[2:-1]
