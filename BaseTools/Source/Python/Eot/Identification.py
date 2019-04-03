## @file
# This file is used to define the identification of INF/DEC/DSC files
#
# Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

## Identification
#
# This class defined basic Identification information structure which is used by INF/DEC/DSC files
#
# @param object:          Inherited from object class
#
# @var FileName:          To store data for Filename
# @var FileFullPath:      To store data for full path of the file
# @var FileRelativePath:  To store data for relative path of the file
# @var RunStatus:       Status of build system running
#
class Identification(object):
    def __init__(self):
        self.FileName = ''
        self.FileFullPath = ''
        self.FileRelativePath = ''
        self.PackagePath = ''

    ## GetFileName
    #
    # Reserved
    #
    def GetFileName(self, FileFullPath, FileRelativePath):
        pass

    ## GetFileName
    #
    # Reserved
    #
    def GetFileFullPath(self, FileName, FileRelativePath):
        pass

    ## GetFileName
    #
    # Reserved
    #
    def GetFileRelativePath(self, FileName, FileFullPath):
        pass

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    id = Identification()
