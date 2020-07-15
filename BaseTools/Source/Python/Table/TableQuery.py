## @file
# This file is used to create/update/query/erase table for Queries
#
# Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import Common.EdkLogger as EdkLogger
from Common.StringUtils import ConvertToSqlString
from Table.Table import Table

## TableQuery
#
# This class defined a table used for Query
#
# @param object:       Inherited from object class
#
#
class TableQuery(Table):
    def __init__(self, Cursor):
        Table.__init__(self, Cursor)
        self.Table = 'Query'

    ## Create table
    #
    # Create table Query
    #
    # @param ID:                 ID of a Query
    # @param Name:               Name of a Query
    # @param Modifier:           Modifier of a Query
    # @param Value:              Type of a Query
    # @param Model:              Model of a Query
    #
    def Create(self):
        SqlCommand = """create table IF NOT EXISTS %s(ID INTEGER PRIMARY KEY,
                                                      Name TEXT DEFAULT '',
                                                      Modifier TEXT DEFAULT '',
                                                      Value TEXT DEFAULT '',
                                                      Model INTEGER DEFAULT 0
                                                     )""" % self.Table
        Table.Create(self, SqlCommand)

    ## Insert table
    #
    # Insert a record into table Query
    #
    # @param ID:                 ID of a Query
    # @param Name:               Name of a Query
    # @param Modifier:           Modifier of a Query
    # @param Value:              Value of a Query
    # @param Model:              Model of a Query
    #
    def Insert(self, Name, Modifier, Value, Model):
        self.ID = self.ID + 1
        SqlCommand = """insert into %s values(%s, '%s', '%s', '%s', %s)""" \
                                           % (self.Table, self.ID, Name, Modifier, Value, Model)
        Table.Insert(self, SqlCommand)

        return self.ID
