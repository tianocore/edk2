## @file
# This file is used to define common parsing related functions used in parsing
# Inf/Dsc/Makefile process
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
import Common.LongFilePathOs as os, re
import Common.EdkLogger as EdkLogger
from Common.DataType import *
from CommonDataClass.DataClass import *
from Common.String import CleanString, GetSplitValueList, ReplaceMacro
import EotGlobalData
from Common.Misc import sdict
from Common.String import GetSplitList
from Common.LongFilePathSupport import OpenLongFilePath as open

## PreProcess() method
#
#  Pre process a file
#
#  1. Remove all comments
#  2. Merge multiple lines code to one line
#
#  @param  Filename: Name of the file to be parsed
#  @param  MergeMultipleLines: Switch for if merge multiple lines
#  @param  LineNo: Default line no
#
#  @return Lines: The file contents after remvoing comments
#
def PreProcess(Filename, MergeMultipleLines = True, LineNo = -1):
    Lines = []
    Filename = os.path.normpath(Filename)
    if not os.path.isfile(Filename):
        EdkLogger.error("Eot", EdkLogger.FILE_NOT_FOUND, ExtraData=Filename)

    IsFindBlockComment = False
    IsFindBlockCode = False
    ReservedLine = ''
    ReservedLineLength = 0
    for Line in open(Filename, 'r'):
        Line = Line.strip()
        # Remove comment block
        if Line.find(TAB_COMMENT_EDK_START) > -1:
            ReservedLine = GetSplitList(Line, TAB_COMMENT_EDK_START, 1)[0]
            IsFindBlockComment = True
        if Line.find(TAB_COMMENT_EDK_END) > -1:
            Line = ReservedLine + GetSplitList(Line, TAB_COMMENT_EDK_END, 1)[1]
            ReservedLine = ''
            IsFindBlockComment = False
        if IsFindBlockComment:
            Lines.append('')
            continue

        # Remove comments at tail and remove spaces again
        Line = CleanString(Line)
        if Line == '':
            Lines.append('')
            continue

        if MergeMultipleLines:
            # Add multiple lines to one line
            if IsFindBlockCode and Line[-1] != TAB_SLASH:
                ReservedLine = (ReservedLine + TAB_SPACE_SPLIT + Line).strip()
                Lines.append(ReservedLine)
                for Index in (0, ReservedLineLength):
                    Lines.append('')
                ReservedLine = ''
                ReservedLineLength = 0
                IsFindBlockCode = False
                continue
            if Line[-1] == TAB_SLASH:
                ReservedLine = ReservedLine +  TAB_SPACE_SPLIT + Line[0:-1].strip()
                ReservedLineLength = ReservedLineLength + 1
                IsFindBlockCode = True
                continue

        Lines.append(Line)

    return Lines

## AddToGlobalMacro() method
#
#  Add a macro to EotGlobalData.gMACRO
#
#  @param  Name: Name of the macro
#  @param  Value: Value of the macro
#
def AddToGlobalMacro(Name, Value):
    Value = ReplaceMacro(Value, EotGlobalData.gMACRO, True)
    EotGlobalData.gMACRO[Name] = Value

## AddToSelfMacro() method
#
#  Parse a line of macro definition and add it to a macro set
#
#  @param  SelfMacro: The self macro set
#  @param  Line: The line of a macro definition
#
#  @return Name: Name of macro
#  @return Value: Value of macro
#
def AddToSelfMacro(SelfMacro, Line):
    Name, Value = '', ''
    List = GetSplitValueList(Line, TAB_EQUAL_SPLIT, 1)
    if len(List) == 2:
        Name = List[0]
        Value = List[1]
        Value = ReplaceMacro(Value, EotGlobalData.gMACRO, True)
        Value = ReplaceMacro(Value, SelfMacro, True)
        SelfMacro[Name] = Value

    return (Name, Value)

