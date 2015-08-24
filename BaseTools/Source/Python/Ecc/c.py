## @file
# This file is used to be the c coding style checking of ECC tool
#
# Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

import sys
import Common.LongFilePathOs as os
import re
import string
import CodeFragmentCollector
import FileProfile
from CommonDataClass import DataClass
import Database
from Common import EdkLogger
from EccToolError import *
import EccGlobalData
import MetaDataParser

IncludeFileListDict = {}
AllIncludeFileListDict = {}
IncludePathListDict = {}
ComplexTypeDict = {}
SUDict = {}
IgnoredKeywordList = ['EFI_ERROR']

def GetIgnoredDirListPattern():
    skipList = list(EccGlobalData.gConfig.SkipDirList) + ['.svn']
    DirString = string.join(skipList, '|')
    p = re.compile(r'.*[\\/](?:%s)[\\/]?.*' % DirString)
    return p

def GetFuncDeclPattern():
    p = re.compile(r'(?:EFIAPI|EFI_BOOT_SERVICE|EFI_RUNTIME_SERVICE)?\s*[_\w]+\s*\(.*\)$', re.DOTALL)
    return p

def GetArrayPattern():
    p = re.compile(r'[_\w]*\s*[\[.*\]]+')
    return p

def GetTypedefFuncPointerPattern():
    p = re.compile('[_\w\s]*\([\w\s]*\*+\s*[_\w]+\s*\)\s*\(.*\)', re.DOTALL)
    return p

def GetDB():
    return EccGlobalData.gDb

def GetConfig():
    return EccGlobalData.gConfig

def PrintErrorMsg(ErrorType, Msg, TableName, ItemId):
    Msg = Msg.replace('\n', '').replace('\r', '')
    MsgPartList = Msg.split()
    Msg = ''
    for Part in MsgPartList:
        Msg += Part
        Msg += ' '
    GetDB().TblReport.Insert(ErrorType, OtherMsg=Msg, BelongsToTable=TableName, BelongsToItem=ItemId)

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

def SuOccurInTypedef (Su, TdList):
    for Td in TdList:
        if Su.StartPos[0] == Td.StartPos[0] and Su.EndPos[0] == Td.EndPos[0]:
            return True
    return False

def GetIdentifierList():
    IdList = []
    for comment in FileProfile.CommentList:
        IdComment = DataClass.IdentifierClass(-1, '', '', '', comment.Content, DataClass.MODEL_IDENTIFIER_COMMENT, -1, -1, comment.StartPos[0], comment.StartPos[1], comment.EndPos[0], comment.EndPos[1])
        IdList.append(IdComment)

    for pp in FileProfile.PPDirectiveList:
        Type = GetIdType(pp.Content)
        IdPP = DataClass.IdentifierClass(-1, '', '', '', pp.Content, Type, -1, -1, pp.StartPos[0], pp.StartPos[1], pp.EndPos[0], pp.EndPos[1])
        IdList.append(IdPP)

    for pe in FileProfile.PredicateExpressionList:
        IdPE = DataClass.IdentifierClass(-1, '', '', '', pe.Content, DataClass.MODEL_IDENTIFIER_PREDICATE_EXPRESSION, -1, -1, pe.StartPos[0], pe.StartPos[1], pe.EndPos[0], pe.EndPos[1])
        IdList.append(IdPE)

    FuncDeclPattern = GetFuncDeclPattern()
    ArrayPattern = GetArrayPattern()
    for var in FileProfile.VariableDeclarationList:
        DeclText = var.Declarator.lstrip()
        FuncPointerPattern = GetTypedefFuncPointerPattern()
        if FuncPointerPattern.match(DeclText):
            continue
        VarNameStartLine = var.NameStartPos[0]
        VarNameStartColumn = var.NameStartPos[1]
        FirstChar = DeclText[0]
        while not FirstChar.isalpha() and FirstChar != '_':
            if FirstChar == '*':
                var.Modifier += '*'
                VarNameStartColumn += 1
                DeclText = DeclText.lstrip('*')
            elif FirstChar == '\r':
                DeclText = DeclText.lstrip('\r\n').lstrip('\r')
                VarNameStartLine += 1
                VarNameStartColumn = 0
            elif FirstChar == '\n':
                DeclText = DeclText.lstrip('\n')
                VarNameStartLine += 1
                VarNameStartColumn = 0
            elif FirstChar == ' ':
                DeclText = DeclText.lstrip(' ')
                VarNameStartColumn += 1
            elif FirstChar == '\t':
                DeclText = DeclText.lstrip('\t')
                VarNameStartColumn += 8
            else:
                DeclText = DeclText[1:]
                VarNameStartColumn += 1
            FirstChar = DeclText[0]

        var.Declarator = DeclText
        if FuncDeclPattern.match(var.Declarator):
            DeclSplitList = var.Declarator.split('(')
            FuncName = DeclSplitList[0].strip()
            FuncNamePartList = FuncName.split()
            if len(FuncNamePartList) > 1:
                FuncName = FuncNamePartList[-1].strip()
                NameStart = DeclSplitList[0].rfind(FuncName)
                var.Declarator = var.Declarator[NameStart:]
                if NameStart > 0:
                    var.Modifier += ' ' + DeclSplitList[0][0:NameStart]
                    Index = 0
                    PreChar = ''
                    while Index < NameStart:
                        FirstChar = DeclSplitList[0][Index]
                        if DeclSplitList[0][Index:].startswith('EFIAPI'):
                            Index += 6
                            VarNameStartColumn += 6
                            PreChar = ''
                            continue
                        elif FirstChar == '\r':
                            Index += 1
                            VarNameStartLine += 1
                            VarNameStartColumn = 0
                        elif FirstChar == '\n':
                            Index += 1
                            if PreChar != '\r':
                                VarNameStartLine += 1
                                VarNameStartColumn = 0
                        elif FirstChar == ' ':
                            Index += 1
                            VarNameStartColumn += 1
                        elif FirstChar == '\t':
                            Index += 1
                            VarNameStartColumn += 8
                        else:
                            Index += 1
                            VarNameStartColumn += 1
                        PreChar = FirstChar
            IdVar = DataClass.IdentifierClass(-1, var.Modifier, '', var.Declarator, FuncName, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION, -1, -1, var.StartPos[0], var.StartPos[1], VarNameStartLine, VarNameStartColumn)
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

                IdVar = DataClass.IdentifierClass(-1, var.Modifier, '', Name, (len(DeclList) > 1 and [DeclList[1]]or [''])[0], DataClass.MODEL_IDENTIFIER_VARIABLE, -1, -1, var.StartPos[0], var.StartPos[1], VarNameStartLine, VarNameStartColumn)
                IdList.append(IdVar)
        else:
            DeclList = var.Declarator.split('=')
            Name = DeclList[0].strip()
            if ArrayPattern.match(Name):
                LSBPos = var.Declarator.find('[')
                var.Modifier += ' ' + Name[LSBPos:]
                Name = Name[0:LSBPos]
            IdVar = DataClass.IdentifierClass(-1, var.Modifier, '', Name, (len(DeclList) > 1 and [DeclList[1]]or [''])[0], DataClass.MODEL_IDENTIFIER_VARIABLE, -1, -1, var.StartPos[0], var.StartPos[1], VarNameStartLine, VarNameStartColumn)
            IdList.append(IdVar)

    for enum in FileProfile.EnumerationDefinitionList:
        LBPos = enum.Content.find('{')
        RBPos = enum.Content.find('}')
        Name = enum.Content[4:LBPos].strip()
        Value = enum.Content[LBPos + 1:RBPos]
        IdEnum = DataClass.IdentifierClass(-1, '', '', Name, Value, DataClass.MODEL_IDENTIFIER_ENUMERATE, -1, -1, enum.StartPos[0], enum.StartPos[1], enum.EndPos[0], enum.EndPos[1])
        IdList.append(IdEnum)

    for su in FileProfile.StructUnionDefinitionList:
        if SuOccurInTypedef(su, FileProfile.TypedefDefinitionList):
            continue
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
            Value = su.Content[LBPos:RBPos + 1]
        IdPE = DataClass.IdentifierClass(-1, '', '', Name, Value, Type, -1, -1, su.StartPos[0], su.StartPos[1], su.EndPos[0], su.EndPos[1])
        IdList.append(IdPE)

    TdFuncPointerPattern = GetTypedefFuncPointerPattern()
    for td in FileProfile.TypedefDefinitionList:
        Modifier = ''
        Name = td.ToType
        Value = td.FromType
        if TdFuncPointerPattern.match(td.ToType):
            Modifier = td.FromType
            LBPos = td.ToType.find('(')
            TmpStr = td.ToType[LBPos + 1:].strip()
            StarPos = TmpStr.find('*')
            if StarPos != -1:
                Modifier += ' ' + TmpStr[0:StarPos]
            while TmpStr[StarPos] == '*':
#                Modifier += ' ' + '*'
                StarPos += 1
            TmpStr = TmpStr[StarPos:].strip()
            RBPos = TmpStr.find(')')
            Name = TmpStr[0:RBPos]
            Value = 'FP' + TmpStr[RBPos + 1:]
        else:
            while Name.startswith('*'):
                Value += ' ' + '*'
                Name = Name.lstrip('*').strip()

        if Name.find('[') != -1:
            LBPos = Name.find('[')
            RBPos = Name.rfind(']')
            Value += Name[LBPos : RBPos + 1]
            Name = Name[0 : LBPos]

        IdTd = DataClass.IdentifierClass(-1, Modifier, '', Name, Value, DataClass.MODEL_IDENTIFIER_TYPEDEF, -1, -1, td.StartPos[0], td.StartPos[1], td.EndPos[0], td.EndPos[1])
        IdList.append(IdTd)

    for funcCall in FileProfile.FunctionCallingList:
        IdFC = DataClass.IdentifierClass(-1, '', '', funcCall.FuncName, funcCall.ParamList, DataClass.MODEL_IDENTIFIER_FUNCTION_CALLING, -1, -1, funcCall.StartPos[0], funcCall.StartPos[1], funcCall.EndPos[0], funcCall.EndPos[1])
        IdList.append(IdFC)
    return IdList

def StripNonAlnumChars(Str):
    StrippedStr = ''
    for Char in Str:
        if Char.isalnum():
            StrippedStr += Char
    return StrippedStr

