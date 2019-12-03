## @file
# Override built in function file.open to provide support for long file path
#
# Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
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
