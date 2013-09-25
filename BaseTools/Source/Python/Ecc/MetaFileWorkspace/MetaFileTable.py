## @file
# This file is used to create/update/query/erase a meta file table
#
# Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
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
import uuid

import Common.EdkLogger as EdkLogger
import EccGlobalData

from MetaDataTable import Table
from MetaDataTable import ConvertToSqlString
from CommonDataClass.DataClass import MODEL_FILE_DSC, MODEL_FILE_DEC, MODEL_FILE_INF, \
                                      MODEL_FILE_OTHERS

class MetaFileTable(Table):
    ## Constructor 
    def __init__(self, Cursor, MetaFile, FileType, TableName, Temporary = False):
        self.MetaFile = MetaFile
        self.TblFile = EccGlobalData.gDb.TblFile
        if (FileType == MODEL_FILE_INF):
            TableName = "Inf"
        if (FileType == MODEL_FILE_DSC):
            if Temporary:
                TableName = "_%s_%s" % ("Dsc", uuid.uuid4().hex)
            else:
                TableName = "Dsc"
        if (FileType == MODEL_FILE_DEC):
            TableName = "Dec"

        Table.__init__(self, Cursor, TableName, 0, Temporary)
        self.Create(False)


## Python class representation of table storing module data
class ModuleTable(MetaFileTable):
    _COLUMN_ = '''
        ID REAL PRIMARY KEY,
        Model INTEGER NOT NULL,
        Value1 TEXT NOT NULL,
        Value2 TEXT,
        Value3 TEXT,
        Scope1 TEXT,
        Scope2 TEXT,
        BelongsToItem REAL NOT NULL,
        BelongsToFile SINGLE NOT NULL,
        StartLine INTEGER NOT NULL,
        StartColumn INTEGER NOT NULL,
        EndLine INTEGER NOT NULL,
        EndColumn INTEGER NOT NULL,
        Enabled INTEGER DEFAULT 0
        '''
    # used as table end flag, in case the changes to database is not committed to db file
    _DUMMY_ = "-1, -1, '====', '====', '====', '====', '====', -1, -1, -1, -1, -1, -1, -1"

    ## Constructor
    def __init__(self, Cursor):
        MetaFileTable.__init__(self, Cursor, '', MODEL_FILE_INF, "Inf", False)

    ## Insert a record into table Inf
    #
    # @param Model:          Model of a Inf item
    # @param Value1:         Value1 of a Inf item
    # @param Value2:         Value2 of a Inf item
    # @param Value3:         Value3 of a Inf item
    # @param Scope1:         Arch of a Inf item
    # @param Scope2          Platform os a Inf item
    # @param BelongsToItem:  The item belongs to which another item
    # @param StartLine:      StartLine of a Inf item
    # @param StartColumn:    StartColumn of a Inf item
    # @param EndLine:        EndLine of a Inf item
    # @param EndColumn:      EndColumn of a Inf item
    # @param Enabled:        If this item enabled
    #
    def Insert(self, Model, Value1, Value2, Value3, Scope1='COMMON', Scope2='COMMON',
               BelongsToItem=-1, BelongsToFile = -1, StartLine=-1, StartColumn=-1, EndLine=-1, EndColumn=-1, Enabled=0):
        (Value1, Value2, Value3, Scope1, Scope2) = ConvertToSqlString((Value1, Value2, Value3, Scope1, Scope2))
        return Table.Insert(
                        self, 
                        Model, 
                        Value1, 
                        Value2, 
                        Value3, 
                        Scope1, 
                        Scope2,
                        BelongsToItem,
                        BelongsToFile, 
                        StartLine, 
                        StartColumn, 
                        EndLine, 
                        EndColumn, 
                        Enabled
                        )

    ## Query table
    #
    # @param    Model:      The Model of Record 
    # @param    Arch:       The Arch attribute of Record 
    # @param    Platform    The Platform attribute of Record 
    #
    # @retval:       A recordSet of all found records 
    #
    def Query(self, Model, Arch=None, Platform=None):
        ConditionString = "Model=%s AND Enabled>=0" % Model
        ValueString = "Value1,Value2,Value3,Scope1,Scope2,ID,StartLine"

        if Arch != None and Arch != 'COMMON':
            ConditionString += " AND (Scope1='%s' OR Scope1='COMMON')" % Arch
        if Platform != None and Platform != 'COMMON':
            ConditionString += " AND (Scope2='%s' OR Scope2='COMMON' OR Scope2='DEFAULT')" % Platform

        SqlCommand = "SELECT %s FROM %s WHERE %s" % (ValueString, self.Table, ConditionString)
        return self.Exec(SqlCommand)