## GetIncludeListOfFile() method
#
#  Get the include path list for a source file
#
#  1. Find the source file belongs to which INF file
#  2. Find the inf's package
#  3. Return the include path list of the package
#
#  @param  WorkSpace: WORKSPACE path
#  @param  Filepath: File path
#  @param  Db: Eot database
#
#  @return IncludeList: A list of include directories
#
def GetIncludeListOfFile(WorkSpace, Filepath, Db):
    IncludeList = []
    Filepath = os.path.normpath(Filepath)
    SqlCommand = """
                select Value1 from Inf where Model = %s and BelongsToFile in(
                    select distinct B.BelongsToFile from File as A left join Inf as B
                        where A.ID = B.BelongsToFile and B.Model = %s and (A.Path || '%s' || B.Value1) = '%s')""" \
                % (MODEL_META_DATA_PACKAGE, MODEL_EFI_SOURCE_FILE, '\\', Filepath)
    RecordSet = Db.TblFile.Exec(SqlCommand)
    for Record in RecordSet:
        DecFullPath = os.path.normpath(os.path.join(WorkSpace, Record[0]))
        (DecPath, DecName) = os.path.split(DecFullPath)
        SqlCommand = """select Value1 from Dec where BelongsToFile =
                           (select ID from File where FullPath = '%s') and Model = %s""" \
                    % (DecFullPath, MODEL_EFI_INCLUDE)
        NewRecordSet = Db.TblDec.Exec(SqlCommand)
        for NewRecord in NewRecordSet:
            IncludePath = os.path.normpath(os.path.join(DecPath, NewRecord[0]))
            if IncludePath not in IncludeList:
                IncludeList.append(IncludePath)

    return IncludeList

## GetTableList() method
#
#  Search table file and find all small tables
#
#  @param  FileModelList: Model code for the file list
#  @param  Table: Table to insert records
#  @param  Db: Eot database
#
#  @return TableList: A list of tables
#
def GetTableList(FileModelList, Table, Db):
    TableList = []
    SqlCommand = """select ID, FullPath from File where Model in %s""" % str(FileModelList)
    RecordSet = Db.TblFile.Exec(SqlCommand)
    for Record in RecordSet:
        TableName = Table + str(Record[0])
        TableList.append([TableName, Record[1]])

    return TableList

## GetAllIncludeDir() method
#
#  Find all Include directories
#
#  @param  Db: Eot database
#
#  @return IncludeList: A list of include directories
#
def GetAllIncludeDirs(Db):
    IncludeList = []
    SqlCommand = """select distinct Value1 from Inf where Model = %s order by Value1""" % MODEL_EFI_INCLUDE
    RecordSet = Db.TblInf.Exec(SqlCommand)

    for Record in RecordSet:
        IncludeList.append(Record[0])

    return IncludeList

## GetAllIncludeFiles() method
#
#  Find all Include files
#
#  @param  Db: Eot database
#
#  @return IncludeFileList: A list of include files
#
def GetAllIncludeFiles(Db):
    IncludeList = GetAllIncludeDirs(Db)
    IncludeFileList = []

    for Dir in IncludeList:
        if os.path.isdir(Dir):
            SubDir = os.listdir(Dir)
            for Item in SubDir:
                if os.path.isfile(Item):
                    IncludeFileList.append(Item)

    return IncludeFileList

## GetAllSourceFiles() method
#
#  Find all source files
#
#  @param  Db: Eot database
#
#  @return SourceFileList: A list of source files
#
def GetAllSourceFiles(Db):
    SourceFileList = []
    SqlCommand = """select distinct Value1 from Inf where Model = %s order by Value1""" % MODEL_EFI_SOURCE_FILE
    RecordSet = Db.TblInf.Exec(SqlCommand)

    for Record in RecordSet:
        SourceFileList.append(Record[0])

    return SourceFileList

## GetAllFiles() method
#
#  Find all files, both source files and include files
#
#  @param  Db: Eot database
#
#  @return FileList: A list of files
#
def GetAllFiles(Db):
    FileList = []
    IncludeFileList = GetAllIncludeFiles(Db)
    SourceFileList = GetAllSourceFiles(Db)
    for Item in IncludeFileList:
        if os.path.isfile(Item) and Item not in FileList:
            FileList.append(Item)
    for Item in SourceFileList:
        if os.path.isfile(Item) and Item not in FileList:
            FileList.append(Item)

    return FileList