def GetParamList(FuncDeclarator, FuncNameLine=0, FuncNameOffset=0):
    FuncDeclarator = StripComments(FuncDeclarator)
    ParamIdList = []
    #DeclSplitList = FuncDeclarator.split('(')
    LBPos = FuncDeclarator.find('(')
    #if len(DeclSplitList) < 2:
    if LBPos == -1:
        return ParamIdList
    #FuncName = DeclSplitList[0]
    FuncName = FuncDeclarator[0:LBPos]
    #ParamStr = DeclSplitList[1].rstrip(')')
    ParamStr = FuncDeclarator[LBPos + 1:].rstrip(')')
    LineSkipped = 0
    OffsetSkipped = 0
    TailChar = FuncName[-1]
    while not TailChar.isalpha() and TailChar != '_':

        if TailChar == '\n':
            FuncName = FuncName.rstrip('\r\n').rstrip('\n')
            LineSkipped += 1
            OffsetSkipped = 0
        elif TailChar == '\r':
            FuncName = FuncName.rstrip('\r')
            LineSkipped += 1
            OffsetSkipped = 0
        elif TailChar == ' ':
            FuncName = FuncName.rstrip(' ')
            OffsetSkipped += 1
        elif TailChar == '\t':
            FuncName = FuncName.rstrip('\t')
            OffsetSkipped += 8
        else:
            FuncName = FuncName[:-1]
        TailChar = FuncName[-1]

    OffsetSkipped += 1 #skip '('

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
        # ignore array length if exists.
        LBIndex = ParamName.find('[')
        if LBIndex != -1:
            ParamName = ParamName[0:LBIndex]

        Start = RightSpacePos
        Index = 0
        PreChar = ''
        while Index < Start:
            FirstChar = p[Index]

            if FirstChar == '\r':
                Index += 1
                LineSkipped += 1
                OffsetSkipped = 0
            elif FirstChar == '\n':
                Index += 1
                if PreChar != '\r':
                    LineSkipped += 1
                    OffsetSkipped = 0
            elif FirstChar == ' ':
                Index += 1
                OffsetSkipped += 1
            elif FirstChar == '\t':
                Index += 1
                OffsetSkipped += 8
            else:
                Index += 1
                OffsetSkipped += 1
            PreChar = FirstChar

        ParamBeginLine = FuncNameLine + LineSkipped
        ParamBeginOffset = FuncNameOffset + OffsetSkipped

        Index = Start + len(ParamName)
        PreChar = ''
        while Index < len(p):
            FirstChar = p[Index]

            if FirstChar == '\r':
                Index += 1
                LineSkipped += 1
                OffsetSkipped = 0
            elif FirstChar == '\n':
                Index += 1
                if PreChar != '\r':
                    LineSkipped += 1
                    OffsetSkipped = 0
            elif FirstChar == ' ':
                Index += 1
                OffsetSkipped += 1
            elif FirstChar == '\t':
                Index += 1
                OffsetSkipped += 8
            else:
                Index += 1
                OffsetSkipped += 1
            PreChar = FirstChar

        ParamEndLine = FuncNameLine + LineSkipped
        ParamEndOffset = FuncNameOffset + OffsetSkipped
        if ParamName != '...':
            ParamName = StripNonAlnumChars(ParamName)
        IdParam = DataClass.IdentifierClass(-1, ParamModifier, '', ParamName, '', DataClass.MODEL_IDENTIFIER_PARAMETER, -1, -1, ParamBeginLine, ParamBeginOffset, ParamEndLine, ParamEndOffset)
        ParamIdList.append(IdParam)

        OffsetSkipped += 1 #skip ','

    return ParamIdList

def GetFunctionList():
    FuncObjList = []
    for FuncDef in FileProfile.FunctionDefinitionList:
        ParamIdList = []
        DeclText = FuncDef.Declarator.lstrip()
        FuncNameStartLine = FuncDef.NamePos[0]
        FuncNameStartColumn = FuncDef.NamePos[1]
        FirstChar = DeclText[0]
        while not FirstChar.isalpha() and FirstChar != '_':
            if FirstChar == '*':
                FuncDef.Modifier += '*'
                FuncNameStartColumn += 1
                DeclText = DeclText.lstrip('*')
            elif FirstChar == '\r':
                DeclText = DeclText.lstrip('\r\n').lstrip('\r')
                FuncNameStartLine += 1
                FuncNameStartColumn = 0
            elif FirstChar == '\n':
                DeclText = DeclText.lstrip('\n')
                FuncNameStartLine += 1
                FuncNameStartColumn = 0
            elif FirstChar == ' ':
                DeclText = DeclText.lstrip(' ')
                FuncNameStartColumn += 1
            elif FirstChar == '\t':
                DeclText = DeclText.lstrip('\t')
                FuncNameStartColumn += 8
            else:
                DeclText = DeclText[1:]
                FuncNameStartColumn += 1
            FirstChar = DeclText[0]

        FuncDef.Declarator = DeclText
        DeclSplitList = FuncDef.Declarator.split('(')
        if len(DeclSplitList) < 2:
            continue

        FuncName = DeclSplitList[0]
        FuncNamePartList = FuncName.split()
        if len(FuncNamePartList) > 1:
            FuncName = FuncNamePartList[-1]
            NameStart = DeclSplitList[0].rfind(FuncName)
            if NameStart > 0:
                FuncDef.Modifier += ' ' + DeclSplitList[0][0:NameStart]
                Index = 0
                PreChar = ''
                while Index < NameStart:
                    FirstChar = DeclSplitList[0][Index]
                    if DeclSplitList[0][Index:].startswith('EFIAPI'):
                        Index += 6
                        FuncNameStartColumn += 6
                        PreChar = ''
                        continue
                    elif FirstChar == '\r':
                        Index += 1
                        FuncNameStartLine += 1
                        FuncNameStartColumn = 0
                    elif FirstChar == '\n':
                        Index += 1
                        if PreChar != '\r':
                            FuncNameStartLine += 1
                            FuncNameStartColumn = 0
                    elif FirstChar == ' ':
                        Index += 1
                        FuncNameStartColumn += 1
                    elif FirstChar == '\t':
                        Index += 1
                        FuncNameStartColumn += 8
                    else:
                        Index += 1
                        FuncNameStartColumn += 1
                    PreChar = FirstChar

        FuncObj = DataClass.FunctionClass(-1, FuncDef.Declarator, FuncDef.Modifier, FuncName.strip(), '', FuncDef.StartPos[0], FuncDef.StartPos[1], FuncDef.EndPos[0], FuncDef.EndPos[1], FuncDef.LeftBracePos[0], FuncDef.LeftBracePos[1], -1, ParamIdList, [], FuncNameStartLine, FuncNameStartColumn)
        FuncObjList.append(FuncObj)

    return FuncObjList

def GetFileModificationTimeFromDB(FullFileName):
    TimeValue = 0.0
    Db = GetDB()
    SqlStatement = """ select TimeStamp
                       from File
                       where FullPath = \'%s\'
                   """ % (FullFileName)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        TimeValue = Result[0]
    return TimeValue

def CollectSourceCodeDataIntoDB(RootDir):
    FileObjList = []
    tuple = os.walk(RootDir)
    IgnoredPattern = GetIgnoredDirListPattern()
    ParseErrorFileList = []

    for dirpath, dirnames, filenames in tuple:
        if IgnoredPattern.match(dirpath.upper()):
            continue

        for Dir in dirnames:
            Dirname = os.path.join(dirpath, Dir)
            if os.path.islink(Dirname):
                Dirname = os.path.realpath(Dirname)
                if os.path.isdir(Dirname):
                    # symlinks to directories are treated as directories
                    dirnames.remove(Dir)
                    dirnames.append(Dirname)

        for f in filenames:
            if f.lower() in EccGlobalData.gConfig.SkipFileList:
                continue
            collector = None
            FullName = os.path.normpath(os.path.join(dirpath, f))
            model = DataClass.MODEL_FILE_OTHERS
            if os.path.splitext(f)[1] in ('.h', '.c'):
                EdkLogger.info("Parsing " + FullName)
                model = f.endswith('c') and DataClass.MODEL_FILE_C or DataClass.MODEL_FILE_H
                collector = CodeFragmentCollector.CodeFragmentCollector(FullName)
                try:
                    collector.ParseFile()
                except UnicodeError:
                    ParseErrorFileList.append(FullName)
                    collector.CleanFileProfileBuffer()
                    collector.ParseFileWithClearedPPDirective()
#                collector.PrintFragments()
            BaseName = os.path.basename(f)
            DirName = os.path.dirname(FullName)
            Ext = os.path.splitext(f)[1].lstrip('.')
            ModifiedTime = os.path.getmtime(FullName)
            FileObj = DataClass.FileClass(-1, BaseName, Ext, DirName, FullName, model, ModifiedTime, GetFunctionList(), GetIdentifierList(), [])
            FileObjList.append(FileObj)
            if collector:
                collector.CleanFileProfileBuffer()

    if len(ParseErrorFileList) > 0:
        EdkLogger.info("Found unrecoverable error during parsing:\n\t%s\n" % "\n\t".join(ParseErrorFileList))

    Db = GetDB()
    for file in FileObjList:
        if file.ExtName.upper() not in ['INF', 'DEC', 'DSC', 'FDF']:
            Db.InsertOneFile(file)

    Db.UpdateIdentifierBelongsToFunction()

def GetTableID(FullFileName, ErrorMsgList=None):
    if ErrorMsgList == None:
        ErrorMsgList = []

    Db = GetDB()
    SqlStatement = """ select ID
                       from File
                       where FullPath like '%s'
                   """ % FullFileName
    ResultSet = Db.TblFile.Exec(SqlStatement)

    FileID = -1
    for Result in ResultSet:
        if FileID != -1:
            ErrorMsgList.append('Duplicate file ID found in DB for file %s' % FullFileName)
            return - 2
        FileID = Result[0]
    if FileID == -1:
        ErrorMsgList.append('NO file ID found in DB for file %s' % FullFileName)
        return - 1
    return FileID

def GetIncludeFileList(FullFileName):
    if os.path.splitext(FullFileName)[1].upper() not in ('.H'):
        return []
    IFList = IncludeFileListDict.get(FullFileName)
    if IFList != None:
        return IFList

    FileID = GetTableID(FullFileName)
    if FileID < 0:
        return []

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Value
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_INCLUDE)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    IncludeFileListDict[FullFileName] = ResultSet
    return ResultSet

def GetFullPathOfIncludeFile(Str, IncludePathList):
    for IncludePath in IncludePathList:
        FullPath = os.path.join(IncludePath, Str)
        FullPath = os.path.normpath(FullPath)
        if os.path.exists(FullPath):
            return FullPath
    return None

def GetAllIncludeFiles(FullFileName):
    if AllIncludeFileListDict.get(FullFileName) != None:
        return AllIncludeFileListDict.get(FullFileName)

    FileDirName = os.path.dirname(FullFileName)
    IncludePathList = IncludePathListDict.get(FileDirName)
    if IncludePathList == None:
        IncludePathList = MetaDataParser.GetIncludeListOfFile(EccGlobalData.gWorkspace, FullFileName, GetDB())
        if FileDirName not in IncludePathList:
            IncludePathList.insert(0, FileDirName)
        IncludePathListDict[FileDirName] = IncludePathList
    IncludeFileQueue = []
    for IncludeFile in GetIncludeFileList(FullFileName):
        FileName = IncludeFile[0].lstrip('#').strip()
        FileName = FileName.lstrip('include').strip()
        FileName = FileName.strip('\"')
        FileName = FileName.lstrip('<').rstrip('>').strip()
        FullPath = GetFullPathOfIncludeFile(FileName, IncludePathList)
        if FullPath != None:
            IncludeFileQueue.append(FullPath)

    i = 0
    while i < len(IncludeFileQueue):
        for IncludeFile in GetIncludeFileList(IncludeFileQueue[i]):
            FileName = IncludeFile[0].lstrip('#').strip()
            FileName = FileName.lstrip('include').strip()
            FileName = FileName.strip('\"')
            FileName = FileName.lstrip('<').rstrip('>').strip()
            FullPath = GetFullPathOfIncludeFile(FileName, IncludePathList)
            if FullPath != None and FullPath not in IncludeFileQueue:
                IncludeFileQueue.insert(i + 1, FullPath)
        i += 1

    AllIncludeFileListDict[FullFileName] = IncludeFileQueue
    return IncludeFileQueue

def GetPredicateListFromPredicateExpStr(PES):

    PredicateList = []
    i = 0
    PredicateBegin = 0
    #PredicateEnd = 0
    LogicOpPos = -1
    p = GetFuncDeclPattern()
    while i < len(PES) - 1:
        if (PES[i].isalnum() or PES[i] == '_' or PES[i] == '*') and LogicOpPos > PredicateBegin:
            PredicateBegin = i
        if (PES[i] == '&' and PES[i + 1] == '&') or (PES[i] == '|' and PES[i + 1] == '|'):
            LogicOpPos = i
            Exp = PES[PredicateBegin:i].strip()
            # Exp may contain '.' or '->'
            TmpExp = Exp.replace('.', '').replace('->', '')
            if p.match(TmpExp):
                PredicateList.append(Exp)
            else:
                PredicateList.append(Exp.rstrip(';').rstrip(')').strip())
        i += 1

    if PredicateBegin > LogicOpPos:
        while PredicateBegin < len(PES):
            if PES[PredicateBegin].isalnum() or PES[PredicateBegin] == '_' or PES[PredicateBegin] == '*':
                break
            PredicateBegin += 1
        Exp = PES[PredicateBegin:len(PES)].strip()
        # Exp may contain '.' or '->'
        TmpExp = Exp.replace('.', '').replace('->', '')
        if p.match(TmpExp):
            PredicateList.append(Exp)
        else:
            PredicateList.append(Exp.rstrip(';').rstrip(')').strip())
    return PredicateList

