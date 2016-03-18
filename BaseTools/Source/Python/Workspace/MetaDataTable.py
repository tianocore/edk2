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
import Common.LongFilePathOs as os

import Common.EdkLogger as EdkLogger
from CommonDataClass import DataClass
from CommonDataClass.DataClass import FileClass

## Convert to SQL required string format
def ConvertToSqlString(StringList):
    return map(lambda s: "'" + s.replace("'", "''") + "'", StringList)

## TableFile
#
# This class defined a common table
#
# @param object:     Inherited from object class
#
# @param Cursor:     Cursor of the database
# @param TableName:  Name of the table
#
class Table(object):
    _COLUMN_ = ''
    _ID_STEP_ = 1
    _ID_MAX_ = 0x80000000
    _DUMMY_ = 0

    def __init__(self, Cursor, Name='', IdBase=0, Temporary=False):
        self.Cur = Cursor
        self.Table = Name
        self.IdBase = int(IdBase)
        self.ID = int(IdBase)
        self.Temporary = Temporary

    def __str__(self):
        return self.Table

    ## Create table
    #
    # Create a table
    #
    def Create(self, NewTable=True):
        if NewTable:
            self.Drop()

        if self.Temporary:
            SqlCommand = """create temp table IF NOT EXISTS %s (%s)""" % (self.Table, self._COLUMN_)
        else:
            SqlCommand = """create table IF NOT EXISTS %s (%s)""" % (self.Table, self._COLUMN_)
        EdkLogger.debug(EdkLogger.DEBUG_8, SqlCommand)
        self.Cur.execute(SqlCommand)
        self.ID = self.GetId()

    ## Insert table
    #
    # Insert a record into a table
    #
    def Insert(self, *Args):
        self.ID = self.ID + self._ID_STEP_
        if self.ID >= (self.IdBase + self._ID_MAX_):
            self.ID = self.IdBase + self._ID_STEP_
        Values = ", ".join([str(Arg) for Arg in Args])
        SqlCommand = "insert into %s values(%s, %s)" % (self.Table, self.ID, Values)
        EdkLogger.debug(EdkLogger.DEBUG_5, SqlCommand)
        self.Cur.execute(SqlCommand)
        return self.ID

    ## Query table
    #
    # Query all records of the table
    #
    def Query(self):
        SqlCommand = """select * from %s""" % self.Table
        self.Cur.execute(SqlCommand)
        for Rs in self.Cur:
            EdkLogger.verbose(str(Rs))
        TotalCount = self.GetId()

    ## Drop a table
    #
    # Drop the table
    #
    def Drop(self):
        SqlCommand = """drop table IF EXISTS %s""" % self.Table
        self.Cur.execute(SqlCommand)

    ## Get count
    #
    # Get a count of all records of the table
    #
    # @retval Count:  Total count of all records
    #
    def GetCount(self):
        SqlCommand = """select count(ID) from %s""" % self.Table
        Record = self.Cur.execute(SqlCommand).fetchall()
        return Record[0][0]

    def GetId(self):
        SqlCommand = """select max(ID) from %s""" % self.Table
        Record = self.Cur.execute(SqlCommand).fetchall()
        Id = Record[0][0]
        if Id == None:
            Id = self.IdBase
        return Id

    ## Init the ID of the table
    #
    # Init the ID of the table
    #
    def InitID(self):
        self.ID = self.GetId()

    ## Exec
    #
    # Exec Sql Command, return result
    #
    # @param SqlCommand:  The SqlCommand to be executed
    #
    # @retval RecordSet:  The result after executed
    #
    def Exec(self, SqlCommand):
        EdkLogger.debug(EdkLogger.DEBUG_5, SqlCommand)
        self.Cur.execute(SqlCommand)
        RecordSet = self.Cur.fetchall()
        return RecordSet

    def SetEndFlag(self):
        self.Exec("insert into %s values(%s)" % (self.Table, self._DUMMY_))
        #
        # Need to execution commit for table data changed.
        #
        self.Cur.connection.commit()

    def IsIntegral(self):
        Result = self.Exec("select min(ID) from %s" % (self.Table))
        if Result[0][0] != -1:
            return False
        return True

    def GetAll(self):
        return self.Exec("select * from %s where ID > 0 order by ID" % (self.Table))

