## @file
# This file is used to create/update/query/erase table for ECC reports
#
# Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import Common.EdkLogger as EdkLogger
import Common.LongFilePathOs as os, time
from Table import Table
from Common.String import ConvertToSqlString2
import Eot.EotToolError as EotToolError
import Eot.EotGlobalData as EotGlobalData

## TableReport
#
# This class defined a table used for data model
# 
# @param object:       Inherited from object class
#
#
class TableEotReport(Table):
    def __init__(self, Cursor):
        Table.__init__(self, Cursor)
        self.Table = 'Report'
    
    ## Create table
    #
    # Create table report
    #
    #
    def Create(self):
        SqlCommand = """create table IF NOT EXISTS %s (ID INTEGER PRIMARY KEY,
                                                       ModuleID INTEGER DEFAULT -1,
                                                       ModuleName TEXT DEFAULT '',
                                                       ModuleGuid TEXT DEFAULT '',
                                                       SourceFileID INTEGER DEFAULT -1,
                                                       SourceFileFullPath TEXT DEFAULT '',
                                                       ItemName TEXT DEFAULT '',
                                                       ItemType TEXT DEFAULT '',
                                                       ItemMode TEXT DEFAULT '',
                                                       GuidName TEXT DEFAULT '',
                                                       GuidMacro TEXT DEFAULT '',
                                                       GuidValue TEXT DEFAULT '',
                                                       BelongsToFunction TEXT DEFAULT '',
                                                       Enabled INTEGER DEFAULT 0
                                                      )""" % self.Table
        Table.Create(self, SqlCommand)

    ## Insert table
    #
    # Insert a record into table report
    #
    #
    def Insert(self, ModuleID = -1, ModuleName = '', ModuleGuid = '', SourceFileID = -1, SourceFileFullPath = '', \
               ItemName = '', ItemType = '', ItemMode = '', GuidName = '', GuidMacro = '', GuidValue = '', BelongsToFunction = '', Enabled = 0):
        self.ID = self.ID + 1
        SqlCommand = """insert into %s values(%s, %s, '%s', '%s', %s, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %s)""" \
                     % (self.Table, self.ID, ModuleID, ModuleName, ModuleGuid, SourceFileID, SourceFileFullPath, \
                        ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, Enabled)
        Table.Insert(self, SqlCommand)
        
    def GetMaxID(self):
        SqlCommand = """select max(ID) from %s""" % self.Table
        self.Cur.execute(SqlCommand)
        for Item in self.Cur:
            return Item[0]