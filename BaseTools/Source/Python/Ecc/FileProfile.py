## @file
# fragments of source file
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

import re
import os
from ParserWarning import Warning

CommentList = []
PPDirectiveList = []
PredicateExpressionList = []
FunctionDefinitionList = []
VariableDeclarationList = []
EnumerationDefinitionList = []
StructUnionDefinitionList = []
TypedefDefinitionList = []
FunctionCallingList = []

## record file data when parsing source
#
# May raise Exception when opening file.
#
class FileProfile :
    
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  FileName    The file that to be parsed
    #
    def __init__(self, FileName):
        self.FileLinesList = []
        self.FileLinesListFromFile = []
        try:
            fsock = open(FileName, "rb", 0)
            try:
                self.FileLinesListFromFile = fsock.readlines()
            finally:
                fsock.close()

        except IOError:
            raise Warning("Error when opening file %s" % FileName)
        
        