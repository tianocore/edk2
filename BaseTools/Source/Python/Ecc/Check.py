## @file
# This file is used to define checkpoints used by ECC tool
#
# Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
# Copyright (c) 2008 - 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
from __future__ import absolute_import
import Common.LongFilePathOs as os
import re
from CommonDataClass.DataClass import *
import Common.DataType as DT
from Ecc.EccToolError import *
from Ecc.MetaDataParser import ParseHeaderCommentSection
from Ecc import EccGlobalData
from Ecc import c
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.MultipleWorkspace import MultipleWorkspace as mws

## Check
#
# This class is to define checkpoints used by ECC tool
#
# @param object:          Inherited from object class
#
class Check(object):
    def __init__(self):
        pass

    # Check all required checkpoints
    def Check(self):
        self.GeneralCheck()
        self.MetaDataFileCheck()
        self.DoxygenCheck()
        self.IncludeFileCheck()
        self.PredicateExpressionCheck()
        self.DeclAndDataTypeCheck()
        self.FunctionLayoutCheck()
        self.NamingConventionCheck()
        self.SmmCommParaCheck()

    def SmmCommParaCheck(self):
        self.SmmCommParaCheckBufferType()


    # Check if SMM communication function has correct parameter type
    # 1. Get function calling with instance./->Communicate() interface
    # and make sure the protocol instance is of type EFI_SMM_COMMUNICATION_PROTOCOL.
    # 2. Find the origin of the 2nd parameter of Communicate() interface, if -
    #    a. it is a local buffer on stack
    #       report error.
    #    b. it is a global buffer, check the driver that holds the global buffer is of type DXE_RUNTIME_DRIVER
    #       report success.
    #    c. it is a buffer by AllocatePage/AllocatePool (may be wrapped by nested function calls),
    #       check the EFI_MEMORY_TYPE to be EfiRuntimeServicesCode,EfiRuntimeServicesData,
    #       EfiACPIMemoryNVS or EfiReservedMemoryType
    #       report success.
    #    d. it is a buffer located via EFI_SYSTEM_TABLE.ConfigurationTable (may be wrapped by nested function calls)
    #       report warning to indicate human code review.
    #    e. it is a buffer from other kind of pointers (may need to trace into nested function calls to locate),
    #       repeat checks in a.b.c and d.
    def SmmCommParaCheckBufferType(self):
        if EccGlobalData.gConfig.SmmCommParaCheckBufferType == '1' or EccGlobalData.gConfig.SmmCommParaCheckAll == '1':
            EdkLogger.quiet("Checking SMM communication parameter type ...")
            # Get all EFI_SMM_COMMUNICATION_PROTOCOL interface
            CommApiList = []
            for IdentifierTable in EccGlobalData.gIdentifierTableList:
                SqlCommand = """select ID, Name, BelongsToFile from %s
                                where Modifier = 'EFI_SMM_COMMUNICATION_PROTOCOL*' """ % (IdentifierTable)
                RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                if RecordSet:
                    for Record in RecordSet:
                        if Record[1] not in CommApiList:
                            CommApiList.append(Record[1])
            # For each interface, check the second parameter
            for CommApi in CommApiList:
                for IdentifierTable in EccGlobalData.gIdentifierTableList:
                    SqlCommand = """select ID, Name, Value, BelongsToFile, StartLine from %s
                    where Name = '%s->Communicate' and Model = %s""" \
                    % (IdentifierTable, CommApi, MODEL_IDENTIFIER_FUNCTION_CALLING)
                    RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                    if RecordSet:
                        # print IdentifierTable
                        for Record in RecordSet:
                            # Get the second parameter for Communicate function
                            SecondPara = Record[2].split(',')[1].strip()
                            SecondParaIndex = None
                            if SecondPara.startswith('&'):
                                SecondPara = SecondPara[1:]
                            if SecondPara.endswith(']'):
                                SecondParaIndex = SecondPara[SecondPara.find('[') + 1:-1]
                                SecondPara = SecondPara[:SecondPara.find('[')]
                            # Get the ID
                            Id = Record[0]
                            # Get the BelongsToFile
                            BelongsToFile = Record[3]
                            # Get the source file path
                            SqlCommand = """select FullPath from File where ID = %s""" % BelongsToFile
                            NewRecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                            FullPath = NewRecordSet[0][0]
                            # Get the line no of function calling
                            StartLine = Record[4]
                            # Get the module type
                            SqlCommand = """select Value3 from INF where BelongsToFile = (select ID from File
                                            where Path = (select Path from File where ID = %s) and Model = 1011)
                                            and Value2 = 'MODULE_TYPE'""" % BelongsToFile
                            NewRecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                            ModuleType = NewRecordSet[0][0] if NewRecordSet else None

                            # print BelongsToFile, FullPath, StartLine, ModuleType, SecondPara

                            Value = FindPara(FullPath, SecondPara, StartLine)
                            # Find the value of the parameter
                            if Value:
                                if 'AllocatePage' in Value \
                                    or 'AllocatePool' in Value \
                                    or 'AllocateRuntimePool' in Value \
                                    or 'AllocateZeroPool' in Value:
                                    pass
                                else:
                                    if '->' in Value:
                                        if not EccGlobalData.gException.IsException(
                                               ERROR_SMM_COMM_PARA_CHECK_BUFFER_TYPE, Value):
                                            EccGlobalData.gDb.TblReport.Insert(ERROR_SMM_COMM_PARA_CHECK_BUFFER_TYPE,
                                                                               OtherMsg="Please review the buffer type"
                                                                               + "is correct or not. If it is correct" +
                                                                               " please add [%s] to exception list"
                                                                               % Value,
                                                                               BelongsToTable=IdentifierTable,
                                                                               BelongsToItem=Id)
                                    else:
                                        if not EccGlobalData.gException.IsException(
                                               ERROR_SMM_COMM_PARA_CHECK_BUFFER_TYPE, Value):
                                            EccGlobalData.gDb.TblReport.Insert(ERROR_SMM_COMM_PARA_CHECK_BUFFER_TYPE,
                                                                               OtherMsg="Please review the buffer type"
                                                                               + "is correct or not. If it is correct" +
                                                                               " please add [%s] to exception list"
                                                                               % Value,
                                                                               BelongsToTable=IdentifierTable,
                                                                               BelongsToItem=Id)


                            # Not find the value of the parameter
                            else:
                                SqlCommand = """select ID, Modifier, Name, Value, Model, BelongsToFunction from %s
                                                where Name = '%s' and StartLine < %s order by StartLine DESC""" \
                                                % (IdentifierTable, SecondPara, StartLine)
                                NewRecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                                if NewRecordSet:
                                    Value = NewRecordSet[0][1]
                                    if 'AllocatePage' in Value \
                                        or 'AllocatePool' in Value \
                                        or 'AllocateRuntimePool' in Value \
                                        or 'AllocateZeroPool' in Value:
                                        pass
                                    else:
                                        if not EccGlobalData.gException.IsException(
                                            ERROR_SMM_COMM_PARA_CHECK_BUFFER_TYPE, Value):
                                            EccGlobalData.gDb.TblReport.Insert(ERROR_SMM_COMM_PARA_CHECK_BUFFER_TYPE,
                                                                               OtherMsg="Please review the buffer type"
                                                                               + "is correct or not. If it is correct" +
                                                                               " please add [%s] to exception list"
                                                                               % Value,
                                                                               BelongsToTable=IdentifierTable,
                                                                               BelongsToItem=Id)
                                else:
                                    pass

    # Check UNI files
    def UniCheck(self):
        if EccGlobalData.gConfig.GeneralCheckUni == '1' or EccGlobalData.gConfig.GeneralCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking whether UNI file is UTF-16 ...")
            SqlCommand = """select ID, FullPath, ExtName from File where ExtName like 'uni'"""
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                File = Record[1]
                FileIn = open(File, 'rb').read(2)
                if FileIn != '\xff\xfe':
                    OtherMsg = "File %s is not a valid UTF-16 UNI file" % Record[1]
                    EccGlobalData.gDb.TblReport.Insert(ERROR_GENERAL_CHECK_UNI, OtherMsg=OtherMsg, BelongsToTable='File', BelongsToItem=Record[0])

    # General Checking
    def GeneralCheck(self):
        self.GeneralCheckNonAcsii()
        self.UniCheck()
        self.GeneralCheckNoTab()
        self.GeneralCheckLineEnding()
        self.GeneralCheckTrailingWhiteSpaceLine()

    # Check whether NO Tab is used, replaced with spaces
    def GeneralCheckNoTab(self):
        if EccGlobalData.gConfig.GeneralCheckNoTab == '1' or EccGlobalData.gConfig.GeneralCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking No TAB used in file ...")
            SqlCommand = """select ID, FullPath, ExtName from File where ExtName in ('.dec', '.inf', '.dsc', 'c', 'h')"""
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                if Record[2].upper() not in EccGlobalData.gConfig.BinaryExtList:
                    op = open(Record[1]).readlines()
                    IndexOfLine = 0
                    for Line in op:
                        IndexOfLine += 1
                        IndexOfChar = 0
                        for Char in Line:
                            IndexOfChar += 1
                            if Char == '\t':
                                OtherMsg = "File %s has TAB char at line %s column %s" % (Record[1], IndexOfLine, IndexOfChar)
                                EccGlobalData.gDb.TblReport.Insert(ERROR_GENERAL_CHECK_NO_TAB, OtherMsg=OtherMsg, BelongsToTable='File', BelongsToItem=Record[0])

    # Check Only use CRLF (Carriage Return Line Feed) line endings.
    def GeneralCheckLineEnding(self):
        if EccGlobalData.gConfig.GeneralCheckLineEnding == '1' or EccGlobalData.gConfig.GeneralCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking line ending in file ...")
            SqlCommand = """select ID, FullPath, ExtName from File where ExtName in ('.dec', '.inf', '.dsc', 'c', 'h')"""
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                if Record[2].upper() not in EccGlobalData.gConfig.BinaryExtList:
                    op = open(Record[1], 'rb').readlines()
                    IndexOfLine = 0
                    for Line in op:
                        IndexOfLine += 1
                        if not bytes.decode(Line).endswith('\r\n'):
                            OtherMsg = "File %s has invalid line ending at line %s" % (Record[1], IndexOfLine)
                            EccGlobalData.gDb.TblReport.Insert(ERROR_GENERAL_CHECK_INVALID_LINE_ENDING, OtherMsg=OtherMsg, BelongsToTable='File', BelongsToItem=Record[0])

    # Check if there is no trailing white space in one line.
    def GeneralCheckTrailingWhiteSpaceLine(self):
        if EccGlobalData.gConfig.GeneralCheckTrailingWhiteSpaceLine == '1' or EccGlobalData.gConfig.GeneralCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking trailing white space line in file ...")
            SqlCommand = """select ID, FullPath, ExtName from File where ExtName in ('.dec', '.inf', '.dsc', 'c', 'h')"""
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                if Record[2].upper() not in EccGlobalData.gConfig.BinaryExtList:
                    op = open(Record[1], 'r').readlines()
                    IndexOfLine = 0
                    for Line in op:
                        IndexOfLine += 1
                        if Line.replace('\r', '').replace('\n', '').endswith(' '):
                            OtherMsg = "File %s has trailing white spaces at line %s" % (Record[1], IndexOfLine)
                            EccGlobalData.gDb.TblReport.Insert(ERROR_GENERAL_CHECK_TRAILING_WHITE_SPACE_LINE, OtherMsg=OtherMsg, BelongsToTable='File', BelongsToItem=Record[0])

    # Check whether file has non ACSII char
    def GeneralCheckNonAcsii(self):
        if EccGlobalData.gConfig.GeneralCheckNonAcsii == '1' or EccGlobalData.gConfig.GeneralCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking Non-ACSII char in file ...")
            SqlCommand = """select ID, FullPath, ExtName from File where ExtName in ('.dec', '.inf', '.dsc', 'c', 'h')"""
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                if Record[2].upper() not in EccGlobalData.gConfig.BinaryExtList:
                    op = open(Record[1]).readlines()
                    IndexOfLine = 0
                    for Line in op:
                        IndexOfLine += 1
                        IndexOfChar = 0
                        for Char in Line:
                            IndexOfChar += 1
                            if ord(Char) > 126:
                                OtherMsg = "File %s has Non-ASCII char at line %s column %s" % (Record[1], IndexOfLine, IndexOfChar)
                                EccGlobalData.gDb.TblReport.Insert(ERROR_GENERAL_CHECK_NON_ACSII, OtherMsg=OtherMsg, BelongsToTable='File', BelongsToItem=Record[0])

    # C Function Layout Checking
    def FunctionLayoutCheck(self):
        self.FunctionLayoutCheckReturnType()
        self.FunctionLayoutCheckModifier()
        self.FunctionLayoutCheckName()
        self.FunctionLayoutCheckPrototype()
        self.FunctionLayoutCheckBody()
        self.FunctionLayoutCheckLocalVariable()
        self.FunctionLayoutCheckDeprecated()

    # To check if the deprecated functions are used
    def FunctionLayoutCheckDeprecated(self):
        if EccGlobalData.gConfig.CFunctionLayoutCheckNoDeprecated == '1' or EccGlobalData.gConfig.CFunctionLayoutCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking function no deprecated one being used ...")

            DeprecatedFunctionSet = ('UnicodeValueToString',
                                     'AsciiValueToString',
                                     'StrCpy',
                                     'StrnCpy',
                                     'StrCat',
                                     'StrnCat',
                                     'UnicodeStrToAsciiStr',
                                     'AsciiStrCpy',
                                     'AsciiStrnCpy',
                                     'AsciiStrCat',
                                     'AsciiStrnCat',
                                     'AsciiStrToUnicodeStr',
                                     'PcdSet8',
                                     'PcdSet16',
                                     'PcdSet32',
                                     'PcdSet64',
                                     'PcdSetPtr',
                                     'PcdSetBool',
                                     'PcdSetEx8',
                                     'PcdSetEx16',
                                     'PcdSetEx32',
                                     'PcdSetEx64',
                                     'PcdSetExPtr',
                                     'PcdSetExBool',
                                     'LibPcdSet8',
                                     'LibPcdSet16',
                                     'LibPcdSet32',
                                     'LibPcdSet64',
                                     'LibPcdSetPtr',
                                     'LibPcdSetBool',
                                     'LibPcdSetEx8',
                                     'LibPcdSetEx16',
                                     'LibPcdSetEx32',
                                     'LibPcdSetEx64',
                                     'LibPcdSetExPtr',
                                     'LibPcdSetExBool',
                                     'GetVariable',
                                     'GetEfiGlobalVariable',
                                     )

            for IdentifierTable in EccGlobalData.gIdentifierTableList:
                SqlCommand = """select ID, Name, BelongsToFile from %s
                                where Model = %s """ % (IdentifierTable, MODEL_IDENTIFIER_FUNCTION_CALLING)
                RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                for Record in RecordSet:
                    for Key in DeprecatedFunctionSet:
                        if Key == Record[1]:
                            if not EccGlobalData.gException.IsException(ERROR_C_FUNCTION_LAYOUT_CHECK_NO_DEPRECATE, Key):
                                OtherMsg = 'The function [%s] is deprecated which should NOT be used' % Key
                                EccGlobalData.gDb.TblReport.Insert(ERROR_C_FUNCTION_LAYOUT_CHECK_NO_DEPRECATE,
                                                                   OtherMsg=OtherMsg,
                                                                   BelongsToTable=IdentifierTable,
                                                                   BelongsToItem=Record[0])

    def WalkTree(self):
        IgnoredPattern = c.GetIgnoredDirListPattern()
        for Dirpath, Dirnames, Filenames in os.walk(EccGlobalData.gTarget):
            for Dir in Dirnames:
                Dirname = os.path.join(Dirpath, Dir)
                if os.path.islink(Dirname):
                    Dirname = os.path.realpath(Dirname)
                    if os.path.isdir(Dirname):
                        # symlinks to directories are treated as directories
                        Dirnames.remove(Dir)
                        Dirnames.append(Dirname)
            if IgnoredPattern.match(Dirpath.upper()):
                continue
            for f in Filenames[:]:
                if f.lower() in EccGlobalData.gConfig.SkipFileList:
                    Filenames.remove(f)
            yield (Dirpath, Dirnames, Filenames)

    # Check whether return type exists and in the first line
    def FunctionLayoutCheckReturnType(self):
        if EccGlobalData.gConfig.CFunctionLayoutCheckReturnType == '1' or EccGlobalData.gConfig.CFunctionLayoutCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking function layout return type ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.c', '.h'):
