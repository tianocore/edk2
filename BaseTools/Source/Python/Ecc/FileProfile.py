## @file
# fragments of source file
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#

from __future__ import absolute_import
import re
import Common.LongFilePathOs as os
from Ecc.ParserWarning import Warning
from Common.LongFilePathSupport import OpenLongFilePath as open

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
            fsock = open(FileName, "r")
            try:
                self.FileLinesListFromFile = fsock.readlines()
            finally:
                fsock.close()

        except IOError:
            raise Warning("Error when opening file %s" % FileName)