def GetCNameList(Lvalue, StarList=[]):
    Lvalue += ' '
    i = 0
    SearchBegin = 0
    VarStart = -1
    VarEnd = -1
    VarList = []

    while SearchBegin < len(Lvalue):
        while i < len(Lvalue):
            if Lvalue[i].isalnum() or Lvalue[i] == '_':
                if VarStart == -1:
                    VarStart = i
                VarEnd = i
                i += 1
            elif VarEnd != -1:
                VarList.append(Lvalue[VarStart:VarEnd + 1])
                i += 1
                break
            else:
                if VarStart == -1 and Lvalue[i] == '*':
                    StarList.append('*')
                i += 1
        if VarEnd == -1:
            break


        DotIndex = Lvalue[VarEnd:].find('.')
        ArrowIndex = Lvalue[VarEnd:].find('->')
        if DotIndex == -1 and ArrowIndex == -1:
            break
        elif DotIndex == -1 and ArrowIndex != -1:
            SearchBegin = VarEnd + ArrowIndex
        elif ArrowIndex == -1 and DotIndex != -1:
            SearchBegin = VarEnd + DotIndex
        else:
            SearchBegin = VarEnd + ((DotIndex < ArrowIndex) and DotIndex or ArrowIndex)

        i = SearchBegin
        VarStart = -1
        VarEnd = -1

    return VarList

def SplitPredicateByOp(Str, Op, IsFuncCalling=False):

    Name = Str.strip()
    Value = None

    if IsFuncCalling:
        Index = 0
        LBFound = False
        UnmatchedLBCount = 0
        while Index < len(Str):
            while not LBFound and Str[Index] != '_' and not Str[Index].isalnum():
                Index += 1

            while not LBFound and (Str[Index].isalnum() or Str[Index] == '_'):
                Index += 1
            # maybe type-cast at the begining, skip it.
            RemainingStr = Str[Index:].lstrip()
            if RemainingStr.startswith(')') and not LBFound:
                Index += 1
                continue

            if RemainingStr.startswith('(') and not LBFound:
                LBFound = True

            if Str[Index] == '(':
                UnmatchedLBCount += 1
                Index += 1
                continue

            if Str[Index] == ')':
                UnmatchedLBCount -= 1
                Index += 1
                if UnmatchedLBCount == 0:
                    break
                continue

            Index += 1

        if UnmatchedLBCount > 0:
            return [Name]

        IndexInRemainingStr = Str[Index:].find(Op)
        if IndexInRemainingStr == -1:
            return [Name]

        Name = Str[0:Index + IndexInRemainingStr].strip()
        Value = Str[Index + IndexInRemainingStr + len(Op):].strip().strip(')')
        return [Name, Value]

    TmpStr = Str.rstrip(';').rstrip(')')
    while True:
        Index = TmpStr.rfind(Op)
        if Index == -1:
            return [Name]

        if Str[Index - 1].isalnum() or Str[Index - 1].isspace() or Str[Index - 1] == ')' or Str[Index - 1] == ']':
            Name = Str[0:Index].strip()
            Value = Str[Index + len(Op):].strip()
            return [Name, Value]

        TmpStr = Str[0:Index - 1]

def SplitPredicateStr(Str):

    Str = Str.lstrip('(')
    IsFuncCalling = False
    p = GetFuncDeclPattern()
    TmpStr = Str.replace('.', '').replace('->', '')
    if p.match(TmpStr):
        IsFuncCalling = True

    PredPartList = SplitPredicateByOp(Str, '==', IsFuncCalling)
    if len(PredPartList) > 1:
        return [PredPartList, '==']

    PredPartList = SplitPredicateByOp(Str, '!=', IsFuncCalling)
    if len(PredPartList) > 1:
        return [PredPartList, '!=']

    PredPartList = SplitPredicateByOp(Str, '>=', IsFuncCalling)
    if len(PredPartList) > 1:
        return [PredPartList, '>=']

    PredPartList = SplitPredicateByOp(Str, '<=', IsFuncCalling)
    if len(PredPartList) > 1:
        return [PredPartList, '<=']

    PredPartList = SplitPredicateByOp(Str, '>', IsFuncCalling)
    if len(PredPartList) > 1:
        return [PredPartList, '>']

    PredPartList = SplitPredicateByOp(Str, '<', IsFuncCalling)
    if len(PredPartList) > 1:
        return [PredPartList, '<']

    return [[Str, None], None]

def GetFuncContainsPE(ExpLine, ResultSet):
    for Result in ResultSet:
        if Result[0] < ExpLine and Result[1] > ExpLine:
            return Result
    return None

def PatternInModifier(Modifier, SubStr):
    PartList = Modifier.split()
    for Part in PartList:
        if Part == SubStr:
            return True
    return False

def GetDataTypeFromModifier(ModifierStr):
    MList = ModifierStr.split()
    ReturnType = ''
    for M in MList:
        if M in EccGlobalData.gConfig.ModifierList:
            continue
        # remove array sufix
        if M.startswith('[') or M.endswith(']'):
            continue
        ReturnType += M + ' '

    ReturnType = ReturnType.strip()
    if len(ReturnType) == 0:
        ReturnType = 'VOID'
    return ReturnType

def DiffModifier(Str1, Str2):
    PartList1 = Str1.split()
    PartList2 = Str2.split()
    if PartList1 == PartList2:
        return False
    else:
        return True

def GetTypedefDict(FullFileName):

    Dict = ComplexTypeDict.get(FullFileName)
    if Dict != None:
        return Dict

    FileID = GetTableID(FullFileName)
    FileTable = 'Identifier' + str(FileID)
    Db = GetDB()
    SqlStatement = """ select Modifier, Name, Value, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_TYPEDEF)
    ResultSet = Db.TblFile.Exec(SqlStatement)

    Dict = {}
    for Result in ResultSet:
        if len(Result[0]) == 0:
            Dict[Result[1]] = Result[2]

    IncludeFileList = GetAllIncludeFiles(FullFileName)
    for F in IncludeFileList:
        FileID = GetTableID(F)
        if FileID < 0:
            continue

        FileTable = 'Identifier' + str(FileID)
        SqlStatement = """ select Modifier, Name, Value, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_TYPEDEF)
        ResultSet = Db.TblFile.Exec(SqlStatement)

        for Result in ResultSet:
            if not Result[2].startswith('FP ('):
                Dict[Result[1]] = Result[2]
            else:
                if len(Result[0]) == 0:
                    Dict[Result[1]] = 'VOID'
                else:
                    Dict[Result[1]] = GetDataTypeFromModifier(Result[0])

    ComplexTypeDict[FullFileName] = Dict
    return Dict

def GetSUDict(FullFileName):

    Dict = SUDict.get(FullFileName)
    if Dict != None:
        return Dict

    FileID = GetTableID(FullFileName)
    FileTable = 'Identifier' + str(FileID)
    Db = GetDB()
    SqlStatement = """ select Name, Value, ID
                       from %s
                       where Model = %d or Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_STRUCTURE, DataClass.MODEL_IDENTIFIER_UNION)
    ResultSet = Db.TblFile.Exec(SqlStatement)

    Dict = {}
    for Result in ResultSet:
        if len(Result[1]) > 0:
            Dict[Result[0]] = Result[1]

    IncludeFileList = GetAllIncludeFiles(FullFileName)
    for F in IncludeFileList:
        FileID = GetTableID(F)
        if FileID < 0:
            continue

        FileTable = 'Identifier' + str(FileID)
        SqlStatement = """ select Name, Value, ID
                       from %s
                       where Model = %d or Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_STRUCTURE, DataClass.MODEL_IDENTIFIER_UNION)
        ResultSet = Db.TblFile.Exec(SqlStatement)

        for Result in ResultSet:
            if len(Result[1]) > 0:
                Dict[Result[0]] = Result[1]

    SUDict[FullFileName] = Dict
    return Dict

def StripComments(Str):
    Str += '   '
    ListFromStr = list(Str)

    InComment = False
    DoubleSlashComment = False
    Index = 0
    while Index < len(ListFromStr):
        # meet new line, then no longer in a comment for //
        if ListFromStr[Index] == '\n':
            if InComment and DoubleSlashComment:
                InComment = False
                DoubleSlashComment = False
            Index += 1
        # check for */ comment end
        elif InComment and not DoubleSlashComment and ListFromStr[Index] == '*' and ListFromStr[Index + 1] == '/':
            ListFromStr[Index] = ' '
            Index += 1
            ListFromStr[Index] = ' '
            Index += 1
            InComment = False
        # set comments to spaces
        elif InComment:
            ListFromStr[Index] = ' '
            Index += 1
        # check for // comment
        elif ListFromStr[Index] == '/' and ListFromStr[Index + 1] == '/' and ListFromStr[Index + 2] != '\n':
            InComment = True
            DoubleSlashComment = True

        # check for /* comment start
        elif ListFromStr[Index] == '/' and ListFromStr[Index + 1] == '*':
            ListFromStr[Index] = ' '
            Index += 1
            ListFromStr[Index] = ' '
            Index += 1
            InComment = True
        else:
            Index += 1

    # restore from List to String
    Str = "".join(ListFromStr)
    Str = Str.rstrip(' ')

    return Str

def GetFinalTypeValue(Type, FieldName, TypedefDict, SUDict):
    Value = TypedefDict.get(Type)
    if Value == None:
        Value = SUDict.get(Type)
    if Value == None:
        return None

    LBPos = Value.find('{')
    while LBPos == -1:
        FTList = Value.split()
        for FT in FTList:
            if FT not in ('struct', 'union'):
                Value = TypedefDict.get(FT)
                if Value == None:
                    Value = SUDict.get(FT)
                break

        if Value == None:
            return None

        LBPos = Value.find('{')

#    RBPos = Value.find('}')
    Fields = Value[LBPos + 1:]
    Fields = StripComments(Fields)
    FieldsList = Fields.split(';')
    for Field in FieldsList:
        Field = Field.strip()
        Index = Field.rfind(FieldName)
        if Index < 1:
            continue
        if not Field[Index - 1].isalnum():
            if Index + len(FieldName) == len(Field):
                Type = GetDataTypeFromModifier(Field[0:Index])
                return Type.strip()
            else:
            # For the condition that the field in struct is an array with [] sufixes...
                if not Field[Index + len(FieldName)].isalnum():
                    Type = GetDataTypeFromModifier(Field[0:Index])
                    return Type.strip()

    return None

def GetRealType(Type, TypedefDict, TargetType=None):
    if TargetType != None and Type == TargetType:
            return Type
    while TypedefDict.get(Type):
        Type = TypedefDict.get(Type)
        if TargetType != None and Type == TargetType:
            return Type
    return Type

def GetTypeInfo(RefList, Modifier, FullFileName, TargetType=None):
    TypedefDict = GetTypedefDict(FullFileName)
    SUDict = GetSUDict(FullFileName)
    Type = GetDataTypeFromModifier(Modifier).replace('*', '').strip()

    Type = Type.split()[-1]
    Index = 0
    while Index < len(RefList):
        FieldName = RefList[Index]
        FromType = GetFinalTypeValue(Type, FieldName, TypedefDict, SUDict)
        if FromType == None:
            return None
        # we want to determine the exact type.
        if TargetType != None:
            Type = FromType.split()[0]
        # we only want to check if it is a pointer
        else:
            Type = FromType
            if Type.find('*') != -1 and Index == len(RefList) - 1:
                return Type
            Type = FromType.split()[0]

        Index += 1

    Type = GetRealType(Type, TypedefDict, TargetType)

    return Type