## Python class representation of table storing package data
class PackageTable(MetaFileTable):
    _COLUMN_ = '''
        ID REAL PRIMARY KEY,
        Model INTEGER NOT NULL,
        Value1 TEXT NOT NULL,
        Value2 TEXT,
        Value3 TEXT,
        Scope1 TEXT,
        Scope2 TEXT,
        BelongsToItem REAL NOT NULL,
        BelongsToFile SINGLE NOT NULL,
        StartLine INTEGER NOT NULL,
        StartColumn INTEGER NOT NULL,
        EndLine INTEGER NOT NULL,
        EndColumn INTEGER NOT NULL,
        Enabled INTEGER DEFAULT 0
        '''
    # used as table end flag, in case the changes to database is not committed to db file
    _DUMMY_ = "-1, -1, '====', '====', '====', '====', '====', -1, -1, -1, -1, -1, -1, -1"

    ## Constructor
    def __init__(self, Cursor):
        MetaFileTable.__init__(self, Cursor, '', MODEL_FILE_DEC, "Dec", False)

    ## Insert table
    #
    # Insert a record into table Dec
    #
    # @param Model:          Model of a Dec item
    # @param Value1:         Value1 of a Dec item
    # @param Value2:         Value2 of a Dec item
    # @param Value3:         Value3 of a Dec item
    # @param Scope1:         Arch of a Dec item
    # @param Scope2:         Module type of a Dec item
    # @param BelongsToItem:  The item belongs to which another item
    # @param StartLine:      StartLine of a Dec item
    # @param StartColumn:    StartColumn of a Dec item
    # @param EndLine:        EndLine of a Dec item
    # @param EndColumn:      EndColumn of a Dec item
    # @param Enabled:        If this item enabled
    #
    def Insert(self, Model, Value1, Value2, Value3, Scope1='COMMON', Scope2='COMMON',
               BelongsToItem=-1, BelongsToFile = -1, StartLine=-1, StartColumn=-1, EndLine=-1, EndColumn=-1, Enabled=0):
        (Value1, Value2, Value3, Scope1, Scope2) = ConvertToSqlString((Value1, Value2, Value3, Scope1, Scope2))
        return Table.Insert(
                        self, 
                        Model, 
                        Value1, 
                        Value2, 
                        Value3, 
                        Scope1, 
                        Scope2,
                        BelongsToItem,
                        BelongsToFile, 
                        StartLine, 
                        StartColumn, 
                        EndLine, 
                        EndColumn, 
                        Enabled
                        )

    ## Query table
    #
    # @param    Model:  The Model of Record 
    # @param    Arch:   The Arch attribute of Record 
    #
    # @retval:       A recordSet of all found records 
    #
    def Query(self, Model, Arch=None):
        ConditionString = "Model=%s AND Enabled>=0" % Model
        ValueString = "Value1,Value2,Value3,Scope1,ID,StartLine"

        if Arch != None and Arch != 'COMMON':
            ConditionString += " AND (Scope1='%s' OR Scope1='COMMON')" % Arch

        SqlCommand = "SELECT %s FROM %s WHERE %s" % (ValueString, self.Table, ConditionString)
        return self.Exec(SqlCommand)

