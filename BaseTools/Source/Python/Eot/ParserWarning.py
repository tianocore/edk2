## @file
# Warning information of Eot
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
class Warning (Exception):
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  File        The FDF name
    #   @param  Line        The Line number that error occurs
    #
    def __init__(self, Str, File = None, Line = None):
        self.message = Str
        self.FileName = File
        self.LineNumber = Line
        self.ToolName = 'EOT'
