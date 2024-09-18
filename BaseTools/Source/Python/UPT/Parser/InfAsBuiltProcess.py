## @file
# This file is used to provide method for process AsBuilt INF file. It will consumed by InfParser
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
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

from Library.StringUtils import GetSplitValueList
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
def GetLibInstanceInfo(String, WorkSpace, LineNo, CurrentInfFileName):

    FileGuidString = ""
    VerString = ""

    OriginalString = String
    String = String.strip()
    if not String:
        return None, None
    #
    # Remove "#" characters at the beginning
    #
    String = GetHelpStringByRemoveHashKey(String)
    String = String.strip()

    #
    # To deal with library instance specified by GUID and version
    #
    RegFormatGuidPattern = re.compile(r"\s*([0-9a-fA-F]){8}-"
                                       "([0-9a-fA-F]){4}-"
                                       "([0-9a-fA-F]){4}-"
                                       "([0-9a-fA-F]){4}-"
                                       r"([0-9a-fA-F]){12}\s*")
    VersionPattern = re.compile(r'[\t\s]*\d+(\.\d+)?[\t\s]*')
    GuidMatchedObj = RegFormatGuidPattern.search(String)

    if String.upper().startswith('GUID') and GuidMatchedObj and 'Version' in String:
        VersionStr = String[String.upper().find('VERSION') + 8:]
        VersionMatchedObj = VersionPattern.search(VersionStr)
        if VersionMatchedObj:
            Guid = GuidMatchedObj.group().strip()
            Version = VersionMatchedObj.group().strip()
            return Guid, Version

    #
    # To deal with library instance specified by file name
    #
    FileLinesList = GetFileLineContent(String, WorkSpace, LineNo, OriginalString)


    ReFindFileGuidPattern = re.compile(r"^\s*FILE_GUID\s*=.*$")
    ReFindVerStringPattern = re.compile(r"^\s*VERSION_STRING\s*=.*$")

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

    RePackageHeader = re.compile(r'^\s*\[Packages.*\].*$')
    ReDefineHeader = re.compile(r'^\s*\[Defines].*$')

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

            if Name is not None:
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
            if Name is not None:
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
        return []

    #
    # Validate file exist/format.
    #
    if not IsValidPath(FileName, WorkSpace):
        return []

    FileLinesList = []

    try:
        FullFileName = FullFileName.replace('\\', '/')
        Inputfile = open(FullFileName, "r")
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

##
# Get all INF files from current workspace
#
#
def GetInfsFromWorkSpace(WorkSpace):
    InfFiles = []
    for top, dirs, files in os.walk(WorkSpace):
        dirs = dirs # just for pylint
        for File in files:
            if File.upper().endswith(".INF"):
                InfFiles.append(os.path.join(top, File))

    return InfFiles

##
# Get GUID and version from library instance file
#
#
def GetGuidVerFormLibInstance(Guid, Version, WorkSpace, CurrentInfFileName):
    for InfFile in GetInfsFromWorkSpace(WorkSpace):
        try:
            if InfFile.strip().upper() == CurrentInfFileName.strip().upper():
                continue
            InfFile = InfFile.replace('\\', '/')
            if InfFile not in GlobalData.gLIBINSTANCEDICT:
                InfFileObj = open(InfFile, "r")
                GlobalData.gLIBINSTANCEDICT[InfFile] = InfFileObj
            else:
                InfFileObj = GlobalData.gLIBINSTANCEDICT[InfFile]

        except BaseException:
            Logger.Error("InfParser",
                         ToolError.FILE_READ_FAILURE,
                         ST.ERR_FILE_OPEN_FAILURE,
                         File=InfFile)
        try:
            FileLinesList = InfFileObj.readlines()
            FileLinesList = ProcessLineExtender(FileLinesList)

            ReFindFileGuidPattern = re.compile(r"^\s*FILE_GUID\s*=.*$")
            ReFindVerStringPattern = re.compile(r"^\s*VERSION_STRING\s*=.*$")

            for Line in FileLinesList:
                if ReFindFileGuidPattern.match(Line):
                    FileGuidString = Line
                if ReFindVerStringPattern.match(Line):
                    VerString = Line

            if FileGuidString:
                FileGuidString = GetSplitValueList(FileGuidString, '=', 1)[1]
            if VerString:
                VerString = GetSplitValueList(VerString, '=', 1)[1]

            if FileGuidString.strip().upper() == Guid.upper() and \
                VerString.strip().upper() == Version.upper():
                return Guid, Version

        except BaseException:
            Logger.Error("InfParser", ToolError.FILE_READ_FAILURE, ST.ERR_FILE_OPEN_FAILURE, File=InfFile)
        finally:
            InfFileObj.close()

    return '', ''


