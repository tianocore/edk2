## @file
# preprocess source file
#
#  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import sys
import os
import re
import CodeFragmentCollector
import FileProfile
from CommonDataClass import DataClass
from Common import EdkLogger
from EotToolError import *
import EotGlobalData

# Global Dicts
IncludeFileListDict = {}
IncludePathListDict = {}
ComplexTypeDict = {}
SUDict = {}

## GetFuncDeclPattern() method
#
#  Get the pattern of function declaration
#
#  @return p:    the pattern of function declaration
#
def GetFuncDeclPattern():
    p = re.compile(r'(EFIAPI|EFI_BOOT_SERVICE|EFI_RUNTIME_SERVICE)?\s*[_\w]+\s*\(.*\).*', re.DOTALL)
    return p

## GetArrayPattern() method
#
#  Get the pattern of array
#
#  @return p:    the pattern of array
#
def GetArrayPattern():
    p = re.compile(r'[_\w]*\s*[\[.*\]]+')
    return p

## GetTypedefFuncPointerPattern() method
#
#  Get the pattern of function pointer
#
#  @return p:    the pattern of function pointer
#
def GetTypedefFuncPointerPattern():
    p = re.compile('[_\w\s]*\([\w\s]*\*+\s*[_\w]+\s*\)\s*\(.*\)', re.DOTALL)
    return p

## GetDB() method
#
#  Get global database instance
#
#  @return EotGlobalData.gDb:    the global database instance
#
def GetDB():
    return EotGlobalData.gDb

## PrintErrorMsg() method
#
#  print error message
#
#  @param ErrorType: Type of error
#  @param Msg: Error message
#  @param TableName: table name of error found
#  @param ItemId: id of item
#
def PrintErrorMsg(ErrorType, Msg, TableName, ItemId):
    Msg = Msg.replace('\n', '').replace('\r', '')
    MsgPartList = Msg.split()
    Msg = ''
    for Part in MsgPartList:
        Msg += Part
        Msg += ' '
    GetDB().TblReport.Insert(ErrorType, OtherMsg = Msg, BelongsToTable = TableName, BelongsToItem = ItemId)

## GetIdType() method
#
#  Find type of input string
#
#  @param Str: String to be parsed
#
#  @return Type: The type of the string
#
def GetIdType(Str):
    Type = DataClass.MODEL_UNKNOWN
    Str = Str.replace('#', '# ')
    List = Str.split()
    if List[1] == 'include':
        Type = DataClass.MODEL_IDENTIFIER_INCLUDE
    elif List[1] == 'define':
        Type = DataClass.MODEL_IDENTIFIER_MACRO_DEFINE
    elif List[1] == 'ifdef':
        Type = DataClass.MODEL_IDENTIFIER_MACRO_IFDEF
    elif List[1] == 'ifndef':
        Type = DataClass.MODEL_IDENTIFIER_MACRO_IFNDEF
    elif List[1] == 'endif':
        Type = DataClass.MODEL_IDENTIFIER_MACRO_ENDIF
    elif List[1] == 'pragma':
        Type = DataClass.MODEL_IDENTIFIER_MACRO_PROGMA
    else:
        Type = DataClass.MODEL_UNKNOWN
    return Type

