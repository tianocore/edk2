## @file
# This file is used to define common parsing related functions used in parsing 
# INF/DEC/DSC process
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
#

'''
Parsing
'''

##
# Import Modules
#
import os.path
import re

from Library.String import RaiseParserError
from Library.String import GetSplitValueList
from Library.String import CheckFileType
from Library.String import CheckFileExist
from Library.String import CleanString
from Library.String import NormPath

from Logger.ToolError import FILE_NOT_FOUND
from Logger.ToolError import FatalError
from Logger.ToolError import FORMAT_INVALID

from Library import DataType

from Library.Misc import GuidStructureStringToGuidString
from Library.Misc import CheckGuidRegFormat
from Logger import StringTable as ST
import Logger.Log as Logger

from Parser.DecParser import Dec

gPKG_INFO_DICT = {}

## GetBuildOption
#
# Parse a string with format "[<Family>:]<ToolFlag>=Flag"
# Return (Family, ToolFlag, Flag)
#
# @param String:  String with BuildOption statement
# @param File:    The file which defines build option, used in error report
#
def GetBuildOption(String, File, LineNo=-1):
    (Family, ToolChain, Flag) = ('', '', '')
    if String.find(DataType.TAB_EQUAL_SPLIT) < 0:
        RaiseParserError(String, 'BuildOptions', File, \
                         '[<Family>:]<ToolFlag>=Flag', LineNo)
    else:
        List = GetSplitValueList(String, DataType.TAB_EQUAL_SPLIT, MaxSplit=1)
        if List[0].find(':') > -1:
            Family = List[0][ : List[0].find(':')].strip()
            ToolChain = List[0][List[0].find(':') + 1 : ].strip()
        else:
            ToolChain = List[0].strip()
        Flag = List[1].strip()
    return (Family, ToolChain, Flag)

## Get Library Class
#
# Get Library of Dsc as <LibraryClassKeyWord>|<LibraryInstance>
#
# @param Item:           String as <LibraryClassKeyWord>|<LibraryInstance>
# @param ContainerFile:  The file which describes the library class, used for 
#                        error report
#
def GetLibraryClass(Item, ContainerFile, WorkspaceDir, LineNo=-1):
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

## Get Library Class
#
# Get Library of Dsc as <LibraryClassKeyWord>[|<LibraryInstance>]
# [|<TokenSpaceGuidCName>.<PcdCName>]
#
# @param Item:           String as <LibraryClassKeyWord>|<LibraryInstance>
# @param ContainerFile:  The file which describes the library class, used for 
#                        error report
#
def GetLibraryClassOfInf(Item, ContainerFile, WorkspaceDir, LineNo = -1):
    ItemList = GetSplitValueList((Item[0] + DataType.TAB_VALUE_SPLIT * 2))
    SupMod = DataType.SUP_MODULE_LIST_STRING

    if len(ItemList) > 5:
        RaiseParserError\
        (Item[0], 'LibraryClasses', ContainerFile, \
         '<LibraryClassKeyWord>[|<LibraryInstance>]\
         [|<TokenSpaceGuidCName>.<PcdCName>]')
    else:
        CheckFileType(ItemList[1], '.Inf', ContainerFile, 'LibraryClasses', \
                      Item[0], LineNo)
        CheckFileExist(WorkspaceDir, ItemList[1], ContainerFile, \
                       'LibraryClasses', Item[0], LineNo)
        if ItemList[2] != '':
            CheckPcdTokenInfo(ItemList[2], 'LibraryClasses', \
                              ContainerFile, LineNo)
        if Item[1] != '':
            SupMod = Item[1]

    return (ItemList[0], ItemList[1], ItemList[2], SupMod)

## CheckPcdTokenInfo
#
# Check if PcdTokenInfo is following <TokenSpaceGuidCName>.<PcdCName>
#
# @param TokenInfoString:  String to be checked
# @param Section:          Used for error report
# @param File:             Used for error report
#
def CheckPcdTokenInfo(TokenInfoString, Section, File, LineNo=-1):
    Format = '<TokenSpaceGuidCName>.<PcdCName>'
    if TokenInfoString != '' and TokenInfoString != None:
        TokenInfoList = GetSplitValueList(TokenInfoString, DataType.TAB_SPLIT)
        if len(TokenInfoList) == 2:
            return True

    RaiseParserError(TokenInfoString, Section, File, Format, LineNo)

