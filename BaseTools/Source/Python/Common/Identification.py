## @file
# This file is used to define the identification of INF/DEC/DSC files
#
# Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