def GetVarInfo(PredVarList, FuncRecord, FullFileName, IsFuncCall=False, TargetType=None, StarList=None):

    PredVar = PredVarList[0]
    FileID = GetTableID(FullFileName)

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    # search variable in include files

    # it is a function call, search function declarations and definitions
    if IsFuncCall:
        SqlStatement = """ select Modifier, ID
                       from %s
                       where Model = %d and Value = \'%s\'
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION, PredVar)
        ResultSet = Db.TblFile.Exec(SqlStatement)

        for Result in ResultSet:
            Type = GetDataTypeFromModifier(Result[0]).split()[-1]
            TypedefDict = GetTypedefDict(FullFileName)
            Type = GetRealType(Type, TypedefDict, TargetType)
            return Type

        IncludeFileList = GetAllIncludeFiles(FullFileName)
        for F in IncludeFileList:
            FileID = GetTableID(F)
            if FileID < 0:
                continue

            FileTable = 'Identifier' + str(FileID)
            SqlStatement = """ select Modifier, ID
                           from %s
                           where Model = %d and Value = \'%s\'
                       """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION, PredVar)
            ResultSet = Db.TblFile.Exec(SqlStatement)

            for Result in ResultSet:
                Type = GetDataTypeFromModifier(Result[0]).split()[-1]
                TypedefDict = GetTypedefDict(FullFileName)
                Type = GetRealType(Type, TypedefDict, TargetType)
                return Type

        FileID = GetTableID(FullFileName)
        SqlStatement = """ select Modifier, ID
                       from Function
                       where BelongsToFile = %d and Name = \'%s\'
                   """ % (FileID, PredVar)
        ResultSet = Db.TblFile.Exec(SqlStatement)

        for Result in ResultSet:
            Type = GetDataTypeFromModifier(Result[0]).split()[-1]
            TypedefDict = GetTypedefDict(FullFileName)
            Type = GetRealType(Type, TypedefDict, TargetType)
            return Type

        for F in IncludeFileList:
            FileID = GetTableID(F)
            if FileID < 0:
                continue

            FileTable = 'Identifier' + str(FileID)
            SqlStatement = """ select Modifier, ID
                           from Function
                           where BelongsToFile = %d and Name = \'%s\'
                       """ % (FileID, PredVar)
            ResultSet = Db.TblFile.Exec(SqlStatement)

            for Result in ResultSet:
                Type = GetDataTypeFromModifier(Result[0]).split()[-1]
                TypedefDict = GetTypedefDict(FullFileName)
                Type = GetRealType(Type, TypedefDict, TargetType)
                return Type

        return None

    # really variable, search local variable first
    SqlStatement = """ select Modifier, ID
                       from %s
                       where Model = %d and Name = \'%s\' and StartLine >= %d and StartLine <= %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_VARIABLE, PredVar, FuncRecord[0], FuncRecord[1])
    ResultSet = Db.TblFile.Exec(SqlStatement)
    VarFound = False
    for Result in ResultSet:
        if len(PredVarList) > 1:
            Type = GetTypeInfo(PredVarList[1:], Result[0], FullFileName, TargetType)
            return Type
        else:
#            Type = GetDataTypeFromModifier(Result[0]).split()[-1]
            TypeList = GetDataTypeFromModifier(Result[0]).split()
            Type = TypeList[-1]
            if len(TypeList) > 1 and StarList != None:
                for Star in StarList:
                    Type = Type.strip()
                    Type = Type.rstrip(Star)
                # Get real type after de-reference pointers.
                if len(Type.strip()) == 0:
                    Type = TypeList[-2]
            TypedefDict = GetTypedefDict(FullFileName)
            Type = GetRealType(Type, TypedefDict, TargetType)
            return Type

    # search function parameters second
    ParamList = GetParamList(FuncRecord[2])
    for Param in ParamList:
        if Param.Name.strip() == PredVar:
            if len(PredVarList) > 1:
                Type = GetTypeInfo(PredVarList[1:], Param.Modifier, FullFileName, TargetType)
                return Type
            else:
                TypeList = GetDataTypeFromModifier(Param.Modifier).split()
                Type = TypeList[-1]
                if Type == '*' and len(TypeList) >= 2:
                    Type = TypeList[-2]
                if len(TypeList) > 1 and StarList != None:
                    for Star in StarList:
                        Type = Type.strip()
                        Type = Type.rstrip(Star)
                    # Get real type after de-reference pointers.
                    if len(Type.strip()) == 0:
                        Type = TypeList[-2]
                TypedefDict = GetTypedefDict(FullFileName)
                Type = GetRealType(Type, TypedefDict, TargetType)
                return Type

    # search global variable next
    SqlStatement = """ select Modifier, ID
           from %s
           where Model = %d and Name = \'%s\' and BelongsToFunction = -1
       """ % (FileTable, DataClass.MODEL_IDENTIFIER_VARIABLE, PredVar)
    ResultSet = Db.TblFile.Exec(SqlStatement)

    for Result in ResultSet:
        if len(PredVarList) > 1:
            Type = GetTypeInfo(PredVarList[1:], Result[0], FullFileName, TargetType)
            return Type
        else:
            TypeList = GetDataTypeFromModifier(Result[0]).split()
            Type = TypeList[-1]
            if len(TypeList) > 1 and StarList != None:
                for Star in StarList:
                    Type = Type.strip()
                    Type = Type.rstrip(Star)
                # Get real type after de-reference pointers.
                if len(Type.strip()) == 0:
                    Type = TypeList[-2]
            TypedefDict = GetTypedefDict(FullFileName)
            Type = GetRealType(Type, TypedefDict, TargetType)
            return Type

    IncludeFileList = GetAllIncludeFiles(FullFileName)
    for F in IncludeFileList:
        FileID = GetTableID(F)
        if FileID < 0:
            continue

        FileTable = 'Identifier' + str(FileID)
        SqlStatement = """ select Modifier, ID
                       from %s
                       where Model = %d and BelongsToFunction = -1 and Name = \'%s\'
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_VARIABLE, PredVar)
        ResultSet = Db.TblFile.Exec(SqlStatement)

        for Result in ResultSet:
            if len(PredVarList) > 1:
                Type = GetTypeInfo(PredVarList[1:], Result[0], FullFileName, TargetType)
                return Type
            else:
                TypeList = GetDataTypeFromModifier(Result[0]).split()
                Type = TypeList[-1]
                if len(TypeList) > 1 and StarList != None:
                    for Star in StarList:
                        Type = Type.strip()
                        Type = Type.rstrip(Star)
                    # Get real type after de-reference pointers.
                    if len(Type.strip()) == 0:
                        Type = TypeList[-2]
                TypedefDict = GetTypedefDict(FullFileName)
                Type = GetRealType(Type, TypedefDict, TargetType)
                return Type

def GetTypeFromArray(Type, Var):
    Count = Var.count('[')

    while Count > 0:
        Type = Type.strip()
        Type = Type.rstrip('*')
        Count = Count - 1

    return Type

def CheckFuncLayoutReturnType(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Modifier, ID, StartLine, StartColumn, EndLine, Value
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        ReturnType = GetDataTypeFromModifier(Result[0])
        TypeStart = ReturnType.split()[0]
        FuncName = Result[5]
        if EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_RETURN_TYPE, FuncName):
            continue
        Index = Result[0].find(TypeStart)
        if Index != 0 or Result[3] != 0:
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_RETURN_TYPE, '[%s] Return Type should appear at the start of line' % FuncName, FileTable, Result[1])

        if Result[2] == Result[4]:
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_RETURN_TYPE, '[%s] Return Type should appear on its own line' % FuncName, FileTable, Result[1])

    SqlStatement = """ select Modifier, ID, StartLine, StartColumn, FunNameStartLine, Name
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        ReturnType = GetDataTypeFromModifier(Result[0])
        TypeStart = ReturnType.split()[0]
        FuncName = Result[5]
        if EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_RETURN_TYPE, FuncName):
            continue
        Result0 = Result[0]
        if Result0.upper().startswith('STATIC'):
            Result0 = Result0[6:].strip()
        Index = Result0.find(ReturnType)
        if Index != 0 or Result[3] != 0:
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_RETURN_TYPE, '[%s] Return Type should appear at the start of line' % FuncName, 'Function', Result[1])

def CheckFuncLayoutModifier(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Modifier, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        ReturnType = GetDataTypeFromModifier(Result[0])
        TypeStart = ReturnType.split()[0]
#        if len(ReturnType) == 0:
#            continue
        Index = Result[0].find(TypeStart)
        if Index != 0:
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_OPTIONAL_FUNCTIONAL_MODIFIER, '', FileTable, Result[1])

    SqlStatement = """ select Modifier, ID
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        ReturnType = GetDataTypeFromModifier(Result[0])
        TypeStart = ReturnType.split()[0]
#        if len(ReturnType) == 0:
#            continue
        Result0 = Result[0]
        if Result0.upper().startswith('STATIC'):
            Result0 = Result0[6:].strip()
        Index = Result0.find(TypeStart)
        if Index != 0:
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_OPTIONAL_FUNCTIONAL_MODIFIER, '', 'Function', Result[1])

def CheckFuncLayoutName(FullFileName):
    ErrorMsgList = []
    # Parameter variable format pattern.
    Pattern = re.compile(r'^[A-Z]+\S*[a-z]\S*$')
    ParamIgnoreList = ('VOID', '...')
    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Name, ID, EndColumn, Value
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        FuncName = Result[3]
        if EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME, FuncName):
            continue
        if Result[2] != 0:
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME, 'Function name [%s] should appear at the start of a line' % FuncName, FileTable, Result[1])
        ParamList = GetParamList(Result[0])
        if len(ParamList) == 0:
            continue
        StartLine = 0
        for Param in ParamList:
            if Param.StartLine <= StartLine:
                PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME, 'Parameter %s should be in its own line.' % Param.Name, FileTable, Result[1])
            if Param.StartLine - StartLine > 1:
                PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME, 'Empty line appears before Parameter %s.' % Param.Name, FileTable, Result[1])
            if not Pattern.match(Param.Name) and not Param.Name in ParamIgnoreList and not EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME, Param.Name):
                PrintErrorMsg(ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME, 'Parameter [%s] NOT follow naming convention.' % Param.Name, FileTable, Result[1])
            StartLine = Param.StartLine

        if not Result[0].endswith('\n  )') and not Result[0].endswith('\r  )'):
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME, '\')\' should be on a new line and indented two spaces', FileTable, Result[1])

    SqlStatement = """ select Modifier, ID, FunNameStartColumn, Name
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        FuncName = Result[3]
        if EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME, FuncName):
            continue
        if Result[2] != 0:
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME, 'Function name [%s] should appear at the start of a line' % FuncName, 'Function', Result[1])
        ParamList = GetParamList(Result[0])
        if len(ParamList) == 0:
            continue
        StartLine = 0
        for Param in ParamList:
            if Param.StartLine <= StartLine:
                PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME, 'Parameter %s should be in its own line.' % Param.Name, 'Function', Result[1])
            if Param.StartLine - StartLine > 1:
                PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME, 'Empty line appears before Parameter %s.' % Param.Name, 'Function', Result[1])
            if not Pattern.match(Param.Name) and not Param.Name in ParamIgnoreList and not EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME, Param.Name):
                PrintErrorMsg(ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME, 'Parameter [%s] NOT follow naming convention.' % Param.Name, FileTable, Result[1])
            StartLine = Param.StartLine
        if not Result[0].endswith('\n  )') and not Result[0].endswith('\r  )'):
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_NAME, '\')\' should be on a new line and indented two spaces', 'Function', Result[1])

