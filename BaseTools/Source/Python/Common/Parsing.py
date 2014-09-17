## @file
# This file is used to define common parsing related functions used in parsing INF/DEC/DSC process
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
from String import *
from CommonDataClass.DataClass import *
from DataType import *

## ParseDefineMacro
#
# Search whole table to find all defined Macro and replaced them with the real values
#
def ParseDefineMacro2(Table, RecordSets, GlobalMacro):
    Macros = {}
    #
    # Find all DEFINE macros in section [Header] and its section
    #
    SqlCommand = """select Value1, Value2, BelongsToItem, StartLine, Arch from %s
                    where Model = %s
                    and Enabled > -1""" % (Table.Table, MODEL_META_DATA_DEFINE)
    RecordSet = Table.Exec(SqlCommand)
    for Record in RecordSet:
        Macros[Record[0]] = Record[1]

    #
    # Overrided by Global Macros
    #
    for Key in GlobalMacro.keys():
        Macros[Key] = GlobalMacro[Key]

    #
    # Replace the Macros
    #
    for Key in RecordSets.keys():
        if RecordSets[Key] != []:
            for Item in RecordSets[Key]:
                Item[0] = ReplaceMacro(Item[0], Macros)

## ParseDefineMacro
#
# Search whole table to find all defined Macro and replaced them with the real values
#
def ParseDefineMacro(Table, GlobalMacro):
    Macros = {}
    #
    # Find all DEFINE macros
    #
    SqlCommand = """select Value1, Value2, BelongsToItem, StartLine, Arch from %s
                    where Model = %s
                    and Enabled > -1""" % (Table.Table, MODEL_META_DATA_DEFINE)
    RecordSet = Table.Exec(SqlCommand)
    for Record in RecordSet:
#***************************************************************************************************************************************************
#            The follow SqlCommand (expr replace) is not supported in Sqlite 3.3.4 which is used in Python 2.5                                     *
#            Reserved Only                                                                                                                         *
#            SqlCommand = """update %s set Value1 = replace(Value1, '%s', '%s')                                                                    *
#                            where ID in (select ID from %s                                                                                        *
#                                         where Model = %s                                                                                         *
#                                         and Value1 like '%%%s%%'                                                                                 *
#                                         and StartLine > %s                                                                                       *
#                                         and Enabled > -1                                                                                         *
#                                         and Arch = '%s')""" % \                                                                                  *
#                                         (self.TblDsc.Table, Record[0], Record[1], self.TblDsc.Table, Record[2], Record[1], Record[3], Record[4]) *
#***************************************************************************************************************************************************
        Macros[Record[0]] = Record[1]

    #
    # Overrided by Global Macros
    #
    for Key in GlobalMacro.keys():
        Macros[Key] = GlobalMacro[Key]

    #
    # Found all defined macro and replaced
    #
    SqlCommand = """select ID, Value1 from %s
                    where Model != %s
                    and Value1 like '%%$(%%' and Value1 like '%%)%%'
                    and Enabled > -1"""  % (Table.Table, MODEL_META_DATA_DEFINE)
    FoundRecords = Table.Exec(SqlCommand)
    for FoundRecord in FoundRecords:
        NewValue = ReplaceMacro(FoundRecord[1], Macros)
        SqlCommand = """update %s set Value1 = '%s'
                        where ID = %s""" % (Table.Table, ConvertToSqlString2(NewValue), FoundRecord[0])
        Table.Exec(SqlCommand)

##QueryDefinesItem
#
# Search item of section [Defines] by name, return its values
#
# @param Table: The Table to be executed
# @param Name:  The Name of item of section [Defines]
# @param Arch:  The Arch of item of section [Defines]
#
# @retval RecordSet: A list of all matched records
#
def QueryDefinesItem(Table, Name, Arch, BelongsToFile):
    SqlCommand = """select Value2 from %s
                    where Model = %s
                    and Value1 = '%s'
                    and Arch = '%s'
                    and BelongsToFile = %s
                    and Enabled > -1""" % (Table.Table, MODEL_META_DATA_HEADER, ConvertToSqlString2(Name), ConvertToSqlString2(Arch), BelongsToFile)
    RecordSet = Table.Exec(SqlCommand)
    if len(RecordSet) < 1:
        SqlCommand = """select Value2 from %s
                    where Model = %s
                    and Value1 = '%s'
                    and Arch = '%s'
                    and BelongsToFile = %s
                    and Enabled > -1""" % (Table.Table, MODEL_META_DATA_HEADER, ConvertToSqlString2(Name), ConvertToSqlString2(TAB_ARCH_COMMON.upper()), BelongsToFile)
        RecordSet = Table.Exec(SqlCommand)
    if len(RecordSet) == 1:
        if Name == TAB_INF_DEFINES_LIBRARY_CLASS:
            return [RecordSet[0][0]]
        else:
            return GetSplitValueList(RecordSet[0][0])
    elif len(RecordSet) < 1:
        return ['']
    elif len(RecordSet) > 1:
        RetVal = []
        for Record in RecordSet:
            if Name == TAB_INF_DEFINES_LIBRARY_CLASS:
                RetVal.append(Record[0])
            else:
                Items = GetSplitValueList(Record[0])
                for Item in Items:
                    RetVal.append(Item)
        return RetVal

