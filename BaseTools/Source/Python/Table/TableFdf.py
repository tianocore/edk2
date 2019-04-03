## @file
# This file is used to create/update/query/erase table for fdf datas
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import Common.EdkLogger as EdkLogger
import CommonDataClass.DataClass as DataClass
from Table.Table import Table
from Common.StringUtils import ConvertToSqlString

## TableFdf
#
# This class defined a table used for data model
#
# @param object:       Inherited from object class
#
#
class TableFdf(Table):
    def __init__(self, Cursor):
        Table.__init__(self, Cursor)
        self.Table = 'Fdf'

    ## Create table
    #
    # Create table Fdf
    #
    # @param ID:             ID of a Fdf item
    # @param Model:          Model of a Fdf item
    # @param Value1:         Value1 of a Fdf item
    # @param Value2:         Value2 of a Fdf item
    # @param Value3:         Value3 of a Fdf item
    # @param Arch:           Arch of a Fdf item
    # @param BelongsToItem:  The item belongs to which another item
    # @param BelongsToFile:  The item belongs to which fdf file
    # @param StartLine:      StartLine of a Fdf item
    # @param StartColumn:    StartColumn of a Fdf item
    # @param EndLine:        EndLine of a Fdf item
    # @param EndColumn:      EndColumn of a Fdf item
    # @param Enabled:        If this item enabled
    #
    def Create(self):
        SqlCommand = """create table IF NOT EXISTS %s (ID INTEGER PRIMARY KEY,
                                                       Model INTEGER NOT NULL,
                                                       Value1 VARCHAR NOT NULL,
                                                       Value2 VARCHAR,
                                                       Value3 VARCHAR,
                                                       Scope1 VarCHAR,
                                                       Scope2 VarCHAR,
                                                       BelongsToItem SINGLE NOT NULL,
                                                       BelongsToFile SINGLE NOT NULL,
                                                       StartLine INTEGER NOT NULL,
                                                       StartColumn INTEGER NOT NULL,
                                                       EndLine INTEGER NOT NULL,
                                                       EndColumn INTEGER NOT NULL,
                                                       Enabled INTEGER DEFAULT 0
                                                      )""" % self.Table
        Table.Create(self, SqlCommand)

    ## Insert table
    #
    # Insert a record into table Fdf
    #
    # @param ID:             ID of a Fdf item
    # @param Model:          Model of a Fdf item
    # @param Value1:         Value1 of a Fdf item
    # @param Value2:         Value2 of a Fdf item
    # @param Value3:         Value3 of a Fdf item
    # @param Arch:           Arch of a Fdf item
    # @param BelongsToItem:  The item belongs to which another item
    # @param BelongsToFile:  The item belongs to which fdf file
    # @param StartLine:      StartLine of a Fdf item
    # @param StartColumn:    StartColumn of a Fdf item
    # @param EndLine:        EndLine of a Fdf item
    # @param EndColumn:      EndColumn of a Fdf item
    # @param Enabled:        If this item enabled
    #
    def Insert(self, Model, Value1, Value2, Value3, Scope1, Scope2, BelongsToItem, BelongsToFile, StartLine, StartColumn, EndLine, EndColumn, Enabled):
        self.ID = self.ID + 1
        (Value1, Value2, Value3, Scope1, Scope2) = ConvertToSqlString((Value1, Value2, Value3, Scope1, Scope2))
        SqlCommand = """insert into %s values(%s, %s, '%s', '%s', '%s', '%s', '%s', %s, %s, %s, %s, %s, %s, %s)""" \
                     % (self.Table, self.ID, Model, Value1, Value2, Value3, Scope1, Scope2, BelongsToItem, BelongsToFile, StartLine, StartColumn, EndLine, EndColumn, Enabled)
        Table.Insert(self, SqlCommand)

        return self.ID

    ## Query table
    #
    # @param Model:  The Model of Record
    #
    # @retval:       A recordSet of all found records
    #
    def Query(self, Model):
        SqlCommand = """select ID, Value1, Value2, Value3, Scope1, Scope2, BelongsToItem, BelongsToFile, StartLine from %s
                        where Model = %s
                        and Enabled > -1""" % (self.Table, Model)
        EdkLogger.debug(4, "SqlCommand: %s" % SqlCommand)
        self.Cur.execute(SqlCommand)
        return self.Cur.fetchall()
