## @file
# This file is used to provide method for process AsBuilt INF file. It will consumed by InfParser
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
'''
InfAsBuiltProcess
'''
## Import modules
#

import os
import re
from Library import GlobalData

import Logger.Log as Logger
from Logger import StringTable as ST
from Logger import ToolError

from Library.String import GetSplitValueList
from Library.Misc import GetHelpStringByRemoveHashKey
from Library.Misc import ValidFile
from Library.Misc import ProcessLineExtender
from Library.ParserValidate import IsValidPath
from Library.Parsing import MacroParser
from Parser.InfParserMisc import InfExpandMacro

from Library import DataType as DT

## GetLibInstanceInfo
#
# Get the information from Library Instance INF file.
#
# @param string.  A string start with # and followed by INF file path
# @param WorkSpace. The WorkSpace directory used to combined with INF file path.
#
# @return GUID, Version
def GetLibInstanceInfo(String, WorkSpace, LineNo):
    
    FileGuidString = ""
    VerString = ""
    
    OrignalString = String 
    String = String.strip()
    if not String:
        return None, None
    #
    # Remove "#" characters at the beginning
    #
    String = GetHelpStringByRemoveHashKey(String)
    String = String.strip()
    
    FileLinesList = GetFileLineContent(String, WorkSpace, LineNo, OrignalString)

        
    ReFindFileGuidPattern = re.compile("^\s*FILE_GUID\s*=.*$")
    ReFindVerStringPattern = re.compile("^\s*VERSION_STRING\s*=.*$")
    
    FileLinesList = ProcessLineExtender(FileLinesList)

    for Line in FileLinesList:
        if ReFindFileGuidPattern.match(Line):
            FileGuidString = Line
        if ReFindVerStringPattern.match(Line):
            VerString = Line
    
    if FileGuidString:
        FileGuidString = GetSplitValueList(FileGuidString, '=', 1)[1]
    if VerString:
        VerString = GetSplitValueList(VerString, '=', 1)[1]
        
    return FileGuidString, VerString
    
## GetPackageListInfo
#
# Get the package information from INF file.
#
# @param string.  A string start with # and followed by INF file path
# @param WorkSpace. The WorkSpace directory used to combined with INF file path.
#
# @return GUID, Version
def GetPackageListInfo(FileNameString, WorkSpace, LineNo): 
    PackageInfoList = []
    DefineSectionMacros = {}
    PackageSectionMacros = {}
    
    FileLinesList = GetFileLineContent(FileNameString, WorkSpace, LineNo, '')
    
    RePackageHeader = re.compile('^\s*\[Packages.*\].*$')
    ReDefineHeader = re.compile('^\s*\[Defines].*$')
    
    PackageHederFlag = False
    DefineHeaderFlag = False
    LineNo = -1
    for Line in FileLinesList:
        LineNo += 1
        Line = Line.strip()
        
        if Line.startswith('['):
            PackageHederFlag = False
            DefineHeaderFlag = False            
        
        if Line.startswith("#"):
            continue
        
        if not Line:
            continue        
        
        #
        # Found [Packages] section 
        #
        if RePackageHeader.match(Line):
            PackageHederFlag = True
            continue
        
        #
        # Found [Define] section
        #
        if ReDefineHeader.match(Line):
            DefineHeaderFlag = True
            continue
        
        if DefineHeaderFlag:
            #
            # Find Macro
            #
            Name, Value = MacroParser((Line, LineNo),
                                      FileNameString,
                                      DT.MODEL_META_DATA_HEADER,
                                      DefineSectionMacros) 
                       
            if Name != None:
                DefineSectionMacros[Name] = Value               
                continue
            
        if PackageHederFlag:

            #
            # Find Macro
            #
            Name, Value = MacroParser((Line, LineNo),
                                      FileNameString,
                                      DT.MODEL_META_DATA_PACKAGE,
                                      DefineSectionMacros)
            if Name != None:
                PackageSectionMacros[Name] = Value               
                continue
            
            #
            # Replace with Local section Macro and [Defines] section Macro.
            #            
            Line = InfExpandMacro(Line, (FileNameString, Line, LineNo), DefineSectionMacros, PackageSectionMacros, True)
            
            Line = GetSplitValueList(Line, "#", 1)[0]
            Line = GetSplitValueList(Line, "|", 1)[0]
            PackageInfoList.append(Line)
     
    return PackageInfoList    
    
def GetFileLineContent(FileName, WorkSpace, LineNo, OriginalString):
    
    if not LineNo:
        LineNo = -1

    #
    # Validate file name exist.
    #
    FullFileName = os.path.normpath(os.path.realpath(os.path.join(WorkSpace, FileName)))
    if not (ValidFile(FullFileName)):
        Logger.Error("InfParser", 
                     ToolError.FORMAT_INVALID,
                     ST.ERR_FILELIST_EXIST%(FileName),
                     File=GlobalData.gINF_MODULE_NAME,
                     Line=LineNo, 
                     ExtraData=OriginalString)
    
    #
    # Validate file exist/format.
    #
    if IsValidPath(FileName, WorkSpace):
        IsValidFileFlag = True
    else:
        Logger.Error("InfParser", 
                     ToolError.FORMAT_INVALID,
                     ST.ERR_INF_PARSER_FILE_NOT_EXIST_OR_NAME_INVALID%(FileName),
                     File=GlobalData.gINF_MODULE_NAME, 
                     Line=LineNo, 
                     ExtraData=OriginalString)
        return False
    
    FileLinesList = []
    
    if IsValidFileFlag:  
        try:
            FullFileName = FullFileName.replace('\\', '/')
            Inputfile = open(FullFileName, "rb", 0)
            try:
                FileLinesList = Inputfile.readlines()
            except BaseException:
                Logger.Error("InfParser", ToolError.FILE_READ_FAILURE, ST.ERR_FILE_OPEN_FAILURE, File=FullFileName)
            finally:
                Inputfile.close()
        except BaseException:
            Logger.Error("InfParser", 
                         ToolError.FILE_READ_FAILURE, 
                         ST.ERR_FILE_OPEN_FAILURE,
                         File=FullFileName)
        
        FileLinesList = ProcessLineExtender(FileLinesList)
    
    return FileLinesList
    