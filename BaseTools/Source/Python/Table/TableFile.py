## @file
# This file is used to create/update/query/erase table for files
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
from Table import Table
from Common.String import ConvertToSqlString
import Common.LongFilePathOs as os
from CommonDataClass.DataClass import FileClass

## TableFile
#
# This class defined a table used for file
# 
# @param object:       Inherited from object class
#
class TableFile(Table):
    def __init__(self, Cursor):
        Table.__init__(self, Cursor)
        self.Table = 'File'
    
    ## Create table
    #
    # Create table File
    #
    # @param ID:        ID of a File
    # @param Name:      Name of a File
    # @param ExtName:   ExtName of a File
    # @param Path:      Path of a File
    # @param FullPath:  FullPath of a File
    # @param Model:     Model of a File
    # @param TimeStamp: TimeStamp of a File
    #
    def Create(self):
        SqlCommand = """create table IF NOT EXISTS %s (ID INTEGER PRIMARY KEY,
                                                       Name VARCHAR NOT NULL,
                                                       ExtName VARCHAR,
                                                       Path VARCHAR,
                                                       FullPath VARCHAR NOT NULL,
                                                       Model INTEGER DEFAULT 0,
                                                       TimeStamp VARCHAR NOT NULL
                                                      )""" % self.Table
        Table.Create(self, SqlCommand)

    ## Insert table
    #
    # Insert a record into table File
    #
    # @param ID:        ID of a File
    # @param Name:      Name of a File
    # @param ExtName:   ExtName of a File
    # @param Path:      Path of a File
    # @param FullPath:  FullPath of a File
    # @param Model:     Model of a File
    # @param TimeStamp: TimeStamp of a File
    #
    def Insert(self, Name, ExtName, Path, FullPath, Model, TimeStamp):
        self.ID = self.ID + 1
        (Name, ExtName, Path, FullPath) = ConvertToSqlString((Name, ExtName, Path, FullPath))
        SqlCommand = """insert into %s values(%s, '%s', '%s', '%s', '%s', %s, '%s')""" \
                                           % (self.Table, self.ID, Name, ExtName, Path, FullPath, Model, TimeStamp)
        Table.Insert(self, SqlCommand)
        
        return self.ID
    ## InsertFile
    #
    # Insert one file to table
    #
    # @param FileFullPath:  The full path of the file
    # @param Model:         The model of the file 
    # 
    # @retval FileID:       The ID after record is inserted
    #
    def InsertFile(self, FileFullPath, Model):
        (Filepath, Name) = os.path.split(FileFullPath)
        (Root, Ext) = os.path.splitext(FileFullPath)
        TimeStamp = os.stat(FileFullPath)[8]
        File = FileClass(-1, Name, Ext, Filepath, FileFullPath, Model, '', [], [], [])
        return self.Insert(File.Name, File.ExtName, File.Path, File.FullPath, File.Model, TimeStamp)
    
    ## Get ID of a given file
    #
    #   @param  FilePath    Path of file
    #
    #   @retval ID          ID value of given file in the table
    #
    def GetFileId(self, File):
        QueryScript = "select ID from %s where FullPath = '%s'" % (self.Table, str(File))
        RecordList = self.Exec(QueryScript)
        if len(RecordList) == 0:
            return None
        return RecordList[0][0]