## ParseConditionalStatement() method
#
#  Parse conditional statement
#
#  @param Line: One line to be parsed
#  @param Macros: A set of all macro
#  @param StatusSet: A set of all status
#
#  @retval True: Find keyword of conditional statement
#  @retval False: Not find keyword of conditional statement
#
def ParseConditionalStatement(Line, Macros, StatusSet):
    NewLine = Line.upper()
    if NewLine.find(TAB_IF_EXIST.upper()) > -1:
        IfLine = Line[NewLine.find(TAB_IF_EXIST) + len(TAB_IF_EXIST) + 1:].strip()
        IfLine = ReplaceMacro(IfLine, EotGlobalData.gMACRO, True)
        IfLine = ReplaceMacro(IfLine, Macros, True)
        IfLine = IfLine.replace("\"", '')
        IfLine = IfLine.replace("(", '')
        IfLine = IfLine.replace(")", '')
        Status = os.path.exists(os.path.normpath(IfLine))
        StatusSet.append([Status])
        return True
    if NewLine.find(TAB_IF_DEF.upper()) > -1:
        IfLine = Line[NewLine.find(TAB_IF_DEF) + len(TAB_IF_DEF) + 1:].strip()
        Status = False
        if IfLine in Macros or IfLine in EotGlobalData.gMACRO:
            Status = True
        StatusSet.append([Status])
        return True
    if NewLine.find(TAB_IF_N_DEF.upper()) > -1:
        IfLine = Line[NewLine.find(TAB_IF_N_DEF) + len(TAB_IF_N_DEF) + 1:].strip()
        Status = False
        if IfLine not in Macros and IfLine not in EotGlobalData.gMACRO:
            Status = True
        StatusSet.append([Status])
        return True
    if NewLine.find(TAB_IF.upper()) > -1:
        IfLine = Line[NewLine.find(TAB_IF) + len(TAB_IF) + 1:].strip()
        Status = ParseConditionalStatementMacros(IfLine, Macros)
        StatusSet.append([Status])
        return True
    if NewLine.find(TAB_ELSE_IF.upper()) > -1:
        IfLine = Line[NewLine.find(TAB_ELSE_IF) + len(TAB_ELSE_IF) + 1:].strip()
        Status = ParseConditionalStatementMacros(IfLine, Macros)
        StatusSet[-1].append(Status)
        return True
    if NewLine.find(TAB_ELSE.upper()) > -1:
        Status = False
        for Item in StatusSet[-1]:
            Status = Status or Item
        StatusSet[-1].append(not Status)
        return True
    if NewLine.find(TAB_END_IF.upper()) > -1:
        StatusSet.pop()
        return True

    return False

## ParseConditionalStatement() method
#
#  Parse conditional statement with Macros
#
#  @param Line: One line to be parsed
#  @param Macros: A set of macros
#
#  @return Line: New line after replacing macros
#
def ParseConditionalStatementMacros(Line, Macros):
    if Line.upper().find('DEFINED(') > -1 or Line.upper().find('EXIST') > -1:
        return False
    Line = ReplaceMacro(Line, EotGlobalData.gMACRO, True)
    Line = ReplaceMacro(Line, Macros, True)
    Line = Line.replace("&&", "and")
    Line = Line.replace("||", "or")
    return eval(Line)

## GetConditionalStatementStatus() method
#
#  1. Assume the latest status as True
#  2. Pop the top status of status set, previous status
#  3. Compare the latest one and the previous one and get new status
#
#  @param StatusSet: A set of all status
#
#  @return Status: The final status
#
def GetConditionalStatementStatus(StatusSet):
    Status = True
    for Item in StatusSet:
        Status = Status and Item[-1]

    return Status

## SearchBelongsToFunction() method
#
#  Search all functions belong to the file
#
#  @param BelongsToFile: File id
#  @param StartLine: Start line of search scope
#  @param EndLine: End line of search scope
#
#  @return: The found function
#
def SearchBelongsToFunction(BelongsToFile, StartLine, EndLine):
    SqlCommand = """select ID, Name from Function where BelongsToFile = %s and StartLine <= %s and EndLine >= %s""" %(BelongsToFile, StartLine, EndLine)
    RecordSet = EotGlobalData.gDb.TblFunction.Exec(SqlCommand)
    if RecordSet != []:
        return RecordSet[0][0], RecordSet[0][1]
    else:
        return -1, ''

