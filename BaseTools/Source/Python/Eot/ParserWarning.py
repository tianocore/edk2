## @file
# Warning information of Eot
#
#  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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