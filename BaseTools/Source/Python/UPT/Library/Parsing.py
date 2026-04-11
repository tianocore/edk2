## @file
# This file is used to define common parsing related functions used in parsing
# INF/DEC/DSC process
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
Parsing
'''
from __future__ import absolute_import

##
# Import Modules
#
import os.path
import re

from Library.StringUtils import RaiseParserError
from Library.StringUtils import GetSplitValueList
from Library.StringUtils import CheckFileType
from Library.StringUtils import CheckFileExist
from Library.StringUtils import CleanString
from Library.StringUtils import NormPath

from Logger.ToolError import FILE_NOT_FOUND
from Logger.ToolError import FatalError
from Logger.ToolError import FORMAT_INVALID

from Library import DataType

from Library.Misc import GuidStructureStringToGuidString
from Library.Misc import CheckGuidRegFormat
from Logger import StringTable as ST
import Logger.Log as Logger

from Parser.DecParser import Dec
from . import GlobalData

gPKG_INFO_DICT = {}

## Get Library Class
#
# Get Library of Dsc as <LibraryClassKeyWord>|<LibraryInstance>
#
# @param Item:           String as <LibraryClassKeyWord>|<LibraryInstance>
# @param ContainerFile:  The file which describes the library class, used for
#                        error report
#
def GetLibraryClass(Item, ContainerFile, WorkspaceDir, LineNo= -1):
    List = GetSplitValueList(Item[0])
    SupMod = DataType.SUP_MODULE_LIST_STRING
    if len(List) != 2:
        RaiseParserError(Item[0], 'LibraryClasses', ContainerFile, \
                         '<LibraryClassKeyWord>|<LibraryInstance>')
    else:
        CheckFileType(List[1], '.Inf', ContainerFile, \
                      'library class instance', Item[0], LineNo)
        CheckFileExist(WorkspaceDir, List[1], ContainerFile, \
                       'LibraryClasses', Item[0], LineNo)
        if Item[1] != '':
            SupMod = Item[1]

    return (List[0], List[1], SupMod)

## CheckPcdTokenInfo
#
# Check if PcdTokenInfo is following <TokenSpaceGuidCName>.<PcdCName>
#
# @param TokenInfoString:  String to be checked
# @param Section:          Used for error report
# @param File:             Used for error report
#
def CheckPcdTokenInfo(TokenInfoString, Section, File, LineNo= -1):
    Format = '<TokenSpaceGuidCName>.<PcdCName>'
    if TokenInfoString != '' and TokenInfoString is not None:
        TokenInfoList = GetSplitValueList(TokenInfoString, DataType.TAB_SPLIT)
        if len(TokenInfoList) == 2:
            return True

    RaiseParserError(TokenInfoString, Section, File, Format, LineNo)

## Get Binary
#
# Get Binary of Inf as <Filename>[|<Family>[|<TagName>[|<ToolCode>
# [|<PcdFeatureFlag>]]]]
#
# @param Item:           String as <Filename>[|<Family>[|<TagName>
#                        [|<ToolCode>[|<PcdFeatureFlag>]]]]
# @param ContainerFile:  The file which describes the library class,
#                        used for error report
#
def GetBinary(Item, ContainerFile, LineNo= -1):
    ItemNew = Item + DataType.TAB_VALUE_SPLIT
    List = GetSplitValueList(ItemNew)
    if len(List) < 3 or len(List) > 5:
        RaiseParserError(Item, 'Binaries', ContainerFile, \
                         "<FileType>|<Filename>[|<Target>\
                         [|<TokenSpaceGuidCName>.<PcdCName>]]", LineNo)

    if len(List) >= 4:
        if List[3] != '':
            CheckPcdTokenInfo(List[3], 'Binaries', ContainerFile, LineNo)
        return (List[0], List[1], List[2], List[3])
    elif len(List) == 3:
        return (List[0], List[1], List[2], '')

## GetPackage
#
# Get Package of Inf as <PackagePath>[|<PcdFeatureFlag>]
#
# @param Item:           String as <PackagePath>[|<PcdFeatureFlag>]
# @param Type:           Type of parsing string
# @param ContainerFile:  The file which describes the library class,
#                        used for error report
#
def GetPackage(Item, ContainerFile, FileRelativePath, LineNo= -1):
    ItemNew = Item + DataType.TAB_VALUE_SPLIT
    List = GetSplitValueList(ItemNew)
    CheckFileType(List[0], '.Dec', ContainerFile, 'package', List[0], LineNo)
    CheckFileExist(FileRelativePath, List[0], ContainerFile, 'Packages', \
                   List[0], LineNo)
    if List[1] != '':
        CheckPcdTokenInfo(List[1], 'Packages', ContainerFile, LineNo)

    return (List[0], List[1])

## Parse DEFINE statement
#
# Get DEFINE macros
#
# @param LineValue:  A DEFINE line value
# @param StartLine:  A DEFINE start line
# @param Table:      A table
# @param FileID:     File ID
# @param Filename:   File name
# @param SectionName:  DEFINE section name
# @param SectionModel:  DEFINE section model
# @param Arch:   DEFINE arch
#
def ParseDefine(LineValue, StartLine, Table, FileID, SectionName, \
                SectionModel, Arch):
    Logger.Debug(Logger.DEBUG_2, ST.MSG_DEFINE_STATEMENT_FOUND % (LineValue, \
                                                                  SectionName))
    Define = \
    GetSplitValueList(CleanString\
                      (LineValue[LineValue.upper().\
                                 find(DataType.TAB_DEFINE.upper() + ' ') + \
                                 len(DataType.TAB_DEFINE + ' ') : ]), \
                                 DataType.TAB_EQUAL_SPLIT, 1)
    Table.Insert(DataType.MODEL_META_DATA_DEFINE, Define[0], Define[1], '', \
                 '', '', Arch, SectionModel, FileID, StartLine, -1, \
                 StartLine, -1, 0)

## GetPkgInfoFromDec
#
# get package name, guid, version info from dec files
#
# @param Path:   File path
#
def GetPkgInfoFromDec(Path):
    PkgName = None
    PkgGuid = None
    PkgVersion = None

    Path = Path.replace('\\', '/')

    if not os.path.exists(Path):
        Logger.Error("\nUPT", FILE_NOT_FOUND, File=Path)

    if Path in gPKG_INFO_DICT:
        return gPKG_INFO_DICT[Path]

    try:
        DecParser = None
        if Path not in GlobalData.gPackageDict:
            DecParser = Dec(Path)
            GlobalData.gPackageDict[Path] = DecParser
        else:
            DecParser = GlobalData.gPackageDict[Path]

        PkgName = DecParser.GetPackageName()
        PkgGuid = DecParser.GetPackageGuid()
        PkgVersion = DecParser.GetPackageVersion()
        gPKG_INFO_DICT[Path] = (PkgName, PkgGuid, PkgVersion)
        return PkgName, PkgGuid, PkgVersion
    except FatalError:
        return None, None, None


## GetWorkspacePackage
#
# Get a list of workspace package information.
#
def GetWorkspacePackage():
    DecFileList = []
    WorkspaceDir = GlobalData.gWORKSPACE
    PackageDir = GlobalData.gPACKAGE_PATH
    for PkgRoot in [WorkspaceDir] + PackageDir:
        for Root, Dirs, Files in os.walk(PkgRoot):
            if 'CVS' in Dirs:
                Dirs.remove('CVS')
            if '.svn' in Dirs:
                Dirs.remove('.svn')
            for Dir in Dirs:
                if Dir.startswith('.'):
                    Dirs.remove(Dir)
            for FileSp in Files:
                if FileSp.startswith('.'):
                    continue
                Ext = os.path.splitext(FileSp)[1]
                if Ext.lower() in ['.dec']:
                    DecFileList.append\
                    (os.path.normpath(os.path.join(Root, FileSp)))
    #
    # abstract package guid, version info from DecFile List
    #
    PkgList = []
    for DecFile in DecFileList:
        (PkgName, PkgGuid, PkgVersion) = GetPkgInfoFromDec(DecFile)
        if PkgName and PkgGuid and PkgVersion:
            PkgList.append((PkgName, PkgGuid, PkgVersion, DecFile))

    return PkgList

## GetWorkspaceModule
#
# Get a list of workspace modules.
#
def GetWorkspaceModule():
    InfFileList = []
    WorkspaceDir = GlobalData.gWORKSPACE
    for Root, Dirs, Files in os.walk(WorkspaceDir):
        if 'CVS' in Dirs:
            Dirs.remove('CVS')
        if '.svn' in Dirs:
            Dirs.remove('.svn')
        if 'Build' in Dirs:
            Dirs.remove('Build')
        for Dir in Dirs:
            if Dir.startswith('.'):
                Dirs.remove(Dir)
        for FileSp in Files:
            if FileSp.startswith('.'):
                continue
            Ext = os.path.splitext(FileSp)[1]
            if Ext.lower() in ['.inf']:
                InfFileList.append\
                (os.path.normpath(os.path.join(Root, FileSp)))

    return InfFileList

## MacroParser used to parse macro definition
#
# @param Line:            The content contain linestring and line number
# @param FileName:        The meta-file file name
# @param SectionType:     Section for the Line belong to
# @param FileLocalMacros: A list contain Macro defined in [Defines] section.
#
def MacroParser(Line, FileName, SectionType, FileLocalMacros):
    MacroDefPattern = re.compile("^(DEFINE)[ \t]+")
    LineContent = Line[0]
    LineNo = Line[1]
    Match = MacroDefPattern.match(LineContent)
    if not Match:
        #
        # Not 'DEFINE/EDK_GLOBAL' statement, call decorated method
        #
        return None, None

    TokenList = GetSplitValueList(LineContent[Match.end(1):], \
                                  DataType.TAB_EQUAL_SPLIT, 1)
    #
    # Syntax check
    #
    if not TokenList[0]:
        Logger.Error('Parser', FORMAT_INVALID, ST.ERR_MACRONAME_NOGIVEN,
                        ExtraData=LineContent, File=FileName, Line=LineNo)
    if len(TokenList) < 2:
        Logger.Error('Parser', FORMAT_INVALID, ST.ERR_MACROVALUE_NOGIVEN,
                        ExtraData=LineContent, File=FileName, Line=LineNo)

    Name, Value = TokenList

    #
    # DEFINE defined macros
    #
    if SectionType == DataType.MODEL_META_DATA_HEADER:
        FileLocalMacros[Name] = Value

    ReIsValidMacroName = re.compile(r"^[A-Z][A-Z0-9_]*$", re.DOTALL)
    if ReIsValidMacroName.match(Name) is None:
        Logger.Error('Parser',
                     FORMAT_INVALID,
                     ST.ERR_MACRONAME_INVALID % (Name),
                     ExtraData=LineContent,
                     File=FileName,
                     Line=LineNo)

    # Validate MACRO Value
    #
    # <MacroDefinition> ::=  [<Comments>]{0,}
    #                       "DEFINE" <MACRO> "=" [{<PATH>} {<VALUE>}] <EOL>
    # <Value>           ::=  {<NumVal>} {<Boolean>} {<AsciiString>} {<GUID>}
    #                        {<CString>} {<UnicodeString>} {<CArray>}
    #
    # The definition of <NumVal>, <PATH>, <Boolean>, <GUID>, <CString>,
    # <UnicodeString>, <CArray> are subset of <AsciiString>.
    #
    ReIsValidMacroValue = re.compile(r"^[\x20-\x7e]*$", re.DOTALL)
    if ReIsValidMacroValue.match(Value) is None:
        Logger.Error('Parser',
                     FORMAT_INVALID,
                     ST.ERR_MACROVALUE_INVALID % (Value),
                     ExtraData=LineContent,
                     File=FileName,
                     Line=LineNo)

    return Name, Value

## GenSection
#
# generate section contents
#
# @param  SectionName:  indicate the name of the section, details refer to
#                       INF, DEC specs
# @param  SectionDict:  section statement dict, key is SectionAttrs(arch,
#                       moduletype or platform may exist as needed) list
#                       separated by space,
#                       value is statement
#
def GenSection(SectionName, SectionDict, SplitArch=True, NeedBlankLine=False):
    Content = ''
    for SectionAttrs in SectionDict:
        StatementList = SectionDict[SectionAttrs]
        if SectionAttrs and SectionName != 'Defines' and SectionAttrs.strip().upper() != DataType.TAB_ARCH_COMMON:
            if SplitArch:
                ArchList = GetSplitValueList(SectionAttrs, DataType.TAB_SPACE_SPLIT)
            else:
                if SectionName != 'UserExtensions':
                    ArchList = GetSplitValueList(SectionAttrs, DataType.TAB_COMMENT_SPLIT)
                else:
                    ArchList = [SectionAttrs]
            for Index in range(0, len(ArchList)):
                ArchList[Index] = ConvertArchForInstall(ArchList[Index])
            Section = '[' + SectionName + '.' + (', ' + SectionName + '.').join(ArchList) + ']'
        else:
            Section = '[' + SectionName + ']'
        Content += '\n' + Section + '\n'
        if StatementList is not None:
            for Statement in StatementList:
                LineList = Statement.split('\n')
                NewStatement = ""
                for Line in LineList:
                    # ignore blank comment
                    if not Line.replace("#", '').strip() and SectionName not in ('Defines', 'Hob', 'Event', 'BootMode'):
                        continue
                    # add two space before non-comments line except the comments in Defines section
                    if Line.strip().startswith('#') and SectionName == 'Defines':
                        NewStatement += "%s\n" % Line
                        continue
                    NewStatement += "  %s\n" % Line
                if NeedBlankLine:
                    Content += NewStatement + '\n'
                else:
                    Content += NewStatement

        if NeedBlankLine:
            Content = Content[:-1]
    if not Content.replace('\\n', '').strip():
        return ''
    return Content

## ConvertArchForInstall
# if Arch.upper() is in "IA32", "X64", "IPF", and "EBC", it must be upper case.  "common" must be lower case.
# Anything else, the case must be preserved
#
# @param Arch: the arch string that need to be converted, it should be stripped before pass in
# @return: the arch string that get converted
#
def ConvertArchForInstall(Arch):
    if Arch.upper() in [DataType.TAB_ARCH_IA32, DataType.TAB_ARCH_X64,
                                   DataType.TAB_ARCH_IPF, DataType.TAB_ARCH_EBC]:
        Arch = Arch.upper()
    elif Arch.upper() == DataType.TAB_ARCH_COMMON:
        Arch = Arch.lower()

    return Arch