## GetIdentifierList() method
#
#  Get id of all files
#
#  @return IdList: The list of all id of files
#
def GetIdentifierList():
    IdList = []

    for pp in FileProfile.PPDirectiveList:
        Type = GetIdType(pp.Content)
        IdPP = DataClass.IdentifierClass(-1, '', '', '', pp.Content, Type, -1, -1, pp.StartPos[0],pp.StartPos[1],pp.EndPos[0],pp.EndPos[1])
        IdList.append(IdPP)

    for ae in FileProfile.AssignmentExpressionList:
        IdAE = DataClass.IdentifierClass(-1, ae.Operator, '', ae.Name, ae.Value, DataClass.MODEL_IDENTIFIER_ASSIGNMENT_EXPRESSION, -1, -1, ae.StartPos[0],ae.StartPos[1],ae.EndPos[0],ae.EndPos[1])
        IdList.append(IdAE)

    FuncDeclPattern = GetFuncDeclPattern()
    ArrayPattern = GetArrayPattern()
    for var in FileProfile.VariableDeclarationList:
        DeclText = var.Declarator.strip()
        while DeclText.startswith('*'):
            var.Modifier += '*'
            DeclText = DeclText.lstrip('*').strip()
        var.Declarator = DeclText
        if FuncDeclPattern.match(var.Declarator):
            DeclSplitList = var.Declarator.split('(')
            FuncName = DeclSplitList[0]
            FuncNamePartList = FuncName.split()
            if len(FuncNamePartList) > 1:
                FuncName = FuncNamePartList[-1]
                Index = 0
                while Index < len(FuncNamePartList) - 1:
                    var.Modifier += ' ' + FuncNamePartList[Index]
                    var.Declarator = var.Declarator.lstrip().lstrip(FuncNamePartList[Index])
                    Index += 1
            IdVar = DataClass.IdentifierClass(-1, var.Modifier, '', var.Declarator, '', DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION, -1, -1, var.StartPos[0],var.StartPos[1],var.EndPos[0],var.EndPos[1])
            IdList.append(IdVar)
            continue

        if var.Declarator.find('{') == -1:
            for decl in var.Declarator.split(','):
                DeclList = decl.split('=')
                Name = DeclList[0].strip()
                if ArrayPattern.match(Name):
                    LSBPos = var.Declarator.find('[')
                    var.Modifier += ' ' + Name[LSBPos:]
                    Name = Name[0:LSBPos]

                IdVar = DataClass.IdentifierClass(-1, var.Modifier, '', Name, (len(DeclList) > 1 and [DeclList[1]]or [''])[0], DataClass.MODEL_IDENTIFIER_VARIABLE, -1, -1, var.StartPos[0],var.StartPos[1],var.EndPos[0],var.EndPos[1])
                IdList.append(IdVar)
        else:
            DeclList = var.Declarator.split('=')
            Name = DeclList[0].strip()
            if ArrayPattern.match(Name):
                LSBPos = var.Declarator.find('[')
                var.Modifier += ' ' + Name[LSBPos:]
                Name = Name[0:LSBPos]
            IdVar = DataClass.IdentifierClass(-1, var.Modifier, '', Name, (len(DeclList) > 1 and [DeclList[1]]or [''])[0], DataClass.MODEL_IDENTIFIER_VARIABLE, -1, -1, var.StartPos[0],var.StartPos[1],var.EndPos[0],var.EndPos[1])
            IdList.append(IdVar)

    for enum in FileProfile.EnumerationDefinitionList:
        LBPos = enum.Content.find('{')
        RBPos = enum.Content.find('}')
        Name = enum.Content[4:LBPos].strip()
        Value = enum.Content[LBPos+1:RBPos]
        IdEnum = DataClass.IdentifierClass(-1, '', '', Name, Value, DataClass.MODEL_IDENTIFIER_ENUMERATE, -1, -1, enum.StartPos[0],enum.StartPos[1],enum.EndPos[0],enum.EndPos[1])
        IdList.append(IdEnum)

    for su in FileProfile.StructUnionDefinitionList:
        Type = DataClass.MODEL_IDENTIFIER_STRUCTURE
        SkipLen = 6
        if su.Content.startswith('union'):
            Type = DataClass.MODEL_IDENTIFIER_UNION
            SkipLen = 5
        LBPos = su.Content.find('{')
        RBPos = su.Content.find('}')
        if LBPos == -1 or RBPos == -1:
            Name = su.Content[SkipLen:].strip()
            Value = ''
        else:
            Name = su.Content[SkipLen:LBPos].strip()
            Value = su.Content[LBPos+1:RBPos]
        IdPE = DataClass.IdentifierClass(-1, '', '', Name, Value, Type, -1, -1, su.StartPos[0],su.StartPos[1],su.EndPos[0],su.EndPos[1])
        IdList.append(IdPE)

    TdFuncPointerPattern = GetTypedefFuncPointerPattern()
    for td in FileProfile.TypedefDefinitionList:
        Modifier = ''
        Name = td.ToType
        Value = td.FromType
        if TdFuncPointerPattern.match(td.ToType):
            Modifier = td.FromType
            LBPos = td.ToType.find('(')
            TmpStr = td.ToType[LBPos+1:].strip()
            StarPos = TmpStr.find('*')
            if StarPos != -1:
                Modifier += ' ' + TmpStr[0:StarPos]
            while TmpStr[StarPos] == '*':
                Modifier += ' ' + '*'
                StarPos += 1
            TmpStr = TmpStr[StarPos:].strip()
            RBPos = TmpStr.find(')')
            Name = TmpStr[0:RBPos]
            Value = 'FP' + TmpStr[RBPos + 1:]

        IdTd = DataClass.IdentifierClass(-1, Modifier, '', Name, Value, DataClass.MODEL_IDENTIFIER_TYPEDEF, -1, -1, td.StartPos[0],td.StartPos[1],td.EndPos[0],td.EndPos[1])
        IdList.append(IdTd)

    for funcCall in FileProfile.FunctionCallingList:
        IdFC = DataClass.IdentifierClass(-1, '', '', funcCall.FuncName, funcCall.ParamList, DataClass.MODEL_IDENTIFIER_FUNCTION_CALLING, -1, -1, funcCall.StartPos[0],funcCall.StartPos[1],funcCall.EndPos[0],funcCall.EndPos[1])
        IdList.append(IdFC)
    return IdList

