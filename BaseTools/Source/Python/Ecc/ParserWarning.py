## @file
# This file is used to be the warning class of ECC tool
#
# Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

## The exception class that used to report error messages when preprocessing
#
# Currently the "ToolName" is set to be "ECC PP".
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
        self.ToolName = 'ECC PP'