## Get Pcd
#
# Get Pcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|<Value>
# [|<Type>|<MaximumDatumSize>]
#
# @param Item:           String as <PcdTokenSpaceGuidCName>.<TokenCName>|
#                        <Value>[|<Type>|<MaximumDatumSize>]
# @param ContainerFile:  The file which describes the pcd, used for error 
#                        report

#
def GetPcd(Item, Type, ContainerFile, LineNo=-1):
    TokenGuid, TokenName, Value, MaximumDatumSize, Token = '', '', '', '', ''
    List = GetSplitValueList(Item + DataType.TAB_VALUE_SPLIT * 2)

    if len(List) < 4 or len(List) > 6:
        RaiseParserError(Item, 'Pcds' + Type, ContainerFile, \
                         '<PcdTokenSpaceGuidCName>.<TokenCName>|<Value>\
                         [|<Type>|<MaximumDatumSize>]', LineNo)
    else:
        Value = List[1]
        MaximumDatumSize = List[2]
        Token = List[3]

    if CheckPcdTokenInfo(List[0], 'Pcds' + Type, ContainerFile, LineNo):
        (TokenGuid, TokenName) = GetSplitValueList(List[0], DataType.TAB_SPLIT)

    return (TokenName, TokenGuid, Value, MaximumDatumSize, Token, Type)

## Get FeatureFlagPcd
#
# Get FeatureFlagPcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|TRUE/FALSE
#
# @param Item:           String as <PcdTokenSpaceGuidCName>
#                        .<TokenCName>|TRUE/FALSE
# @param ContainerFile:  The file which describes the pcd, used for error 
#                        report
#
def GetFeatureFlagPcd(Item, Type, ContainerFile, LineNo=-1):
    TokenGuid, TokenName, Value = '', '', ''
    List = GetSplitValueList(Item)
    if len(List) != 2:
        RaiseParserError(Item, 'Pcds' + Type, ContainerFile, \
                         '<PcdTokenSpaceGuidCName>.<TokenCName>|TRUE/FALSE', \
                         LineNo)
    else:
        Value = List[1]
    if CheckPcdTokenInfo(List[0], 'Pcds' + Type, ContainerFile, LineNo):
        (TokenGuid, TokenName) = GetSplitValueList(List[0], DataType.TAB_SPLIT)

    return (TokenName, TokenGuid, Value, Type)

## Get DynamicDefaultPcd
#
# Get DynamicDefaultPcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>
# |<Value>[|<DatumTyp>[|<MaxDatumSize>]]
#
# @param Item:           String as <PcdTokenSpaceGuidCName>.<TokenCName>|
#                        TRUE/FALSE
# @param ContainerFile:  The file which describes the pcd, used for error 
#                        report
#
def GetDynamicDefaultPcd(Item, Type, ContainerFile, LineNo=-1):
    TokenGuid, TokenName, Value, DatumTyp, MaxDatumSize = '', '', '', '', ''
    List = GetSplitValueList(Item + DataType.TAB_VALUE_SPLIT * 2)
    if len(List) < 4 or len(List) > 8:
        RaiseParserError(Item, 'Pcds' + Type, ContainerFile, \
                         '<PcdTokenSpaceGuidCName>.<TokenCName>|<Value>\
                         [|<DatumTyp>[|<MaxDatumSize>]]', LineNo)
    else:
        Value = List[1]
        DatumTyp = List[2]
        MaxDatumSize = List[3]
    if CheckPcdTokenInfo(List[0], 'Pcds' + Type, ContainerFile, LineNo):
        (TokenGuid, TokenName) = GetSplitValueList(List[0], DataType.TAB_SPLIT)

    return (TokenName, TokenGuid, Value, DatumTyp, MaxDatumSize, Type)