#                        FullName = os.path.join(Dirpath, F)
#                        c.CheckFuncLayoutReturnType(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                c.CheckFuncLayoutReturnType(FullName)

    # Check whether any optional functional modifiers exist and next to the return type
    def FunctionLayoutCheckModifier(self):
        if EccGlobalData.gConfig.CFunctionLayoutCheckOptionalFunctionalModifier == '1' or EccGlobalData.gConfig.CFunctionLayoutCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking function layout modifier ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.c', '.h'):
#                        FullName = os.path.join(Dirpath, F)
#                        c.CheckFuncLayoutModifier(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                c.CheckFuncLayoutModifier(FullName)

    # Check whether the next line contains the function name, left justified, followed by the beginning of the parameter list
    # Check whether the closing parenthesis is on its own line and also indented two spaces
    def FunctionLayoutCheckName(self):
        if EccGlobalData.gConfig.CFunctionLayoutCheckFunctionName == '1' or EccGlobalData.gConfig.CFunctionLayoutCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking function layout function name ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.c', '.h'):
#                        FullName = os.path.join(Dirpath, F)
#                        c.CheckFuncLayoutName(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                c.CheckFuncLayoutName(FullName)

    # Check whether the function prototypes in include files have the same form as function definitions
    def FunctionLayoutCheckPrototype(self):
        if EccGlobalData.gConfig.CFunctionLayoutCheckFunctionPrototype == '1' or EccGlobalData.gConfig.CFunctionLayoutCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking function layout function prototype ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        EdkLogger.quiet("[PROTOTYPE]" + FullName)