##QueryDefinesItem
#
# Search item of section [Defines] by name, return its values
#
# @param Table: The Table to be executed
# @param Name:  The Name of item of section [Defines]
# @param Arch:  The Arch of item of section [Defines]
#
# @retval RecordSet: A list of all matched records
#
def QueryDefinesItem2(Table, Arch, BelongsToFile):
    SqlCommand = """select Value1, Value2, StartLine from %s
                    where Model = %s
                    and Arch = '%s'
                    and BelongsToFile = %s
                    and Enabled > -1""" % (Table.Table, MODEL_META_DATA_HEADER, ConvertToSqlString2(Arch), BelongsToFile)
    RecordSet = Table.Exec(SqlCommand)
    if len(RecordSet) < 1:
        SqlCommand = """select Value1, Value2, StartLine from %s
                    where Model = %s
                    and Arch = '%s'
                    and BelongsToFile = %s
                    and Enabled > -1""" % (Table.Table, MODEL_META_DATA_HEADER, ConvertToSqlString2(TAB_ARCH_COMMON), BelongsToFile)
        RecordSet = Table.Exec(SqlCommand)

    return RecordSet

##QueryDscItem
#
# Search all dsc item for a specific section
#
# @param Table: The Table to be executed
# @param Model:  The type of section
#
# @retval RecordSet: A list of all matched records
#
def QueryDscItem(Table, Model, BelongsToItem, BelongsToFile):
    SqlCommand = """select Value1, Arch, StartLine, ID, Value2 from %s
                    where Model = %s
                    and BelongsToItem = %s
                    and BelongsToFile = %s
                    and Enabled > -1""" % (Table.Table, Model, BelongsToItem, BelongsToFile)
    return Table.Exec(SqlCommand)

##QueryDecItem
#
# Search all dec item for a specific section
#
# @param Table: The Table to be executed
# @param Model:  The type of section
#
# @retval RecordSet: A list of all matched records
#
def QueryDecItem(Table, Model, BelongsToItem):
    SqlCommand = """select Value1, Arch, StartLine, ID, Value2 from %s
                    where Model = %s
                    and BelongsToItem = %s
                    and Enabled > -1""" % (Table.Table, Model, BelongsToItem)
    return Table.Exec(SqlCommand)

##QueryInfItem
#
# Search all dec item for a specific section
#
# @param Table: The Table to be executed
# @param Model: The type of section
#
# @retval RecordSet: A list of all matched records
#
def QueryInfItem(Table, Model, BelongsToItem):
    SqlCommand = """select Value1, Arch, StartLine, ID, Value2 from %s
                    where Model = %s
                    and BelongsToItem = %s
                    and Enabled > -1""" % (Table.Table, Model, BelongsToItem)
    return Table.Exec(SqlCommand)

## GetBuildOption
#
# Parse a string with format "[<Family>:]<ToolFlag>=Flag"
# Return (Family, ToolFlag, Flag)
#
# @param String:  String with BuildOption statement
# @param File:    The file which defines build option, used in error report
#
# @retval truple() A truple structure as (Family, ToolChain, Flag)
#
def GetBuildOption(String, File, LineNo = -1):
    (Family, ToolChain, Flag) = ('', '', '')
    if String.find(TAB_EQUAL_SPLIT) < 0:
        RaiseParserError(String, 'BuildOptions', File, '[<Family>:]<ToolFlag>=Flag', LineNo)
    else:
        List = GetSplitValueList(String, TAB_EQUAL_SPLIT, MaxSplit = 1)
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
# @param ContainerFile:  The file which describes the library class, used for error report
#
# @retval (LibraryClassKeyWord, LibraryInstance, [SUP_MODULE_LIST]) Formatted Library Item
#
def GetLibraryClass(Item, ContainerFile, WorkspaceDir, LineNo = -1):
    List = GetSplitValueList(Item[0])
    SupMod = SUP_MODULE_LIST_STRING
    if len(List) != 2:
        RaiseParserError(Item[0], 'LibraryClasses', ContainerFile, '<LibraryClassKeyWord>|<LibraryInstance>')
    else:
        CheckFileType(List[1], '.Inf', ContainerFile, 'library class instance', Item[0], LineNo)
        CheckFileExist(WorkspaceDir, List[1], ContainerFile, 'LibraryClasses', Item[0], LineNo)
        if Item[1] != '':
            SupMod = Item[1]

    return (List[0], List[1], SupMod)

