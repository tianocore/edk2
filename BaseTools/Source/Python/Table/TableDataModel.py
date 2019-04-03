## @file
# This file is used to create/update/query/erase table for data models
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

## TableDataModel
#
# This class defined a table used for data model
#
# @param object:       Inherited from object class
#
#
class TableDataModel(Table):
    def __init__(self, Cursor):
        Table.__init__(self, Cursor)
        self.Table = 'DataModel'

    ## Create table
    #
    # Create table DataModel
    #
    # @param ID:           ID of a ModelType
    # @param CrossIndex:   CrossIndex of a ModelType
    # @param Name:         Name of a ModelType
    # @param Description:  Description of a ModelType
    #
    def Create(self):
        SqlCommand = """create table IF NOT EXISTS %s (ID INTEGER PRIMARY KEY,
                                                       CrossIndex INTEGER NOT NULL,
                                                       Name VARCHAR NOT NULL,
                                                       Description VARCHAR
                                                      )""" % self.Table
        Table.Create(self, SqlCommand)

    ## Insert table
    #
    # Insert a record into table DataModel
    #
    # @param ID:           ID of a ModelType
    # @param CrossIndex:   CrossIndex of a ModelType
    # @param Name:         Name of a ModelType
    # @param Description:  Description of a ModelType
    #
    def Insert(self, CrossIndex, Name, Description):
        self.ID = self.ID + 1
        (Name, Description) = ConvertToSqlString((Name, Description))
        SqlCommand = """insert into %s values(%s, %s, '%s', '%s')""" % (self.Table, self.ID, CrossIndex, Name, Description)
        Table.Insert(self, SqlCommand)

        return self.ID

    ## Init table
    #
    # Create all default records of table DataModel
    #
    def InitTable(self):
        EdkLogger.verbose("\nInitialize table DataModel started ...")
        for Item in DataClass.MODEL_LIST:
            CrossIndex = Item[1]
            Name = Item[0]
            Description = Item[0]
            self.Insert(CrossIndex, Name, Description)
        EdkLogger.verbose("Initialize table DataModel ... DONE!")

    ## Get CrossIndex
    #
    # Get a model's cross index from its name
    #
    # @param ModelName:    Name of the model
    # @retval CrossIndex:  CrossIndex of the model
    #
    def GetCrossIndex(self, ModelName):
        CrossIndex = -1
        SqlCommand = """select CrossIndex from DataModel where name = '""" + ModelName + """'"""
        self.Cur.execute(SqlCommand)
        for Item in self.Cur:
            CrossIndex = Item[0]

        return CrossIndex
