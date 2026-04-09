## @file
# This file is used to create/update/query/erase table for Identifiers
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import Common.EdkLogger as EdkLogger
from Common.StringUtils import ConvertToSqlString
from Table.Table import Table

## TableIdentifier
#
# This class defined a table used for Identifier
#
# @param object:       Inherited from object class
#
#
class TableIdentifier(Table):
    def __init__(self, Cursor):
        Table.__init__(self, Cursor)
        self.Table = 'Identifier'

    ## Create table
    #
    # Create table Identifier
    #
    # @param ID:                 ID of a Identifier
    # @param Modifier:           Modifier of a Identifier
    # @param Type:               Type of a Identifier
    # @param Name:               Name of a Identifier
    # @param Value:              Value of a Identifier
    # @param Model:              Model of a Identifier
    # @param BelongsToFile:      The Identifier belongs to which file
    # @param BelongsToFunction:  The Identifier belongs to which function
    # @param StartLine:          StartLine of a Identifier
    # @param StartColumn:        StartColumn of a Identifier
    # @param EndLine:            EndLine of a Identifier
    # @param EndColumn:          EndColumn of a Identifier
    #
    def Create(self):
        SqlCommand = """create table IF NOT EXISTS %s(ID INTEGER PRIMARY KEY,
                                                      Modifier VARCHAR,
                                                      Type VARCHAR,
                                                      Name VARCHAR NOT NULL,
                                                      Value VARCHAR NOT NULL,
                                                      Model INTEGER NOT NULL,
                                                      BelongsToFile SINGLE NOT NULL,
                                                      BelongsToFunction SINGLE DEFAULT -1,
                                                      StartLine INTEGER NOT NULL,
                                                      StartColumn INTEGER NOT NULL,
                                                      EndLine INTEGER NOT NULL,
                                                      EndColumn INTEGER NOT NULL
                                                     )""" % self.Table
        Table.Create(self, SqlCommand)

    ## Insert table
    #
    # Insert a record into table Identifier
    #
    # @param ID:                 ID of a Identifier
    # @param Modifier:           Modifier of a Identifier
    # @param Type:               Type of a Identifier
    # @param Name:               Name of a Identifier
    # @param Value:              Value of a Identifier
    # @param Model:              Model of a Identifier
    # @param BelongsToFile:      The Identifier belongs to which file
    # @param BelongsToFunction:  The Identifier belongs to which function
    # @param StartLine:          StartLine of a Identifier
    # @param StartColumn:        StartColumn of a Identifier
    # @param EndLine:            EndLine of a Identifier
    # @param EndColumn:          EndColumn of a Identifier
    #
    def Insert(self, Modifier, Type, Name, Value, Model, BelongsToFile, BelongsToFunction, StartLine, StartColumn, EndLine, EndColumn):
        self.ID = self.ID + 1
        (Modifier, Type, Name, Value) = ConvertToSqlString((Modifier, Type, Name, Value))
        SqlCommand = """insert into %s values(%s, '%s', '%s', '%s', '%s', %s, %s, %s, %s, %s, %s, %s)""" \
                                           % (self.Table, self.ID, Modifier, Type, Name, Value, Model, BelongsToFile, BelongsToFunction, StartLine, StartColumn, EndLine, EndColumn)
        Table.Insert(self, SqlCommand)

        return self.ID