#                        c.CheckFuncLayoutPrototype(FullName)
            for FullName in EccGlobalData.gCFileList:
                EdkLogger.quiet("[PROTOTYPE]" + FullName)
                c.CheckFuncLayoutPrototype(FullName)

    # Check whether the body of a function is contained by open and close braces that must be in the first column
    def FunctionLayoutCheckBody(self):
        if EccGlobalData.gConfig.CFunctionLayoutCheckFunctionBody == '1' or EccGlobalData.gConfig.CFunctionLayoutCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking function layout function body ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        c.CheckFuncLayoutBody(FullName)
            for FullName in EccGlobalData.gCFileList:
                c.CheckFuncLayoutBody(FullName)

    # Check whether the data declarations is the first code in a module.
    # self.CFunctionLayoutCheckDataDeclaration = 1
    # Check whether no initialization of a variable as part of its declaration
    def FunctionLayoutCheckLocalVariable(self):
        if EccGlobalData.gConfig.CFunctionLayoutCheckNoInitOfVariable == '1' or EccGlobalData.gConfig.CFunctionLayoutCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking function layout local variables ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        c.CheckFuncLayoutLocalVariable(FullName)

            for FullName in EccGlobalData.gCFileList:
                c.CheckFuncLayoutLocalVariable(FullName)

    # Check whether no use of STATIC for functions
    # self.CFunctionLayoutCheckNoStatic = 1

    # Declarations and Data Types Checking
    def DeclAndDataTypeCheck(self):
        self.DeclCheckNoUseCType()
        self.DeclCheckInOutModifier()
        self.DeclCheckEFIAPIModifier()
        self.DeclCheckEnumeratedType()
        self.DeclCheckStructureDeclaration()
        self.DeclCheckSameStructure()
        self.DeclCheckUnionType()


    # Check whether no use of int, unsigned, char, void, long in any .c, .h or .asl files.
    def DeclCheckNoUseCType(self):
        if EccGlobalData.gConfig.DeclarationDataTypeCheckNoUseCType == '1' or EccGlobalData.gConfig.DeclarationDataTypeCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking Declaration No use C type ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.h', '.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        c.CheckDeclNoUseCType(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                c.CheckDeclNoUseCType(FullName)

    # Check whether the modifiers IN, OUT, OPTIONAL, and UNALIGNED are used only to qualify arguments to a function and should not appear in a data type declaration
    def DeclCheckInOutModifier(self):
        if EccGlobalData.gConfig.DeclarationDataTypeCheckInOutModifier == '1' or EccGlobalData.gConfig.DeclarationDataTypeCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking Declaration argument modifier ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.h', '.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        c.CheckDeclArgModifier(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                c.CheckDeclArgModifier(FullName)

    # Check whether the EFIAPI modifier should be used at the entry of drivers, events, and member functions of protocols
    def DeclCheckEFIAPIModifier(self):
        if EccGlobalData.gConfig.DeclarationDataTypeCheckEFIAPIModifier == '1' or EccGlobalData.gConfig.DeclarationDataTypeCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            pass

    # Check whether Enumerated Type has a 'typedef' and the name is capital
    def DeclCheckEnumeratedType(self):
        if EccGlobalData.gConfig.DeclarationDataTypeCheckEnumeratedType == '1' or EccGlobalData.gConfig.DeclarationDataTypeCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking Declaration enum typedef ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.h', '.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        EdkLogger.quiet("[ENUM]" + FullName)
#                        c.CheckDeclEnumTypedef(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                EdkLogger.quiet("[ENUM]" + FullName)
                c.CheckDeclEnumTypedef(FullName)

    # Check whether Structure Type has a 'typedef' and the name is capital
    def DeclCheckStructureDeclaration(self):
        if EccGlobalData.gConfig.DeclarationDataTypeCheckStructureDeclaration == '1' or EccGlobalData.gConfig.DeclarationDataTypeCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking Declaration struct typedef ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.h', '.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        EdkLogger.quiet("[STRUCT]" + FullName)
#                        c.CheckDeclStructTypedef(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                EdkLogger.quiet("[STRUCT]" + FullName)
                c.CheckDeclStructTypedef(FullName)

    # Check whether having same Structure
    def DeclCheckSameStructure(self):
        if EccGlobalData.gConfig.DeclarationDataTypeCheckSameStructure == '1' or EccGlobalData.gConfig.DeclarationDataTypeCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking same struct ...")
            AllStructure = {}
            for IdentifierTable in EccGlobalData.gIdentifierTableList:
                SqlCommand = """select ID, Name, BelongsToFile from %s where Model = %s""" % (IdentifierTable, MODEL_IDENTIFIER_STRUCTURE)
                RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                for Record in RecordSet:
                    if Record[1] != '':
                        if Record[1] not in AllStructure.keys():
                            AllStructure[Record[1]] = Record[2]
                        else:
                            ID = AllStructure[Record[1]]
                            SqlCommand = """select FullPath from File where ID = %s """ % ID
                            NewRecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                            OtherMsg = "The structure name '%s' is duplicate" % Record[1]
                            if NewRecordSet != []:
                                OtherMsg = "The structure name [%s] is duplicate with the one defined in %s, maybe struct NOT typedefed or the typedef new type NOT used to qualify variables" % (Record[1], NewRecordSet[0][0])
                            if not EccGlobalData.gException.IsException(ERROR_DECLARATION_DATA_TYPE_CHECK_SAME_STRUCTURE, Record[1]):
                                EccGlobalData.gDb.TblReport.Insert(ERROR_DECLARATION_DATA_TYPE_CHECK_SAME_STRUCTURE, OtherMsg=OtherMsg, BelongsToTable=IdentifierTable, BelongsToItem=Record[0])

    # Check whether Union Type has a 'typedef' and the name is capital
    def DeclCheckUnionType(self):
        if EccGlobalData.gConfig.DeclarationDataTypeCheckUnionType == '1' or EccGlobalData.gConfig.DeclarationDataTypeCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking Declaration union typedef ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.h', '.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        EdkLogger.quiet("[UNION]" + FullName)
#                        c.CheckDeclUnionTypedef(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                EdkLogger.quiet("[UNION]" + FullName)
                c.CheckDeclUnionTypedef(FullName)

    # Predicate Expression Checking
    def PredicateExpressionCheck(self):
        self.PredicateExpressionCheckBooleanValue()
        self.PredicateExpressionCheckNonBooleanOperator()
        self.PredicateExpressionCheckComparisonNullType()

    # Check whether Boolean values, variable type BOOLEAN not use explicit comparisons to TRUE or FALSE
    def PredicateExpressionCheckBooleanValue(self):
        if EccGlobalData.gConfig.PredicateExpressionCheckBooleanValue == '1' or EccGlobalData.gConfig.PredicateExpressionCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking predicate expression Boolean value ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        EdkLogger.quiet("[BOOLEAN]" + FullName)
#                        c.CheckBooleanValueComparison(FullName)
            for FullName in EccGlobalData.gCFileList:
                EdkLogger.quiet("[BOOLEAN]" + FullName)
                c.CheckBooleanValueComparison(FullName)

    # Check whether Non-Boolean comparisons use a compare operator (==, !=, >, < >=, <=).
    def PredicateExpressionCheckNonBooleanOperator(self):
        if EccGlobalData.gConfig.PredicateExpressionCheckNonBooleanOperator == '1' or EccGlobalData.gConfig.PredicateExpressionCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking predicate expression Non-Boolean variable...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        EdkLogger.quiet("[NON-BOOLEAN]" + FullName)
#                        c.CheckNonBooleanValueComparison(FullName)
            for FullName in EccGlobalData.gCFileList:
                EdkLogger.quiet("[NON-BOOLEAN]" + FullName)
                c.CheckNonBooleanValueComparison(FullName)

    # Check whether a comparison of any pointer to zero must be done via the NULL type
    def PredicateExpressionCheckComparisonNullType(self):
        if EccGlobalData.gConfig.PredicateExpressionCheckComparisonNullType == '1' or EccGlobalData.gConfig.PredicateExpressionCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking predicate expression NULL pointer ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        EdkLogger.quiet("[POINTER]" + FullName)
#                        c.CheckPointerNullComparison(FullName)
            for FullName in EccGlobalData.gCFileList:
                EdkLogger.quiet("[POINTER]" + FullName)
                c.CheckPointerNullComparison(FullName)

    # Include file checking
    def IncludeFileCheck(self):
        self.IncludeFileCheckIfndef()
        self.IncludeFileCheckData()
        self.IncludeFileCheckSameName()

    # Check whether having include files with same name
    def IncludeFileCheckSameName(self):
        if EccGlobalData.gConfig.IncludeFileCheckSameName == '1' or EccGlobalData.gConfig.IncludeFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking same header file name ...")
            SqlCommand = """select ID, FullPath from File
                            where Model = 1002 order by Name """
            RecordDict = {}
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                List = Record[1].replace('/', '\\').split('\\')
                if len(List) >= 2:
                    Key = List[-2] + '\\' + List[-1]
                else:
                    Key = List[0]
                if Key not in RecordDict:
                    RecordDict[Key] = [Record]
                else:
                    RecordDict[Key].append(Record)

            for Key in RecordDict:
                if len(RecordDict[Key]) > 1:
                    for Item in RecordDict[Key]:
                        Path = mws.relpath(Item[1], EccGlobalData.gWorkspace)
                        if not EccGlobalData.gException.IsException(ERROR_INCLUDE_FILE_CHECK_NAME, Path):
                            EccGlobalData.gDb.TblReport.Insert(ERROR_INCLUDE_FILE_CHECK_NAME, OtherMsg="The file name for [%s] is duplicate" % Path, BelongsToTable='File', BelongsToItem=Item[0])

    # Check whether all include file contents is guarded by a #ifndef statement.
    def IncludeFileCheckIfndef(self):
        if EccGlobalData.gConfig.IncludeFileCheckIfndefStatement == '1' or EccGlobalData.gConfig.IncludeFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking header file ifndef ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.h'):
#                        FullName = os.path.join(Dirpath, F)
#                        MsgList = c.CheckHeaderFileIfndef(FullName)
            for FullName in EccGlobalData.gHFileList:
                MsgList = c.CheckHeaderFileIfndef(FullName)

    # Check whether include files NOT contain code or define data variables
    def IncludeFileCheckData(self):
        if EccGlobalData.gConfig.IncludeFileCheckData == '1' or EccGlobalData.gConfig.IncludeFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking header file data ...")

            # Get all typedef functions
            gAllTypedefFun = []
            for IdentifierTable in EccGlobalData.gIdentifierTableList:
                SqlCommand = """select Name from %s
                                where Model = %s """ % (IdentifierTable, MODEL_IDENTIFIER_TYPEDEF)
                RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                for Record in RecordSet:
                    if Record[0].startswith('('):
                        gAllTypedefFun.append(Record[0])

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.h'):
#                        FullName = os.path.join(Dirpath, F)
#                        MsgList = c.CheckHeaderFileData(FullName)
            for FullName in EccGlobalData.gHFileList:
                MsgList = c.CheckHeaderFileData(FullName, gAllTypedefFun)

    # Doxygen document checking
    def DoxygenCheck(self):
        self.DoxygenCheckFileHeader()
        self.DoxygenCheckFunctionHeader()
        self.DoxygenCheckCommentDescription()
        self.DoxygenCheckCommentFormat()
        self.DoxygenCheckCommand()

    # Check whether the file headers are followed Doxygen special documentation blocks in section 2.3.5
    def DoxygenCheckFileHeader(self):
        if EccGlobalData.gConfig.DoxygenCheckFileHeader == '1' or EccGlobalData.gConfig.DoxygenCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking Doxygen file header ...")

            for Dirpath, Dirnames, Filenames in self.WalkTree():
                for F in Filenames:
                    Ext = os.path.splitext(F)[1]
                    if Ext in ('.h', '.c'):
                        FullName = os.path.join(Dirpath, F)
                        MsgList = c.CheckFileHeaderDoxygenComments(FullName)
                    elif Ext in ('.inf', '.dec', '.dsc', '.fdf'):
                        FullName = os.path.join(Dirpath, F)
                        op = open(FullName).readlines()
                        FileLinesList = op
                        LineNo             = 0
                        CurrentSection     = MODEL_UNKNOWN
                        HeaderSectionLines       = []
                        HeaderCommentStart = False
                        HeaderCommentEnd   = False

                        for Line in FileLinesList:
                            LineNo   = LineNo + 1
                            Line     = Line.strip()
                            if (LineNo < len(FileLinesList) - 1):
                                NextLine = FileLinesList[LineNo].strip()

                            #
                            # blank line
                            #
                            if (Line == '' or not Line) and LineNo == len(FileLinesList):
                                LastSectionFalg = True

                            #
                            # check whether file header comment section started
                            #
                            if Line.startswith('#') and \
                                (Line.find('@file') > -1) and \
                                not HeaderCommentStart:
                                if CurrentSection != MODEL_UNKNOWN:
                                    SqlStatement = """ select ID from File where FullPath like '%s'""" % FullName
                                    ResultSet = EccGlobalData.gDb.TblFile.Exec(SqlStatement)
                                    for Result in ResultSet:
                                        Msg = 'INF/DEC/DSC/FDF file header comment should begin with ""## @file"" or ""# @file""at the very top file'
                                        EccGlobalData.gDb.TblReport.Insert(ERROR_DOXYGEN_CHECK_FILE_HEADER, Msg, "File", Result[0])

                                else:
                                    CurrentSection = MODEL_IDENTIFIER_FILE_HEADER
                                    #
                                    # Append the first line to section lines.
                                    #
                                    HeaderSectionLines.append((Line, LineNo))
                                    HeaderCommentStart = True
                                    continue

                            #
                            # Collect Header content.
                            #
                            if (Line.startswith('#') and CurrentSection == MODEL_IDENTIFIER_FILE_HEADER) and\
                                HeaderCommentStart and not Line.startswith('##') and not\
                                HeaderCommentEnd and NextLine != '':
                                HeaderSectionLines.append((Line, LineNo))
                                continue
                            #
                            # Header content end
                            #
                            if (Line.startswith('##') or not Line.strip().startswith("#")) and HeaderCommentStart \
                                and not HeaderCommentEnd:
                                if Line.startswith('##'):
                                    HeaderCommentEnd = True
                                HeaderSectionLines.append((Line, LineNo))
                                ParseHeaderCommentSection(HeaderSectionLines, FullName)
                                break
                        if HeaderCommentStart == False:
                            SqlStatement = """ select ID from File where FullPath like '%s'""" % FullName
                            ResultSet = EccGlobalData.gDb.TblFile.Exec(SqlStatement)
                            for Result in ResultSet:
                                Msg = 'INF/DEC/DSC/FDF file header comment should begin with ""## @file"" or ""# @file"" at the very top file'
                                EccGlobalData.gDb.TblReport.Insert(ERROR_DOXYGEN_CHECK_FILE_HEADER, Msg, "File", Result[0])
                        if HeaderCommentEnd == False:
                            SqlStatement = """ select ID from File where FullPath like '%s'""" % FullName
                            ResultSet = EccGlobalData.gDb.TblFile.Exec(SqlStatement)
                            for Result in ResultSet:
                                Msg = 'INF/DEC/DSC/FDF file header comment should end with ""##"" at the end of file header comment block'
                                # Check whether File header Comment End with '##'
                                if EccGlobalData.gConfig.HeaderCheckFileCommentEnd == '1' or EccGlobalData.gConfig.HeaderCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
                                    EccGlobalData.gDb.TblReport.Insert(ERROR_DOXYGEN_CHECK_FILE_HEADER, Msg, "File", Result[0])



    # Check whether the function headers are followed Doxygen special documentation blocks in section 2.3.5
    def DoxygenCheckFunctionHeader(self):
        if EccGlobalData.gConfig.DoxygenCheckFunctionHeader == '1' or EccGlobalData.gConfig.DoxygenCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking Doxygen function header ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.h', '.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        MsgList = c.CheckFuncHeaderDoxygenComments(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                MsgList = c.CheckFuncHeaderDoxygenComments(FullName)


    # Check whether the first line of text in a comment block is a brief description of the element being documented.
    # The brief description must end with a period.
    def DoxygenCheckCommentDescription(self):
        if EccGlobalData.gConfig.DoxygenCheckCommentDescription == '1' or EccGlobalData.gConfig.DoxygenCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            pass

    # Check whether comment lines with '///< ... text ...' format, if it is used, it should be after the code section.
    def DoxygenCheckCommentFormat(self):
        if EccGlobalData.gConfig.DoxygenCheckCommentFormat == '1' or EccGlobalData.gConfig.DoxygenCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking Doxygen comment ///< ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.h', '.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        MsgList = c.CheckDoxygenTripleForwardSlash(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                MsgList = c.CheckDoxygenTripleForwardSlash(FullName)

    # Check whether only Doxygen commands allowed to mark the code are @bug and @todo.
    def DoxygenCheckCommand(self):
        if EccGlobalData.gConfig.DoxygenCheckCommand == '1' or EccGlobalData.gConfig.DoxygenCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking Doxygen command ...")

#            for Dirpath, Dirnames, Filenames in self.WalkTree():
#                for F in Filenames:
#                    if os.path.splitext(F)[1] in ('.h', '.c'):
#                        FullName = os.path.join(Dirpath, F)
#                        MsgList = c.CheckDoxygenCommand(FullName)
            for FullName in EccGlobalData.gCFileList + EccGlobalData.gHFileList:
                MsgList = c.CheckDoxygenCommand(FullName)

    # Meta-Data File Processing Checking
    def MetaDataFileCheck(self):
        self.MetaDataFileCheckPathName()
        self.MetaDataFileCheckGenerateFileList()
        self.MetaDataFileCheckLibraryInstance()
        self.MetaDataFileCheckLibraryInstanceDependent()
        self.MetaDataFileCheckLibraryInstanceOrder()
        self.MetaDataFileCheckLibraryNoUse()
        self.MetaDataFileCheckLibraryDefinedInDec()
        self.MetaDataFileCheckBinaryInfInFdf()
        self.MetaDataFileCheckPcdDuplicate()
        self.MetaDataFileCheckPcdFlash()
        self.MetaDataFileCheckPcdNoUse()
        self.MetaDataFileCheckGuidDuplicate()
        self.MetaDataFileCheckModuleFileNoUse()
        self.MetaDataFileCheckPcdType()
        self.MetaDataFileCheckModuleFileGuidDuplication()
        self.MetaDataFileCheckModuleFileGuidFormat()
        self.MetaDataFileCheckModuleFileProtocolFormat()
        self.MetaDataFileCheckModuleFilePpiFormat()
        self.MetaDataFileCheckModuleFilePcdFormat()

    # Check whether each file defined in meta-data exists
    def MetaDataFileCheckPathName(self):
        if EccGlobalData.gConfig.MetaDataFileCheckPathName == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            # This item is covered when parsing Inf/Dec/Dsc files
            pass

    # Generate a list for all files defined in meta-data files
    def MetaDataFileCheckGenerateFileList(self):
        if EccGlobalData.gConfig.MetaDataFileCheckGenerateFileList == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            # This item is covered when parsing Inf/Dec/Dsc files
            pass

    # Check whether all Library Instances defined for a given module (or dependent library instance) match the module's type.
    # Each Library Instance must specify the Supported Module Types in its Inf file,
    # and any module specifying the library instance must be one of the supported types.
    def MetaDataFileCheckLibraryInstance(self):
        if EccGlobalData.gConfig.MetaDataFileCheckLibraryInstance == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for library instance type issue ...")
            SqlCommand = """select A.ID, A.Value3, B.Value3 from Inf as A left join Inf as B
                            where A.Value2 = 'LIBRARY_CLASS' and A.Model = %s
                            and B.Value2 = 'MODULE_TYPE' and B.Model = %s and A.BelongsToFile = B.BelongsToFile
                            group by A.BelongsToFile""" % (MODEL_META_DATA_HEADER, MODEL_META_DATA_HEADER)
            RecordSet = EccGlobalData.gDb.TblInf.Exec(SqlCommand)
            LibraryClasses = {}
            for Record in RecordSet:
                List = Record[1].split('|', 1)
                SupModType = []
                if len(List) == 1:
                    SupModType = DT.SUP_MODULE_LIST_STRING.split(DT.TAB_VALUE_SPLIT)
                elif len(List) == 2:
                    SupModType = List[1].split()

                if List[0] not in LibraryClasses:
                    LibraryClasses[List[0]] = SupModType
                else:
                    for Item in SupModType:
                        if Item not in LibraryClasses[List[0]]:
                            LibraryClasses[List[0]].append(Item)

                if Record[2] != DT.SUP_MODULE_BASE and Record[2] not in SupModType:
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_2, OtherMsg="The Library Class '%s' does not specify its supported module types" % (List[0]), BelongsToTable='Inf', BelongsToItem=Record[0])

            SqlCommand = """select A.ID, A.Value1, B.Value3 from Inf as A left join Inf as B
                            where A.Model = %s and B.Value2 = '%s' and B.Model = %s
                            and B.BelongsToFile = A.BelongsToFile""" \
                            % (MODEL_EFI_LIBRARY_CLASS, 'MODULE_TYPE', MODEL_META_DATA_HEADER)
            RecordSet = EccGlobalData.gDb.TblInf.Exec(SqlCommand)
            # Merge all LibraryClasses' supmodlist
            RecordDict = {}
            for Record in RecordSet:
                if Record[1] not in RecordDict:
                    RecordDict[Record[1]] = [str(Record[2])]
                else:
                    if Record[2] not in RecordDict[Record[1]]:
                        RecordDict[Record[1]].append(Record[2])

            for Record in RecordSet:
                if Record[1] in LibraryClasses:
                    if Record[2] not in LibraryClasses[Record[1]] and DT.SUP_MODULE_BASE not in RecordDict[Record[1]]:
                        if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_1, Record[1]):
                            EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_1, OtherMsg="The type of Library Class [%s] defined in Inf file does not match the type of the module" % (Record[1]), BelongsToTable='Inf', BelongsToItem=Record[0])
                else:
                    if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_1, Record[1]):
                        EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_1, OtherMsg="The type of Library Class [%s] defined in Inf file does not match the type of the module" % (Record[1]), BelongsToTable='Inf', BelongsToItem=Record[0])

    # Check whether a Library Instance has been defined for all dependent library classes
    def MetaDataFileCheckLibraryInstanceDependent(self):
        if EccGlobalData.gConfig.MetaDataFileCheckLibraryInstanceDependent == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for library instance dependent issue ...")
            SqlCommand = """select ID, Value1, Value2 from Dsc where Model = %s""" % MODEL_EFI_LIBRARY_CLASS
            LibraryClasses = EccGlobalData.gDb.TblDsc.Exec(SqlCommand)
            for LibraryClass in LibraryClasses:
                if LibraryClass[1].upper() == 'NULL' or LibraryClass[1].startswith('!ifdef') or LibraryClass[1].startswith('!ifndef') or LibraryClass[1].endswith('!endif'):
                    continue
                else:
                    LibraryIns = os.path.normpath(mws.join(EccGlobalData.gWorkspace, LibraryClass[2]))
                    SkipDirString = '|'.join(EccGlobalData.gConfig.SkipDirList)
                    p = re.compile(r'.*[\\/](?:%s^\S)[\\/]?.*' % SkipDirString)
                    if p.match(os.path.split(LibraryIns)[0].upper()):
                        continue
                    SqlCommand = """select Value3 from Inf where BelongsToFile =
                                    (select ID from File where lower(FullPath) = lower('%s'))
                                    and Value2 = '%s'""" % (LibraryIns, DT.PLATFORM_COMPONENT_TYPE_LIBRARY_CLASS)
                    RecordSet = EccGlobalData.gDb.TblInf.Exec(SqlCommand)
                    IsFound = False
                    for Record in RecordSet:
                        LibName = Record[0].split('|', 1)[0]
                        if LibraryClass[1] == LibName:
                            IsFound = True
                    if not IsFound:
                        if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_DEPENDENT, LibraryClass[1]):
                            EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_LIBRARY_INSTANCE_DEPENDENT, OtherMsg="The Library Class [%s] is not specified in '%s'" % (LibraryClass[1], LibraryClass[2]), BelongsToTable='Dsc', BelongsToItem=LibraryClass[0])

    # Check whether the Library Instances specified by the LibraryClasses sections are listed in order of dependencies
    def MetaDataFileCheckLibraryInstanceOrder(self):
        if EccGlobalData.gConfig.MetaDataFileCheckLibraryInstanceOrder == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            # This checkpoint is not necessary for Ecc check
            pass

    # Check whether the unnecessary inclusion of library classes in the Inf file
    # Check whether the unnecessary duplication of library classe names in the DSC file
    def MetaDataFileCheckLibraryNoUse(self):
        if EccGlobalData.gConfig.MetaDataFileCheckLibraryNoUse == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for library instance not used ...")
            SqlCommand = """select ID, Value1 from Inf as A where A.Model = %s and A.Value1 not in (select B.Value1 from Dsc as B where Model = %s)""" % (MODEL_EFI_LIBRARY_CLASS, MODEL_EFI_LIBRARY_CLASS)
            RecordSet = EccGlobalData.gDb.TblInf.Exec(SqlCommand)
            for Record in RecordSet:
                if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_LIBRARY_NO_USE, Record[1]):
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_LIBRARY_NO_USE, OtherMsg="The Library Class [%s] is not used in any platform" % (Record[1]), BelongsToTable='Inf', BelongsToItem=Record[0])
            SqlCommand = """
                         select A.ID, A.Value1, A.BelongsToFile, A.StartLine, B.StartLine from Dsc as A left join Dsc as B
                         where A.Model = %s and B.Model = %s and A.Scope1 = B.Scope1 and A.Scope2 = B.Scope2 and A.ID != B.ID
                         and A.Value1 = B.Value1 and A.Value2 != B.Value2 and A.BelongsToItem = -1 and B.BelongsToItem = -1 and A.StartLine != B.StartLine and B.BelongsToFile = A.BelongsToFile""" \
                            % (MODEL_EFI_LIBRARY_CLASS, MODEL_EFI_LIBRARY_CLASS)
            RecordSet = EccGlobalData.gDb.TblDsc.Exec(SqlCommand)
            for Record in RecordSet:
                if Record[3] and Record[4] and Record[3] != Record[4] and Record[1] != 'NULL':
                    SqlCommand = """select FullPath from File where ID = %s""" % (Record[2])
                    FilePathList = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                    for FilePath in FilePathList:
                        if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_LIBRARY_NAME_DUPLICATE, Record[1]):
                            EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_LIBRARY_NAME_DUPLICATE, OtherMsg="The Library Class [%s] is duplicated in '%s' line %s and line %s." % (Record[1], FilePath, Record[3], Record[4]), BelongsToTable='Dsc', BelongsToItem=Record[0])

    # Check the header file in Include\Library directory whether be defined in the package DEC file.
    def MetaDataFileCheckLibraryDefinedInDec(self):
        if EccGlobalData.gConfig.MetaDataFileCheckLibraryDefinedInDec == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for library instance whether be defined in the package dec file ...")
            SqlCommand = """
                    select A.Value1, A.StartLine, A.ID, B.Value1 from Inf as A left join Dec as B
                    on A.Model = B.Model and A.Value1 = B.Value1 where A.Model=%s
                    """ % MODEL_EFI_LIBRARY_CLASS
            RecordSet = EccGlobalData.gDb.TblDsc.Exec(SqlCommand)
            for Record in RecordSet:
                LibraryInInf, Line, ID, LibraryDec = Record
                if not LibraryDec:
                    if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_LIBRARY_NOT_DEFINED, LibraryInInf):
                        EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_LIBRARY_NOT_DEFINED, \
                                            OtherMsg="The Library Class [%s] in %s line is not defined in the associated package file." % (LibraryInInf, Line),
                                            BelongsToTable='Inf', BelongsToItem=ID)

    # Check whether an Inf file is specified in the FDF file, but not in the Dsc file, then the Inf file must be for a Binary module only
    def MetaDataFileCheckBinaryInfInFdf(self):
        if EccGlobalData.gConfig.MetaDataFileCheckBinaryInfInFdf == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for non-binary modules defined in FDF files ...")
            SqlCommand = """select A.ID, A.Value1 from Fdf as A
                         where A.Model = %s
                         and A.Enabled > -1
                         and A.Value1 not in
                         (select B.Value1 from Dsc as B
                         where B.Model = %s
                         and B.Enabled > -1)""" % (MODEL_META_DATA_COMPONENT, MODEL_META_DATA_COMPONENT)
            RecordSet = EccGlobalData.gDb.TblFdf.Exec(SqlCommand)
            for Record in RecordSet:
                FdfID = Record[0]
                FilePath = Record[1]
                FilePath = os.path.normpath(mws.join(EccGlobalData.gWorkspace, FilePath))
                SqlCommand = """select ID from Inf where Model = %s and BelongsToFile = (select ID from File where FullPath like '%s')
                                """ % (MODEL_EFI_SOURCE_FILE, FilePath)
                NewRecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                if NewRecordSet != []:
                    if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_BINARY_INF_IN_FDF, FilePath):
                        EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_BINARY_INF_IN_FDF, OtherMsg="File [%s] defined in FDF file and not in DSC file must be a binary module" % (FilePath), BelongsToTable='Fdf', BelongsToItem=FdfID)

    # Check whether a PCD is set in a Dsc file or the FDF file, but not in both.
    def MetaDataFileCheckPcdDuplicate(self):
        if EccGlobalData.gConfig.MetaDataFileCheckPcdDuplicate == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for duplicate PCDs defined in both DSC and FDF files ...")
            SqlCommand = """
                         select A.ID, A.Value1, A.Value2, A.BelongsToFile, B.ID, B.Value1, B.Value2, B.BelongsToFile from Dsc as A, Fdf as B
                         where A.Model >= %s and A.Model < %s
                         and B.Model >= %s and B.Model < %s
                         and A.Value1 = B.Value1
                         and A.Value2 = B.Value2
                         and A.Enabled > -1
                         and B.Enabled > -1
                         group by A.ID
                         """ % (MODEL_PCD, MODEL_META_DATA_HEADER, MODEL_PCD, MODEL_META_DATA_HEADER)
            RecordSet = EccGlobalData.gDb.TblDsc.Exec(SqlCommand)
            for Record in RecordSet:
                SqlCommand1 = """select Name from File where ID = %s""" % Record[3]
                SqlCommand2 = """select Name from File where ID = %s""" % Record[7]
                DscFileName = os.path.splitext(EccGlobalData.gDb.TblDsc.Exec(SqlCommand1)[0][0])[0]
                FdfFileName = os.path.splitext(EccGlobalData.gDb.TblDsc.Exec(SqlCommand2)[0][0])[0]
                if DscFileName != FdfFileName:
                    continue
                if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_PCD_DUPLICATE, Record[1] + '.' + Record[2]):
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_PCD_DUPLICATE, OtherMsg="The PCD [%s] is defined in both FDF file and DSC file" % (Record[1] + '.' + Record[2]), BelongsToTable='Dsc', BelongsToItem=Record[0])
                if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_PCD_DUPLICATE, Record[5] + '.' + Record[6]):
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_PCD_DUPLICATE, OtherMsg="The PCD [%s] is defined in both FDF file and DSC file" % (Record[5] + '.' + Record[6]), BelongsToTable='Fdf', BelongsToItem=Record[4])

            EdkLogger.quiet("Checking for duplicate PCDs defined in DEC files ...")
            SqlCommand = """
                         select A.ID, A.Value1, A.Value2, A.Model, B.Model from Dec as A left join Dec as B
                         where A.Model >= %s and A.Model < %s
                         and B.Model >= %s and B.Model < %s
                         and A.Value1 = B.Value1
                         and A.Value2 = B.Value2
                         and A.Scope1 = B.Scope1
                         and A.ID != B.ID
                         and A.Model = B.Model
                         and A.Enabled > -1
                         and B.Enabled > -1
                         and A.BelongsToFile = B.BelongsToFile
                         group by A.ID
                         """ % (MODEL_PCD, MODEL_META_DATA_HEADER, MODEL_PCD, MODEL_META_DATA_HEADER)
            RecordSet = EccGlobalData.gDb.TblDec.Exec(SqlCommand)
            for Record in RecordSet:
                RecordCat = Record[1] + '.' + Record[2]
                if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_PCD_DUPLICATE, RecordCat):
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_PCD_DUPLICATE, OtherMsg="The PCD [%s] is defined duplicated in DEC file" % RecordCat, BelongsToTable='Dec', BelongsToItem=Record[0])

    # Check whether PCD settings in the FDF file can only be related to flash.
    def MetaDataFileCheckPcdFlash(self):
        if EccGlobalData.gConfig.MetaDataFileCheckPcdFlash == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking only Flash related PCDs are used in FDF ...")
            SqlCommand = """
                         select ID, Value1, Value2, BelongsToFile from Fdf as A
                         where A.Model >= %s and Model < %s
                         and A.Enabled > -1
                         and A.Value2 not like '%%Flash%%'
                         """ % (MODEL_PCD, MODEL_META_DATA_HEADER)
            RecordSet = EccGlobalData.gDb.TblFdf.Exec(SqlCommand)
            for Record in RecordSet:
                if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_PCD_FLASH, Record[1] + '.' + Record[2]):
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_PCD_FLASH, OtherMsg="The PCD [%s] defined in FDF file is not related to Flash" % (Record[1] + '.' + Record[2]), BelongsToTable='Fdf', BelongsToItem=Record[0])

    # Check whether PCDs used in Inf files but not specified in Dsc or FDF files
    def MetaDataFileCheckPcdNoUse(self):
        if EccGlobalData.gConfig.MetaDataFileCheckPcdNoUse == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for non-specified PCDs ...")
            SqlCommand = """
                         select ID, Value1, Value2, BelongsToFile from Inf as A
                         where A.Model >= %s and Model < %s
                         and A.Enabled > -1
                         and (A.Value1, A.Value2) not in
                             (select Value1, Value2 from Dsc as B
                              where B.Model >= %s and B.Model < %s
                              and B.Enabled > -1)
                         and (A.Value1, A.Value2) not in
                             (select Value1, Value2 from Fdf as C
                              where C.Model >= %s and C.Model < %s
                              and C.Enabled > -1)
                         """ % (MODEL_PCD, MODEL_META_DATA_HEADER, MODEL_PCD, MODEL_META_DATA_HEADER, MODEL_PCD, MODEL_META_DATA_HEADER)
            RecordSet = EccGlobalData.gDb.TblInf.Exec(SqlCommand)
            for Record in RecordSet:
                if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_PCD_NO_USE, Record[1] + '.' + Record[2]):
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_PCD_NO_USE, OtherMsg="The PCD [%s] defined in INF file is not specified in either DSC or FDF files" % (Record[1] + '.' + Record[2]), BelongsToTable='Inf', BelongsToItem=Record[0])

    # Check whether having duplicate guids defined for Guid/Protocol/Ppi
    def MetaDataFileCheckGuidDuplicate(self):
        if EccGlobalData.gConfig.MetaDataFileCheckGuidDuplicate == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for duplicate GUID/PPI/PROTOCOL ...")
            # Check Guid
            self.CheckGuidProtocolPpi(ERROR_META_DATA_FILE_CHECK_DUPLICATE_GUID, MODEL_EFI_GUID, EccGlobalData.gDb.TblDec)
            self.CheckGuidProtocolPpi(ERROR_META_DATA_FILE_CHECK_DUPLICATE_GUID, MODEL_EFI_GUID, EccGlobalData.gDb.TblDsc)
            self.CheckGuidProtocolPpiValue(ERROR_META_DATA_FILE_CHECK_DUPLICATE_GUID, MODEL_EFI_GUID)
            # Check protocol
            self.CheckGuidProtocolPpi(ERROR_META_DATA_FILE_CHECK_DUPLICATE_PROTOCOL, MODEL_EFI_PROTOCOL, EccGlobalData.gDb.TblDec)
            self.CheckGuidProtocolPpi(ERROR_META_DATA_FILE_CHECK_DUPLICATE_PROTOCOL, MODEL_EFI_PROTOCOL, EccGlobalData.gDb.TblDsc)
            self.CheckGuidProtocolPpiValue(ERROR_META_DATA_FILE_CHECK_DUPLICATE_PROTOCOL, MODEL_EFI_PROTOCOL)
            # Check ppi
            self.CheckGuidProtocolPpi(ERROR_META_DATA_FILE_CHECK_DUPLICATE_PPI, MODEL_EFI_PPI, EccGlobalData.gDb.TblDec)
            self.CheckGuidProtocolPpi(ERROR_META_DATA_FILE_CHECK_DUPLICATE_PPI, MODEL_EFI_PPI, EccGlobalData.gDb.TblDsc)
            self.CheckGuidProtocolPpiValue(ERROR_META_DATA_FILE_CHECK_DUPLICATE_PPI, MODEL_EFI_PPI)

    # Check whether all files under module directory are described in INF files
    def MetaDataFileCheckModuleFileNoUse(self):
        if EccGlobalData.gConfig.MetaDataFileCheckModuleFileNoUse == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for no used module files ...")
            SqlCommand = """
                         select upper(Path) from File where ID in (select BelongsToFile from Inf where BelongsToFile != -1)
                         """
            InfPathSet = EccGlobalData.gDb.TblInf.Exec(SqlCommand)
            InfPathList = []
            for Item in InfPathSet:
                if Item[0] not in InfPathList:
                    InfPathList.append(Item[0])
            SqlCommand = """
                         select ID, Path, FullPath from File where upper(FullPath) not in
                            (select upper(A.Path) || '%s' || upper(B.Value1) from File as A, INF as B
                            where A.ID in (select BelongsToFile from INF where Model = %s group by BelongsToFile) and
                            B.BelongsToFile = A.ID and B.Model = %s)
                            and (Model = %s or Model = %s)
                        """ % (os.sep, MODEL_EFI_SOURCE_FILE, MODEL_EFI_SOURCE_FILE, MODEL_FILE_C, MODEL_FILE_H)
            RecordSet = EccGlobalData.gDb.TblInf.Exec(SqlCommand)
            for Record in RecordSet:
                Path = Record[1]
                Path = Path.upper().replace('\X64', '').replace('\IA32', '').replace('\EBC', '').replace('\IPF', '').replace('\ARM', '')
                if Path in InfPathList:
                    if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_MODULE_FILE_NO_USE, Record[2]):
                        EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_MODULE_FILE_NO_USE, OtherMsg="The source file [%s] is existing in module directory but it is not described in INF file." % (Record[2]), BelongsToTable='File', BelongsToItem=Record[0])

    # Check whether the PCD is correctly used in C function via its type
    def MetaDataFileCheckPcdType(self):
        if EccGlobalData.gConfig.MetaDataFileCheckPcdType == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for pcd type in c code function usage ...")
            SqlCommand = """
                         select ID, Model, Value1, Value2, BelongsToFile from INF where Model > %s and Model < %s
                         """ % (MODEL_PCD, MODEL_META_DATA_HEADER)
            PcdSet = EccGlobalData.gDb.TblInf.Exec(SqlCommand)
            for Pcd in PcdSet:
                Model = Pcd[1]
                PcdName = Pcd[2]
                if Pcd[3]:
                    PcdName = Pcd[3]
                BelongsToFile = Pcd[4]
                SqlCommand = """
                             select ID from File where FullPath in
                            (select B.Path || '%s' || A.Value1 from INF as A, File as B where A.Model = %s and A.BelongsToFile = %s
                             and B.ID = %s and (B.Model = %s or B.Model = %s))
                             """ % (os.sep, MODEL_EFI_SOURCE_FILE, BelongsToFile, BelongsToFile, MODEL_FILE_C, MODEL_FILE_H)
                TableSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
                for Tbl in TableSet:
                    TblName = 'Identifier' + str(Tbl[0])
                    SqlCommand = """
                                 select Name, ID from %s where value like '%s' and Model = %s
                                 """ % (TblName, PcdName, MODEL_IDENTIFIER_FUNCTION_CALLING)
                    RecordSet = EccGlobalData.gDb.TblInf.Exec(SqlCommand)
                    TblNumber = TblName.replace('Identifier', '')
                    for Record in RecordSet:
                        FunName = Record[0]
                        if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_PCD_TYPE, FunName):
                            if Model in [MODEL_PCD_FIXED_AT_BUILD] and not FunName.startswith('FixedPcdGet'):
                                EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_PCD_TYPE, OtherMsg="The pcd '%s' is defined as a FixPcd but now it is called by c function [%s]" % (PcdName, FunName), BelongsToTable=TblName, BelongsToItem=Record[1])
                            if Model in [MODEL_PCD_FEATURE_FLAG] and (not FunName.startswith('FeaturePcdGet') and not FunName.startswith('FeaturePcdSet')):
                                EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_PCD_TYPE, OtherMsg="The pcd '%s' is defined as a FeaturePcd but now it is called by c function [%s]" % (PcdName, FunName), BelongsToTable=TblName, BelongsToItem=Record[1])
                            if Model in [MODEL_PCD_PATCHABLE_IN_MODULE] and (not FunName.startswith('PatchablePcdGet') and not FunName.startswith('PatchablePcdSet')):
                                EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_PCD_TYPE, OtherMsg="The pcd '%s' is defined as a PatchablePcd but now it is called by c function [%s]" % (PcdName, FunName), BelongsToTable=TblName, BelongsToItem=Record[1])

            #ERROR_META_DATA_FILE_CHECK_PCD_TYPE
        pass

    # Internal worker function to get the INF workspace relative path from FileID
    def GetInfFilePathFromID(self, FileID):
        Table = EccGlobalData.gDb.TblFile
        SqlCommand = """select A.FullPath from %s as A where A.ID = %s""" % (Table.Table, FileID)
        RecordSet = Table.Exec(SqlCommand)
        Path = ""
        for Record in RecordSet:
            Path = mws.relpath(Record[0], EccGlobalData.gWorkspace)
        return Path

    # Check whether two module INFs under one workspace has the same FILE_GUID value
    def MetaDataFileCheckModuleFileGuidDuplication(self):
        if EccGlobalData.gConfig.MetaDataFileCheckModuleFileGuidDuplication == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking for pcd type in c code function usage ...")
            Table = EccGlobalData.gDb.TblInf
            SqlCommand = """
                         select A.ID, A.Value3, A.BelongsToFile, B.BelongsToFile from %s as A, %s as B
                         where A.Value2 = 'FILE_GUID' and B.Value2 = 'FILE_GUID' and
                         A.Value3 = B.Value3 and A.ID != B.ID group by A.ID
                         """ % (Table.Table, Table.Table)
            RecordSet = Table.Exec(SqlCommand)
            for Record in RecordSet:
                InfPath1 = self.GetInfFilePathFromID(Record[2])
                InfPath2 = self.GetInfFilePathFromID(Record[3])
                if InfPath1 and InfPath2:
                    if not EccGlobalData.gException.IsException(ERROR_META_DATA_FILE_CHECK_MODULE_FILE_GUID_DUPLICATION, InfPath1):
                        Msg = "The FILE_GUID of INF file [%s] is duplicated with that of %s" % (InfPath1, InfPath2)
                        EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_MODULE_FILE_GUID_DUPLICATION, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])


    # Check Guid Format in module INF
    def MetaDataFileCheckModuleFileGuidFormat(self):
        if EccGlobalData.gConfig.MetaDataFileCheckModuleFileGuidFormat == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Check Guid Format in module INF ...")
            Table = EccGlobalData.gDb.TblInf
            SqlCommand = """
                         select ID, Value1, Usage, BelongsToFile from %s where Model = %s group by ID
                         """ % (Table.Table, MODEL_EFI_GUID)
            RecordSet = Table.Exec(SqlCommand)
            for Record in RecordSet:
                Value1 = Record[1]
                Value2 = Record[2]
                GuidCommentList = []
                InfPath = self.GetInfFilePathFromID(Record[3])
                Msg = "The GUID format of %s in INF file [%s] does not follow rules" % (Value1, InfPath)
                if Value2.startswith(DT.TAB_SPECIAL_COMMENT):
                    GuidCommentList = Value2[2:].split(DT.TAB_SPECIAL_COMMENT)
                    if GuidCommentList[0].strip().startswith(DT.TAB_INF_USAGE_UNDEFINED):
                        continue
                    elif len(GuidCommentList) > 1:
                        if not GuidCommentList[0].strip().startswith((DT.TAB_INF_USAGE_PRO,
                                                                      DT.TAB_INF_USAGE_SOME_PRO,
                                                                      DT.TAB_INF_USAGE_CON,
                                                                      DT.TAB_INF_USAGE_SOME_CON)):
                            EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_GUID, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])
                        if not (GuidCommentList[1].strip()).startswith(DT.TAB_INF_GUIDTYPE_VAR) and \
                            not GuidCommentList[1].strip().startswith((DT.TAB_INF_GUIDTYPE_EVENT,
                                                                       DT.TAB_INF_GUIDTYPE_HII,
                                                                       DT.TAB_INF_GUIDTYPE_FILE,
                                                                       DT.TAB_INF_GUIDTYPE_HOB,
                                                                       DT.TAB_INF_GUIDTYPE_FV,
                                                                       DT.TAB_INF_GUIDTYPE_ST,
                                                                       DT.TAB_INF_GUIDTYPE_TSG,
                                                                       DT.TAB_INF_GUIDTYPE_GUID,
                                                                       DT.TAB_INF_GUIDTYPE_PROTOCOL,
                                                                       DT.TAB_INF_GUIDTYPE_PPI,
                                                                       DT.TAB_INF_USAGE_UNDEFINED)):
                                EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_GUID, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])
                    else:
                        EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_GUID, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])
                else:
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_GUID, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])

    # Check Protocol Format in module INF
    def MetaDataFileCheckModuleFileProtocolFormat(self):
        if EccGlobalData.gConfig.MetaDataFileCheckModuleFileProtocolFormat == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Check Protocol Format in module INF ...")
            Table = EccGlobalData.gDb.TblInf
            SqlCommand = """
                         select ID, Value1, Usage, BelongsToFile from %s where Model = %s group by ID
                         """ % (Table.Table, MODEL_EFI_PROTOCOL)
            RecordSet = Table.Exec(SqlCommand)
            for Record in RecordSet:
                Value1 = Record[1]
                Value2 = Record[2]
                GuidCommentList = []
                InfPath = self.GetInfFilePathFromID(Record[3])
                Msg = "The Protocol format of %s in INF file [%s] does not follow rules" % (Value1, InfPath)
                if Value2.startswith(DT.TAB_SPECIAL_COMMENT):
                    GuidCommentList = Value2[2:].split(DT.TAB_SPECIAL_COMMENT)
                    if len(GuidCommentList) >= 1:
                        if not GuidCommentList[0].strip().startswith((DT.TAB_INF_USAGE_PRO,
                                                                      DT.TAB_INF_USAGE_SOME_PRO,
                                                                      DT.TAB_INF_USAGE_CON,
                                                                      DT.TAB_INF_USAGE_SOME_CON,
                                                                      DT.TAB_INF_USAGE_NOTIFY,
                                                                      DT.TAB_INF_USAGE_TO_START,
                                                                      DT.TAB_INF_USAGE_BY_START,
                                                                      DT.TAB_INF_USAGE_UNDEFINED)):
                            EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_PROTOCOL, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])
                else:
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_PROTOCOL, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])


    # Check Ppi Format in module INF
    def MetaDataFileCheckModuleFilePpiFormat(self):
        if EccGlobalData.gConfig.MetaDataFileCheckModuleFilePpiFormat == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Check Ppi Format in module INF ...")
            Table = EccGlobalData.gDb.TblInf
            SqlCommand = """
                         select ID, Value1, Usage, BelongsToFile from %s where Model = %s group by ID
                         """ % (Table.Table, MODEL_EFI_PPI)
            RecordSet = Table.Exec(SqlCommand)
            for Record in RecordSet:
                Value1 = Record[1]
                Value2 = Record[2]
                GuidCommentList = []
                InfPath = self.GetInfFilePathFromID(Record[3])
                Msg = "The Ppi format of %s in INF file [%s] does not follow rules" % (Value1, InfPath)
                if Value2.startswith(DT.TAB_SPECIAL_COMMENT):
                    GuidCommentList = Value2[2:].split(DT.TAB_SPECIAL_COMMENT)
                    if len(GuidCommentList) >= 1:
                        if not GuidCommentList[0].strip().startswith((DT.TAB_INF_USAGE_PRO,
                                                                      DT.TAB_INF_USAGE_SOME_PRO,
                                                                      DT.TAB_INF_USAGE_CON,
                                                                      DT.TAB_INF_USAGE_SOME_CON,
                                                                      DT.TAB_INF_USAGE_NOTIFY,
                                                                      DT.TAB_INF_USAGE_UNDEFINED)):
                            EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_PPI, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])
                else:
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_PPI, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])

    # Check Pcd Format in module INF
    def MetaDataFileCheckModuleFilePcdFormat(self):
        if EccGlobalData.gConfig.MetaDataFileCheckModuleFilePcdFormat == '1' or EccGlobalData.gConfig.MetaDataFileCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Check Pcd Format in module INF ...")
            Table = EccGlobalData.gDb.TblInf
            SqlCommand = """
                         select ID, Model, Value1, Value2, Usage, BelongsToFile from %s where Model >= %s and Model < %s group by ID
                         """ % (Table.Table, MODEL_PCD, MODEL_META_DATA_HEADER)
            RecordSet = Table.Exec(SqlCommand)
            for Record in RecordSet:
                Model = Record[1]
                PcdName = Record[2] + '.' + Record[3]
                Usage = Record[4]
                PcdCommentList = []
                InfPath = self.GetInfFilePathFromID(Record[5])
                Msg = "The Pcd format of %s in INF file [%s] does not follow rules" % (PcdName, InfPath)
                if Usage.startswith(DT.TAB_SPECIAL_COMMENT):
                    PcdCommentList = Usage[2:].split(DT.TAB_SPECIAL_COMMENT)
                    if len(PcdCommentList) >= 1:
                        if Model in [MODEL_PCD_FIXED_AT_BUILD, MODEL_PCD_FEATURE_FLAG] \
                            and not PcdCommentList[0].strip().startswith((DT.TAB_INF_USAGE_SOME_PRO,
                                                                          DT.TAB_INF_USAGE_CON,
                                                                          DT.TAB_INF_USAGE_UNDEFINED)):
                            EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_PCD, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])
                        if Model in [MODEL_PCD_PATCHABLE_IN_MODULE, MODEL_PCD_DYNAMIC, MODEL_PCD_DYNAMIC_EX] \
                            and not PcdCommentList[0].strip().startswith((DT.TAB_INF_USAGE_PRO,
                                                                          DT.TAB_INF_USAGE_SOME_PRO,
                                                                          DT.TAB_INF_USAGE_CON,
                                                                          DT.TAB_INF_USAGE_SOME_CON,
                                                                          DT.TAB_INF_USAGE_UNDEFINED)):
                            EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_PCD, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])
                else:
                    EccGlobalData.gDb.TblReport.Insert(ERROR_META_DATA_FILE_CHECK_FORMAT_PCD, OtherMsg=Msg, BelongsToTable=Table.Table, BelongsToItem=Record[0])

    # Check whether these is duplicate Guid/Ppi/Protocol name
    def CheckGuidProtocolPpi(self, ErrorID, Model, Table):
        Name = ''
        if Model == MODEL_EFI_GUID:
            Name = 'guid'
        if Model == MODEL_EFI_PROTOCOL:
            Name = 'protocol'
        if Model == MODEL_EFI_PPI:
            Name = 'ppi'
        SqlCommand = """
                     select A.ID, A.Value1 from %s as A, %s as B
                     where A.Model = %s and B.Model = %s
                     and A.Value1 like B.Value1 and A.ID != B.ID
                     and A.Scope1 = B.Scope1
                     and A.Enabled > -1
                     and B.Enabled > -1
                     group by A.ID
                     """ % (Table.Table, Table.Table, Model, Model)
        RecordSet = Table.Exec(SqlCommand)
        for Record in RecordSet:
            if not EccGlobalData.gException.IsException(ErrorID, Record[1]):
                EccGlobalData.gDb.TblReport.Insert(ErrorID, OtherMsg="The %s name [%s] is defined more than one time" % (Name.upper(), Record[1]), BelongsToTable=Table.Table, BelongsToItem=Record[0])

    # Check whether these is duplicate Guid/Ppi/Protocol value
    def CheckGuidProtocolPpiValue(self, ErrorID, Model):
        Name = ''
        Table = EccGlobalData.gDb.TblDec
        if Model == MODEL_EFI_GUID:
            Name = 'guid'
        if Model == MODEL_EFI_PROTOCOL:
            Name = 'protocol'
        if Model == MODEL_EFI_PPI:
            Name = 'ppi'
        SqlCommand = """
                     select A.ID, A.Value1, A.Value2 from %s as A, %s as B
                     where A.Model = %s and B.Model = %s
                     and A.Value2 like B.Value2 and A.ID != B.ID
                     and A.Scope1 = B.Scope1 and A.Value1 != B.Value1
                     group by A.ID
                     """ % (Table.Table, Table.Table, Model, Model)
        RecordSet = Table.Exec(SqlCommand)
        for Record in RecordSet:
            if not EccGlobalData.gException.IsException(ErrorID, Record[2]):
                EccGlobalData.gDb.TblReport.Insert(ErrorID, OtherMsg="The %s value [%s] is used more than one time" % (Name.upper(), Record[2]), BelongsToTable=Table.Table, BelongsToItem=Record[0])

    # Naming Convention Check
    def NamingConventionCheck(self):
        if EccGlobalData.gConfig.NamingConventionCheckDefineStatement == '1' \
        or EccGlobalData.gConfig.NamingConventionCheckTypedefStatement == '1' \
        or EccGlobalData.gConfig.NamingConventionCheckIfndefStatement == '1' \
        or EccGlobalData.gConfig.NamingConventionCheckVariableName == '1' \
        or EccGlobalData.gConfig.NamingConventionCheckSingleCharacterVariable == '1' \
        or EccGlobalData.gConfig.NamingConventionCheckAll == '1'\
        or EccGlobalData.gConfig.CheckAll == '1':
            for Dirpath, Dirnames, Filenames in self.WalkTree():
                for F in Filenames:
                    if os.path.splitext(F)[1] in ('.h', '.c'):
                        FullName = os.path.join(Dirpath, F)
                        Id = c.GetTableID(FullName)
                        if Id < 0:
                            continue
                        FileTable = 'Identifier' + str(Id)
                        self.NamingConventionCheckDefineStatement(FileTable)
                        self.NamingConventionCheckTypedefStatement(FileTable)
                        self.NamingConventionCheckVariableName(FileTable)
                        self.NamingConventionCheckSingleCharacterVariable(FileTable)
                        if os.path.splitext(F)[1] in ('.h'):
                            self.NamingConventionCheckIfndefStatement(FileTable)

        self.NamingConventionCheckPathName()
        self.NamingConventionCheckFunctionName()

    # Check whether only capital letters are used for #define declarations
    def NamingConventionCheckDefineStatement(self, FileTable):
        if EccGlobalData.gConfig.NamingConventionCheckDefineStatement == '1' or EccGlobalData.gConfig.NamingConventionCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking naming convention of #define statement ...")

            SqlCommand = """select ID, Value from %s where Model = %s""" % (FileTable, MODEL_IDENTIFIER_MACRO_DEFINE)
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                Name = Record[1].strip().split()[1]
                if Name.find('(') != -1:
                    Name = Name[0:Name.find('(')]
                if Name.upper() != Name:
                    if not EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_DEFINE_STATEMENT, Name):
                        EccGlobalData.gDb.TblReport.Insert(ERROR_NAMING_CONVENTION_CHECK_DEFINE_STATEMENT, OtherMsg="The #define name [%s] does not follow the rules" % (Name), BelongsToTable=FileTable, BelongsToItem=Record[0])

    # Check whether only capital letters are used for typedef declarations
    def NamingConventionCheckTypedefStatement(self, FileTable):
        if EccGlobalData.gConfig.NamingConventionCheckTypedefStatement == '1' or EccGlobalData.gConfig.NamingConventionCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking naming convention of #typedef statement ...")

            SqlCommand = """select ID, Name from %s where Model = %s""" % (FileTable, MODEL_IDENTIFIER_TYPEDEF)
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                Name = Record[1].strip()
                if Name != '' and Name is not None:
                    if Name[0] == '(':
                        Name = Name[1:Name.find(')')]
                    if Name.find('(') > -1:
                        Name = Name[Name.find('(') + 1 : Name.find(')')]
                    Name = Name.replace('WINAPI', '')
                    Name = Name.replace('*', '').strip()
                    if Name.upper() != Name:
                        if not EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_TYPEDEF_STATEMENT, Name):
                            EccGlobalData.gDb.TblReport.Insert(ERROR_NAMING_CONVENTION_CHECK_TYPEDEF_STATEMENT, OtherMsg="The #typedef name [%s] does not follow the rules" % (Name), BelongsToTable=FileTable, BelongsToItem=Record[0])

    # Check whether the #ifndef at the start of an include file uses both prefix and postfix underscore characters, '_'.
    def NamingConventionCheckIfndefStatement(self, FileTable):
        if EccGlobalData.gConfig.NamingConventionCheckIfndefStatement == '1' or EccGlobalData.gConfig.NamingConventionCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking naming convention of #ifndef statement ...")

            SqlCommand = """select ID, Value from %s where Model = %s""" % (FileTable, MODEL_IDENTIFIER_MACRO_IFNDEF)
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                Name = Record[1].replace('#ifndef', '').strip()
                if Name[0] == '_' or Name[-1] != '_' or Name[-2] == '_':
                    if not EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_IFNDEF_STATEMENT, Name):
                        EccGlobalData.gDb.TblReport.Insert(ERROR_NAMING_CONVENTION_CHECK_IFNDEF_STATEMENT, OtherMsg="The #ifndef name [%s] does not follow the rules" % (Name), BelongsToTable=FileTable, BelongsToItem=Record[0])

    # Rule for path name, variable name and function name
    # 1. First character should be upper case
    # 2. Existing lower case in a word
    # 3. No space existence
    # Check whether the path name followed the rule
    def NamingConventionCheckPathName(self):
        if EccGlobalData.gConfig.NamingConventionCheckPathName == '1' or EccGlobalData.gConfig.NamingConventionCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking naming convention of file path name ...")
            Pattern = re.compile(r'^[A-Z]+\S*[a-z]\S*$')
            SqlCommand = """select ID, Name from File"""
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                if not Pattern.match(Record[1]):
                    if not EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_PATH_NAME, Record[1]):
                        EccGlobalData.gDb.TblReport.Insert(ERROR_NAMING_CONVENTION_CHECK_PATH_NAME, OtherMsg="The file path [%s] does not follow the rules" % (Record[1]), BelongsToTable='File', BelongsToItem=Record[0])

    # Rule for path name, variable name and function name
    # 1. First character should be upper case
    # 2. Existing lower case in a word
    # 3. No space existence
    # 4. Global variable name must start with a 'g'
    # Check whether the variable name followed the rule
    def NamingConventionCheckVariableName(self, FileTable):
        if EccGlobalData.gConfig.NamingConventionCheckVariableName == '1' or EccGlobalData.gConfig.NamingConventionCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking naming convention of variable name ...")
            Pattern = re.compile(r'^[A-Zgm]+\S*[a-z]\S*$')

            SqlCommand = """select ID, Name, Modifier from %s where Model = %s""" % (FileTable, MODEL_IDENTIFIER_VARIABLE)
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                Var = Record[1]
                Modifier = Record[2]
                if Var.startswith('CONST'):
                    Var = Var[5:].lstrip()
                if not Pattern.match(Var) and not (Modifier.endswith('*') and Var.startswith('p')):
                    if not EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME, Record[1]):
                        EccGlobalData.gDb.TblReport.Insert(ERROR_NAMING_CONVENTION_CHECK_VARIABLE_NAME, OtherMsg="The variable name [%s] does not follow the rules" % (Record[1]), BelongsToTable=FileTable, BelongsToItem=Record[0])

    # Rule for path name, variable name and function name
    # 1. First character should be upper case
    # 2. Existing lower case in a word
    # 3. No space existence
    # Check whether the function name followed the rule
    def NamingConventionCheckFunctionName(self):
        if EccGlobalData.gConfig.NamingConventionCheckFunctionName == '1' or EccGlobalData.gConfig.NamingConventionCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking naming convention of function name ...")
            Pattern = re.compile(r'^[A-Z]+\S*[a-z]\S*$')
            SqlCommand = """select ID, Name from Function"""
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                if not Pattern.match(Record[1]):
                    if not EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_FUNCTION_NAME, Record[1]):
                        EccGlobalData.gDb.TblReport.Insert(ERROR_NAMING_CONVENTION_CHECK_FUNCTION_NAME, OtherMsg="The function name [%s] does not follow the rules" % (Record[1]), BelongsToTable='Function', BelongsToItem=Record[0])

    # Check whether NO use short variable name with single character
    def NamingConventionCheckSingleCharacterVariable(self, FileTable):
        if EccGlobalData.gConfig.NamingConventionCheckSingleCharacterVariable == '1' or EccGlobalData.gConfig.NamingConventionCheckAll == '1' or EccGlobalData.gConfig.CheckAll == '1':
            EdkLogger.quiet("Checking naming convention of single character variable name ...")

            SqlCommand = """select ID, Name from %s where Model = %s""" % (FileTable, MODEL_IDENTIFIER_VARIABLE)
            RecordSet = EccGlobalData.gDb.TblFile.Exec(SqlCommand)
            for Record in RecordSet:
                Variable = Record[1].replace('*', '')
                if len(Variable) == 1:
                    if not EccGlobalData.gException.IsException(ERROR_NAMING_CONVENTION_CHECK_SINGLE_CHARACTER_VARIABLE, Record[1]):
                        EccGlobalData.gDb.TblReport.Insert(ERROR_NAMING_CONVENTION_CHECK_SINGLE_CHARACTER_VARIABLE, OtherMsg="The variable name [%s] does not follow the rules" % (Record[1]), BelongsToTable=FileTable, BelongsToItem=Record[0])

def FindPara(FilePath, Para, CallingLine):
    Lines = open(FilePath).readlines()
    Line = ''
    for Index in range(CallingLine - 1, 0, -1):
        # Find the nearest statement for Para
        Line = Lines[Index].strip()
        if Line.startswith('%s = ' % Para):
            Line = Line.strip()
            return Line
            break

    return ''

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    Check = Check()
    Check.Check()