## GetParamList() method
#
#  Get a list of parameters
#
#  @param FuncDeclarator: Function declarator
#  @param FuncNameLine: Line number of function name
#  @param FuncNameOffset: Offset of function name
#
#  @return ParamIdList: A list of parameters
#
def GetParamList(FuncDeclarator, FuncNameLine = 0, FuncNameOffset = 0):
    ParamIdList = []
    DeclSplitList = FuncDeclarator.split('(')
    if len(DeclSplitList) < 2:
        return ParamIdList
    FuncName = DeclSplitList[0]
    ParamStr = DeclSplitList[1].rstrip(')')
    LineSkipped = 0
    OffsetSkipped = 0
    Start = 0
    while FuncName.find('\n', Start) != -1:
        LineSkipped += 1
        OffsetSkipped = 0
        Start += FuncName.find('\n', Start)
        Start += 1
    OffsetSkipped += len(FuncName[Start:])
    OffsetSkipped += 1 #skip '('
    ParamBeginLine = FuncNameLine + LineSkipped
    ParamBeginOffset = OffsetSkipped
    for p in ParamStr.split(','):
        ListP = p.split()
        if len(ListP) == 0:
            continue
        ParamName = ListP[-1]
        DeclText = ParamName.strip()
        RightSpacePos = p.rfind(ParamName)
        ParamModifier = p[0:RightSpacePos]
        if ParamName == 'OPTIONAL':
            if ParamModifier == '':
                ParamModifier += ' ' + 'OPTIONAL'
                DeclText = ''
            else:
                ParamName = ListP[-2]
                DeclText = ParamName.strip()
                RightSpacePos = p.rfind(ParamName)
                ParamModifier = p[0:RightSpacePos]
                ParamModifier += 'OPTIONAL'
        while DeclText.startswith('*'):
            ParamModifier += ' ' + '*'
            DeclText = DeclText.lstrip('*').strip()
        ParamName = DeclText

        Start = 0
        while p.find('\n', Start) != -1:
            LineSkipped += 1
            OffsetSkipped = 0
            Start += p.find('\n', Start)
            Start += 1
        OffsetSkipped += len(p[Start:])

        ParamEndLine = ParamBeginLine + LineSkipped
        ParamEndOffset = OffsetSkipped
        IdParam = DataClass.IdentifierClass(-1, ParamModifier, '', ParamName, '', DataClass.MODEL_IDENTIFIER_PARAMETER, -1, -1, ParamBeginLine, ParamBeginOffset, ParamEndLine, ParamEndOffset)
        ParamIdList.append(IdParam)
        ParamBeginLine = ParamEndLine
        ParamBeginOffset = OffsetSkipped + 1 #skip ','

    return ParamIdList

