## @file
# This file is used to create/update/query/erase a meta file table
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import uuid

import Common.EdkLogger as EdkLogger
from Common.BuildToolError import FORMAT_INVALID

from CommonDataClass.DataClass import MODEL_FILE_DSC, MODEL_FILE_DEC, MODEL_FILE_INF, \
                                      MODEL_FILE_OTHERS
from Common.DataType import *

class MetaFileTable():
    # TRICK: use file ID as the part before '.'
    _ID_STEP_ = 1
    _ID_MAX_ = 99999999

    ## Constructor
    def __init__(self, DB, MetaFile, FileType, Temporary, FromItem=None):
        self.MetaFile = MetaFile
        self.TableName = ""
        self.DB = DB
        self._NumpyTab = None

        self.CurrentContent = []
        DB.TblFile.append([MetaFile.Name,
                        MetaFile.Ext,
                        MetaFile.Dir,
                        MetaFile.Path,
                        FileType,
                        MetaFile.TimeStamp,
                        FromItem])
        self.FileId = len(DB.TblFile)
        self.ID = self.FileId * 10**8
        if Temporary:
            self.TableName = "_%s_%s_%s" % (FileType, len(DB.TblFile), uuid.uuid4().hex)
        else:
            self.TableName = "_%s_%s" % (FileType, len(DB.TblFile))

    def IsIntegrity(self):
        Result = False
        try:
            TimeStamp = self.MetaFile.TimeStamp
            if not self.CurrentContent:
                Result = False
            else:
                Result = self.CurrentContent[-1][0] < 0
        except Exception as Exc:
            EdkLogger.debug(EdkLogger.DEBUG_5, str(Exc))
            return False
        return Result

    def SetEndFlag(self):
        self.CurrentContent.append(self._DUMMY_)

    def GetAll(self):
        return [item for item in self.CurrentContent if item[0] >= 0 and item[-1]>=0]

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
        StartLine INTEGER NOT NULL,
        StartColumn INTEGER NOT NULL,
        EndLine INTEGER NOT NULL,
        EndColumn INTEGER NOT NULL,
        Enabled INTEGER DEFAULT 0
        '''
    # used as table end flag, in case the changes to database is not committed to db file
    _DUMMY_ = [-1, -1, '====', '====', '====', '====', '====', -1, -1, -1, -1, -1, -1]

    ## Constructor
    def __init__(self, Db, MetaFile, Temporary):
        MetaFileTable.__init__(self, Db, MetaFile, MODEL_FILE_INF, Temporary)

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
    def Insert(self, Model, Value1, Value2, Value3, Scope1=TAB_ARCH_COMMON, Scope2=TAB_COMMON,
               BelongsToItem=-1, StartLine=-1, StartColumn=-1, EndLine=-1, EndColumn=-1, Enabled=0):

        (Value1, Value2, Value3, Scope1, Scope2) = (Value1.strip(), Value2.strip(), Value3.strip(), Scope1.strip(), Scope2.strip())
        self.ID = self.ID + self._ID_STEP_
        if self.ID >= (MODEL_FILE_INF + self._ID_MAX_):
            self.ID = MODEL_FILE_INF + self._ID_STEP_

        row = [ self.ID,
                Model,
                Value1,
                Value2,
                Value3,
                Scope1,
                Scope2,
                BelongsToItem,
                StartLine,
                StartColumn,
                EndLine,
                EndColumn,
                Enabled
            ]
        self.CurrentContent.append(row)
        return self.ID

    ## Query table
    #
    # @param    Model:      The Model of Record
    # @param    Arch:       The Arch attribute of Record
    # @param    Platform    The Platform attribute of Record
    #
    # @retval:       A recordSet of all found records
    #
    def Query(self, Model, Arch=None, Platform=None, BelongsToItem=None):

        QueryTab = self.CurrentContent
        result = [item for item in QueryTab if item[1] == Model and item[-1]>=0 ]

        if Arch is not None and Arch != TAB_ARCH_COMMON:
            ArchList = set(['COMMON'])
            ArchList.add(Arch)
            result = [item for item in result if item[5] in ArchList]

        if Platform is not None and Platform != TAB_COMMON:
            Platformlist = set( ['COMMON','DEFAULT'])
            Platformlist.add(Platform)
            result = [item for item in result if item[6] in Platformlist]

        if BelongsToItem is not None:
            result = [item for item in result if item[7] == BelongsToItem]

        result = [ [r[2],r[3],r[4],r[5],r[6],r[0],r[8]] for r in result ]
        return result

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
        StartLine INTEGER NOT NULL,
        StartColumn INTEGER NOT NULL,
        EndLine INTEGER NOT NULL,
        EndColumn INTEGER NOT NULL,
        Enabled INTEGER DEFAULT 0
        '''
    # used as table end flag, in case the changes to database is not committed to db file
    _DUMMY_ = [-1, -1, '====', '====', '====', '====', '====', -1, -1, -1, -1, -1, -1]

    ## Constructor
    def __init__(self, Cursor, MetaFile, Temporary):
        MetaFileTable.__init__(self, Cursor, MetaFile, MODEL_FILE_DEC, Temporary)

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
    def Insert(self, Model, Value1, Value2, Value3, Scope1=TAB_ARCH_COMMON, Scope2=TAB_COMMON,
               BelongsToItem=-1, StartLine=-1, StartColumn=-1, EndLine=-1, EndColumn=-1, Enabled=0):
        (Value1, Value2, Value3, Scope1, Scope2) = (Value1.strip(), Value2.strip(), Value3.strip(), Scope1.strip(), Scope2.strip())
        self.ID = self.ID + self._ID_STEP_

        row = [ self.ID,
                Model,
                Value1,
                Value2,
                Value3,
                Scope1,
                Scope2,
                BelongsToItem,
                StartLine,
                StartColumn,
                EndLine,
                EndColumn,
                Enabled
            ]
        self.CurrentContent.append(row)
        return self.ID

    ## Query table
    #
    # @param    Model:  The Model of Record
    # @param    Arch:   The Arch attribute of Record
    #
    # @retval:       A recordSet of all found records
    #
    def Query(self, Model, Arch=None):

        QueryTab = self.CurrentContent
        result = [item for item in QueryTab if item[1] == Model and item[-1]>=0 ]

        if Arch is not None and Arch != TAB_ARCH_COMMON:
            ArchList = set(['COMMON'])
            ArchList.add(Arch)
            result = [item for item in result if item[5] in ArchList]

        return [[r[2], r[3], r[4], r[5], r[6], r[0], r[8]] for r in result]

    def GetValidExpression(self, TokenSpaceGuid, PcdCName):

        QueryTab = self.CurrentContent
        result = [[item[2], item[8]] for item in QueryTab if item[3] == TokenSpaceGuid and item[4] == PcdCName]
        validateranges = []
        validlists = []
        expressions = []
        try:
            for row in result:
                comment = row[0]

                LineNum = row[1]
                comment = comment.strip("#")
                comment = comment.strip()
                oricomment = comment
                if comment.startswith("@ValidRange"):
                    comment = comment.replace("@ValidRange", "", 1)
                    validateranges.append(comment.split("|")[1].strip())
                if comment.startswith("@ValidList"):
                    comment = comment.replace("@ValidList", "", 1)
                    validlists.append(comment.split("|")[1].strip())
                if comment.startswith("@Expression"):
                    comment = comment.replace("@Expression", "", 1)
                    expressions.append(comment.split("|")[1].strip())
        except Exception as Exc:
            ValidType = ""
            if oricomment.startswith("@ValidRange"):
                ValidType = "@ValidRange"
            if oricomment.startswith("@ValidList"):
                ValidType = "@ValidList"
            if oricomment.startswith("@Expression"):
                ValidType = "@Expression"
            EdkLogger.error('Parser', FORMAT_INVALID, "The syntax for %s of PCD %s.%s is incorrect" % (ValidType, TokenSpaceGuid, PcdCName),
                            ExtraData=oricomment, File=self.MetaFile, Line=LineNum)
            return set(), set(), set()
        return set(validateranges), set(validlists), set(expressions)

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
        Scope3 TEXT,
        BelongsToItem REAL NOT NULL,
        FromItem REAL NOT NULL,
        StartLine INTEGER NOT NULL,
        StartColumn INTEGER NOT NULL,
        EndLine INTEGER NOT NULL,
        EndColumn INTEGER NOT NULL,
        Enabled INTEGER DEFAULT 0
        '''
    # used as table end flag, in case the changes to database is not committed to db file
    _DUMMY_ = [-1, -1, '====', '====', '====', '====', '====','====', -1, -1, -1, -1, -1, -1, -1]

    ## Constructor
    def __init__(self, Cursor, MetaFile, Temporary, FromItem=0):
        MetaFileTable.__init__(self, Cursor, MetaFile, MODEL_FILE_DSC, Temporary, FromItem)

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
    def Insert(self, Model, Value1, Value2, Value3, Scope1=TAB_ARCH_COMMON, Scope2=TAB_COMMON, Scope3=TAB_DEFAULT_STORES_DEFAULT,BelongsToItem=-1,
               FromItem=-1, StartLine=-1, StartColumn=-1, EndLine=-1, EndColumn=-1, Enabled=1):
        (Value1, Value2, Value3, Scope1, Scope2, Scope3) = (Value1.strip(), Value2.strip(), Value3.strip(), Scope1.strip(), Scope2.strip(), Scope3.strip())
        self.ID = self.ID + self._ID_STEP_

        row = [ self.ID,
                Model,
                Value1,
                Value2,
                Value3,
                Scope1,
                Scope2,
                Scope3,
                BelongsToItem,
                FromItem,
                StartLine,
                StartColumn,
                EndLine,
                EndColumn,
                Enabled
            ]
        self.CurrentContent.append(row)
        return self.ID


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

        QueryTab = self.CurrentContent
        result = [item for item in QueryTab if item[1] == Model and item[-1]>0 ]
        if Scope1 is not None and Scope1 != TAB_ARCH_COMMON:
            Sc1 = set(['COMMON'])
            Sc1.add(Scope1)
            result = [item for item in result if item[5] in Sc1]
        Sc2 = set( ['COMMON','DEFAULT'])
        if Scope2 and Scope2 != TAB_COMMON:
            if '.' in Scope2:
                Index = Scope2.index('.')
                NewScope = TAB_COMMON + Scope2[Index:]
                Sc2.add(NewScope)
            Sc2.add(Scope2)
            result = [item for item in result if item[6] in Sc2]

        if BelongsToItem is not None:
            result = [item for item in result if item[8] == BelongsToItem]
        else:
            result = [item for item in result if item[8] < 0]
        if FromItem is not None:
            result = [item for item in result if item[9] == FromItem]

        result = [ [r[2],r[3],r[4],r[5],r[6],r[7],r[0],r[10]] for r in result ]
        return result

    def DisableComponent(self,comp_id):
        for item in self.CurrentContent:
            if item[0] == comp_id or item[8] == comp_id:
                item[-1] = -1

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
    _ObjectCache = {}
    ## Constructor
    def __new__(Class, Cursor, MetaFile, FileType=None, Temporary=False, FromItem=None):
        # no type given, try to find one
        key = (MetaFile.Path, FileType,Temporary,FromItem)
        if key in Class._ObjectCache:
            return Class._ObjectCache[key]
        if not FileType:
            if MetaFile.Type in self._FILE_TYPE_:
                FileType = Class._FILE_TYPE_[MetaFile.Type]
            else:
                FileType = MODEL_FILE_OTHERS

        # don't pass the type around if it's well known
        if FileType == MODEL_FILE_OTHERS:
            Args = (Cursor, MetaFile, FileType, Temporary)
        else:
            Args = (Cursor, MetaFile, Temporary)
        if FromItem:
            Args = Args + (FromItem,)

        # create the storage object and return it to caller
        reval = Class._FILE_TABLE_[FileType](*Args)
        if not Temporary:
            Class._ObjectCache[key] = reval
        return reval