## Get Library Class
#
# Get Library of Dsc as <LibraryClassKeyWord>[|<LibraryInstance>][|<TokenSpaceGuidCName>.<PcdCName>]
#
# @param Item:           String as <LibraryClassKeyWord>|<LibraryInstance>
# @param ContainerFile:  The file which describes the library class, used for error report
#
# @retval (LibraryClassKeyWord, LibraryInstance, [SUP_MODULE_LIST]) Formatted Library Item
#
def GetLibraryClassOfInf(Item, ContainerFile, WorkspaceDir, LineNo = -1):
    ItemList = GetSplitValueList((Item[0] + DataType.TAB_VALUE_SPLIT * 2))
    SupMod = SUP_MODULE_LIST_STRING

    if len(ItemList) > 5:
        RaiseParserError(Item[0], 'LibraryClasses', ContainerFile, '<LibraryClassKeyWord>[|<LibraryInstance>][|<TokenSpaceGuidCName>.<PcdCName>]')
    else:
        CheckFileType(ItemList[1], '.Inf', ContainerFile, 'LibraryClasses', Item[0], LineNo)
        CheckFileExist(WorkspaceDir, ItemList[1], ContainerFile, 'LibraryClasses', Item[0], LineNo)
        if ItemList[2] != '':
            CheckPcdTokenInfo(ItemList[2], 'LibraryClasses', ContainerFile, LineNo)
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
# @retval True PcdTokenInfo is in correct format
#
def CheckPcdTokenInfo(TokenInfoString, Section, File, LineNo = -1):
    Format = '<TokenSpaceGuidCName>.<PcdCName>'
    if TokenInfoString != '' and TokenInfoString != None:
        TokenInfoList = GetSplitValueList(TokenInfoString, TAB_SPLIT)
        if len(TokenInfoList) == 2:
            return True

    RaiseParserError(TokenInfoString, Section, File, Format, LineNo)

## Get Pcd
#
# Get Pcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|<Value>[|<Type>|<MaximumDatumSize>]
#
# @param Item:           String as <PcdTokenSpaceGuidCName>.<TokenCName>|<Value>[|<Type>|<MaximumDatumSize>]
# @param ContainerFile:  The file which describes the pcd, used for error report
#
# @retval (TokenInfo[1], TokenInfo[0], List[1], List[2], List[3], Type)
#
def GetPcd(Item, Type, ContainerFile, LineNo = -1):
    TokenGuid, TokenName, Value, MaximumDatumSize, Token = '', '', '', '', ''
    List = GetSplitValueList(Item + TAB_VALUE_SPLIT * 2)

    if len(List) < 4 or len(List) > 6:
        RaiseParserError(Item, 'Pcds' + Type, ContainerFile, '<PcdTokenSpaceGuidCName>.<TokenCName>|<Value>[|<Type>|<MaximumDatumSize>]', LineNo)
    else:
        Value = List[1]
        MaximumDatumSize = List[2]
        Token = List[3]

    if CheckPcdTokenInfo(List[0], 'Pcds' + Type, ContainerFile, LineNo):
        (TokenGuid, TokenName) = GetSplitValueList(List[0], TAB_SPLIT)

    return (TokenName, TokenGuid, Value, MaximumDatumSize, Token, Type)

