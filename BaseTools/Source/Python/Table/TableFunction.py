## @file
# This file is used to create/update/query/erase table for functions
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import Common.EdkLogger as EdkLogger
from Table.Table import Table
from Common.StringUtils import ConvertToSqlString

## TableFunction
#
# This class defined a table used for function
#
# @param Table:       Inherited from Table class
#
class TableFunction(Table):
    def __init__(self, Cursor):
        Table.__init__(self, Cursor)
        self.Table = 'Function'

    ## Create table
    #
    # Create table Function
    #
    # @param ID:                  ID of a Function
    # @param Header:              Header of a Function
    # @param Modifier:            Modifier of a Function
    # @param Name:                Name of a Function
    # @param ReturnStatement:     ReturnStatement of a Function
    # @param StartLine:           StartLine of a Function
    # @param StartColumn:         StartColumn of a Function
    # @param EndLine:             EndLine of a Function
    # @param EndColumn:           EndColumn of a Function
    # @param BodyStartLine:       StartLine of a Function body
    # @param BodyStartColumn:     StartColumn of a Function body
    # @param BelongsToFile:       The Function belongs to which file
    # @param FunNameStartLine:    StartLine of a Function name
    # @param FunNameStartColumn:  StartColumn of a Function name
    #
    def Create(self):
        SqlCommand = """create table IF NOT EXISTS %s (ID INTEGER PRIMARY KEY,
                                                       Header TEXT,
                                                       Modifier VARCHAR,
                                                       Name VARCHAR NOT NULL,
                                                       ReturnStatement VARCHAR,
                                                       StartLine INTEGER NOT NULL,
                                                       StartColumn INTEGER NOT NULL,
                                                       EndLine INTEGER NOT NULL,
                                                       EndColumn INTEGER NOT NULL,
                                                       BodyStartLine INTEGER NOT NULL,
                                                       BodyStartColumn INTEGER NOT NULL,
                                                       BelongsToFile SINGLE NOT NULL,
                                                       FunNameStartLine INTEGER NOT NULL,
                                                       FunNameStartColumn INTEGER NOT NULL
                                                      )""" % self.Table
        Table.Create(self, SqlCommand)

    ## Insert table
    #
    # Insert a record into table Function
    #
    # @param ID:                  ID of a Function
    # @param Header:              Header of a Function
    # @param Modifier:            Modifier of a Function
    # @param Name:                Name of a Function
    # @param ReturnStatement:     ReturnStatement of a Function
    # @param StartLine:           StartLine of a Function
    # @param StartColumn:         StartColumn of a Function
    # @param EndLine:             EndLine of a Function
    # @param EndColumn:           EndColumn of a Function
    # @param BodyStartLine:       StartLine of a Function body
    # @param BodyStartColumn:     StartColumn of a Function body
    # @param BelongsToFile:       The Function belongs to which file
    # @param FunNameStartLine:    StartLine of a Function name
    # @param FunNameStartColumn:  StartColumn of a Function name
    #
    def Insert(self, Header, Modifier, Name, ReturnStatement, StartLine, StartColumn, EndLine, EndColumn, BodyStartLine, BodyStartColumn, BelongsToFile, FunNameStartLine, FunNameStartColumn):
        self.ID = self.ID + 1
        (Header, Modifier, Name, ReturnStatement) = ConvertToSqlString((Header, Modifier, Name, ReturnStatement))
        SqlCommand = """insert into %s values(%s, '%s', '%s', '%s', '%s', %s, %s, %s, %s, %s, %s, %s, %s, %s)""" \
                                    % (self.Table, self.ID, Header, Modifier, Name, ReturnStatement, StartLine, StartColumn, EndLine, EndColumn, BodyStartLine, BodyStartColumn, BelongsToFile, FunNameStartLine, FunNameStartColumn)
        Table.Insert(self, SqlCommand)

        return self.ID