## SearchPpiCallFunction() method
#
#  Search all used PPI calling function 'PeiServicesReInstallPpi' and 'PeiServicesInstallPpi'
#  Store the result to database
#
#  @param Identifier: Table id
#  @param SourceFileID: Source file id
#  @param SourceFileFullPath: Source file full path
#  @param ItemMode: Mode of the item
#
def SearchPpiCallFunction(Identifier, SourceFileID, SourceFileFullPath, ItemMode):
    ItemName, ItemType, GuidName, GuidMacro, GuidValue = '', 'Ppi', '', '', ''
    SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                    where (Name like '%%%s%%' and Model = %s)""" \
                    % (Identifier, 'PeiServicesReInstallPpi', MODEL_IDENTIFIER_FUNCTION_CALLING)
    BelongsToFunctionID, BelongsToFunction = -1, ''
    Db = EotGlobalData.gDb.TblReport
    RecordSet = Db.Exec(SqlCommand)
    for Record in RecordSet:
        Index = 0
        BelongsToFile, StartLine, EndLine = Record[2], Record[3], Record[4]
        BelongsToFunctionID, BelongsToFunction = SearchBelongsToFunction(BelongsToFile, StartLine, EndLine)
        VariableList = Record[0].split(',')
        for Variable in VariableList:
            Variable = Variable.strip()
            # Get index of the variable
            if Variable.find('[') > -1:
                Index = int(Variable[Variable.find('[') + 1 : Variable.find(']')])
                Variable = Variable[:Variable.find('[')]
            # Get variable name
            if Variable.startswith('&'):
                Variable = Variable[1:]
            # Get variable value
            SqlCommand = """select Value from %s where (Name like '%%%s%%') and Model = %s""" \
                         % (Identifier, Variable, MODEL_IDENTIFIER_VARIABLE)
            NewRecordSet = Db.Exec(SqlCommand)
            if NewRecordSet:
                NewRecord = NewRecordSet[0][0]
                VariableValueList = NewRecord.split('},')
                if len(VariableValueList) > Index:
                    VariableValue = VariableValueList[Index]
                    NewVariableValueList = VariableValue.split(',')
                    if len(NewVariableValueList) > 1:
                        NewVariableValue = NewVariableValueList[1].strip()
                        if NewVariableValue.startswith('&'):
                            Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, NewVariableValue[1:], GuidMacro, GuidValue, BelongsToFunction, 0)
                            continue
                        else:
                            EotGlobalData.gOP_UN_MATCHED.write('%s, %s, %s, %s, %s, %s\n' % (ItemType, ItemMode, SourceFileID, SourceFileFullPath, StartLine, NewParameter))

    ItemName, ItemType, GuidName, GuidMacro, GuidValue = '', 'Ppi', '', '', ''
    SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                    where (Value like '%%%s%%' and Model = %s)""" \
                    % (Identifier, 'PeiServicesInstallPpi', MODEL_IDENTIFIER_ASSIGNMENT_EXPRESSION)
    BelongsToFunctionID, BelongsToFunction = -1, ''
    Db = EotGlobalData.gDb.TblReport
    RecordSet = Db.Exec(SqlCommand)

    SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                    where (Name like '%%%s%%' and Model = %s)""" \
                    % (Identifier, 'PeiServicesInstallPpi', MODEL_IDENTIFIER_FUNCTION_CALLING)
    Db = EotGlobalData.gDb.TblReport
    RecordSet2 = Db.Exec(SqlCommand)

    for Record in RecordSet + RecordSet2:
        if Record == []:
            continue
        Index = 0
        BelongsToFile, StartLine, EndLine = Record[2], Record[3], Record[4]
        BelongsToFunctionID, BelongsToFunction = SearchBelongsToFunction(BelongsToFile, StartLine, EndLine)
        Variable = Record[0].replace('PeiServicesInstallPpi', '').replace('(', '').replace(')', '').replace('&', '').strip()
        Variable = Variable[Variable.find(',') + 1:].strip()
        # Get index of the variable
        if Variable.find('[') > -1:
            Index = int(Variable[Variable.find('[') + 1 : Variable.find(']')])
            Variable = Variable[:Variable.find('[')]
        # Get variable name
        if Variable.startswith('&'):
            Variable = Variable[1:]
        # Get variable value
        SqlCommand = """select Value from %s where (Name like '%%%s%%') and Model = %s""" \
                     % (Identifier, Variable, MODEL_IDENTIFIER_VARIABLE)
        NewRecordSet = Db.Exec(SqlCommand)
        if NewRecordSet:
            NewRecord = NewRecordSet[0][0]
            VariableValueList = NewRecord.split('},')
            for VariableValue in VariableValueList[Index:]:
                NewVariableValueList = VariableValue.split(',')
                if len(NewVariableValueList) > 1:
                    NewVariableValue = NewVariableValueList[1].strip()
                    if NewVariableValue.startswith('&'):
                        Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, NewVariableValue[1:], GuidMacro, GuidValue, BelongsToFunction, 0)
                        continue
                    else:
                        EotGlobalData.gOP_UN_MATCHED.write('%s, %s, %s, %s, %s, %s\n' % (ItemType, ItemMode, SourceFileID, SourceFileFullPath, StartLine, NewParameter))

## SearchPpis() method
#
#  Search all used PPI calling function
#  Store the result to database
#
#  @param SqlCommand: SQL command statement
#  @param Table: Table id
#  @param SourceFileID: Source file id
#  @param SourceFileFullPath: Source file full path
#  @param ItemMode: Mode of the item
#  @param PpiMode: Mode of PPI
#
def SearchPpi(SqlCommand, Table, SourceFileID, SourceFileFullPath, ItemMode, PpiMode = 1):
    ItemName, ItemType, GuidName, GuidMacro, GuidValue = '', 'Ppi', '', '', ''
    BelongsToFunctionID, BelongsToFunction = -1, ''
    Db = EotGlobalData.gDb.TblReport
    RecordSet = Db.Exec(SqlCommand)
    for Record in RecordSet:
        Parameter = GetPpiParameter(Record[0], PpiMode)
        BelongsToFile, StartLine, EndLine = Record[2], Record[3], Record[4]
        # Get BelongsToFunction
        BelongsToFunctionID, BelongsToFunction = SearchBelongsToFunction(BelongsToFile, StartLine, EndLine)

        # Default is Not Found
        IsFound = False

        # For Consumed Ppi
        if ItemMode == 'Consumed':
            if Parameter.startswith('g'):
                Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, Parameter, GuidMacro, GuidValue, BelongsToFunction, 0)
            else:
                EotGlobalData.gOP_UN_MATCHED.write('%s, %s, %s, %s, %s, %s\n' % (ItemType, ItemMode, SourceFileID, SourceFileFullPath, StartLine, Parameter))
            continue

        # Direct Parameter.Guid
        SqlCommand = """select Value from %s where (Name like '%%%s.Guid%%' or Name like '%%%s->Guid%%') and Model = %s""" % (Table, Parameter, Parameter, MODEL_IDENTIFIER_ASSIGNMENT_EXPRESSION)
        NewRecordSet = Db.Exec(SqlCommand)
        for NewRecord in NewRecordSet:
            GuidName = GetParameterName(NewRecord[0])
            Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, 0)
            IsFound = True

        # Defined Parameter
        if not IsFound:
            Key = Parameter
            if Key.rfind(' ') > -1:
                Key = Key[Key.rfind(' ') : ].strip().replace('&', '')
            Value = FindKeyValue(EotGlobalData.gDb.TblFile, Table, Key)
            List = GetSplitValueList(Value.replace('\n', ''), TAB_COMMA_SPLIT)
            if len(List) > 1:
                GuidName = GetParameterName(List[1])
                Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, 0)
                IsFound = True

        # A list Parameter
        if not IsFound:
            Start = Parameter.find('[')
            End = Parameter.find(']')
            if Start > -1 and End > -1 and Start < End:
                try:
                    Index = int(Parameter[Start + 1 : End])
                    Parameter = Parameter[0 : Start]
                    SqlCommand = """select Value from %s where Name = '%s' and Model = %s""" % (Table, Parameter, MODEL_IDENTIFIER_VARIABLE)
                    NewRecordSet = Db.Exec(SqlCommand)
                    for NewRecord in NewRecordSet:
                        NewParameter = GetSplitValueList(NewRecord[0], '}')[Index]
                        GuidName = GetPpiParameter(NewParameter[NewParameter.find('{') : ])
                        Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, 0)
                        IsFound = True
                except Exception:
                    pass

        # A External Parameter
        if not IsFound:
            SqlCommand = """select File.ID from Inf, File
                            where BelongsToFile = (select BelongsToFile from Inf where Value1 = '%s')
                            and Inf.Model = %s and Inf.Value1 = File.FullPath and File.Model = %s""" % (SourceFileFullPath, MODEL_EFI_SOURCE_FILE, MODEL_FILE_C)
            NewRecordSet = Db.Exec(SqlCommand)
            for NewRecord in NewRecordSet:
                Table = 'Identifier' + str(NewRecord[0])
                SqlCommand = """select Value from %s where Name = '%s' and Modifier = 'EFI_PEI_PPI_DESCRIPTOR' and Model = %s""" % (Table, Parameter, MODEL_IDENTIFIER_VARIABLE)
                PpiSet = Db.Exec(SqlCommand)
                if PpiSet != []:
                    GuidName = GetPpiParameter(PpiSet[0][0])
                    if GuidName != '':
                        Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, 0)
                        IsFound = True
                        break

        if not IsFound:
            EotGlobalData.gOP_UN_MATCHED.write('%s, %s, %s, %s, %s, %s\n' % (ItemType, ItemMode, SourceFileID, SourceFileFullPath, StartLine, Parameter))

## SearchProtocols() method
#
#  Search all used PROTOCOL calling function
#  Store the result to database
#
#  @param SqlCommand: SQL command statement
#  @param Table: Table id
#  @param SourceFileID: Source file id
#  @param SourceFileFullPath: Source file full path
#  @param ItemMode: Mode of the item
#  @param ProtocolMode: Mode of PROTOCOL
#
def SearchProtocols(SqlCommand, Table, SourceFileID, SourceFileFullPath, ItemMode, ProtocolMode):
    ItemName, ItemType, GuidName, GuidMacro, GuidValue = '', 'Protocol', '', '', ''
    BelongsToFunctionID, BelongsToFunction = -1, ''
    Db = EotGlobalData.gDb.TblReport
    RecordSet = Db.Exec(SqlCommand)
    for Record in RecordSet:
        Parameter = ''
        BelongsToFile, StartLine, EndLine = Record[2], Record[3], Record[4]
        # Get BelongsToFunction
        BelongsToFunctionID, BelongsToFunction = SearchBelongsToFunction(BelongsToFile, StartLine, EndLine)

        # Default is Not Found
        IsFound = False

        if ProtocolMode == 0 or ProtocolMode == 1:
            Parameter = GetProtocolParameter(Record[0], ProtocolMode)
            if Parameter.startswith('g') or Parameter.endswith('Guid') or Parameter == 'ShellEnvProtocol' or Parameter == 'ShellInterfaceProtocol':
                GuidName = GetParameterName(Parameter)
                Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, 0)
                IsFound = True

        if ProtocolMode == 2:
            Protocols = GetSplitValueList(Record[0], TAB_COMMA_SPLIT)
            for Protocol in Protocols:
                if Protocol.startswith('&') and Protocol.endswith('Guid'):
                    GuidName = GetParameterName(Protocol)
                    Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, 0)
                    IsFound = True
                else:
                    NewValue = FindKeyValue(EotGlobalData.gDb.TblFile, Table, Protocol)
                    if Protocol != NewValue and NewValue.endswith('Guid'):
                        GuidName = GetParameterName(NewValue)
                        Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, 0)
                        IsFound = True

        if not IsFound:
            if BelongsToFunction in EotGlobalData.gProducedProtocolLibrary or BelongsToFunction in EotGlobalData.gConsumedProtocolLibrary:
                EotGlobalData.gOP_UN_MATCHED_IN_LIBRARY_CALLING.write('%s, %s, %s, %s, %s, %s, %s\n' % (ItemType, ItemMode, SourceFileID, SourceFileFullPath, StartLine, Parameter, BelongsToFunction))
            else:
                EotGlobalData.gOP_UN_MATCHED.write('%s, %s, %s, %s, %s, %s\n' % (ItemType, ItemMode, SourceFileID, SourceFileFullPath, StartLine, Parameter))

## SearchFunctionCalling() method
#
#  Search all used PPI/PROTOCOL calling function by library
#  Store the result to database
#
#  @param SqlCommand: SQL command statement
#  @param Table: Table id
#  @param SourceFileID: Source file id
#  @param SourceFileFullPath: Source file full path
#  @param ItemType: Type of the item, PPI or PROTOCOL
#  @param ItemMode: Mode of item
#
def SearchFunctionCalling(Table, SourceFileID, SourceFileFullPath, ItemType, ItemMode):
    LibraryList = sdict()
    Db = EotGlobalData.gDb.TblReport
    Parameters, ItemName, GuidName, GuidMacro, GuidValue, BelongsToFunction = [], '', '', '', '', ''
    if ItemType == 'Protocol' and ItemMode == 'Produced':
        LibraryList = EotGlobalData.gProducedProtocolLibrary
    elif ItemType == 'Protocol' and ItemMode == 'Consumed':
        LibraryList = EotGlobalData.gConsumedProtocolLibrary
    elif ItemType == 'Protocol' and ItemMode == 'Callback':
        LibraryList = EotGlobalData.gCallbackProtocolLibrary
    elif ItemType == 'Ppi' and ItemMode == 'Produced':
        LibraryList = EotGlobalData.gProducedPpiLibrary
    elif ItemType == 'Ppi' and ItemMode == 'Consumed':
        LibraryList = EotGlobalData.gConsumedPpiLibrary

    for Library in LibraryList:
        Index = LibraryList[Library]
        SqlCommand = """select Value, StartLine from %s
                        where Name like '%%%s%%' and Model = %s""" \
                        % (Table, Library, MODEL_IDENTIFIER_FUNCTION_CALLING)
        RecordSet = Db.Exec(SqlCommand)
        for Record in RecordSet:
            IsFound = False
            if Index == -1:
                ParameterList = GetSplitValueList(Record[0], TAB_COMMA_SPLIT)
                for Parameter in ParameterList:
                    Parameters.append(GetParameterName(Parameter))
            else:
                Parameters = [GetProtocolParameter(Record[0], Index)]
            StartLine = Record[1]
            for Parameter in Parameters:
                if Parameter.startswith('g') or Parameter.endswith('Guid') or Parameter == 'ShellEnvProtocol' or Parameter == 'ShellInterfaceProtocol':
                    GuidName = GetParameterName(Parameter)
                    Db.Insert(-1, '', '', SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, 0)
                    IsFound = True

            if not IsFound:
                EotGlobalData.gOP_UN_MATCHED.write('%s, %s, %s, %s, %s, %s\n' % (ItemType, ItemMode, SourceFileID, SourceFileFullPath, StartLine, Parameter))

## FindProtocols() method
#
#  Find defined protocols
#
#  @param SqlCommand: SQL command statement
#  @param Table: Table id
#  @param SourceFileID: Source file id
#  @param SourceFileFullPath: Source file full path
#  @param ItemName: String of protocol definition
#  @param ItemType: Type of the item, PPI or PROTOCOL
#  @param ItemMode: Mode of item
#
#def FindProtocols(Db, SqlCommand, Table, SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue):
#    BelongsToFunction = ''
#    RecordSet = Db.Exec(SqlCommand)
#    for Record in RecordSet:
#        IsFound = True
#        Parameter = GetProtocolParameter(Record[0])

## GetProtocolParameter() method
#
# Parse string of protocol and find parameters
#
#  @param Parameter: Parameter to be parsed
#  @param Index: The index of the parameter
#
#  @return: call common GetParameter
#
def GetProtocolParameter(Parameter, Index = 1):
    return GetParameter(Parameter, Index)

## GetPpiParameter() method
#
# Parse string of ppi and find parameters
#
#  @param Parameter: Parameter to be parsed
#  @param Index: The index of the parameter
#
#  @return: call common GetParameter
#
def GetPpiParameter(Parameter, Index = 1):
    return GetParameter(Parameter, Index)

## GetParameter() method
#
# Get a parameter by index
#
#  @param Parameter: Parameter to be parsed
#  @param Index: The index of the parameter
#
#  @return Parameter: The found parameter
#
def GetParameter(Parameter, Index = 1):
    ParameterList = GetSplitValueList(Parameter, TAB_COMMA_SPLIT)
    if len(ParameterList) > Index:
        Parameter = GetParameterName(ParameterList[Index])

        return Parameter

    return ''

## GetParameterName() method
#
# Get a parameter name
#
#  @param Parameter: Parameter to be parsed
#
#  @return: The name of parameter
#
def GetParameterName(Parameter):
    if type(Parameter) == type('') and Parameter.startswith('&'):
        return Parameter[1:].replace('{', '').replace('}', '').replace('\r', '').replace('\n', '').strip()
    else:
        return Parameter.strip()

## FindKeyValue() method
#
# Find key value of a variable
#
#  @param Db: Database to be searched
#  @param Table: Table to be searched
#  @param Key: The keyword
#
#  @return Value: The value of the the keyword
#
def FindKeyValue(Db, Table, Key):
    SqlCommand = """select Value from %s where Name = '%s' and (Model = %s or Model = %s)""" % (Table, Key, MODEL_IDENTIFIER_VARIABLE, MODEL_IDENTIFIER_ASSIGNMENT_EXPRESSION)
    RecordSet = Db.Exec(SqlCommand)
    Value = ''
    for Record in RecordSet:
        if Record[0] != 'NULL':
            Value = FindKeyValue(Db, Table, GetParameterName(Record[0]))

    if Value != '':
        return Value
    else:
        return Key

## ParseMapFile() method
#
#  Parse map files to get a dict of 'ModuleName' : {FunName : FunAddress}
#
#  @param Files: A list of map files
#
#  @return AllMaps: An object of all map files
#
def ParseMapFile(Files):
    AllMaps = {}
    CurrentModule = ''
    CurrentMaps = {}
    for File in Files:
        Content = open(File, 'r').readlines()
        for Line in Content:
            Line = CleanString(Line)
            # skip empty line
            if Line == '':
                continue

            if Line.find('(') > -1 and Line.find(')') > -1:
                if CurrentModule != '' and CurrentMaps != {}:
                    AllMaps[CurrentModule] = CurrentMaps
                CurrentModule = Line[:Line.find('(')]
                CurrentMaps = {}
                continue
            else:
                Name = ''
                Address = ''
                List = Line.split()
                Address = List[0]
                if List[1] == 'F' or List[1] == 'FS':
                    Name = List[2]
                else:
                    Name = List[1]
                CurrentMaps[Name] = Address
                continue

    return AllMaps

## ConvertGuid
#
#  Convert a GUID to a GUID with all upper letters
#
#  @param guid:  The GUID to be converted
#
#  @param newGuid: The GUID with all upper letters.
#
def ConvertGuid(guid):
    numList = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
    newGuid = ''
    if guid.startswith('g'):
        guid = guid[1:]
    for i in guid:
        if i.upper() == i and i not in numList:
            newGuid = newGuid + ('_' + i)
        else:
            newGuid = newGuid + i.upper()
    if newGuid.startswith('_'):
        newGuid = newGuid[1:]
    if newGuid.endswith('_'):
        newGuid = newGuid[:-1]

    return newGuid

## ConvertGuid2() method
#
#  Convert a GUID to a GUID with new string instead of old string
#
#  @param guid: The GUID to be converted
#  @param old: Old string to be replaced
#  @param new: New string to replace the old one
#
#  @param newGuid: The GUID after replacement
#
def ConvertGuid2(guid, old, new):
    newGuid = ConvertGuid(guid)
    newGuid = newGuid.replace(old, new)

    return newGuid

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    pass