## Get FeatureFlagPcd
#
# Get FeatureFlagPcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|TRUE/FALSE
#
# @param Item:           String as <PcdTokenSpaceGuidCName>.<TokenCName>|TRUE/FALSE
# @param ContainerFile:  The file which describes the pcd, used for error report
#
# @retval (TokenInfo[1], TokenInfo[0], List[1], Type)
#
def GetFeatureFlagPcd(Item, Type, ContainerFile, LineNo = -1):
    TokenGuid, TokenName, Value = '', '', ''
    List = GetSplitValueList(Item)
    if len(List) != 2:
        RaiseParserError(Item, 'Pcds' + Type, ContainerFile, '<PcdTokenSpaceGuidCName>.<TokenCName>|TRUE/FALSE', LineNo)
    else:
        Value = List[1]
    if CheckPcdTokenInfo(List[0], 'Pcds' + Type, ContainerFile, LineNo):
        (TokenGuid, TokenName) = GetSplitValueList(List[0], DataType.TAB_SPLIT)

    return (TokenName, TokenGuid, Value, Type)

## Get DynamicDefaultPcd
#
# Get DynamicDefaultPcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|<Value>[|<DatumTyp>[|<MaxDatumSize>]]
#
# @param Item:           String as <PcdTokenSpaceGuidCName>.<TokenCName>|TRUE/FALSE
# @param ContainerFile:  The file which describes the pcd, used for error report
#
# @retval (TokenInfo[1], TokenInfo[0], List[1], List[2], List[3], Type)
#
def GetDynamicDefaultPcd(Item, Type, ContainerFile, LineNo = -1):
    TokenGuid, TokenName, Value, DatumTyp, MaxDatumSize = '', '', '', '', ''
    List = GetSplitValueList(Item + TAB_VALUE_SPLIT * 2)
    if len(List) < 4 or len(List) > 8:
        RaiseParserError(Item, 'Pcds' + Type, ContainerFile, '<PcdTokenSpaceGuidCName>.<TokenCName>|<Value>[|<DatumTyp>[|<MaxDatumSize>]]', LineNo)
    else:
        Value = List[1]
        DatumTyp = List[2]
        MaxDatumSize = List[3]
    if CheckPcdTokenInfo(List[0], 'Pcds' + Type, ContainerFile, LineNo):
        (TokenGuid, TokenName) = GetSplitValueList(List[0], TAB_SPLIT)

    return (TokenName, TokenGuid, Value, DatumTyp, MaxDatumSize, Type)

## Get DynamicHiiPcd
#
# Get DynamicHiiPcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|<String>|<VariableGuidCName>|<VariableOffset>[|<DefaultValue>[|<MaximumDatumSize>]]
#
# @param Item:           String as <PcdTokenSpaceGuidCName>.<TokenCName>|TRUE/FALSE
# @param ContainerFile:  The file which describes the pcd, used for error report
#
# @retval (TokenInfo[1], TokenInfo[0], List[1], List[2], List[3], List[4], List[5], Type)
#
def GetDynamicHiiPcd(Item, Type, ContainerFile, LineNo = -1):
    TokenGuid, TokenName, L1, L2, L3, L4, L5 = '', '', '', '', '', '', ''
    List = GetSplitValueList(Item + TAB_VALUE_SPLIT * 2)
    if len(List) < 6 or len(List) > 8:
        RaiseParserError(Item, 'Pcds' + Type, ContainerFile, '<PcdTokenSpaceGuidCName>.<TokenCName>|<String>|<VariableGuidCName>|<VariableOffset>[|<DefaultValue>[|<MaximumDatumSize>]]', LineNo)
    else:
        L1, L2, L3, L4, L5 = List[1], List[2], List[3], List[4], List[5]
    if CheckPcdTokenInfo(List[0], 'Pcds' + Type, ContainerFile, LineNo):
        (TokenGuid, TokenName) = GetSplitValueList(List[0], DataType.TAB_SPLIT)

    return (TokenName, TokenGuid, L1, L2, L3, L4, L5, Type)

## Get DynamicVpdPcd
#
# Get DynamicVpdPcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|<VpdOffset>[|<MaximumDatumSize>]
#
# @param Item:           String as <PcdTokenSpaceGuidCName>.<TokenCName>|TRUE/FALSE
# @param ContainerFile:  The file which describes the pcd, used for error report
#
# @retval (TokenInfo[1], TokenInfo[0], List[1], List[2], Type)
#
def GetDynamicVpdPcd(Item, Type, ContainerFile, LineNo = -1):
    TokenGuid, TokenName, L1, L2 = '', '', '', ''
    List = GetSplitValueList(Item + TAB_VALUE_SPLIT)
    if len(List) < 3 or len(List) > 4:
        RaiseParserError(Item, 'Pcds' + Type, ContainerFile, '<PcdTokenSpaceGuidCName>.<TokenCName>|<VpdOffset>[|<MaximumDatumSize>]', LineNo)
    else:
        L1, L2 = List[1], List[2]
    if CheckPcdTokenInfo(List[0], 'Pcds' + Type, ContainerFile, LineNo):
        (TokenGuid, TokenName) = GetSplitValueList(List[0], DataType.TAB_SPLIT)

    return (TokenName, TokenGuid, L1, L2, Type)