## Get DynamicHiiPcd
#
# Get DynamicHiiPcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|<String>|
# <VariableGuidCName>|<VariableOffset>[|<DefaultValue>[|<MaximumDatumSize>]]
#
# @param Item:           String as <PcdTokenSpaceGuidCName>.<TokenCName>|
#                        TRUE/FALSE
# @param ContainerFile:  The file which describes the pcd, used for error 
#                        report
#
def GetDynamicHiiPcd(Item, Type, ContainerFile, LineNo = -1):
    TokenGuid, TokenName, List1, List2, List3, List4, List5 = \
    '', '', '', '', '', '', ''
    List = GetSplitValueList(Item + DataType.TAB_VALUE_SPLIT * 2)
    if len(List) < 6 or len(List) > 8:
        RaiseParserError(Item, 'Pcds' + Type, ContainerFile, \
                         '<PcdTokenSpaceGuidCName>.<TokenCName>|<String>|\
                         <VariableGuidCName>|<VariableOffset>[|<DefaultValue>\
                         [|<MaximumDatumSize>]]', LineNo)
    else:
        List1, List2, List3, List4, List5 = \
        List[1], List[2], List[3], List[4], List[5]
    if CheckPcdTokenInfo(List[0], 'Pcds' + Type, ContainerFile, LineNo):
        (TokenGuid, TokenName) = GetSplitValueList(List[0], DataType.TAB_SPLIT)

    return (TokenName, TokenGuid, List1, List2, List3, List4, List5, Type)

## Get DynamicVpdPcd
#
# Get DynamicVpdPcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|
# <VpdOffset>[|<MaximumDatumSize>]
#
# @param Item:           String as <PcdTokenSpaceGuidCName>.<TokenCName>
#                        |TRUE/FALSE
# @param ContainerFile:  The file which describes the pcd, used for error 
#                        report
#
def GetDynamicVpdPcd(Item, Type, ContainerFile, LineNo=-1):
    TokenGuid, TokenName, List1, List2 = '', '', '', ''
    List = GetSplitValueList(Item + DataType.TAB_VALUE_SPLIT)
    if len(List) < 3 or len(List) > 4:
        RaiseParserError(Item, 'Pcds' + Type, ContainerFile, \
                         '<PcdTokenSpaceGuidCName>.<TokenCName>|<VpdOffset>\
                         [|<MaximumDatumSize>]', LineNo)
    else:
        List1, List2 = List[1], List[2]
    if CheckPcdTokenInfo(List[0], 'Pcds' + Type, ContainerFile, LineNo):
        (TokenGuid, TokenName) = GetSplitValueList(List[0], DataType.TAB_SPLIT)

    return (TokenName, TokenGuid, List1, List2, Type)