def CheckFuncLayoutPrototype(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    FileTable = 'Identifier' + str(FileID)
    Db = GetDB()
    SqlStatement = """ select Modifier, Header, Name, ID
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    if len(ResultSet) == 0:
        return ErrorMsgList

    FuncDefList = []
    for Result in ResultSet:
        FuncDefList.append(Result)

    SqlStatement = """ select Modifier, Name, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    FuncDeclList = []
    for Result in ResultSet:
        FuncDeclList.append(Result)

    UndeclFuncList = []
    for FuncDef in FuncDefList:
        FuncName = FuncDef[2].strip()
        FuncModifier = FuncDef[0]
        FuncDefHeader = FuncDef[1]
        for FuncDecl in FuncDeclList:
            LBPos = FuncDecl[1].find('(')
            DeclName = FuncDecl[1][0:LBPos].strip()
            DeclModifier = FuncDecl[0]
            if DeclName == FuncName:
                if DiffModifier(FuncModifier, DeclModifier) and not EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE, FuncName):
                    PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE, 'Function [%s] modifier different with prototype.' % FuncName, 'Function', FuncDef[3])
                ParamListOfDef = GetParamList(FuncDefHeader)
                ParamListOfDecl = GetParamList(FuncDecl[1])
                if len(ParamListOfDef) != len(ParamListOfDecl) and not EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_2, FuncName):
                    PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_2, 'Parameter number different in function [%s].' % FuncName, 'Function', FuncDef[3])
                    break

                Index = 0
                while Index < len(ParamListOfDef):
                    if DiffModifier(ParamListOfDef[Index].Modifier, ParamListOfDecl[Index].Modifier) and not EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_3, FuncName):
                        PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_3, 'Parameter %s has different modifier with prototype in function [%s].' % (ParamListOfDef[Index].Name, FuncName), 'Function', FuncDef[3])
                    Index += 1
                break
        else:
            UndeclFuncList.append(FuncDef)

    IncludeFileList = GetAllIncludeFiles(FullFileName)
    FuncDeclList = []
    for F in IncludeFileList:
        FileID = GetTableID(F, ErrorMsgList)
        if FileID < 0:
            continue

        FileTable = 'Identifier' + str(FileID)
        SqlStatement = """ select Modifier, Name, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION)
        ResultSet = Db.TblFile.Exec(SqlStatement)

        for Result in ResultSet:
            FuncDeclList.append(Result)

    for FuncDef in UndeclFuncList:
        FuncName = FuncDef[2].strip()
        FuncModifier = FuncDef[0]
        FuncDefHeader = FuncDef[1]
        for FuncDecl in FuncDeclList:
            LBPos = FuncDecl[1].find('(')
            DeclName = FuncDecl[1][0:LBPos].strip()
            DeclModifier = FuncDecl[0]
            if DeclName == FuncName:
                if DiffModifier(FuncModifier, DeclModifier) and not EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE, FuncName):
                    PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE, 'Function [%s] modifier different with prototype.' % FuncName, 'Function', FuncDef[3])
                ParamListOfDef = GetParamList(FuncDefHeader)
                ParamListOfDecl = GetParamList(FuncDecl[1])
                if len(ParamListOfDef) != len(ParamListOfDecl) and not EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_2, FuncName):
                    PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_2, 'Parameter number different in function [%s].' % FuncName, 'Function', FuncDef[3])
                    break

                Index = 0
                while Index < len(ParamListOfDef):
                    if DiffModifier(ParamListOfDef[Index].Modifier, ParamListOfDecl[Index].Modifier) and not EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_3, FuncName):
                        PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_PROTO_TYPE_3, 'Parameter %s has different modifier with prototype in function [%s].' % (ParamListOfDef[Index].Name, FuncName), 'Function', FuncDef[3])
                    Index += 1
                break

def CheckFuncLayoutBody(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    FileTable = 'Identifier' + str(FileID)
    Db = GetDB()
    SqlStatement = """ select BodyStartColumn, EndColumn, ID
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    if len(ResultSet) == 0:
        return ErrorMsgList
    for Result in ResultSet:
        if Result[0] != 0:
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_BODY, 'open brace should be at the very beginning of a line.', 'Function', Result[2])
        if Result[1] != 0:
            PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_FUNCTION_BODY, 'close brace should be at the very beginning of a line.', 'Function', Result[2])

def CheckFuncLayoutLocalVariable(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select ID
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    if len(ResultSet) == 0:
        return ErrorMsgList
    FL = []
    for Result in ResultSet:
        FL.append(Result)

    for F in FL:
        SqlStatement = """ select Name, Value, ID, Modifier
                       from %s
                       where Model = %d and BelongsToFunction = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_VARIABLE, F[0])
        ResultSet = Db.TblFile.Exec(SqlStatement)
        if len(ResultSet) == 0:
            continue

        for Result in ResultSet:
            if len(Result[1]) > 0 and 'CONST' not in Result[3]:
                PrintErrorMsg(ERROR_C_FUNCTION_LAYOUT_CHECK_NO_INIT_OF_VARIABLE, 'Variable Name: %s' % Result[0], FileTable, Result[2])

def CheckMemberVariableFormat(Name, Value, FileTable, TdId, ModelId):
    ErrMsgList = []
    # Member variable format pattern.
    Pattern = re.compile(r'^[A-Z]+\S*[a-z]\S*$')

    LBPos = Value.find('{')
    RBPos = Value.rfind('}')
    if LBPos == -1 or RBPos == -1:
        return ErrMsgList

    Fields = Value[LBPos + 1 : RBPos]
    Fields = StripComments(Fields).strip()
    NestPos = Fields.find ('struct')
    if NestPos != -1 and (NestPos + len('struct') < len(Fields)):
        if not Fields[NestPos + len('struct') + 1].isalnum():
            if not EccGlobalData.gException.IsException(ERROR_DECLARATION_DATA_TYPE_CHECK_NESTED_STRUCTURE, Name):
                PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_NESTED_STRUCTURE, 'Nested struct in [%s].' % (Name), FileTable, TdId)
            return ErrMsgList
    NestPos = Fields.find ('union')
    if NestPos != -1 and (NestPos + len('union') < len(Fields)):
        if not Fields[NestPos + len('union') + 1].isalnum():
            if not EccGlobalData.gException.IsException(ERROR_DECLARATION_DATA_TYPE_CHECK_NESTED_STRUCTURE, Name):
                PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_NESTED_STRUCTURE, 'Nested union in [%s].' % (Name), FileTable, TdId)
            return ErrMsgList
    NestPos = Fields.find ('enum')
    if NestPos != -1 and (NestPos + len('enum') < len(Fields)):
        if not Fields[NestPos + len('enum') + 1].isalnum():
            if not EccGlobalData.gException.IsException(ERROR_DECLARATION_DATA_TYPE_CHECK_NESTED_STRUCTURE, Name):
                PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_NESTED_STRUCTURE, 'Nested enum in [%s].' % (Name), FileTable, TdId)
            return ErrMsgList

    if ModelId == DataClass.MODEL_IDENTIFIER_ENUMERATE:
        FieldsList = Fields.split(',')
        # deal with enum is pre-assigned a value by function call ( , , , ...)
        QuoteCount = 0
        Index = 0
        RemoveCurrentElement = False
        while Index < len(FieldsList):
            Field = FieldsList[Index]

            if Field.find('(') != -1:
                QuoteCount += 1
                RemoveCurrentElement = True
                Index += 1
                continue

            if Field.find(')') != -1 and QuoteCount > 0:
                QuoteCount -= 1

            if RemoveCurrentElement:
                FieldsList.remove(Field)
                if QuoteCount == 0:
                    RemoveCurrentElement = False
                continue

            if QuoteCount == 0:
                RemoveCurrentElement = False

            Index += 1
    else:
        FieldsList = Fields.split(';')

    for Field in FieldsList:
        Field = Field.strip()
        if Field == '':
            continue
        # For the condition that the field in struct is an array with [] sufixes...
        if Field[-1] == ']':
            LBPos = Field.find('[')
            Field = Field[0:LBPos]
        # For the condition that bit field ": Number"
        if Field.find(':') != -1:
            ColonPos = Field.find(':')
            Field = Field[0:ColonPos]

        Field = Field.strip()
        if Field == '':
            continue
        # Enum could directly assign value to variable
        Field = Field.split('=')[0].strip()
        TokenList = Field.split()
        # Remove pointers before variable
        Token = TokenList[-1]
        if Token in ['OPTIONAL']:
            Token = TokenList[-2]
        if not Pattern.match(Token.lstrip('*')):
            ErrMsgList.append(Token.lstrip('*'))

    return ErrMsgList

def CheckDeclTypedefFormat(FullFileName, ModelId):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Name, StartLine, EndLine, ID, Value
                       from %s
                       where Model = %d
                   """ % (FileTable, ModelId)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    ResultList = []
    for Result in ResultSet:
        ResultList.append(Result)

    ErrorType = ERROR_DECLARATION_DATA_TYPE_CHECK_ALL
    if ModelId == DataClass.MODEL_IDENTIFIER_STRUCTURE:
        ErrorType = ERROR_DECLARATION_DATA_TYPE_CHECK_STRUCTURE_DECLARATION
    elif ModelId == DataClass.MODEL_IDENTIFIER_ENUMERATE:
        ErrorType = ERROR_DECLARATION_DATA_TYPE_CHECK_ENUMERATED_TYPE
    elif ModelId == DataClass.MODEL_IDENTIFIER_UNION:
        ErrorType = ERROR_DECLARATION_DATA_TYPE_CHECK_UNION_TYPE

    SqlStatement = """ select Modifier, Name, Value, StartLine, EndLine, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_TYPEDEF)
    TdSet = Db.TblFile.Exec(SqlStatement)
    TdList = []
    for Td in TdSet:
        TdList.append(Td)
    # Check member variable name format that from typedefs of ONLY this file.
    for Td in TdList:
        Name = Td[1].strip()
        Value = Td[2].strip()
        if Value.startswith('enum'):
            ValueModelId = DataClass.MODEL_IDENTIFIER_ENUMERATE
        elif Value.startswith('struct'):
            ValueModelId = DataClass.MODEL_IDENTIFIER_STRUCTURE
        elif Value.startswith('union'):
            ValueModelId = DataClass.MODEL_IDENTIFIER_UNION
        else:
            continue

        if ValueModelId != ModelId:
            continue
        # Check member variable format.
        ErrMsgList = CheckMemberVariableFormat(Name, Value, FileTable, Td[5], ModelId)
        for ErrMsg in ErrMsgList:
            if EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME, Name + '.' + ErrMsg):
                continue
            PrintErrorMsg(ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME, 'Member variable [%s] NOT follow naming convention.' % (Name + '.' + ErrMsg), FileTable, Td[5])

    # First check in current file to see whether struct/union/enum is typedef-ed.
    UntypedefedList = []
    for Result in ResultList:
        # Check member variable format.
        Name = Result[0].strip()
        Value = Result[4].strip()
        if Value.startswith('enum'):
            ValueModelId = DataClass.MODEL_IDENTIFIER_ENUMERATE
        elif Value.startswith('struct'):
            ValueModelId = DataClass.MODEL_IDENTIFIER_STRUCTURE
        elif Value.startswith('union'):
            ValueModelId = DataClass.MODEL_IDENTIFIER_UNION
        else:
            continue

        if ValueModelId != ModelId:
            continue
        ErrMsgList = CheckMemberVariableFormat(Name, Value, FileTable, Result[3], ModelId)
        for ErrMsg in ErrMsgList:
            if EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME, Result[0] + '.' + ErrMsg):
                continue
            PrintErrorMsg(ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME, 'Member variable [%s] NOT follow naming convention.' % (Result[0] + '.' + ErrMsg), FileTable, Result[3])
        # Check whether it is typedefed.
        Found = False
        for Td in TdList:
            # skip function pointer
            if len(Td[0]) > 0:
                continue
            if Result[1] >= Td[3] and Td[4] >= Result[2]:
                Found = True
                if not Td[1].isupper():
                    PrintErrorMsg(ErrorType, 'Typedef should be UPPER case', FileTable, Td[5])
            if Result[0] in Td[2].split():
                Found = True
                if not Td[1].isupper():
                    PrintErrorMsg(ErrorType, 'Typedef should be UPPER case', FileTable, Td[5])
            if Found:
                break

        if not Found:
            UntypedefedList.append(Result)
            continue

    if len(UntypedefedList) == 0:
        return

    IncludeFileList = GetAllIncludeFiles(FullFileName)
    TdList = []
    for F in IncludeFileList:
        FileID = GetTableID(F, ErrorMsgList)
        if FileID < 0:
            continue

        IncludeFileTable = 'Identifier' + str(FileID)
        SqlStatement = """ select Modifier, Name, Value, StartLine, EndLine, ID
                       from %s
                       where Model = %d
                   """ % (IncludeFileTable, DataClass.MODEL_IDENTIFIER_TYPEDEF)
        ResultSet = Db.TblFile.Exec(SqlStatement)
        TdList.extend(ResultSet)

    for Result in UntypedefedList:

        # Check whether it is typedefed.
        Found = False
        for Td in TdList:

            if len(Td[0]) > 0:
                continue
            if Result[1] >= Td[3] and Td[4] >= Result[2]:
                Found = True
                if not Td[1].isupper():
                    PrintErrorMsg(ErrorType, 'Typedef should be UPPER case', FileTable, Td[5])
            if Result[0] in Td[2].split():
                Found = True
                if not Td[1].isupper():
                    PrintErrorMsg(ErrorType, 'Typedef should be UPPER case', FileTable, Td[5])
            if Found:
                break

        if not Found:
            PrintErrorMsg(ErrorType, 'No Typedef for %s' % Result[0], FileTable, Result[3])
            continue

def CheckDeclStructTypedef(FullFileName):
    CheckDeclTypedefFormat(FullFileName, DataClass.MODEL_IDENTIFIER_STRUCTURE)

def CheckDeclEnumTypedef(FullFileName):
    CheckDeclTypedefFormat(FullFileName, DataClass.MODEL_IDENTIFIER_ENUMERATE)

def CheckDeclUnionTypedef(FullFileName):
    CheckDeclTypedefFormat(FullFileName, DataClass.MODEL_IDENTIFIER_UNION)

def CheckDeclArgModifier(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Modifier, Name, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_VARIABLE)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    ModifierTuple = ('IN', 'OUT', 'OPTIONAL', 'UNALIGNED')
    MAX_MODIFIER_LENGTH = 100
    for Result in ResultSet:
        for Modifier in ModifierTuple:
            if PatternInModifier(Result[0], Modifier) and len(Result[0]) < MAX_MODIFIER_LENGTH:
                PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_IN_OUT_MODIFIER, 'Variable Modifier %s' % Result[0], FileTable, Result[2])
                break

    SqlStatement = """ select Modifier, Name, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        for Modifier in ModifierTuple:
            if PatternInModifier(Result[0], Modifier):
                PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_IN_OUT_MODIFIER, 'Return Type Modifier %s' % Result[0], FileTable, Result[2])
                break

    SqlStatement = """ select Modifier, Header, ID
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        for Modifier in ModifierTuple:
            if PatternInModifier(Result[0], Modifier):
                PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_IN_OUT_MODIFIER, 'Return Type Modifier %s' % Result[0], FileTable, Result[2])
                break

def CheckDeclNoUseCType(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Modifier, Name, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_VARIABLE)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    CTypeTuple = ('int', 'unsigned', 'char', 'void', 'static', 'long')
    for Result in ResultSet:
        for Type in CTypeTuple:
            if PatternInModifier(Result[0], Type):
                PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_NO_USE_C_TYPE, 'Variable type %s' % Type, FileTable, Result[2])
                break

    SqlStatement = """ select Modifier, Name, ID, Value
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        ParamList = GetParamList(Result[1])
        FuncName = Result[3]
        if EccGlobalData.gException.IsException(ERROR_DECLARATION_DATA_TYPE_CHECK_NO_USE_C_TYPE, FuncName):
            continue
        for Type in CTypeTuple:
            if PatternInModifier(Result[0], Type):
                PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_NO_USE_C_TYPE, '%s Return type %s' % (FuncName, Result[0]), FileTable, Result[2])

            for Param in ParamList:
                if PatternInModifier(Param.Modifier, Type):
                    PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_NO_USE_C_TYPE, 'Parameter %s' % Param.Name, FileTable, Result[2])

    SqlStatement = """ select Modifier, Header, ID, Name
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        ParamList = GetParamList(Result[1])
        FuncName = Result[3]
        if EccGlobalData.gException.IsException(ERROR_DECLARATION_DATA_TYPE_CHECK_NO_USE_C_TYPE, FuncName):
            continue
        for Type in CTypeTuple:
            if PatternInModifier(Result[0], Type):
                PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_NO_USE_C_TYPE, '[%s] Return type %s' % (FuncName, Result[0]), FileTable, Result[2])

            for Param in ParamList:
                if PatternInModifier(Param.Modifier, Type):
                    PrintErrorMsg(ERROR_DECLARATION_DATA_TYPE_CHECK_NO_USE_C_TYPE, 'Parameter %s' % Param.Name, FileTable, Result[2])


def CheckPointerNullComparison(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    # cache the found function return type to accelerate later checking in this file.
    FuncReturnTypeDict = {}

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Value, StartLine, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_PREDICATE_EXPRESSION)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    if len(ResultSet) == 0:
        return
    PSL = []
    for Result in ResultSet:
        PSL.append([Result[0], Result[1], Result[2]])

    SqlStatement = """ select BodyStartLine, EndLine, Header, Modifier, ID
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    FL = []
    for Result in ResultSet:
        FL.append([Result[0], Result[1], Result[2], Result[3], Result[4]])

    p = GetFuncDeclPattern()
    for Str in PSL:
        FuncRecord = GetFuncContainsPE(Str[1], FL)
        if FuncRecord == None:
            continue

        for Exp in GetPredicateListFromPredicateExpStr(Str[0]):
            PredInfo = SplitPredicateStr(Exp)
            if PredInfo[1] == None:
                PredVarStr = PredInfo[0][0].strip()
                IsFuncCall = False
                SearchInCache = False
                # PredVarStr may contain '.' or '->'
                TmpStr = PredVarStr.replace('.', '').replace('->', '')
                if p.match(TmpStr):
                    PredVarStr = PredVarStr[0:PredVarStr.find('(')]
                    SearchInCache = True
                    # Only direct function call using IsFuncCall branch. Multi-level ref. function call is considered a variable.
                    if TmpStr.startswith(PredVarStr):
                        IsFuncCall = True

                if PredVarStr.strip() in IgnoredKeywordList:
                    continue
                StarList = []
                PredVarList = GetCNameList(PredVarStr, StarList)
                # No variable found, maybe value first? like (0 == VarName)
                if len(PredVarList) == 0:
                    continue
                if SearchInCache:
                    Type = FuncReturnTypeDict.get(PredVarStr)
                    if Type != None:
                        if Type.find('*') != -1 and Type != 'BOOLEAN*':
                            PrintErrorMsg(ERROR_PREDICATE_EXPRESSION_CHECK_COMPARISON_NULL_TYPE, 'Predicate Expression: %s' % Exp, FileTable, Str[2])
                        continue

                    if PredVarStr in FuncReturnTypeDict:
                        continue

                Type = GetVarInfo(PredVarList, FuncRecord, FullFileName, IsFuncCall, None, StarList)
                if SearchInCache:
                    FuncReturnTypeDict[PredVarStr] = Type
                if Type == None:
                    continue
                Type = GetTypeFromArray(Type, PredVarStr)
                if Type.find('*') != -1 and Type != 'BOOLEAN*':
                    PrintErrorMsg(ERROR_PREDICATE_EXPRESSION_CHECK_COMPARISON_NULL_TYPE, 'Predicate Expression: %s' % Exp, FileTable, Str[2])