## GetComponent
#
# Parse block of the components defined in dsc file
# Set KeyValues as [ ['component name', [lib1, lib2, lib3], [bo1, bo2, bo3], [pcd1, pcd2, pcd3]], ...]
#
# @param Lines:             The content to be parsed
# @param KeyValues:         To store data after parsing
#
# @retval True Get component successfully
#
def GetComponent(Lines, KeyValues):
    (findBlock, findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, False, False, False, False, False)
    ListItem = None
    LibraryClassItem = []
    BuildOption = []
    Pcd = []

    for Line in Lines:
        Line = Line[0]

        #
        # Ignore !include statement
        #
        if Line.upper().find(TAB_INCLUDE.upper() + ' ') > -1 or Line.upper().find(TAB_DEFINE + ' ') > -1:
            continue

        if findBlock == False:
            ListItem = Line
            #
            # find '{' at line tail
            #
            if Line.endswith('{'):
                findBlock = True
                ListItem = CleanString(Line.rsplit('{', 1)[0], DataType.TAB_COMMENT_SPLIT)

        #
        # Parse a block content
        #
        if findBlock:
            if Line.find('<LibraryClasses>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (True, False, False, False, False, False, False)
                continue
            if Line.find('<BuildOptions>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, True, False, False, False, False, False)
                continue
            if Line.find('<PcdsFeatureFlag>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, True, False, False, False, False)
                continue
            if Line.find('<PcdsPatchableInModule>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, True, False, False, False)
                continue
            if Line.find('<PcdsFixedAtBuild>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, False, True, False, False)
                continue
            if Line.find('<PcdsDynamic>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, False, False, True, False)
                continue
            if Line.find('<PcdsDynamicEx>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, False, False, False, True)
                continue
            if Line.endswith('}'):
                #
                # find '}' at line tail
                #
                KeyValues.append([ListItem, LibraryClassItem, BuildOption, Pcd])
                (findBlock, findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, False, False, False, False, False)
                LibraryClassItem, BuildOption, Pcd = [], [], []
                continue

        if findBlock:
            if findLibraryClass:
                LibraryClassItem.append(Line)
            elif findBuildOption:
                BuildOption.append(Line)
            elif findPcdsFeatureFlag:
                Pcd.append((DataType.TAB_PCDS_FEATURE_FLAG_NULL, Line))
            elif findPcdsPatchableInModule:
                Pcd.append((DataType.TAB_PCDS_PATCHABLE_IN_MODULE_NULL, Line))
            elif findPcdsFixedAtBuild:
                Pcd.append((DataType.TAB_PCDS_FIXED_AT_BUILD_NULL, Line))
            elif findPcdsDynamic:
                Pcd.append((DataType.TAB_PCDS_DYNAMIC_DEFAULT_NULL, Line))
            elif findPcdsDynamicEx:
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
# @retval truple() A pair as (InfFilename, ExecFilename)
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
# Set KeyValues as [ ['component name', [lib1, lib2, lib3], [bo1, bo2, bo3], [pcd1, pcd2, pcd3]], ...]
#
# @param Lines:             The content to be parsed
# @param Key:               Reserved
# @param KeyValues:         To store data after parsing
# @param CommentCharacter:  Comment char, used to ignore comment content
#
# @retval True Get component successfully
#
def GetComponents(Lines, Key, KeyValues, CommentCharacter):
    if Lines.find(DataType.TAB_SECTION_END) > -1:
        Lines = Lines.split(DataType.TAB_SECTION_END, 1)[1]
    (findBlock, findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, False, False, False, False, False)
    ListItem = None
    LibraryClassItem = []
    BuildOption = []
    Pcd = []

    LineList = Lines.split('\n')
    for Line in LineList:
        Line = CleanString(Line, CommentCharacter)
        if Line == None or Line == '':
            continue

        if findBlock == False:
            ListItem = Line
            #
            # find '{' at line tail
            #
            if Line.endswith('{'):
                findBlock = True
                ListItem = CleanString(Line.rsplit('{', 1)[0], CommentCharacter)

        #
        # Parse a block content
        #
        if findBlock:
            if Line.find('<LibraryClasses>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (True, False, False, False, False, False, False)
                continue
            if Line.find('<BuildOptions>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, True, False, False, False, False, False)
                continue
            if Line.find('<PcdsFeatureFlag>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, True, False, False, False, False)
                continue
            if Line.find('<PcdsPatchableInModule>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, True, False, False, False)
                continue
            if Line.find('<PcdsFixedAtBuild>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, False, True, False, False)
                continue
            if Line.find('<PcdsDynamic>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, False, False, True, False)
                continue
            if Line.find('<PcdsDynamicEx>') != -1:
                (findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, False, False, False, True)
                continue
            if Line.endswith('}'):
                #
                # find '}' at line tail
                #
                KeyValues.append([ListItem, LibraryClassItem, BuildOption, Pcd])
                (findBlock, findLibraryClass, findBuildOption, findPcdsFeatureFlag, findPcdsPatchableInModule, findPcdsFixedAtBuild, findPcdsDynamic, findPcdsDynamicEx) = (False, False, False, False, False, False, False, False)
                LibraryClassItem, BuildOption, Pcd = [], [], []
                continue

        if findBlock:
            if findLibraryClass:
                LibraryClassItem.append(Line)
            elif findBuildOption:
                BuildOption.append(Line)
            elif findPcdsFeatureFlag:
                Pcd.append((DataType.TAB_PCDS_FEATURE_FLAG, Line))
            elif findPcdsPatchableInModule:
                Pcd.append((DataType.TAB_PCDS_PATCHABLE_IN_MODULE, Line))
            elif findPcdsFixedAtBuild:
                Pcd.append((DataType.TAB_PCDS_FIXED_AT_BUILD, Line))
            elif findPcdsDynamic:
                Pcd.append((DataType.TAB_PCDS_DYNAMIC, Line))
            elif findPcdsDynamicEx:
                Pcd.append((DataType.TAB_PCDS_DYNAMIC_EX, Line))
        else:
            KeyValues.append([ListItem, [], [], []])

    return True

## Get Source
#
# Get Source of Inf as <Filename>[|<Family>[|<TagName>[|<ToolCode>[|<PcdFeatureFlag>]]]]
#
# @param Item:           String as <Filename>[|<Family>[|<TagName>[|<ToolCode>[|<PcdFeatureFlag>]]]]
# @param ContainerFile:  The file which describes the library class, used for error report
#
# @retval (List[0], List[1], List[2], List[3], List[4])
#
def GetSource(Item, ContainerFile, FileRelativePath, LineNo = -1):
    ItemNew = Item + DataType.TAB_VALUE_SPLIT * 4
    List = GetSplitValueList(ItemNew)
    if len(List) < 5 or len(List) > 9:
        RaiseParserError(Item, 'Sources', ContainerFile, '<Filename>[|<Family>[|<TagName>[|<ToolCode>[|<PcdFeatureFlag>]]]]', LineNo)
    List[0] = NormPath(List[0])
    CheckFileExist(FileRelativePath, List[0], ContainerFile, 'Sources', Item, LineNo)
    if List[4] != '':
        CheckPcdTokenInfo(List[4], 'Sources', ContainerFile, LineNo)

    return (List[0], List[1], List[2], List[3], List[4])

## Get Binary
#
# Get Binary of Inf as <Filename>[|<Family>[|<TagName>[|<ToolCode>[|<PcdFeatureFlag>]]]]
#
# @param Item:           String as <Filename>[|<Family>[|<TagName>[|<ToolCode>[|<PcdFeatureFlag>]]]]
# @param ContainerFile:  The file which describes the library class, used for error report
#
# @retval (List[0], List[1], List[2], List[3])
# @retval List
#
def GetBinary(Item, ContainerFile, FileRelativePath, LineNo = -1):
    ItemNew = Item + DataType.TAB_VALUE_SPLIT
    List = GetSplitValueList(ItemNew)
    if len(List) != 4 and len(List) != 5:
        RaiseParserError(Item, 'Binaries', ContainerFile, "<FileType>|<Filename>|<Target>[|<TokenSpaceGuidCName>.<PcdCName>]", LineNo)
    else:
        if List[3] != '':
            CheckPcdTokenInfo(List[3], 'Binaries', ContainerFile, LineNo)

    if len(List) == 4:
        return (List[0], List[1], List[2], List[3])
    elif len(List) == 3:
        return (List[0], List[1], List[2], '')
    elif len(List) == 2:
        return (List[0], List[1], '', '')
    elif len(List) == 1:
        return (List[0], '', '', '')

## Get Guids/Protocols/Ppis
#
# Get Guids/Protocols/Ppis of Inf as <GuidCName>[|<PcdFeatureFlag>]
#
# @param Item:           String as <GuidCName>[|<PcdFeatureFlag>]
# @param Type:           Type of parsing string
# @param ContainerFile:  The file which describes the library class, used for error report
#
# @retval (List[0], List[1])
#
def GetGuidsProtocolsPpisOfInf(Item, Type, ContainerFile, LineNo = -1):
    ItemNew = Item + TAB_VALUE_SPLIT
    List = GetSplitValueList(ItemNew)
    if List[1] != '':
        CheckPcdTokenInfo(List[1], Type, ContainerFile, LineNo)

    return (List[0], List[1])

## Get Guids/Protocols/Ppis
#
# Get Guids/Protocols/Ppis of Dec as <GuidCName>=<GuidValue>
#
# @param Item:           String as <GuidCName>=<GuidValue>
# @param Type:           Type of parsing string
# @param ContainerFile:  The file which describes the library class, used for error report
#
# @retval (List[0], List[1])
#
def GetGuidsProtocolsPpisOfDec(Item, Type, ContainerFile, LineNo = -1):
    List = GetSplitValueList(Item, DataType.TAB_EQUAL_SPLIT)
    if len(List) != 2:
        RaiseParserError(Item, Type, ContainerFile, '<CName>=<GuidValue>', LineNo)

    return (List[0], List[1])

## GetPackage
#
# Get Package of Inf as <PackagePath>[|<PcdFeatureFlag>]
#
# @param Item:           String as <PackagePath>[|<PcdFeatureFlag>]
# @param Type:           Type of parsing string
# @param ContainerFile:  The file which describes the library class, used for error report
#
# @retval (List[0], List[1])
#
def GetPackage(Item, ContainerFile, FileRelativePath, LineNo = -1):
    ItemNew = Item + TAB_VALUE_SPLIT
    List = GetSplitValueList(ItemNew)
    CheckFileType(List[0], '.Dec', ContainerFile, 'package', List[0], LineNo)
    CheckFileExist(FileRelativePath, List[0], ContainerFile, 'Packages', List[0], LineNo)

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
# @retval (TokenSpcCName, TokenCName, Value, ItemType) Formatted Pcd Item
#
def GetPcdOfInf(Item, Type, File, LineNo):
    Format = '<TokenSpaceGuidCName>.<PcdCName>[|<Value>]'
    TokenGuid, TokenName, Value, InfType = '', '', '', ''

    if Type == TAB_PCDS_FIXED_AT_BUILD:
        InfType = TAB_INF_FIXED_PCD
    elif Type == TAB_PCDS_PATCHABLE_IN_MODULE:
        InfType = TAB_INF_PATCH_PCD
    elif Type == TAB_PCDS_FEATURE_FLAG:
        InfType = TAB_INF_FEATURE_PCD
    elif Type == TAB_PCDS_DYNAMIC_EX:
        InfType = TAB_INF_PCD_EX
    elif Type == TAB_PCDS_DYNAMIC:
        InfType = TAB_INF_PCD
    List = GetSplitValueList(Item + DataType.TAB_VALUE_SPLIT)
    if len(List) < 2 or len(List) > 3:
        RaiseParserError(Item, InfType, File, Format, LineNo)
    else:
        Value = List[1]
    TokenInfo = GetSplitValueList(List[0], DataType.TAB_SPLIT)
    if len(TokenInfo) != 2:
        RaiseParserError(Item, InfType, File, Format, LineNo)
    else:
        TokenGuid = TokenInfo[0]
        TokenName = TokenInfo[1]

    return (TokenGuid, TokenName, Value, Type)


## Get Pcd Values of Dec
#
# Get Pcd of Dec as <TokenSpcCName>.<TokenCName>|<Value>|<DatumType>|<Token>
# @retval (TokenSpcCName, TokenCName, Value, DatumType, Token, ItemType) Formatted Pcd Item
#
def GetPcdOfDec(Item, Type, File, LineNo = -1):
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
# 1. Insert a record into TblDec
# Value1: Macro Name
# Value2: Macro Value
#
def ParseDefine(LineValue, StartLine, Table, FileID, Filename, SectionName, SectionModel, Arch):
    EdkLogger.debug(EdkLogger.DEBUG_2, "DEFINE statement '%s' found in section %s" % (LineValue, SectionName))
    Define = GetSplitValueList(CleanString(LineValue[LineValue.upper().find(DataType.TAB_DEFINE.upper() + ' ') + len(DataType.TAB_DEFINE + ' ') : ]), TAB_EQUAL_SPLIT, 1)
    Table.Insert(MODEL_META_DATA_DEFINE, Define[0], Define[1], '', '', '', Arch, SectionModel, FileID, StartLine, -1, StartLine, -1, 0)

## InsertSectionItems
#
# Insert item data of a section to a dict
#
def InsertSectionItems(Model, CurrentSection, SectionItemList, ArchList, ThirdList, RecordSet):
    # Insert each item data of a section
    for Index in range(0, len(ArchList)):
        Arch = ArchList[Index]
        Third = ThirdList[Index]
        if Arch == '':
            Arch = TAB_ARCH_COMMON

        Records = RecordSet[Model]
        for SectionItem in SectionItemList:
            BelongsToItem, EndLine, EndColumn = -1, -1, -1
            LineValue, StartLine, EndLine, Comment = SectionItem[0], SectionItem[1], SectionItem[1], SectionItem[2]

            EdkLogger.debug(4, "Parsing %s ..." %LineValue)
            # And then parse DEFINE statement
            if LineValue.upper().find(DataType.TAB_DEFINE.upper() + ' ') > -1:
                continue

            # At last parse other sections
            ID = -1
            Records.append([LineValue, Arch, StartLine, ID, Third, Comment])

        if RecordSet != {}:
            RecordSet[Model] = Records

## Insert records to database
#
# Insert item data of a section to database
# @param Table:            The Table to be inserted
# @param FileID:           The ID of belonging file
# @param Filename:         The name of belonging file
# @param CurrentSection:   The name of currect section
# @param SectionItemList:  A list of items of the section
# @param ArchList:         A list of arches
# @param ThirdList:        A list of third parameters, ModuleType for LibraryClass and SkuId for Dynamic Pcds
# @param IfDefList:        A list of all conditional statements
# @param RecordSet:        A dict of all parsed records
#
def InsertSectionItemsIntoDatabase(Table, FileID, Filename, Model, CurrentSection, SectionItemList, ArchList, ThirdList, IfDefList, RecordSet):
    #
    # Insert each item data of a section
    #
    for Index in range(0, len(ArchList)):
        Arch = ArchList[Index]
        Third = ThirdList[Index]
        if Arch == '':
            Arch = TAB_ARCH_COMMON

        Records = RecordSet[Model]
        for SectionItem in SectionItemList:
            BelongsToItem, EndLine, EndColumn = -1, -1, -1
            LineValue, StartLine, EndLine = SectionItem[0], SectionItem[1], SectionItem[1]

            EdkLogger.debug(4, "Parsing %s ..." %LineValue)
            #
            # And then parse DEFINE statement
            #
            if LineValue.upper().find(DataType.TAB_DEFINE.upper() + ' ') > -1:
                ParseDefine(LineValue, StartLine, Table, FileID, Filename, CurrentSection, Model, Arch)
                continue

            #
            # At last parse other sections
            #
            ID = Table.Insert(Model, LineValue, Third, Third, '', '', Arch, -1, FileID, StartLine, -1, StartLine, -1, 0)
            Records.append([LineValue, Arch, StartLine, ID, Third])

        if RecordSet != {}:
            RecordSet[Model] = Records

## GenMetaDatSectionItem
def GenMetaDatSectionItem(Key, Value, List):
    if Key not in List:
        List[Key] = [Value]
    else:
        List[Key].append(Value)

## IsValidWord
#
# Check whether the word is valid.
# <Word>   ::=  (a-zA-Z0-9_)(a-zA-Z0-9_-){0,} Alphanumeric characters with
#               optional
#               dash "-" and/or underscore "_" characters. No whitespace
#               characters are permitted.
#
# @param Word:  The word string need to be checked.
#
def IsValidWord(Word):
    if not Word:
        return False
    #
    # The first char should be alpha, _ or Digit.
    #
    if not Word[0].isalnum() and \
       not Word[0] == '_' and \
       not Word[0].isdigit():
        return False

    LastChar = ''
    for Char in Word[1:]:
        if (not Char.isalpha()) and \
           (not Char.isdigit()) and \
           Char != '-' and \
           Char != '_' and \
           Char != '.':
            return False
        if Char == '.' and LastChar == '.':
            return False
        LastChar = Char

    return True
