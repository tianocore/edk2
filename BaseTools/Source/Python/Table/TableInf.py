## @file
# This file is used to create/update/query/erase table for inf datas
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

## TableInf
#
# This class defined a table used for data model
#
# @param object:       Inherited from object class
#
#
class TableInf(Table):
    def __init__(self, Cursor):
        Table.__init__(self, Cursor)
        self.Table = 'Inf'

    ## Create table
    #
    # Create table Inf
    #
    # @param ID:             ID of a Inf item
    # @param Model:          Model of a Inf item
    # @param Value1:         Value1 of a Inf item
    # @param Value2:         Value2 of a Inf item
    # @param Value3:         Value3 of a Inf item
    # @param Value4:         Value4 of a Inf item
    # @param Value5:         Value5 of a Inf item
    # @param Arch:           Arch of a Inf item
    # @param BelongsToItem:  The item belongs to which another item
    # @param BelongsToFile:  The item belongs to which dsc file
    # @param StartLine:      StartLine of a Inf item
    # @param StartColumn:    StartColumn of a Inf item
    # @param EndLine:        EndLine of a Inf item
    # @param EndColumn:      EndColumn of a Inf item
    # @param Enabled:        If this item enabled
    #
    def Create(self):
        SqlCommand = """create table IF NOT EXISTS %s (ID INTEGER PRIMARY KEY,
                                                       Model INTEGER NOT NULL,
                                                       Value1 VARCHAR NOT NULL,
                                                       Value2 VARCHAR,
                                                       Value3 VARCHAR,
                                                       Value4 VARCHAR,
                                                       Value5 VARCHAR,
                                                       Arch VarCHAR,
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
    # Insert a record into table Inf
    #
    # @param ID:             ID of a Inf item
    # @param Model:          Model of a Inf item
    # @param Value1:         Value1 of a Inf item
    # @param Value2:         Value2 of a Inf item
    # @param Value3:         Value3 of a Inf item
    # @param Value4:         Value4 of a Inf item
    # @param Value5:         Value5 of a Inf item
    # @param Arch:           Arch of a Inf item
    # @param BelongsToItem:  The item belongs to which another item
    # @param BelongsToFile:  The item belongs to which dsc file
    # @param StartLine:      StartLine of a Inf item
    # @param StartColumn:    StartColumn of a Inf item
    # @param EndLine:        EndLine of a Inf item
    # @param EndColumn:      EndColumn of a Inf item
    # @param Enabled:        If this item enabled
    #
    def Insert(self, Model, Value1, Value2, Value3, Value4, Value5, Arch, BelongsToItem, BelongsToFile, StartLine, StartColumn, EndLine, EndColumn, Enabled):
        self.ID = self.ID + 1
        (Value1, Value2, Value3, Value4, Value5, Arch) = ConvertToSqlString((Value1, Value2, Value3, Value4, Value5, Arch))
        SqlCommand = """insert into %s values(%s, %s, '%s', '%s', '%s', '%s', '%s', '%s', %s, %s, %s, %s, %s, %s, %s)""" \
                     % (self.Table, self.ID, Model, Value1, Value2, Value3, Value4, Value5, Arch, BelongsToItem, BelongsToFile, StartLine, StartColumn, EndLine, EndColumn, Enabled)
        Table.Insert(self, SqlCommand)

        return self.ID

    ## Query table
    #
    # @param Model:  The Model of Record
    #
    # @retval:       A recordSet of all found records
    #
    def Query(self, Model):
        SqlCommand = """select ID, Value1, Value2, Value3, Arch, BelongsToItem, BelongsToFile, StartLine from %s
                        where Model = %s
                        and Enabled > -1""" % (self.Table, Model)
        EdkLogger.debug(4, "SqlCommand: %s" % SqlCommand)
        self.Cur.execute(SqlCommand)
        return self.Cur.fetchall()