def CheckNonBooleanValueComparison(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    # cache the found function return type to accelerate later checking in this file.
    FuncReturnTypeDict = {}

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Value, StartLine, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_PREDICATE_EXPRESSION)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    if len(ResultSet) == 0:
        return
    PSL = []
    for Result in ResultSet:
        PSL.append([Result[0], Result[1], Result[2]])

    SqlStatement = """ select BodyStartLine, EndLine, Header, Modifier, ID
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    FL = []
    for Result in ResultSet:
        FL.append([Result[0], Result[1], Result[2], Result[3], Result[4]])

    p = GetFuncDeclPattern()
    for Str in PSL:
        FuncRecord = GetFuncContainsPE(Str[1], FL)
        if FuncRecord == None:
            continue

        for Exp in GetPredicateListFromPredicateExpStr(Str[0]):
            PredInfo = SplitPredicateStr(Exp)
            if PredInfo[1] == None:
                PredVarStr = PredInfo[0][0].strip()
                IsFuncCall = False
                SearchInCache = False
                # PredVarStr may contain '.' or '->'
                TmpStr = PredVarStr.replace('.', '').replace('->', '')
                if p.match(TmpStr):
                    PredVarStr = PredVarStr[0:PredVarStr.find('(')]
                    SearchInCache = True
                    # Only direct function call using IsFuncCall branch. Multi-level ref. function call is considered a variable.
                    if TmpStr.startswith(PredVarStr):
                        IsFuncCall = True

                if PredVarStr.strip() in IgnoredKeywordList:
                    continue
                StarList = []
                PredVarList = GetCNameList(PredVarStr, StarList)
                # No variable found, maybe value first? like (0 == VarName)
                if len(PredVarList) == 0:
                    continue

                if SearchInCache:
                    Type = FuncReturnTypeDict.get(PredVarStr)
                    if Type != None:
                        if Type.find('BOOLEAN') == -1:
                            PrintErrorMsg(ERROR_PREDICATE_EXPRESSION_CHECK_NO_BOOLEAN_OPERATOR, 'Predicate Expression: %s' % Exp, FileTable, Str[2])
                        continue

                    if PredVarStr in FuncReturnTypeDict:
                        continue
                Type = GetVarInfo(PredVarList, FuncRecord, FullFileName, IsFuncCall, 'BOOLEAN', StarList)
                if SearchInCache:
                    FuncReturnTypeDict[PredVarStr] = Type
                if Type == None:
                    continue
                if Type.find('BOOLEAN') == -1:
                    PrintErrorMsg(ERROR_PREDICATE_EXPRESSION_CHECK_NO_BOOLEAN_OPERATOR, 'Predicate Expression: %s' % Exp, FileTable, Str[2])


def CheckBooleanValueComparison(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    # cache the found function return type to accelerate later checking in this file.
    FuncReturnTypeDict = {}

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Value, StartLine, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_PREDICATE_EXPRESSION)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    if len(ResultSet) == 0:
        return
    PSL = []
    for Result in ResultSet:
        PSL.append([Result[0], Result[1], Result[2]])

    SqlStatement = """ select BodyStartLine, EndLine, Header, Modifier, ID
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    FL = []
    for Result in ResultSet:
        FL.append([Result[0], Result[1], Result[2], Result[3], Result[4]])

    p = GetFuncDeclPattern()
    for Str in PSL:
        FuncRecord = GetFuncContainsPE(Str[1], FL)
        if FuncRecord == None:
            continue

        for Exp in GetPredicateListFromPredicateExpStr(Str[0]):
            PredInfo = SplitPredicateStr(Exp)
            if PredInfo[1] in ('==', '!=') and PredInfo[0][1] in ('TRUE', 'FALSE'):
                PredVarStr = PredInfo[0][0].strip()
                IsFuncCall = False
                SearchInCache = False
                # PredVarStr may contain '.' or '->'
                TmpStr = PredVarStr.replace('.', '').replace('->', '')
                if p.match(TmpStr):
                    PredVarStr = PredVarStr[0:PredVarStr.find('(')]
                    SearchInCache = True
                    # Only direct function call using IsFuncCall branch. Multi-level ref. function call is considered a variable.
                    if TmpStr.startswith(PredVarStr):
                        IsFuncCall = True

                if PredVarStr.strip() in IgnoredKeywordList:
                    continue
                StarList = []
                PredVarList = GetCNameList(PredVarStr, StarList)
                # No variable found, maybe value first? like (0 == VarName)
                if len(PredVarList) == 0:
                    continue

                if SearchInCache:
                    Type = FuncReturnTypeDict.get(PredVarStr)
                    if Type != None:
                        if Type.find('BOOLEAN') != -1:
                            PrintErrorMsg(ERROR_PREDICATE_EXPRESSION_CHECK_BOOLEAN_VALUE, 'Predicate Expression: %s' % Exp, FileTable, Str[2])
                        continue

                    if PredVarStr in FuncReturnTypeDict:
                        continue

                Type = GetVarInfo(PredVarList, FuncRecord, FullFileName, IsFuncCall, 'BOOLEAN', StarList)
                if SearchInCache:
                    FuncReturnTypeDict[PredVarStr] = Type
                if Type == None:
                    continue
                if Type.find('BOOLEAN') != -1:
                    PrintErrorMsg(ERROR_PREDICATE_EXPRESSION_CHECK_BOOLEAN_VALUE, 'Predicate Expression: %s' % Exp, FileTable, Str[2])