## Python class representation of table storing platform data
class PlatformTable(MetaFileTable):
    _COLUMN_ = '''
        ID REAL PRIMARY KEY,
        Model INTEGER NOT NULL,
        Value1 TEXT NOT NULL,
        Value2 TEXT,
        Value3 TEXT,
        Scope1 TEXT,
        Scope2 TEXT,
        BelongsToItem REAL NOT NULL,
        BelongsToFile SINGLE NOT NULL,
        FromItem REAL NOT NULL,
        StartLine INTEGER NOT NULL,
        StartColumn INTEGER NOT NULL,
        EndLine INTEGER NOT NULL,
        EndColumn INTEGER NOT NULL,
        Enabled INTEGER DEFAULT 0
        '''
    # used as table end flag, in case the changes to database is not committed to db file
    _DUMMY_ = "-1, -1, '====', '====', '====', '====', '====', -1, -1, -1, -1, -1, -1, -1, -1"

    ## Constructor
    def __init__(self, Cursor, MetaFile = '', FileType = MODEL_FILE_DSC, Temporary = False):
        MetaFileTable.__init__(self, Cursor, MetaFile, FileType, "Dsc", Temporary)

    ## Insert table
    #
    # Insert a record into table Dsc
    #
    # @param Model:          Model of a Dsc item
    # @param Value1:         Value1 of a Dsc item
    # @param Value2:         Value2 of a Dsc item
    # @param Value3:         Value3 of a Dsc item
    # @param Scope1:         Arch of a Dsc item
    # @param Scope2:         Module type of a Dsc item
    # @param BelongsToItem:  The item belongs to which another item
    # @param FromItem:       The item belongs to which dsc file
    # @param StartLine:      StartLine of a Dsc item
    # @param StartColumn:    StartColumn of a Dsc item
    # @param EndLine:        EndLine of a Dsc item
    # @param EndColumn:      EndColumn of a Dsc item
    # @param Enabled:        If this item enabled
    #
    def Insert(self, Model, Value1, Value2, Value3, Scope1='COMMON', Scope2='COMMON', BelongsToItem=-1, BelongsToFile = -1,
               FromItem=-1, StartLine=-1, StartColumn=-1, EndLine=-1, EndColumn=-1, Enabled=1):
        (Value1, Value2, Value3, Scope1, Scope2) = ConvertToSqlString((Value1, Value2, Value3, Scope1, Scope2))
        return Table.Insert(
                        self, 
                        Model, 
                        Value1, 
                        Value2, 
                        Value3, 
                        Scope1, 
                        Scope2,
                        BelongsToItem, 
                        BelongsToFile,
                        FromItem,
                        StartLine, 
                        StartColumn, 
                        EndLine, 
                        EndColumn, 
                        Enabled
                        )

    ## Query table
    #
    # @param Model:          The Model of Record 
    # @param Scope1:         Arch of a Dsc item
    # @param Scope2:         Module type of a Dsc item
    # @param BelongsToItem:  The item belongs to which another item
    # @param FromItem:       The item belongs to which dsc file
    #
    # @retval:       A recordSet of all found records 
    #
    def Query(self, Model, Scope1=None, Scope2=None, BelongsToItem=None, FromItem=None):
        ConditionString = "Model=%s AND Enabled>0" % Model
        ValueString = "Value1,Value2,Value3,Scope1,Scope2,ID,StartLine"

        if Scope1 != None and Scope1 != 'COMMON':
            ConditionString += " AND (Scope1='%s' OR Scope1='COMMON')" % Scope1
        if Scope2 != None and Scope2 != 'COMMON':
            ConditionString += " AND (Scope2='%s' OR Scope2='COMMON' OR Scope2='DEFAULT')" % Scope2

        if BelongsToItem != None:
            ConditionString += " AND BelongsToItem=%s" % BelongsToItem
        else:
            ConditionString += " AND BelongsToItem<0"

        if FromItem != None:
            ConditionString += " AND FromItem=%s" % FromItem

        SqlCommand = "SELECT %s FROM %s WHERE %s" % (ValueString, self.Table, ConditionString)
        return self.Exec(SqlCommand)

## Factory class to produce different storage for different type of meta-file
class MetaFileStorage(object):
    _FILE_TABLE_ = {
        MODEL_FILE_INF      :   ModuleTable,
        MODEL_FILE_DEC      :   PackageTable,
        MODEL_FILE_DSC      :   PlatformTable,
        MODEL_FILE_OTHERS   :   MetaFileTable,
    }

    _FILE_TYPE_ = {
        ".inf"  : MODEL_FILE_INF,
        ".dec"  : MODEL_FILE_DEC,
        ".dsc"  : MODEL_FILE_DSC,
    }

    ## Constructor
    def __new__(Class, Cursor, MetaFile, FileType=None, Temporary=False):
        # no type given, try to find one
        if not FileType:
            if MetaFile.Type in self._FILE_TYPE_:
                FileType = Class._FILE_TYPE_[MetaFile.Type]
            else:
                FileType = MODEL_FILE_OTHERS

        # don't pass the type around if it's well known
        if FileType == MODEL_FILE_OTHERS:
            Args = (Cursor, MetaFile, FileType, Temporary)
        else:
            Args = (Cursor, MetaFile, FileType, Temporary)

        # create the storage object and return it to caller
        return Class._FILE_TABLE_[FileType](*Args)

