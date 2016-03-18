## @file
# This file is used to define each component of FDF file
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
from FdfParserLite import FdfParser
from Table.TableFdf import TableFdf
from CommonDataClass.DataClass import MODEL_FILE_FDF, MODEL_PCD, MODEL_META_DATA_COMPONENT
from String import NormPath

## FdfObject
#
# This class defined basic Fdf object which is used by inheriting
# 
# @param object:       Inherited from object class
#
class FdfObject(object):
    def __init__(self):
        object.__init__()

## Fdf
#
# This class defined the structure used in Fdf object
# 
# @param FdfObject:     Inherited from FdfObject class
# @param Filename:      Input value for Ffilename of Fdf file, default is None
# @param WorkspaceDir:  Input value for current workspace directory, default is None
#
class Fdf(FdfObject):
    def __init__(self, Filename = None, IsToDatabase = False, WorkspaceDir = None, Database = None):
        self.WorkspaceDir = WorkspaceDir
        self.IsToDatabase = IsToDatabase
        
        self.Cur = Database.Cur
        self.TblFile = Database.TblFile
        self.TblFdf = Database.TblFdf
        self.FileID = -1
        self.FileList = {}

        #
        # Load Fdf file if filename is not None
        #
        if Filename != None:
            self.LoadFdfFile(Filename)

    #
    # Insert a FDF file record into database
    #
    def InsertFile(self, Filename):
        FileID = -1
        Filename = NormPath(Filename)
        if Filename not in self.FileList:
            FileID = self.TblFile.InsertFile(Filename, MODEL_FILE_FDF)
            self.FileList[Filename] = FileID

        return self.FileList[Filename]
            
    
    ## Load Fdf file
    #
    # Load the file if it exists
    #
    # @param Filename:  Input value for filename of Fdf file
    #
    def LoadFdfFile(self, Filename):     
        FileList = []
        #
        # Parse Fdf file
        #
        Filename = NormPath(Filename)
        Fdf = FdfParser(Filename)
        Fdf.ParseFile()

        #
        # Insert inf file and pcd information
        #
        if self.IsToDatabase:
            (Model, Value1, Value2, Value3, Arch, BelongsToItem, BelongsToFile, StartLine, StartColumn, EndLine, EndColumn, Enabled) = \
            (0, '', '', '', 'COMMON', -1, -1, -1, -1, -1, -1, 0)
            for Index in range(0, len(Fdf.Profile.PcdDict)):
                pass
            for Key in Fdf.Profile.PcdDict.keys():
                Model = MODEL_PCD
                Value1 = ''
                Value2 = ".".join((Key[1], Key[0]))
                FileName = Fdf.Profile.PcdFileLineDict[Key][0]
                StartLine = Fdf.Profile.PcdFileLineDict[Key][1]
                BelongsToFile = self.InsertFile(FileName)
                self.TblFdf.Insert(Model, Value1, Value2, Value3, Arch, BelongsToItem, BelongsToFile, StartLine, StartColumn, EndLine, EndColumn, Enabled)
            for Index in range(0, len(Fdf.Profile.InfList)):
                Model = MODEL_META_DATA_COMPONENT
                Value1 = Fdf.Profile.InfList[Index]
                Value2 = ''
                FileName = Fdf.Profile.InfFileLineList[Index][0]
                StartLine = Fdf.Profile.InfFileLineList[Index][1]
                BelongsToFile = self.InsertFile(FileName)
                self.TblFdf.Insert(Model, Value1, Value2, Value3, Arch, BelongsToItem, BelongsToFile, StartLine, StartColumn, EndLine, EndColumn, Enabled)

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    pass