def CheckHeaderFileData(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select ID, Modifier
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_VARIABLE)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        if not Result[1].startswith('extern'):
            PrintErrorMsg(ERROR_INCLUDE_FILE_CHECK_DATA, 'Variable definition appears in header file', FileTable, Result[0])

    SqlStatement = """ select ID
                       from Function
                       where BelongsToFile = %d
                   """ % FileID
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        PrintErrorMsg(ERROR_INCLUDE_FILE_CHECK_DATA, 'Function definition appears in header file', 'Function', Result[0])

    return ErrorMsgList

def CheckHeaderFileIfndef(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Value, StartLine
                       from %s
                       where Model = %d order by StartLine
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_MACRO_IFNDEF)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    if len(ResultSet) == 0:
        PrintErrorMsg(ERROR_INCLUDE_FILE_CHECK_IFNDEF_STATEMENT_1, '', 'File', FileID)
        return ErrorMsgList
    for Result in ResultSet:
        SqlStatement = """ select Value, EndLine
                       from %s
                       where EndLine < %d
                   """ % (FileTable, Result[1])
        ResultSet = Db.TblFile.Exec(SqlStatement)
        for Result in ResultSet:
            if not Result[0].startswith('/*') and not Result[0].startswith('//'):
                PrintErrorMsg(ERROR_INCLUDE_FILE_CHECK_IFNDEF_STATEMENT_2, '', 'File', FileID)
        break

    SqlStatement = """ select Value
                       from %s
                       where StartLine > (select max(EndLine) from %s where Model = %d)
                   """ % (FileTable, FileTable, DataClass.MODEL_IDENTIFIER_MACRO_ENDIF)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        if not Result[0].startswith('/*') and not Result[0].startswith('//'):
            PrintErrorMsg(ERROR_INCLUDE_FILE_CHECK_IFNDEF_STATEMENT_3, '', 'File', FileID)
    return ErrorMsgList

def CheckDoxygenCommand(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Value, ID
                       from %s
                       where Model = %d or Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_COMMENT, DataClass.MODEL_IDENTIFIER_FUNCTION_HEADER)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    DoxygenCommandList = ['bug', 'todo', 'example', 'file', 'attention', 'param', 'post', 'pre', 'retval', 'return', 'sa', 'since', 'test', 'note', 'par']
    for Result in ResultSet:
        CommentStr = Result[0]
        CommentPartList = CommentStr.split()
        for Part in CommentPartList:
            if Part.upper() == 'BUGBUG':
                PrintErrorMsg(ERROR_DOXYGEN_CHECK_COMMAND, 'Bug should be marked with doxygen tag @bug', FileTable, Result[1])
            if Part.upper() == 'TODO':
                PrintErrorMsg(ERROR_DOXYGEN_CHECK_COMMAND, 'ToDo should be marked with doxygen tag @todo', FileTable, Result[1])
            if Part.startswith('@'):
                if EccGlobalData.gException.IsException(ERROR_DOXYGEN_CHECK_COMMAND, Part):
                    continue
                if Part.lstrip('@').isalpha():
                    if Part.lstrip('@') not in DoxygenCommandList:
                        PrintErrorMsg(ERROR_DOXYGEN_CHECK_COMMAND, 'Unknown doxygen command %s' % Part, FileTable, Result[1])
                else:
                    Index = Part.find('[')
                    if Index == -1:
                        PrintErrorMsg(ERROR_DOXYGEN_CHECK_COMMAND, 'Unknown doxygen command %s' % Part, FileTable, Result[1])
                    RealCmd = Part[1:Index]
                    if RealCmd not in DoxygenCommandList:
                        PrintErrorMsg(ERROR_DOXYGEN_CHECK_COMMAND, 'Unknown doxygen command %s' % Part, FileTable, Result[1])