## GetFunctionList()
#
#  Get a list of functions
#
#  @return FuncObjList: A list of function objects
#
def GetFunctionList():
    FuncObjList = []
    for FuncDef in FileProfile.FunctionDefinitionList:
        ParamIdList = []
        DeclText = FuncDef.Declarator.strip()
        while DeclText.startswith('*'):
            FuncDef.Modifier += '*'
            DeclText = DeclText.lstrip('*').strip()

        FuncDef.Declarator = FuncDef.Declarator.lstrip('*')
        DeclSplitList = FuncDef.Declarator.split('(')
        if len(DeclSplitList) < 2:
            continue

        FuncName = DeclSplitList[0]
        FuncNamePartList = FuncName.split()
        if len(FuncNamePartList) > 1:
            FuncName = FuncNamePartList[-1]
            Index = 0
            while Index < len(FuncNamePartList) - 1:
                FuncDef.Modifier += ' ' + FuncNamePartList[Index]
                Index += 1

        FuncObj = DataClass.FunctionClass(-1, FuncDef.Declarator, FuncDef.Modifier, FuncName.strip(), '', FuncDef.StartPos[0],FuncDef.StartPos[1],FuncDef.EndPos[0],FuncDef.EndPos[1], FuncDef.LeftBracePos[0], FuncDef.LeftBracePos[1], -1, ParamIdList, [])
        FuncObjList.append(FuncObj)

    return FuncObjList

## CreateCCodeDB() method
#
#  Create database for all c code
#
#  @param FileNameList: A list of all c code file names
#
def CreateCCodeDB(FileNameList):
    FileObjList = []
    ParseErrorFileList = []
    ParsedFiles = {}
    for FullName in FileNameList:
        if os.path.splitext(FullName)[1] in ('.h', '.c'):
            if FullName.lower() in ParsedFiles:
                continue
            ParsedFiles[FullName.lower()] = 1
            EdkLogger.info("Parsing " + FullName)
            model = FullName.endswith('c') and DataClass.MODEL_FILE_C or DataClass.MODEL_FILE_H
            collector = CodeFragmentCollector.CodeFragmentCollector(FullName)
            try:
                collector.ParseFile()
            except:
                ParseErrorFileList.append(FullName)
            BaseName = os.path.basename(FullName)
            DirName = os.path.dirname(FullName)
            Ext = os.path.splitext(BaseName)[1].lstrip('.')
            ModifiedTime = os.path.getmtime(FullName)
            FileObj = DataClass.FileClass(-1, BaseName, Ext, DirName, FullName, model, ModifiedTime, GetFunctionList(), GetIdentifierList(), [])
            FileObjList.append(FileObj)
            collector.CleanFileProfileBuffer()

    if len(ParseErrorFileList) > 0:
        EdkLogger.info("Found unrecoverable error during parsing:\n\t%s\n" % "\n\t".join(ParseErrorFileList))

    Db = EotGlobalData.gDb
    for file in FileObjList:
        Db.InsertOneFile(file)

    Db.UpdateIdentifierBelongsToFunction()

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':

    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.QUIET)
    CollectSourceCodeDataIntoDB(sys.argv[1])

    print 'Done!'