## TableFile
#
# This class defined a table used for file
#
# @param object:       Inherited from object class
#
class TableFile(Table):
    _COLUMN_ = '''
        ID INTEGER PRIMARY KEY,
        Name VARCHAR NOT NULL,
        ExtName VARCHAR,
        Path VARCHAR,
        FullPath VARCHAR NOT NULL,
        Model INTEGER DEFAULT 0,
        TimeStamp SINGLE NOT NULL
        '''
    def __init__(self, Cursor):
        Table.__init__(self, Cursor, 'File')

    ## Insert table
    #
    # Insert a record into table File
    #
    # @param Name:      Name of a File
    # @param ExtName:   ExtName of a File
    # @param Path:      Path of a File
    # @param FullPath:  FullPath of a File
    # @param Model:     Model of a File
    # @param TimeStamp: TimeStamp of a File
    #
    def Insert(self, Name, ExtName, Path, FullPath, Model, TimeStamp):
        (Name, ExtName, Path, FullPath) = ConvertToSqlString((Name, ExtName, Path, FullPath))
        return Table.Insert(
            self,
            Name,
            ExtName,
            Path,
            FullPath,
            Model,
            TimeStamp
            )

    ## InsertFile
    #
    # Insert one file to table
    #
    # @param FileFullPath:  The full path of the file
    # @param Model:         The model of the file
    #
    # @retval FileID:       The ID after record is inserted
    #
    def InsertFile(self, File, Model):
        return self.Insert(
                        File.Name,
                        File.Ext,
                        File.Dir,
                        File.Path,
                        Model,
                        File.TimeStamp
                        )

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

    ## Get type of a given file
    #
    #   @param  FileId      ID of a file
    #
    #   @retval file_type   Model value of given file in the table
    #
    def GetFileType(self, FileId):
        QueryScript = "select Model from %s where ID = '%s'" % (self.Table, FileId)
        RecordList = self.Exec(QueryScript)
        if len(RecordList) == 0:
            return None
        return RecordList[0][0]

    ## Get file timestamp of a given file
    #
    #   @param  FileId      ID of file
    #
    #   @retval timestamp   TimeStamp value of given file in the table
    #
    def GetFileTimeStamp(self, FileId):
        QueryScript = "select TimeStamp from %s where ID = '%s'" % (self.Table, FileId)
        RecordList = self.Exec(QueryScript)
        if len(RecordList) == 0:
            return None
        return RecordList[0][0]

    ## Update the timestamp of a given file
    #
    #   @param  FileId      ID of file
    #   @param  TimeStamp   Time stamp of file
    #
    def SetFileTimeStamp(self, FileId, TimeStamp):
        self.Exec("update %s set TimeStamp=%s where ID='%s'" % (self.Table, TimeStamp, FileId))

    ## Get list of file with given type
    #
    #   @param  FileType    Type value of file
    #
    #   @retval file_list   List of files with the given type
    #
    def GetFileList(self, FileType):
        RecordList = self.Exec("select FullPath from %s where Model=%s" % (self.Table, FileType))
        if len(RecordList) == 0:
            return []
        return [R[0] for R in RecordList]

## TableDataModel
#
# This class defined a table used for data model
#
# @param object:       Inherited from object class
#
#
class TableDataModel(Table):
    _COLUMN_ = """
        ID INTEGER PRIMARY KEY,
        CrossIndex INTEGER NOT NULL,
        Name VARCHAR NOT NULL,
        Description VARCHAR
        """
    def __init__(self, Cursor):
        Table.__init__(self, Cursor, 'DataModel')

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
        (Name, Description) = ConvertToSqlString((Name, Description))
        return Table.Insert(self, CrossIndex, Name, Description)

    ## Init table
    #
    # Create all default records of table DataModel
    #
    def InitTable(self):
        EdkLogger.verbose("\nInitialize table DataModel started ...")
        Count = self.GetCount()
        if Count != None and Count != 0:
            return
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