def CheckDoxygenTripleForwardSlash(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()

    SqlStatement = """ select ID, BodyStartLine, BodyStartColumn, EndLine, EndColumn
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    if len(ResultSet) == 0:
        return

    FuncDefSet = []
    for Result in ResultSet:
        FuncDefSet.append(Result)


    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Value, ID, StartLine, StartColumn, EndLine, EndColumn
                       from %s
                       where Model = %d

                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_COMMENT)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    CommentSet = []
    try:
        for Result in ResultSet:
            CommentSet.append(Result)
    except:
        print 'Unrecognized chars in comment of file %s', FullFileName


    for Result in CommentSet:
        CommentStr = Result[0]
        StartLine = Result[2]
        StartColumn = Result[3]
        EndLine = Result[4]
        EndColumn = Result[5]
        if not CommentStr.startswith('///<'):
            continue

        Found = False
        for FuncDef in FuncDefSet:
            if StartLine == FuncDef[1] and StartColumn > FuncDef[2] and EndLine == FuncDef[3] and EndColumn < FuncDef[4]:
                Found = True
                break
            if StartLine > FuncDef[1] and EndLine < FuncDef[3]:
                Found = True
                break
            if StartLine == FuncDef[1] and StartColumn > FuncDef[2] and EndLine < FuncDef[3]:
                Found = True
                break
            if StartLine > FuncDef[1] and EndLine == FuncDef[3] and EndColumn < FuncDef[4]:
                Found = True
                break
        if Found:
            PrintErrorMsg(ERROR_DOXYGEN_CHECK_COMMENT_FORMAT, '', FileTable, Result[1])


def CheckFileHeaderDoxygenComments(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Value, ID
                       from %s
                       where Model = %d and (StartLine = 1 or StartLine = 7 or StartLine = 8) and StartColumn = 0
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_COMMENT)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    if len(ResultSet) == 0:
        PrintErrorMsg(ERROR_HEADER_CHECK_FILE, 'No File License header appear at the very beginning of file.', 'File', FileID)
        return ErrorMsgList

    NoHeaderCommentStartFlag = True
    NoHeaderCommentEndFlag = True
    NoHeaderCommentPeriodFlag = True
    NoCopyrightFlag = True
    NoLicenseFlag = True
    NoRevReferFlag = True
    NextLineIndex = 0
    for Result in ResultSet:
        FileStartFlag = False
        CommentStrList = []
        CommentStr = Result[0].strip()
        CommentStrListTemp = CommentStr.split('\n')
        if (len(CommentStrListTemp) <= 1):
            # For Mac
            CommentStrListTemp = CommentStr.split('\r')
        # Skip the content before the file  header    
        for CommentLine in CommentStrListTemp:
            if CommentLine.strip().startswith('/** @file'):
                FileStartFlag = True
            if FileStartFlag ==  True:
                CommentStrList.append(CommentLine)
                       
        ID = Result[1]
        Index = 0
        if CommentStrList and CommentStrList[0].strip().startswith('/** @file'):
            NoHeaderCommentStartFlag = False
        else:
            continue
        if CommentStrList and CommentStrList[-1].strip().endswith('**/'):
            NoHeaderCommentEndFlag = False
        else:
            continue

        for CommentLine in CommentStrList:
            Index = Index + 1
            NextLineIndex = Index
            if CommentLine.startswith('/** @file'):
                continue
            if CommentLine.startswith('**/'):
                break
            # Check whether C File header Comment content start with two spaces.
            if EccGlobalData.gConfig.HeaderCheckCFileCommentStartSpacesNum == '1' or EccGlobalData.gConfig.HeaderCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
                if CommentLine.startswith('/** @file') == False and CommentLine.startswith('**/') == False and CommentLine.strip() and CommentLine.startswith('  ') == False:
                    PrintErrorMsg(ERROR_HEADER_CHECK_FILE, 'File header comment content should start with two spaces at each line', FileTable, ID)
            
            CommentLine = CommentLine.strip()
            if CommentLine.startswith('Copyright'):
                NoCopyrightFlag = False
                if CommentLine.find('All rights reserved') == -1:
                    for Copyright in EccGlobalData.gConfig.Copyright:
                        if CommentLine.find(Copyright) > -1:
                            PrintErrorMsg(ERROR_HEADER_CHECK_FILE, '""All rights reserved"" announcement should be following the ""Copyright"" at the same line', FileTable, ID)
                            break
                if CommentLine.endswith('<BR>') == -1:
                    PrintErrorMsg(ERROR_HEADER_CHECK_FILE, 'The ""<BR>"" at the end of the Copyright line is required', FileTable, ID)
                if NextLineIndex < len(CommentStrList) and CommentStrList[NextLineIndex].strip().startswith('Copyright') == False and CommentStrList[NextLineIndex].strip():
                    NoLicenseFlag = False
            if CommentLine.startswith('@par Revision Reference:'):
                NoRevReferFlag = False
                RefListFlag = False
                for RefLine in CommentStrList[NextLineIndex:]:
                    if RefLine.strip() and (NextLineIndex + 1) < len(CommentStrList) and CommentStrList[NextLineIndex+1].strip() and CommentStrList[NextLineIndex+1].strip().startswith('**/') == False:
                        RefListFlag = True
                    if RefLine.strip() == False or RefLine.strip().startswith('**/'):
                        RefListFlag = False
                        break
                    # Check whether C File header Comment's each reference at list should begin with a bullet character.
                    if EccGlobalData.gConfig.HeaderCheckCFileCommentReferenceFormat == '1' or EccGlobalData.gConfig.HeaderCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
                        if RefListFlag == True:
                            if RefLine.strip() and RefLine.strip().startswith('**/') == False and RefLine.startswith('  -') == False:                            
                                PrintErrorMsg(ERROR_HEADER_CHECK_FILE, 'Each reference on a separate line should begin with a bullet character ""-"" ', FileTable, ID)                    
    
    if NoHeaderCommentStartFlag:
        PrintErrorMsg(ERROR_DOXYGEN_CHECK_FILE_HEADER, 'File header comment should begin with ""/** @file""', FileTable, ID)
        return
    if NoHeaderCommentEndFlag:
        PrintErrorMsg(ERROR_HEADER_CHECK_FILE, 'File header comment should end with ""**/""', FileTable, ID)
        return
    if NoCopyrightFlag:
        PrintErrorMsg(ERROR_HEADER_CHECK_FILE, 'File header comment missing the ""Copyright""', FileTable, ID)
    #Check whether C File header Comment have the License immediately after the ""Copyright"" line.
    if EccGlobalData.gConfig.HeaderCheckCFileCommentLicenseFormat == '1' or EccGlobalData.gConfig.HeaderCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
        if NoLicenseFlag:
            PrintErrorMsg(ERROR_HEADER_CHECK_FILE, 'File header comment should have the License immediately after the ""Copyright"" line', FileTable, ID)

def CheckFuncHeaderDoxygenComments(FullFileName):
    ErrorMsgList = []

    FileID = GetTableID(FullFileName, ErrorMsgList)
    if FileID < 0:
        return ErrorMsgList

    Db = GetDB()
    FileTable = 'Identifier' + str(FileID)
    SqlStatement = """ select Value, StartLine, EndLine, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_COMMENT)

    ResultSet = Db.TblFile.Exec(SqlStatement)
    CommentSet = []
    try:
        for Result in ResultSet:
            CommentSet.append(Result)
    except:
        print 'Unrecognized chars in comment of file %s', FullFileName

    # Func Decl check
    SqlStatement = """ select Modifier, Name, StartLine, ID, Value
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_DECLARATION)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        FuncName = Result[4]
        FunctionHeaderComment = CheckCommentImmediatelyPrecedeFunctionHeader(Result[1], Result[2], CommentSet)
        if FunctionHeaderComment:
            CheckFunctionHeaderConsistentWithDoxygenComment(Result[0], Result[1], Result[2], FunctionHeaderComment[0], FunctionHeaderComment[1], ErrorMsgList, FunctionHeaderComment[3], FileTable)
        else:
            if EccGlobalData.gException.IsException(ERROR_HEADER_CHECK_FUNCTION, FuncName):
                continue
            ErrorMsgList.append('Line %d :Function %s has NO comment immediately preceding it.' % (Result[2], Result[1]))
            PrintErrorMsg(ERROR_HEADER_CHECK_FUNCTION, 'Function [%s] has NO comment immediately preceding it.' % (FuncName), FileTable, Result[3])

    # Func Def check
    SqlStatement = """ select Value, StartLine, EndLine, ID
                       from %s
                       where Model = %d
                   """ % (FileTable, DataClass.MODEL_IDENTIFIER_FUNCTION_HEADER)

    ResultSet = Db.TblFile.Exec(SqlStatement)
    CommentSet = []
    try:
        for Result in ResultSet:
            CommentSet.append(Result)
    except:
        print 'Unrecognized chars in comment of file %s', FullFileName

    SqlStatement = """ select Modifier, Header, StartLine, ID, Name
                       from Function
                       where BelongsToFile = %d
                   """ % (FileID)
    ResultSet = Db.TblFile.Exec(SqlStatement)
    for Result in ResultSet:
        FuncName = Result[4]
        FunctionHeaderComment = CheckCommentImmediatelyPrecedeFunctionHeader(Result[1], Result[2], CommentSet)
        if FunctionHeaderComment:
            CheckFunctionHeaderConsistentWithDoxygenComment(Result[0], Result[1], Result[2], FunctionHeaderComment[0], FunctionHeaderComment[1], ErrorMsgList, FunctionHeaderComment[3], FileTable)
        else:
            if EccGlobalData.gException.IsException(ERROR_HEADER_CHECK_FUNCTION, FuncName):
                continue
            ErrorMsgList.append('Line %d :Function [%s] has NO comment immediately preceding it.' % (Result[2], Result[1]))
            PrintErrorMsg(ERROR_HEADER_CHECK_FUNCTION, 'Function [%s] has NO comment immediately preceding it.' % (FuncName), 'Function', Result[3])
    return ErrorMsgList

def CheckCommentImmediatelyPrecedeFunctionHeader(FuncName, FuncStartLine, CommentSet):

    for Comment in CommentSet:
        if Comment[2] == FuncStartLine - 1:
            return Comment
    return None

def GetDoxygenStrFromComment(Str):
    DoxygenStrList = []
    ParamTagList = Str.split('@param')
    if len(ParamTagList) > 1:
        i = 1
        while i < len(ParamTagList):
            DoxygenStrList.append('@param' + ParamTagList[i])
            i += 1

    Str = ParamTagList[0]

    RetvalTagList = ParamTagList[-1].split('@retval')
    if len(RetvalTagList) > 1:
        if len(ParamTagList) > 1:
            DoxygenStrList[-1] = '@param' + RetvalTagList[0]
        i = 1
        while i < len(RetvalTagList):
            DoxygenStrList.append('@retval' + RetvalTagList[i])
            i += 1

    ReturnTagList = RetvalTagList[-1].split('@return')
    if len(ReturnTagList) > 1:
        if len(RetvalTagList) > 1:
            DoxygenStrList[-1] = '@retval' + ReturnTagList[0]
        elif len(ParamTagList) > 1:
            DoxygenStrList[-1] = '@param' + ReturnTagList[0]
        i = 1
        while i < len(ReturnTagList):
            DoxygenStrList.append('@return' + ReturnTagList[i])
            i += 1

    if len(DoxygenStrList) > 0:
        DoxygenStrList[-1] = DoxygenStrList[-1].rstrip('--*/')

    return DoxygenStrList

def CheckGeneralDoxygenCommentLayout(Str, StartLine, ErrorMsgList, CommentId= -1, TableName=''):
    #/** --*/ @retval after @param
    if not Str.startswith('/**'):
        ErrorMsgList.append('Line %d : Comment does NOT have prefix /** ' % StartLine)
        PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'Comment does NOT have prefix /** ', TableName, CommentId)
    if not Str.endswith('**/'):
        ErrorMsgList.append('Line %d : Comment does NOT have tail **/ ' % StartLine)
        PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'Comment does NOT have tail **/ ', TableName, CommentId)
    FirstRetvalIndex = Str.find('@retval')
    LastParamIndex = Str.rfind('@param')
    if (FirstRetvalIndex > 0) and (LastParamIndex > 0) and (FirstRetvalIndex < LastParamIndex):
        ErrorMsgList.append('Line %d : @retval appear before @param ' % StartLine)
        PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'in Comment, @retval appear before @param  ', TableName, CommentId)

def CheckFunctionHeaderConsistentWithDoxygenComment(FuncModifier, FuncHeader, FuncStartLine, CommentStr, CommentStartLine, ErrorMsgList, CommentId= -1, TableName=''):

    ParamList = GetParamList(FuncHeader)
    CheckGeneralDoxygenCommentLayout(CommentStr, CommentStartLine, ErrorMsgList, CommentId, TableName)
    DescriptionStr = CommentStr
    DoxygenStrList = GetDoxygenStrFromComment(DescriptionStr)
    if DescriptionStr.find('.') == -1:
        PrintErrorMsg(ERROR_DOXYGEN_CHECK_COMMENT_DESCRIPTION, 'Comment description should end with period \'.\'', TableName, CommentId)
    DoxygenTagNumber = len(DoxygenStrList)
    ParamNumber = len(ParamList)
    for Param in ParamList:
        if Param.Name.upper() == 'VOID' and ParamNumber == 1:
            ParamNumber -= 1
    Index = 0
    if ParamNumber > 0 and DoxygenTagNumber > 0:
        while Index < ParamNumber and Index < DoxygenTagNumber:
            ParamModifier = ParamList[Index].Modifier
            ParamName = ParamList[Index].Name.strip()
            Tag = DoxygenStrList[Index].strip(' ')
            if (not Tag[-1] == ('\n')) and (not Tag[-1] == ('\r')):
                ErrorMsgList.append('Line %d : in Comment, <%s> does NOT end with new line ' % (CommentStartLine, Tag.replace('\n', '').replace('\r', '')))
                PrintErrorMsg(ERROR_HEADER_CHECK_FUNCTION, 'in Comment, <%s> does NOT end with new line ' % (Tag.replace('\n', '').replace('\r', '')), TableName, CommentId)
            TagPartList = Tag.split()
            if len(TagPartList) < 2:
                ErrorMsgList.append('Line %d : in Comment, <%s> does NOT contain doxygen contents ' % (CommentStartLine, Tag.replace('\n', '').replace('\r', '')))
                PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'in Comment, <%s> does NOT contain doxygen contents ' % (Tag.replace('\n', '').replace('\r', '')), TableName, CommentId)
                Index += 1
                continue
            LBPos = Tag.find('[')
            RBPos = Tag.find(']')
            ParamToLBContent = Tag[len('@param'):LBPos].strip()
            if LBPos > 0 and len(ParamToLBContent) == 0 and RBPos > LBPos:
                InOutStr = ''
                ModifierPartList = ParamModifier.split()
                for Part in ModifierPartList:
                    if Part.strip() == 'IN':
                        InOutStr += 'in'
                    if Part.strip() == 'OUT':
                        if InOutStr != '':
                            InOutStr += ', out'
                        else:
                            InOutStr = 'out'

                if InOutStr != '':
                    if Tag.find('[' + InOutStr + ']') == -1:
                        if InOutStr != 'in, out':
                            ErrorMsgList.append('Line %d : in Comment, <%s> does NOT have %s ' % (CommentStartLine, (TagPartList[0] + ' ' + TagPartList[1]).replace('\n', '').replace('\r', ''), '[' + InOutStr + ']'))
                            PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'in Comment, <%s> does NOT have %s ' % ((TagPartList[0] + ' ' + TagPartList[1]).replace('\n', '').replace('\r', ''), '[' + InOutStr + ']'), TableName, CommentId)
                        else:
                            if Tag.find('[in,out]') == -1:
                                ErrorMsgList.append('Line %d : in Comment, <%s> does NOT have %s ' % (CommentStartLine, (TagPartList[0] + ' ' + TagPartList[1]).replace('\n', '').replace('\r', ''), '[' + InOutStr + ']'))
                                PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'in Comment, <%s> does NOT have %s ' % ((TagPartList[0] + ' ' + TagPartList[1]).replace('\n', '').replace('\r', ''), '[' + InOutStr + ']'), TableName, CommentId)


            if Tag.find(ParamName) == -1 and ParamName != 'VOID' and ParamName != 'void':
                ErrorMsgList.append('Line %d : in Comment, <%s> does NOT consistent with parameter name %s ' % (CommentStartLine, (TagPartList[0] + ' ' + TagPartList[1]).replace('\n', '').replace('\r', ''), ParamName))
                PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'in Comment, <%s> does NOT consistent with parameter name %s ' % ((TagPartList[0] + ' ' + TagPartList[1]).replace('\n', '').replace('\r', ''), ParamName), TableName, CommentId)
            Index += 1

        if Index < ParamNumber:
            ErrorMsgList.append('Line %d : Number of doxygen tags in comment less than number of function parameters' % CommentStartLine)
            PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'Number of doxygen tags in comment less than number of function parameters ', TableName, CommentId)
        # VOID return type, NOT VOID*. VOID* should be matched with a doxygen tag.
        if (FuncModifier.find('VOID') != -1 or FuncModifier.find('void') != -1) and FuncModifier.find('*') == -1:

            # assume we allow a return description tag for void func. return. that's why 'DoxygenTagNumber - 1' is used instead of 'DoxygenTagNumber'
            if Index < DoxygenTagNumber - 1 or (Index < DoxygenTagNumber and DoxygenStrList[Index].startswith('@retval')):
                ErrorMsgList.append('Line %d : VOID return type need NO doxygen tags in comment' % CommentStartLine)
                PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'VOID return type need no doxygen tags in comment ', TableName, CommentId)
        else:
            if Index < DoxygenTagNumber and not DoxygenStrList[Index].startswith('@retval') and not DoxygenStrList[Index].startswith('@return'):
                ErrorMsgList.append('Line %d : Number of @param doxygen tags in comment does NOT match number of function parameters' % CommentStartLine)
                PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'Number of @param doxygen tags in comment does NOT match number of function parameters ', TableName, CommentId)
    else:
        if ParamNumber == 0 and DoxygenTagNumber != 0 and ((FuncModifier.find('VOID') != -1 or FuncModifier.find('void') != -1) and FuncModifier.find('*') == -1):
            ErrorMsgList.append('Line %d : VOID return type need NO doxygen tags in comment' % CommentStartLine)
            PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'VOID return type need NO doxygen tags in comment ', TableName, CommentId)
        if ParamNumber != 0 and DoxygenTagNumber == 0:
            ErrorMsgList.append('Line %d : No doxygen tags in comment' % CommentStartLine)
            PrintErrorMsg(ERROR_DOXYGEN_CHECK_FUNCTION_HEADER, 'No doxygen tags in comment ', TableName, CommentId)

if __name__ == '__main__':

#    EdkLogger.Initialize()
#    EdkLogger.SetLevel(EdkLogger.QUIET)
#    CollectSourceCodeDataIntoDB(sys.argv[1])
    try:
        test_file = sys.argv[1]
    except IndexError, v:
        print "Usage: %s filename" % sys.argv[0]
        sys.exit(1)
    MsgList = CheckFuncHeaderDoxygenComments(test_file)
    for Msg in MsgList:
        print Msg
    print 'Done!'
