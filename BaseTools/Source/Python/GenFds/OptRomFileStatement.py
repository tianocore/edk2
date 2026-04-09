## @file
# process OptionROM generation from FILE statement
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import Common.LongFilePathOs as os

from .GenFdsGlobalVariable import GenFdsGlobalVariable
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
    def GenFfs(self, Dict = None, IsMakefile=False):

        if Dict is None:
            Dict = {}

        if self.FileName is not None:
            self.FileName = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.FileName)

        return self.FileName