## GetComponent
#
# Parse block of the components defined in dsc file
# Set KeyValues as [ ['component name', [lib1, lib2, lib3], 
# [bo1, bo2, bo3], [pcd1, pcd2, pcd3]], ...]
#
# @param Lines:             The content to be parsed
# @param KeyValues:         To store data after parsing
#
def GetComponent(Lines, KeyValues):
    (FindBlock, FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
     FindPcdsPatchableInModule, FindPcdsFixedAtBuild, FindPcdsDynamic, \
     FindPcdsDynamicEx) = (False, False, False, False, False, False, False, \
                           False)
    ListItem = None
    LibraryClassItem = []
    BuildOption = []
    Pcd = []

    for Line in Lines:
        Line = Line[0]
        #
        # Ignore !include statement
        #
        if Line.upper().find(DataType.TAB_INCLUDE.upper() + ' ') > -1 or \
        Line.upper().find(DataType.TAB_DEFINE + ' ') > -1:
            continue

        if FindBlock == False:
            ListItem = Line
            #
            # find '{' at line tail
            #
            if Line.endswith('{'):
                FindBlock = True
                ListItem = CleanString(Line.rsplit('{', 1)[0], \
                                       DataType.TAB_COMMENT_SPLIT)

        #
        # Parse a block content
        #
        if FindBlock:
            if Line.find('<LibraryClasses>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (True, False, False, False, False, False, False)
                continue
            if Line.find('<BuildOptions>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, True, False, False, False, False, False)
                continue
            if Line.find('<PcdsFeatureFlag>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, True, False, False, False, False)
                continue
            if Line.find('<PcdsPatchableInModule>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, False, True, False, False, False)
                continue
            if Line.find('<PcdsFixedAtBuild>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, False, False, True, False, False)
                continue
            if Line.find('<PcdsDynamic>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, False, False, False, True, False)
                continue
            if Line.find('<PcdsDynamicEx>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, False, False, False, False, True)
                continue
            if Line.endswith('}'):
                #
                # find '}' at line tail
                #
                KeyValues.append([ListItem, LibraryClassItem, \
                                  BuildOption, Pcd])
                (FindBlock, FindLibraryClass, FindBuildOption, \
                 FindPcdsFeatureFlag, FindPcdsPatchableInModule, \
                 FindPcdsFixedAtBuild, FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, False, False, False, False, False, False)
                LibraryClassItem, BuildOption, Pcd = [], [], []
                continue

        if FindBlock:
            if FindLibraryClass:
                LibraryClassItem.append(Line)
            elif FindBuildOption:
                BuildOption.append(Line)
            elif FindPcdsFeatureFlag:
                Pcd.append((DataType.TAB_PCDS_FEATURE_FLAG_NULL, Line))
            elif FindPcdsPatchableInModule:
                Pcd.append((DataType.TAB_PCDS_PATCHABLE_IN_MODULE_NULL, Line))
            elif FindPcdsFixedAtBuild:
                Pcd.append((DataType.TAB_PCDS_FIXED_AT_BUILD_NULL, Line))
            elif FindPcdsDynamic:
                Pcd.append((DataType.TAB_PCDS_DYNAMIC_DEFAULT_NULL, Line))
            elif FindPcdsDynamicEx:
                Pcd.append((DataType.TAB_PCDS_DYNAMIC_EX_DEFAULT_NULL, Line))
        else:
            KeyValues.append([ListItem, [], [], []])

    return True

## GetExec
#
# Parse a string with format "InfFilename [EXEC = ExecFilename]"
# Return (InfFilename, ExecFilename)
#
# @param String:  String with EXEC statement
#
def GetExec(String):
    InfFilename = ''
    ExecFilename = ''
    if String.find('EXEC') > -1:
        InfFilename = String[ : String.find('EXEC')].strip()
        ExecFilename = String[String.find('EXEC') + len('EXEC') : ].strip()
    else:
        InfFilename = String.strip()

    return (InfFilename, ExecFilename)

## GetComponents
#
# Parse block of the components defined in dsc file
# Set KeyValues as [ ['component name', [lib1, lib2, lib3], [bo1, bo2, bo3], 
# [pcd1, pcd2, pcd3]], ...]
#
# @param Lines:             The content to be parsed
# @param Key:               Reserved
# @param KeyValues:         To store data after parsing
# @param CommentCharacter:  Comment char, used to ignore comment content
#
# @retval True Get component successfully
#
def GetComponents(Lines, KeyValues, CommentCharacter):
    if Lines.find(DataType.TAB_SECTION_END) > -1:
        Lines = Lines.split(DataType.TAB_SECTION_END, 1)[1]
    (FindBlock, FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
     FindPcdsPatchableInModule, FindPcdsFixedAtBuild, FindPcdsDynamic, \
     FindPcdsDynamicEx) = \
     (False, False, False, False, False, False, False, False)
    ListItem = None
    LibraryClassItem = []
    BuildOption = []
    Pcd = []

    LineList = Lines.split('\n')
    for Line in LineList:
        Line = CleanString(Line, CommentCharacter)
        if Line == None or Line == '':
            continue

        if FindBlock == False:
            ListItem = Line
            #
            # find '{' at line tail
            #
            if Line.endswith('{'):
                FindBlock = True
                ListItem = CleanString(Line.rsplit('{', 1)[0], CommentCharacter)

        #
        # Parse a block content
        #
        if FindBlock:
            if Line.find('<LibraryClasses>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (True, False, False, False, False, False, False)
                continue
            if Line.find('<BuildOptions>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, True, False, False, False, False, False)
                continue
            if Line.find('<PcdsFeatureFlag>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, True, False, False, False, False)
                continue
            if Line.find('<PcdsPatchableInModule>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, False, True, False, False, False)
                continue
            if Line.find('<PcdsFixedAtBuild>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, False, False, True, False, False)
                continue
            if Line.find('<PcdsDynamic>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, False, False, False, True, False)
                continue
            if Line.find('<PcdsDynamicEx>') != -1:
                (FindLibraryClass, FindBuildOption, FindPcdsFeatureFlag, \
                 FindPcdsPatchableInModule, FindPcdsFixedAtBuild, \
                 FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, False, False, False, False, True)
                continue
            if Line.endswith('}'):
                #
                # find '}' at line tail
                #
                KeyValues.append([ListItem, LibraryClassItem, BuildOption, \
                                  Pcd])
                (FindBlock, FindLibraryClass, FindBuildOption, \
                 FindPcdsFeatureFlag, FindPcdsPatchableInModule, \
                 FindPcdsFixedAtBuild, FindPcdsDynamic, FindPcdsDynamicEx) = \
                 (False, False, False, False, False, False, False, False)
                LibraryClassItem, BuildOption, Pcd = [], [], []
                continue

        if FindBlock:
            if FindLibraryClass:
                LibraryClassItem.append(Line)
            elif FindBuildOption:
                BuildOption.append(Line)
            elif FindPcdsFeatureFlag:
                Pcd.append((DataType.TAB_PCDS_FEATURE_FLAG, Line))
            elif FindPcdsPatchableInModule:
                Pcd.append((DataType.TAB_PCDS_PATCHABLE_IN_MODULE, Line))
            elif FindPcdsFixedAtBuild:
                Pcd.append((DataType.TAB_PCDS_FIXED_AT_BUILD, Line))
            elif FindPcdsDynamic:
                Pcd.append((DataType.TAB_PCDS_DYNAMIC, Line))
            elif FindPcdsDynamicEx:
                Pcd.append((DataType.TAB_PCDS_DYNAMIC_EX, Line))
        else:
            KeyValues.append([ListItem, [], [], []])

    return True

## Get Source
#
# Get Source of Inf as <Filename>[|<Family>[|<TagName>[|<ToolCode>
# [|<PcdFeatureFlag>]]]]
#
# @param Item:           String as <Filename>[|<Family>[|<TagName>[|<ToolCode>
#                        [|<PcdFeatureFlag>]]]]
# @param ContainerFile:  The file which describes the library class, used 
#                        for error report
#
def GetSource(Item, ContainerFile, FileRelativePath, LineNo=-1):
    ItemNew = Item + DataType.TAB_VALUE_SPLIT * 4
    List = GetSplitValueList(ItemNew)
    if len(List) < 5 or len(List) > 9:
        RaiseParserError(Item, 'Sources', ContainerFile, \
                         '<Filename>[|<Family>[|<TagName>[|<ToolCode>\
                         [|<PcdFeatureFlag>]]]]', LineNo)
    List[0] = NormPath(List[0])
    CheckFileExist(FileRelativePath, List[0], ContainerFile, 'Sources', \
                   Item, LineNo)
    if List[4] != '':
        CheckPcdTokenInfo(List[4], 'Sources', ContainerFile, LineNo)

    return (List[0], List[1], List[2], List[3], List[4])

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
def GetBinary(Item, ContainerFile, LineNo=-1):
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

## Get Guids/Protocols/Ppis
#
# Get Guids/Protocols/Ppis of Inf as <GuidCName>[|<PcdFeatureFlag>]
#
# @param Item:           String as <GuidCName>[|<PcdFeatureFlag>]
# @param Type:           Type of parsing string
# @param ContainerFile:  The file which describes the library class, 
#                        used for error report
#
def GetGuidsProtocolsPpisOfInf(Item):
    ItemNew = Item + DataType.TAB_VALUE_SPLIT
    List = GetSplitValueList(ItemNew)
    return (List[0], List[1])

## Get Guids/Protocols/Ppis
#
# Get Guids/Protocols/Ppis of Dec as <GuidCName>=<GuidValue>
#
# @param Item:           String as <GuidCName>=<GuidValue>
# @param Type:           Type of parsing string
# @param ContainerFile:  The file which describes the library class, 
# used for error report
#
def GetGuidsProtocolsPpisOfDec(Item, Type, ContainerFile, LineNo=-1):
    List = GetSplitValueList(Item, DataType.TAB_EQUAL_SPLIT)
    if len(List) != 2:
        RaiseParserError(Item, Type, ContainerFile, '<CName>=<GuidValue>', \
                         LineNo)
    #
    #convert C-Format Guid to Register Format
    #
    if List[1][0] == '{' and List[1][-1] == '}':
        RegisterFormatGuid = GuidStructureStringToGuidString(List[1])
        if RegisterFormatGuid == '':
            RaiseParserError(Item, Type, ContainerFile, \
                             'CFormat or RegisterFormat', LineNo)
    else:
        if CheckGuidRegFormat(List[1]):
            RegisterFormatGuid = List[1]
        else:
            RaiseParserError(Item, Type, ContainerFile, \
                             'CFormat or RegisterFormat', LineNo) 

    return (List[0], RegisterFormatGuid)

## GetPackage
#
# Get Package of Inf as <PackagePath>[|<PcdFeatureFlag>]
#
# @param Item:           String as <PackagePath>[|<PcdFeatureFlag>]
# @param Type:           Type of parsing string
# @param ContainerFile:  The file which describes the library class, 
#                        used for error report
#
def GetPackage(Item, ContainerFile, FileRelativePath, LineNo=-1):
    ItemNew = Item + DataType.TAB_VALUE_SPLIT
    List = GetSplitValueList(ItemNew)
    CheckFileType(List[0], '.Dec', ContainerFile, 'package', List[0], LineNo)
    CheckFileExist(FileRelativePath, List[0], ContainerFile, 'Packages', \
                   List[0], LineNo)
    if List[1] != '':
        CheckPcdTokenInfo(List[1], 'Packages', ContainerFile, LineNo)

    return (List[0], List[1])

## Get Pcd Values of Inf
#
# Get Pcd of Inf as <TokenSpaceGuidCName>.<PcdCName>[|<Value>]
#
# @param Item:  The string describes pcd
# @param Type:  The type of Pcd
# @param File:  The file which describes the pcd, used for error report
#
def GetPcdOfInf(Item, Type, File, LineNo):
    Format = '<TokenSpaceGuidCName>.<PcdCName>[|<Value>]'
    TokenGuid, TokenName, Value, InfType = '', '', '', ''

    if Type == DataType.TAB_PCDS_FIXED_AT_BUILD:
        InfType = DataType.TAB_INF_FIXED_PCD
    elif Type == DataType.TAB_PCDS_PATCHABLE_IN_MODULE:
        InfType = DataType.TAB_INF_PATCH_PCD
    elif Type == DataType.TAB_PCDS_FEATURE_FLAG:
        InfType = DataType.TAB_INF_FEATURE_PCD
    elif Type == DataType.TAB_PCDS_DYNAMIC_EX:
        InfType = DataType.TAB_INF_PCD_EX
    elif Type == DataType.TAB_PCDS_DYNAMIC:
        InfType = DataType.TAB_INF_PCD
    List = GetSplitValueList(Item, DataType.TAB_VALUE_SPLIT, 1)
    TokenInfo = GetSplitValueList(List[0], DataType.TAB_SPLIT)
    if len(TokenInfo) != 2:
        RaiseParserError(Item, InfType, File, Format, LineNo)
    else:
        TokenGuid = TokenInfo[0]
        TokenName = TokenInfo[1]

    if len(List) > 1:
        Value = List[1]
    else:
        Value = None
    return (TokenGuid, TokenName, Value, InfType)


## Get Pcd Values of Dec
#
# Get Pcd of Dec as <TokenSpcCName>.<TokenCName>|<Value>|<DatumType>|<Token>
# @param Item:  Pcd item
# @param Type:  Pcd type
# @param File:  Dec file
# @param LineNo:  Line number
#
def GetPcdOfDec(Item, Type, File, LineNo=-1):
    Format = '<TokenSpaceGuidCName>.<PcdCName>|<Value>|<DatumType>|<Token>'
    TokenGuid, TokenName, Value, DatumType, Token = '', '', '', '', ''
    List = GetSplitValueList(Item)
    if len(List) != 4:
        RaiseParserError(Item, 'Pcds' + Type, File, Format, LineNo)
    else:
        Value = List[1]
        DatumType = List[2]
        Token = List[3]
    TokenInfo = GetSplitValueList(List[0], DataType.TAB_SPLIT)
    if len(TokenInfo) != 2:
        RaiseParserError(Item, 'Pcds' + Type, File, Format, LineNo)
    else:
        TokenGuid = TokenInfo[0]
        TokenName = TokenInfo[1]

    return (TokenGuid, TokenName, Value, DatumType, Token, Type)

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

## InsertSectionItems
#
# Insert item data of a section to a dict
#
# @param Model:   A model
# @param CurrentSection:   Current section
# @param SectionItemList:   Section item list
# @param ArchList:   Arch list
# @param ThirdList:   Third list
# @param RecordSet:   Record set
#
def InsertSectionItems(Model, SectionItemList, ArchList, \
                       ThirdList, RecordSet):
    #
    # Insert each item data of a section
    #
    for Index in range(0, len(ArchList)):
        Arch = ArchList[Index]
        Third = ThirdList[Index]
        if Arch == '':
            Arch = DataType.TAB_ARCH_COMMON

        Records = RecordSet[Model]
        for SectionItem in SectionItemList:
            LineValue, StartLine, Comment = SectionItem[0], \
            SectionItem[1], SectionItem[2]

            Logger.Debug(4, ST.MSG_PARSING %LineValue)
            #
            # And then parse DEFINE statement
            #
            if LineValue.upper().find(DataType.TAB_DEFINE.upper() + ' ') > -1:
                continue
            #
            # At last parse other sections
            #
            IdNum = -1
            Records.append([LineValue, Arch, StartLine, IdNum, Third, Comment])

        if RecordSet != {}:
            RecordSet[Model] = Records

## GenMetaDatSectionItem
#
# @param Key:    A key
# @param Value:  A value
# @param List:   A list
#
def GenMetaDatSectionItem(Key, Value, List):
    if Key not in List:
        List[Key] = [Value]
    else:
        List[Key].append(Value)
        
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
        Logger.Error("\nUPT", FILE_NOT_FOUND, File = Path)  

    if Path in gPKG_INFO_DICT:
        return gPKG_INFO_DICT[Path]

    try:
        DecParser = Dec(Path)
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
    WorkspaceDir = os.environ["WORKSPACE"]
    for Root, Dirs, Files in os.walk(WorkspaceDir):
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
    WorkspaceDir = os.environ["WORKSPACE"]
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
    if ReIsValidMacroName.match(Name) == None:
        Logger.Error('Parser', 
                     FORMAT_INVALID, 
                     ST.ERR_MACRONAME_INVALID%(Name),
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
    if ReIsValidMacroValue.match(Value) == None:
        Logger.Error('Parser', 
                     FORMAT_INVALID, 
                     ST.ERR_MACROVALUE_INVALID%(Value),
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
#                       seperated by space, 
#                       value is statement
#
def GenSection(SectionName, SectionDict, SplitArch=True):
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
            for Index in xrange(0, len(ArchList)):
                ArchList[Index] = ConvertArchForInstall(ArchList[Index])
            Section = '[' + SectionName + '.' + (', ' + SectionName + '.').join(ArchList) + ']'
        else:
            Section = '[' + SectionName + ']'
        Content += '\n\n' + Section + '\n'
        if StatementList != None:
            for Statement in StatementList:
                Content += Statement + '\n'

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
