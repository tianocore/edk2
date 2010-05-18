## @file
# process OptionROM generation from FILE statement
#
#  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import os

from GenFdsGlobalVariable import GenFdsGlobalVariable
## 
#
#
class OptRomFileStatement:
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.FileName = None
        self.FileType = None
        self.OverrideAttribs = None

    ## GenFfs() method
    #
    #   Generate FFS
    #
    #   @param  self        The object pointer
    #   @param  Dict        dictionary contains macro and value pair
    #   @retval string      Generated FFS file name
    #
    def GenFfs(self, Dict = {}):
        
        if self.FileName != None:
            self.FileName = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.FileName)
        
        return self.FileName



