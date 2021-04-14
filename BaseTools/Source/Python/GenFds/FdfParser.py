## @file
# parse FDF file
#
#  Copyright (c) 2007 - 2021, Intel Corporation. All rights reserved.<BR>
#  Copyright (c) 2015, Hewlett Packard Enterprise Development, L.P.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import print_function
from __future__ import absolute_import
from re import compile, DOTALL
from string import hexdigits
from uuid import UUID

from Common.BuildToolError import *
from Common import EdkLogger
from Common.Misc import PathClass, tdict, ProcessDuplicatedInf, GuidStructureStringToGuidString
from Common.StringUtils import NormPath, ReplaceMacro
from Common import GlobalData
from Common.Expression import *
from Common.DataType import *
from Common.MultipleWorkspace import MultipleWorkspace as mws
import Common.LongFilePathOs as os
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.RangeExpression import RangeExpression
from collections import OrderedDict

from .Fd import FD
from .Region import Region
from .Fv import FV
from .AprioriSection import AprioriSection
from .FfsInfStatement import FfsInfStatement
from .FfsFileStatement import FileStatement
from .VerSection import VerSection
from .UiSection import UiSection
from .FvImageSection import FvImageSection
from .DataSection import DataSection
from .DepexSection import DepexSection
from .CompressSection import CompressSection
from .GuidSection import GuidSection
from .Capsule import EFI_CERT_TYPE_PKCS7_GUID, EFI_CERT_TYPE_RSA2048_SHA256_GUID, Capsule
from .CapsuleData import CapsuleFfs, CapsulePayload, CapsuleFv, CapsuleFd, CapsuleAnyFile, CapsuleAfile
from .RuleComplexFile import RuleComplexFile
from .RuleSimpleFile import RuleSimpleFile
from .EfiSection import EfiSection
from .OptionRom import OPTIONROM
from .OptRomInfStatement import OptRomInfStatement, OverrideAttribs
from .OptRomFileStatement import OptRomFileStatement
from .GenFdsGlobalVariable import GenFdsGlobalVariable

T_CHAR_CR = '\r'
T_CHAR_TAB = '\t'
T_CHAR_DOUBLE_QUOTE = '\"'
T_CHAR_SINGLE_QUOTE = '\''
T_CHAR_BRACE_R = '}'

SEPARATORS = {TAB_EQUAL_SPLIT, TAB_VALUE_SPLIT, TAB_COMMA_SPLIT, '{', T_CHAR_BRACE_R}
ALIGNMENTS = {"Auto", "8", "16", "32", "64", "128", "512", "1K", "4K", "32K", "64K", "128K",
                                    "256K", "512K", "1M", "2M", "4M", "8M", "16M"}
ALIGNMENT_NOAUTO = ALIGNMENTS - {"Auto"}
CR_LB_SET = {T_CHAR_CR, TAB_LINE_BREAK}

RegionSizePattern = compile("\s*(?P<base>(?:0x|0X)?[a-fA-F0-9]+)\s*\|\s*(?P<size>(?:0x|0X)?[a-fA-F0-9]+)\s*")
RegionSizeGuidPattern = compile("\s*(?P<base>\w+\.\w+[\.\w\[\]]*)\s*\|\s*(?P<size>\w+\.\w+[\.\w\[\]]*)\s*")
RegionOffsetPcdPattern = compile("\s*(?P<base>\w+\.\w+[\.\w\[\]]*)\s*$")
ShortcutPcdPattern = compile("\s*\w+\s*=\s*(?P<value>(?:0x|0X)?[a-fA-F0-9]+)\s*\|\s*(?P<name>\w+\.\w+)\s*")
BaseAddrValuePattern = compile('^0[xX][0-9a-fA-F]+')
FileExtensionPattern = compile(r'([a-zA-Z][a-zA-Z0-9]*)')
TokenFindPattern = compile(r'([a-zA-Z0-9\-]+|\$\(TARGET\)|\*)_([a-zA-Z0-9\-]+|\$\(TOOL_CHAIN_TAG\)|\*)_([a-zA-Z0-9\-]+|\$\(ARCH\)|\*)')
AllIncludeFileList = []

# Get the closest parent
def GetParentAtLine (Line):
    for Profile in AllIncludeFileList:
        if Profile.IsLineInFile(Line):
            return Profile
    return None

# Check include loop
def IsValidInclude (File, Line):
    for Profile in AllIncludeFileList:
        if Profile.IsLineInFile(Line) and Profile.FileName == File:
            return False

    return True

def GetRealFileLine (File, Line):
    InsertedLines = 0
    for Profile in AllIncludeFileList:
        if Profile.IsLineInFile(Line):
            return Profile.GetLineInFile(Line)
        elif Line >= Profile.InsertStartLineNumber and Profile.Level == 1:
            InsertedLines += Profile.GetTotalLines()

    return (File, Line - InsertedLines)

## The exception class that used to report error messages when parsing FDF
#
# Currently the "ToolName" is set to be "FdfParser".
#
class Warning (Exception):
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Str         The message to record
    #   @param  File        The FDF name
    #   @param  Line        The Line number that error occurs
    #
    def __init__(self, Str, File = None, Line = None):
        FileLineTuple = GetRealFileLine(File, Line)
        self.FileName = FileLineTuple[0]
        self.LineNumber = FileLineTuple[1]
        self.OriginalLineNumber = Line
        self.Message = Str
        self.ToolName = 'FdfParser'

    def __str__(self):
        return self.Message

    # helper functions to facilitate consistency in warnings
    # each function is for a different common warning
    @staticmethod
    def Expected(Str, File, Line):
        return Warning("expected {}".format(Str), File, Line)
    @staticmethod
    def ExpectedEquals(File, Line):
        return Warning.Expected("'='", File, Line)
    @staticmethod
    def ExpectedCurlyOpen(File, Line):
        return Warning.Expected("'{'", File, Line)
    @staticmethod
    def ExpectedCurlyClose(File, Line):
        return Warning.Expected("'}'", File, Line)
    @staticmethod
    def ExpectedBracketClose(File, Line):
        return Warning.Expected("']'", File, Line)

## The Include file content class that used to record file data when parsing include file
#
# May raise Exception when opening file.
#
class IncludeFileProfile:
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  FileName    The file that to be parsed
    #
    def __init__(self, FileName):
        self.FileName = FileName
        self.FileLinesList = []
        try:
            with open(FileName, "r") as fsock:
                self.FileLinesList = fsock.readlines()
                for index, line in enumerate(self.FileLinesList):
                    if not line.endswith(TAB_LINE_BREAK):
                        self.FileLinesList[index] += TAB_LINE_BREAK
        except:
            EdkLogger.error("FdfParser", FILE_OPEN_FAILURE, ExtraData=FileName)

        self.InsertStartLineNumber = None
        self.InsertAdjust = 0
        self.IncludeFileList = []
        self.Level = 1 # first level include file

    def GetTotalLines(self):
        TotalLines = self.InsertAdjust + len(self.FileLinesList)

        for Profile in self.IncludeFileList:
            TotalLines += Profile.GetTotalLines()

        return TotalLines

    def IsLineInFile(self, Line):
        if Line >= self.InsertStartLineNumber and Line < self.InsertStartLineNumber + self.GetTotalLines():
            return True

        return False

    def GetLineInFile(self, Line):
        if not self.IsLineInFile (Line):
            return (self.FileName, -1)

        InsertedLines = self.InsertStartLineNumber

        for Profile in self.IncludeFileList:
            if Profile.IsLineInFile(Line):
                return Profile.GetLineInFile(Line)
            elif Line >= Profile.InsertStartLineNumber:
                InsertedLines += Profile.GetTotalLines()

        return (self.FileName, Line - InsertedLines + 1)

## The FDF content class that used to record file data when parsing FDF
#
# May raise Exception when opening file.
#
class FileProfile:
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  FileName    The file that to be parsed
    #
    def __init__(self, FileName):
        self.FileLinesList = []
        try:
            with open(FileName, "r") as fsock:
                self.FileLinesList = fsock.readlines()

        except:
            EdkLogger.error("FdfParser", FILE_OPEN_FAILURE, ExtraData=FileName)

        self.FileName = FileName
        self.PcdDict = OrderedDict()
        self.PcdLocalDict = OrderedDict()
        self.InfList = []
        self.InfDict = {'ArchTBD':[]}
        # ECC will use this Dict and List information
        self.PcdFileLineDict = {}
        self.InfFileLineList = []

        self.FdDict = {}
        self.FdNameNotSet = False
        self.FvDict = {}
        self.CapsuleDict = {}
        self.RuleDict = {}
        self.OptRomDict = {}
        self.FmpPayloadDict = {}

## The syntax parser for FDF
#
# PreprocessFile method should be called prior to ParseFile
# CycleReferenceCheck method can detect cycles in FDF contents
#
# GetNext*** procedures mean these procedures will get next token first, then make judgement.
# Get*** procedures mean these procedures will make judgement on current token only.
#
class FdfParser:
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  FileName    The file that to be parsed
    #
    def __init__(self, FileName):
        self.Profile = FileProfile(FileName)
        self.FileName = FileName
        self.CurrentLineNumber = 1
        self.CurrentOffsetWithinLine = 0
        self.CurrentFdName = None
        self.CurrentFvName = None
        self._Token = ""
        self._SkippedChars = ""
        GlobalData.gFdfParser = self

        # Used to section info
        self._CurSection = []
        # Key: [section name, UI name, arch]
        # Value: {MACRO_NAME: MACRO_VALUE}
        self._MacroDict = tdict(True, 3)
        self._PcdDict = OrderedDict()

        self._WipeOffArea = []
        if GenFdsGlobalVariable.WorkSpaceDir == '':
            GenFdsGlobalVariable.WorkSpaceDir = os.getenv("WORKSPACE")

    ## _SkipWhiteSpace() method
    #
    #   Skip white spaces from current char.
    #
    #   @param  self        The object pointer
    #
    def _SkipWhiteSpace(self):
        while not self._EndOfFile():
            if self._CurrentChar() in {TAB_PRINTCHAR_NUL, T_CHAR_CR, TAB_LINE_BREAK, TAB_SPACE_SPLIT, T_CHAR_TAB}:
                self._SkippedChars += str(self._CurrentChar())
                self._GetOneChar()
            else:
                return
        return

    ## _EndOfFile() method
    #
    #   Judge current buffer pos is at file end
    #
    #   @param  self        The object pointer
    #   @retval True        Current File buffer position is at file end
    #   @retval False       Current File buffer position is NOT at file end
    #
    def _EndOfFile(self):
        NumberOfLines = len(self.Profile.FileLinesList)
        SizeOfLastLine = len(self.Profile.FileLinesList[-1])
        if self.CurrentLineNumber == NumberOfLines and self.CurrentOffsetWithinLine >= SizeOfLastLine - 1:
            return True
        if self.CurrentLineNumber > NumberOfLines:
            return True
        return False

    ## _EndOfLine() method
    #
    #   Judge current buffer pos is at line end
    #
    #   @param  self        The object pointer
    #   @retval True        Current File buffer position is at line end
    #   @retval False       Current File buffer position is NOT at line end
    #
    def _EndOfLine(self):
        if self.CurrentLineNumber > len(self.Profile.FileLinesList):
            return True
        SizeOfCurrentLine = len(self.Profile.FileLinesList[self.CurrentLineNumber - 1])
        if self.CurrentOffsetWithinLine >= SizeOfCurrentLine:
            return True
        return False

    ## Rewind() method
    #
    #   Reset file data buffer to the initial state
    #
    #   @param  self        The object pointer
    #   @param  DestLine    Optional new destination line number.
    #   @param  DestOffset  Optional new destination offset.
    #
    def Rewind(self, DestLine = 1, DestOffset = 0):
        self.CurrentLineNumber = DestLine
        self.CurrentOffsetWithinLine = DestOffset

    ## _UndoOneChar() method
    #
    #   Go back one char in the file buffer
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully go back one char
    #   @retval False       Not able to go back one char as file beginning reached
    #
    def _UndoOneChar(self):
        if self.CurrentLineNumber == 1 and self.CurrentOffsetWithinLine == 0:
            return False
        elif self.CurrentOffsetWithinLine == 0:
            self.CurrentLineNumber -= 1
            self.CurrentOffsetWithinLine = len(self._CurrentLine()) - 1
        else:
            self.CurrentOffsetWithinLine -= 1
        return True

    ## _GetOneChar() method
    #
    #   Move forward one char in the file buffer
    #
    #   @param  self        The object pointer
    #
    def _GetOneChar(self):
        if self.CurrentOffsetWithinLine == len(self.Profile.FileLinesList[self.CurrentLineNumber - 1]) - 1:
            self.CurrentLineNumber += 1
            self.CurrentOffsetWithinLine = 0
        else:
            self.CurrentOffsetWithinLine += 1

    ## _CurrentChar() method
    #
    #   Get the char pointed to by the file buffer pointer
    #
    #   @param  self        The object pointer
    #   @retval Char        Current char
    #
    def _CurrentChar(self):
        return self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine]

    ## _NextChar() method
    #
    #   Get the one char pass the char pointed to by the file buffer pointer
    #
    #   @param  self        The object pointer
    #   @retval Char        Next char
    #
    def _NextChar(self):
        if self.CurrentOffsetWithinLine == len(self.Profile.FileLinesList[self.CurrentLineNumber - 1]) - 1:
            return self.Profile.FileLinesList[self.CurrentLineNumber][0]
        return self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine + 1]

    ## _SetCurrentCharValue() method
    #
    #   Modify the value of current char
    #
    #   @param  self        The object pointer
    #   @param  Value       The new value of current char
    #
    def _SetCurrentCharValue(self, Value):
        self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine] = Value

    ## _CurrentLine() method
    #
    #   Get the list that contains current line contents
    #
    #   @param  self        The object pointer
    #   @retval List        current line contents
    #
    def _CurrentLine(self):
        return self.Profile.FileLinesList[self.CurrentLineNumber - 1]

    def _StringToList(self):
        self.Profile.FileLinesList = [list(s) for s in self.Profile.FileLinesList]
        if not self.Profile.FileLinesList:
            EdkLogger.error('FdfParser', FILE_READ_FAILURE, 'The file is empty!', File=self.FileName)
        self.Profile.FileLinesList[-1].append(' ')

    def _ReplaceFragment(self, StartPos, EndPos, Value = ' '):
        if StartPos[0] == EndPos[0]:
            Offset = StartPos[1]
            while Offset <= EndPos[1]:
                self.Profile.FileLinesList[StartPos[0]][Offset] = Value
                Offset += 1
            return

        Offset = StartPos[1]
        while self.Profile.FileLinesList[StartPos[0]][Offset] not in CR_LB_SET:
            self.Profile.FileLinesList[StartPos[0]][Offset] = Value
            Offset += 1

        Line = StartPos[0]
        while Line < EndPos[0]:
            Offset = 0
            while self.Profile.FileLinesList[Line][Offset] not in CR_LB_SET:
                self.Profile.FileLinesList[Line][Offset] = Value
                Offset += 1
            Line += 1

        Offset = 0
        while Offset <= EndPos[1]:
            self.Profile.FileLinesList[EndPos[0]][Offset] = Value
            Offset += 1

    def _SetMacroValue(self, Macro, Value):
        if not self._CurSection:
            return

        MacroDict = {}
        if not self._MacroDict[self._CurSection[0], self._CurSection[1], self._CurSection[2]]:
            self._MacroDict[self._CurSection[0], self._CurSection[1], self._CurSection[2]] = MacroDict
        else:
            MacroDict = self._MacroDict[self._CurSection[0], self._CurSection[1], self._CurSection[2]]
        MacroDict[Macro] = Value

    def _GetMacroValue(self, Macro):
        # Highest priority
        if Macro in GlobalData.gCommandLineDefines:
            return GlobalData.gCommandLineDefines[Macro]
        if Macro in GlobalData.gGlobalDefines:
            return GlobalData.gGlobalDefines[Macro]

        if self._CurSection:
            MacroDict = self._MacroDict[
                        self._CurSection[0],
                        self._CurSection[1],
                        self._CurSection[2]
            ]
            if MacroDict and Macro in MacroDict:
                return MacroDict[Macro]

        # Lowest priority
        if Macro in GlobalData.gPlatformDefines:
            return GlobalData.gPlatformDefines[Macro]
        return None

    def _SectionHeaderParser(self, Section):
        # [Defines]
        # [FD.UiName]: use dummy instead if UI name is optional
        # [FV.UiName]
        # [Capsule.UiName]
        # [Rule]: don't take rule section into account, macro is not allowed in this section
        # [OptionRom.DriverName]
        self._CurSection = []
        Section = Section.strip()[1:-1].upper().replace(' ', '').strip(TAB_SPLIT)
        ItemList = Section.split(TAB_SPLIT)
        Item = ItemList[0]
        if Item == '' or Item == 'RULE':
            return

        if Item == TAB_COMMON_DEFINES.upper():
            self._CurSection = [TAB_COMMON, TAB_COMMON, TAB_COMMON]
        elif len(ItemList) > 1:
            self._CurSection = [ItemList[0], ItemList[1], TAB_COMMON]
        elif len(ItemList) > 0:
            self._CurSection = [ItemList[0], 'DUMMY', TAB_COMMON]

    ## PreprocessFile() method
    #
    #   Preprocess file contents, replace comments with spaces.
    #   In the end, rewind the file buffer pointer to the beginning
    #   BUGBUG: No !include statement processing contained in this procedure
    #   !include statement should be expanded at the same FileLinesList[CurrentLineNumber - 1]
    #
    #   @param  self        The object pointer
    #
    def PreprocessFile(self):
        self.Rewind()
        InComment = False
        DoubleSlashComment = False
        HashComment = False
        # HashComment in quoted string " " is ignored.
        InString = False

        while not self._EndOfFile():

            if self._CurrentChar() == T_CHAR_DOUBLE_QUOTE and not InComment:
                InString = not InString
            # meet new line, then no longer in a comment for // and '#'
            if self._CurrentChar() == TAB_LINE_BREAK:
                self.CurrentLineNumber += 1
                self.CurrentOffsetWithinLine = 0
                if InComment and DoubleSlashComment:
                    InComment = False
                    DoubleSlashComment = False
                if InComment and HashComment:
                    InComment = False
                    HashComment = False
            # check for */ comment end
            elif InComment and not DoubleSlashComment and not HashComment and self._CurrentChar() == TAB_STAR and self._NextChar() == TAB_BACK_SLASH:
                self._SetCurrentCharValue(TAB_SPACE_SPLIT)
                self._GetOneChar()
                self._SetCurrentCharValue(TAB_SPACE_SPLIT)
                self._GetOneChar()
                InComment = False
            # set comments to spaces
            elif InComment:
                self._SetCurrentCharValue(TAB_SPACE_SPLIT)
                self._GetOneChar()
            # check for // comment
            elif self._CurrentChar() == TAB_BACK_SLASH and self._NextChar() == TAB_BACK_SLASH and not self._EndOfLine():
                InComment = True
                DoubleSlashComment = True
            # check for '#' comment
            elif self._CurrentChar() == TAB_COMMENT_SPLIT and not self._EndOfLine() and not InString:
                InComment = True
                HashComment = True
            # check for /* comment start
            elif self._CurrentChar() == TAB_BACK_SLASH and self._NextChar() == TAB_STAR:
                self._SetCurrentCharValue(TAB_SPACE_SPLIT)
                self._GetOneChar()
                self._SetCurrentCharValue(TAB_SPACE_SPLIT)
                self._GetOneChar()
                InComment = True
            else:
                self._GetOneChar()

        # restore from ListOfList to ListOfString
        self.Profile.FileLinesList = ["".join(list) for list in self.Profile.FileLinesList]
        self.Rewind()

    ## PreprocessIncludeFile() method
    #
    #   Preprocess file contents, replace !include statements with file contents.
    #   In the end, rewind the file buffer pointer to the beginning
    #
    #   @param  self        The object pointer
    #
    def PreprocessIncludeFile(self):
      # nested include support
        Processed = False
        MacroDict = {}
        while self._GetNextToken():

            if self._Token == TAB_DEFINE:
                if not self._GetNextToken():
                    raise Warning.Expected("Macro name", self.FileName, self.CurrentLineNumber)
                Macro = self._Token
                if not self._IsToken(TAB_EQUAL_SPLIT):
                    raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
                Value = self._GetExpression()
                MacroDict[Macro] = Value

            elif self._Token == TAB_INCLUDE:
                Processed = True
                IncludeLine = self.CurrentLineNumber
                IncludeOffset = self.CurrentOffsetWithinLine - len(TAB_INCLUDE)
                if not self._GetNextToken():
                    raise Warning.Expected("include file name", self.FileName, self.CurrentLineNumber)
                IncFileName = self._Token
                PreIndex = 0
                StartPos = IncFileName.find('$(', PreIndex)
                EndPos = IncFileName.find(')', StartPos+2)
                while StartPos != -1 and EndPos != -1:
                    Macro = IncFileName[StartPos+2: EndPos]
                    MacroVal = self._GetMacroValue(Macro)
                    if not MacroVal:
                        if Macro in MacroDict:
                            MacroVal = MacroDict[Macro]
                    if MacroVal is not None:
                        IncFileName = IncFileName.replace('$(' + Macro + ')', MacroVal, 1)
                        if MacroVal.find('$(') != -1:
                            PreIndex = StartPos
                        else:
                            PreIndex = StartPos + len(MacroVal)
                    else:
                        raise Warning("The Macro %s is not defined" %Macro, self.FileName, self.CurrentLineNumber)
                    StartPos = IncFileName.find('$(', PreIndex)
                    EndPos = IncFileName.find(')', StartPos+2)

                IncludedFile = NormPath(IncFileName)
                #
                # First search the include file under the same directory as FDF file
                #
                IncludedFile1 = PathClass(IncludedFile, os.path.dirname(self.FileName))
                ErrorCode = IncludedFile1.Validate()[0]
                if ErrorCode != 0:
                    #
                    # Then search the include file under the same directory as DSC file
                    #
                    PlatformDir = ''
                    if GenFdsGlobalVariable.ActivePlatform:
                        PlatformDir = GenFdsGlobalVariable.ActivePlatform.Dir
                    elif GlobalData.gActivePlatform:
                        PlatformDir = GlobalData.gActivePlatform.MetaFile.Dir
                    IncludedFile1 = PathClass(IncludedFile, PlatformDir)
                    ErrorCode = IncludedFile1.Validate()[0]
                    if ErrorCode != 0:
                        #
                        # Also search file under the WORKSPACE directory
                        #
                        IncludedFile1 = PathClass(IncludedFile, GlobalData.gWorkspace)
                        ErrorCode = IncludedFile1.Validate()[0]
                        if ErrorCode != 0:
                            raise Warning("The include file does not exist under below directories: \n%s\n%s\n%s\n"%(os.path.dirname(self.FileName), PlatformDir, GlobalData.gWorkspace),
                                          self.FileName, self.CurrentLineNumber)

                if not IsValidInclude (IncludedFile1.Path, self.CurrentLineNumber):
                    raise Warning("The include file {0} is causing a include loop.\n".format (IncludedFile1.Path), self.FileName, self.CurrentLineNumber)

                IncFileProfile = IncludeFileProfile(IncludedFile1.Path)

                CurrentLine = self.CurrentLineNumber
                CurrentOffset = self.CurrentOffsetWithinLine
                # list index of the insertion, note that line number is 'CurrentLine + 1'
                InsertAtLine = CurrentLine
                ParentProfile = GetParentAtLine (CurrentLine)
                if ParentProfile is not None:
                    ParentProfile.IncludeFileList.insert(0, IncFileProfile)
                    IncFileProfile.Level = ParentProfile.Level + 1
                IncFileProfile.InsertStartLineNumber = InsertAtLine + 1
                # deal with remaining portions after "!include filename", if exists.
                if self._GetNextToken():
                    if self.CurrentLineNumber == CurrentLine:
                        RemainingLine = self._CurrentLine()[CurrentOffset:]
                        self.Profile.FileLinesList.insert(self.CurrentLineNumber, RemainingLine)
                        IncFileProfile.InsertAdjust += 1
                        self.CurrentLineNumber += 1
                        self.CurrentOffsetWithinLine = 0

                for Line in IncFileProfile.FileLinesList:
                    self.Profile.FileLinesList.insert(InsertAtLine, Line)
                    self.CurrentLineNumber += 1
                    InsertAtLine += 1

                # reversely sorted to better determine error in file
                AllIncludeFileList.insert(0, IncFileProfile)

                # comment out the processed include file statement
                TempList = list(self.Profile.FileLinesList[IncludeLine - 1])
                TempList.insert(IncludeOffset, TAB_COMMENT_SPLIT)
                self.Profile.FileLinesList[IncludeLine - 1] = ''.join(TempList)
            if Processed: # Nested and back-to-back support
                self.Rewind(DestLine = IncFileProfile.InsertStartLineNumber - 1)
                Processed = False
        # Preprocess done.
        self.Rewind()

    @staticmethod
    def _GetIfListCurrentItemStat(IfList):
        if len(IfList) == 0:
            return True

        for Item in IfList:
            if Item[1] == False:
                return False

        return True

    ## PreprocessConditionalStatement() method
    #
    #   Preprocess conditional statement.
    #   In the end, rewind the file buffer pointer to the beginning
    #
    #   @param  self        The object pointer
    #
    def PreprocessConditionalStatement(self):
        # IfList is a stack of if branches with elements of list [Pos, CondSatisfied, BranchDetermined]
        IfList = []
        RegionLayoutLine = 0
        ReplacedLine = -1
        while self._GetNextToken():
            # Determine section name and the location dependent macro
            if self._GetIfListCurrentItemStat(IfList):
                if self._Token.startswith(TAB_SECTION_START):
                    Header = self._Token
                    if not self._Token.endswith(TAB_SECTION_END):
                        self._SkipToToken(TAB_SECTION_END)
                        Header += self._SkippedChars
                    if Header.find('$(') != -1:
                        raise Warning("macro cannot be used in section header", self.FileName, self.CurrentLineNumber)
                    self._SectionHeaderParser(Header)
                    continue
                # Replace macros except in RULE section or out of section
                elif self._CurSection and ReplacedLine != self.CurrentLineNumber:
                    ReplacedLine = self.CurrentLineNumber
                    self._UndoToken()
                    CurLine = self.Profile.FileLinesList[ReplacedLine - 1]
                    PreIndex = 0
                    StartPos = CurLine.find('$(', PreIndex)
                    EndPos = CurLine.find(')', StartPos+2)
                    while StartPos != -1 and EndPos != -1 and self._Token not in {TAB_IF_DEF, TAB_IF_N_DEF, TAB_IF, TAB_ELSE_IF}:
                        MacroName = CurLine[StartPos+2: EndPos]
                        MacroValue = self._GetMacroValue(MacroName)
                        if MacroValue is not None:
                            CurLine = CurLine.replace('$(' + MacroName + ')', MacroValue, 1)
                            if MacroValue.find('$(') != -1:
                                PreIndex = StartPos
                            else:
                                PreIndex = StartPos + len(MacroValue)
                        else:
                            PreIndex = EndPos + 1
                        StartPos = CurLine.find('$(', PreIndex)
                        EndPos = CurLine.find(')', StartPos+2)
                    self.Profile.FileLinesList[ReplacedLine - 1] = CurLine
                    continue

            if self._Token == TAB_DEFINE:
                if self._GetIfListCurrentItemStat(IfList):
                    if not self._CurSection:
                        raise Warning("macro cannot be defined in Rule section or out of section", self.FileName, self.CurrentLineNumber)
                    DefineLine = self.CurrentLineNumber - 1
                    DefineOffset = self.CurrentOffsetWithinLine - len(TAB_DEFINE)
                    if not self._GetNextToken():
                        raise Warning.Expected("Macro name", self.FileName, self.CurrentLineNumber)
                    Macro = self._Token
                    if not self._IsToken(TAB_EQUAL_SPLIT):
                        raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

                    Value = self._GetExpression()
                    self._SetMacroValue(Macro, Value)
                    self._WipeOffArea.append(((DefineLine, DefineOffset), (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
            elif self._Token == 'SET':
                if not self._GetIfListCurrentItemStat(IfList):
                    continue
                SetLine = self.CurrentLineNumber - 1
                SetOffset = self.CurrentOffsetWithinLine - len('SET')
                PcdPair = self._GetNextPcdSettings()
                PcdName = "%s.%s" % (PcdPair[1], PcdPair[0])
                if not self._IsToken(TAB_EQUAL_SPLIT):
                    raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

                Value = self._GetExpression()
                Value = self._EvaluateConditional(Value, self.CurrentLineNumber, 'eval', True)

                self._PcdDict[PcdName] = Value

                self.Profile.PcdDict[PcdPair] = Value
                self.SetPcdLocalation(PcdPair)
                FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
                self.Profile.PcdFileLineDict[PcdPair] = FileLineTuple

                self._WipeOffArea.append(((SetLine, SetOffset), (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
            elif self._Token in {TAB_IF_DEF, TAB_IF_N_DEF, TAB_IF}:
                IfStartPos = (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - len(self._Token))
                IfList.append([IfStartPos, None, None])

                CondLabel = self._Token
                Expression = self._GetExpression()

                if CondLabel == TAB_IF:
                    ConditionSatisfied = self._EvaluateConditional(Expression, IfList[-1][0][0] + 1, 'eval')
                else:
                    ConditionSatisfied = self._EvaluateConditional(Expression, IfList[-1][0][0] + 1, 'in')
                    if CondLabel == TAB_IF_N_DEF:
                        ConditionSatisfied = not ConditionSatisfied

                BranchDetermined = ConditionSatisfied
                IfList[-1] = [IfList[-1][0], ConditionSatisfied, BranchDetermined]
                if ConditionSatisfied:
                    self._WipeOffArea.append((IfList[-1][0], (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
            elif self._Token in {TAB_ELSE_IF, TAB_ELSE}:
                ElseStartPos = (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - len(self._Token))
                if len(IfList) <= 0:
                    raise Warning("Missing !if statement", self.FileName, self.CurrentLineNumber)

                if IfList[-1][1]:
                    IfList[-1] = [ElseStartPos, False, True]
                    self._WipeOffArea.append((ElseStartPos, (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
                else:
                    self._WipeOffArea.append((IfList[-1][0], ElseStartPos))
                    IfList[-1] = [ElseStartPos, True, IfList[-1][2]]
                    if self._Token == TAB_ELSE_IF:
                        Expression = self._GetExpression()
                        ConditionSatisfied = self._EvaluateConditional(Expression, IfList[-1][0][0] + 1, 'eval')
                        IfList[-1] = [IfList[-1][0], ConditionSatisfied, IfList[-1][2]]

                    if IfList[-1][1]:
                        if IfList[-1][2]:
                            IfList[-1][1] = False
                        else:
                            IfList[-1][2] = True
                            self._WipeOffArea.append((IfList[-1][0], (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
            elif self._Token == '!endif':
                if len(IfList) <= 0:
                    raise Warning("Missing !if statement", self.FileName, self.CurrentLineNumber)
                if IfList[-1][1]:
                    self._WipeOffArea.append(((self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - len('!endif')), (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
                else:
                    self._WipeOffArea.append((IfList[-1][0], (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))

                IfList.pop()
            elif not IfList:    # Don't use PCDs inside conditional directive
                if self.CurrentLineNumber <= RegionLayoutLine:
                    # Don't try the same line twice
                    continue
                SetPcd = ShortcutPcdPattern.match(self.Profile.FileLinesList[self.CurrentLineNumber - 1])
                if SetPcd:
                    self._PcdDict[SetPcd.group('name')] = SetPcd.group('value')
                    RegionLayoutLine = self.CurrentLineNumber
                    continue
                RegionSize = RegionSizePattern.match(self.Profile.FileLinesList[self.CurrentLineNumber - 1])
                if not RegionSize:
                    RegionLayoutLine = self.CurrentLineNumber
                    continue
                RegionSizeGuid = RegionSizeGuidPattern.match(self.Profile.FileLinesList[self.CurrentLineNumber])
                if not RegionSizeGuid:
                    RegionLayoutLine = self.CurrentLineNumber + 1
                    continue
                self._PcdDict[RegionSizeGuid.group('base')] = RegionSize.group('base')
                self._PcdDict[RegionSizeGuid.group('size')] = RegionSize.group('size')
                RegionLayoutLine = self.CurrentLineNumber + 1

        if IfList:
            raise Warning("Missing !endif", self.FileName, self.CurrentLineNumber)
        self.Rewind()

    def _CollectMacroPcd(self):
        MacroDict = {}

        # PCD macro
        MacroDict.update(GlobalData.gPlatformPcds)
        MacroDict.update(self._PcdDict)

        # Lowest priority
        MacroDict.update(GlobalData.gPlatformDefines)

        if self._CurSection:
            # Defines macro
            ScopeMacro = self._MacroDict[TAB_COMMON, TAB_COMMON, TAB_COMMON]
            if ScopeMacro:
                MacroDict.update(ScopeMacro)

            # Section macro
            ScopeMacro = self._MacroDict[
                        self._CurSection[0],
                        self._CurSection[1],
                        self._CurSection[2]
            ]
            if ScopeMacro:
                MacroDict.update(ScopeMacro)

        MacroDict.update(GlobalData.gGlobalDefines)
        MacroDict.update(GlobalData.gCommandLineDefines)
        for Item in GlobalData.BuildOptionPcd:
            if isinstance(Item, tuple):
                continue
            PcdName, TmpValue = Item.split(TAB_EQUAL_SPLIT)
            TmpValue = BuildOptionValue(TmpValue, {})
            MacroDict[PcdName.strip()] = TmpValue
        # Highest priority

        return MacroDict

    def _EvaluateConditional(self, Expression, Line, Op = None, Value = None):
        MacroPcdDict = self._CollectMacroPcd()
        if Op == 'eval':
            try:
                if Value:
                    return ValueExpression(Expression, MacroPcdDict)(True)
                else:
                    return ValueExpression(Expression, MacroPcdDict)()
            except WrnExpression as Excpt:
                #
                # Catch expression evaluation warning here. We need to report
                # the precise number of line and return the evaluation result
                #
                EdkLogger.warn('Parser', "Suspicious expression: %s" % str(Excpt),
                                File=self.FileName, ExtraData=self._CurrentLine(),
                                Line=Line)
                return Excpt.result
            except Exception as Excpt:
                if hasattr(Excpt, 'Pcd'):
                    if Excpt.Pcd in GlobalData.gPlatformOtherPcds:
                        Info = GlobalData.gPlatformOtherPcds[Excpt.Pcd]
                        raise Warning("Cannot use this PCD (%s) in an expression as"
                                      " it must be defined in a [PcdsFixedAtBuild] or [PcdsFeatureFlag] section"
                                      " of the DSC file (%s), and it is currently defined in this section:"
                                      " %s, line #: %d." % (Excpt.Pcd, GlobalData.gPlatformOtherPcds['DSCFILE'], Info[0], Info[1]),
                                      self.FileName, Line)
                    else:
                        raise Warning("PCD (%s) is not defined in DSC file (%s)" % (Excpt.Pcd, GlobalData.gPlatformOtherPcds['DSCFILE']),
                                      self.FileName, Line)
                else:
                    raise Warning(str(Excpt), self.FileName, Line)
        else:
            if Expression.startswith('$(') and Expression[-1] == ')':
                Expression = Expression[2:-1]
            return Expression in MacroPcdDict

    ## _IsToken() method
    #
    #   Check whether input string is found from current char position along
    #   If found, the string value is put into self._Token
    #
    #   @param  self        The object pointer
    #   @param  String      The string to search
    #   @param  IgnoreCase  Indicate case sensitive/non-sensitive search, default is case sensitive
    #   @retval True        Successfully find string, file buffer pointer moved forward
    #   @retval False       Not able to find string, file buffer pointer not changed
    #
    def _IsToken(self, String, IgnoreCase = False):
        self._SkipWhiteSpace()

        # Only consider the same line, no multi-line token allowed
        StartPos = self.CurrentOffsetWithinLine
        index = -1
        if IgnoreCase:
            index = self._CurrentLine()[self.CurrentOffsetWithinLine: ].upper().find(String.upper())
        else:
            index = self._CurrentLine()[self.CurrentOffsetWithinLine: ].find(String)
        if index == 0:
            self.CurrentOffsetWithinLine += len(String)
            self._Token = self._CurrentLine()[StartPos: self.CurrentOffsetWithinLine]
            return True
        return False

    ## _IsKeyword() method
    #
    #   Check whether input keyword is found from current char position along, whole word only!
    #   If found, the string value is put into self._Token
    #
    #   @param  self        The object pointer
    #   @param  Keyword     The string to search
    #   @param  IgnoreCase  Indicate case sensitive/non-sensitive search, default is case sensitive
    #   @retval True        Successfully find string, file buffer pointer moved forward
    #   @retval False       Not able to find string, file buffer pointer not changed
    #
    def _IsKeyword(self, KeyWord, IgnoreCase = False):
        self._SkipWhiteSpace()

        # Only consider the same line, no multi-line token allowed
        StartPos = self.CurrentOffsetWithinLine
        index = -1
        if IgnoreCase:
            index = self._CurrentLine()[self.CurrentOffsetWithinLine: ].upper().find(KeyWord.upper())
        else:
            index = self._CurrentLine()[self.CurrentOffsetWithinLine: ].find(KeyWord)
        if index == 0:
            followingChar = self._CurrentLine()[self.CurrentOffsetWithinLine + len(KeyWord)]
            if not str(followingChar).isspace() and followingChar not in SEPARATORS:
                return False
            self.CurrentOffsetWithinLine += len(KeyWord)
            self._Token = self._CurrentLine()[StartPos: self.CurrentOffsetWithinLine]
            return True
        return False

    def _GetExpression(self):
        Line = self.Profile.FileLinesList[self.CurrentLineNumber - 1]
        Index = len(Line) - 1
        while Line[Index] in CR_LB_SET:
            Index -= 1
        ExpressionString = self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine:Index+1]
        self.CurrentOffsetWithinLine += len(ExpressionString)
        ExpressionString = ExpressionString.strip()
        return ExpressionString

    ## _GetNextWord() method
    #
    #   Get next C name from file lines
    #   If found, the string value is put into self._Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a C name string, file buffer pointer moved forward
    #   @retval False       Not able to find a C name string, file buffer pointer not changed
    #
    def _GetNextWord(self):
        self._SkipWhiteSpace()
        if self._EndOfFile():
            return False

        TempChar = self._CurrentChar()
        StartPos = self.CurrentOffsetWithinLine
        if (TempChar >= 'a' and TempChar <= 'z') or (TempChar >= 'A' and TempChar <= 'Z') or TempChar == '_':
            self._GetOneChar()
            while not self._EndOfLine():
                TempChar = self._CurrentChar()
                if (TempChar >= 'a' and TempChar <= 'z') or (TempChar >= 'A' and TempChar <= 'Z') \
                or (TempChar >= '0' and TempChar <= '9') or TempChar == '_' or TempChar == '-':
                    self._GetOneChar()

                else:
                    break

            self._Token = self._CurrentLine()[StartPos: self.CurrentOffsetWithinLine]
            return True

        return False

    def _GetNextPcdWord(self):
        self._SkipWhiteSpace()
        if self._EndOfFile():
            return False

        TempChar = self._CurrentChar()
        StartPos = self.CurrentOffsetWithinLine
        if (TempChar >= 'a' and TempChar <= 'z') or (TempChar >= 'A' and TempChar <= 'Z') or TempChar == '_' or TempChar == TAB_SECTION_START or TempChar == TAB_SECTION_END:
            self._GetOneChar()
            while not self._EndOfLine():
                TempChar = self._CurrentChar()
                if (TempChar >= 'a' and TempChar <= 'z') or (TempChar >= 'A' and TempChar <= 'Z') \
                or (TempChar >= '0' and TempChar <= '9') or TempChar == '_' or TempChar == '-' or TempChar == TAB_SECTION_START or TempChar == TAB_SECTION_END:
                    self._GetOneChar()

                else:
                    break

            self._Token = self._CurrentLine()[StartPos: self.CurrentOffsetWithinLine]
            return True

        return False

    ## _GetNextToken() method
    #
    #   Get next token unit before a separator
    #   If found, the string value is put into self._Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a token unit, file buffer pointer moved forward
    #   @retval False       Not able to find a token unit, file buffer pointer not changed
    #
    def _GetNextToken(self):
        # Skip leading spaces, if exist.
        self._SkipWhiteSpace()
        if self._EndOfFile():
            return False
        # Record the token start position, the position of the first non-space char.
        StartPos = self.CurrentOffsetWithinLine
        StartLine = self.CurrentLineNumber
        while StartLine == self.CurrentLineNumber:
            TempChar = self._CurrentChar()
            # Try to find the end char that is not a space and not in separator tuple.
            # That is, when we got a space or any char in the tuple, we got the end of token.
            if not str(TempChar).isspace() and TempChar not in SEPARATORS:
                self._GetOneChar()
            # if we happen to meet a separator as the first char, we must proceed to get it.
            # That is, we get a token that is a separator char. normally it is the boundary of other tokens.
            elif StartPos == self.CurrentOffsetWithinLine and TempChar in SEPARATORS:
                self._GetOneChar()
                break
            else:
                break
#        else:
#            return False

        EndPos = self.CurrentOffsetWithinLine
        if self.CurrentLineNumber != StartLine:
            EndPos = len(self.Profile.FileLinesList[StartLine-1])
        self._Token = self.Profile.FileLinesList[StartLine-1][StartPos: EndPos]
        if self._Token.lower() in {TAB_IF, TAB_END_IF, TAB_ELSE_IF, TAB_ELSE, TAB_IF_DEF, TAB_IF_N_DEF, TAB_ERROR, TAB_INCLUDE}:
            self._Token = self._Token.lower()
        if StartPos != self.CurrentOffsetWithinLine:
            return True
        else:
            return False

    ## _GetNextGuid() method
    #
    #   Get next token unit before a separator
    #   If found, the GUID string is put into self._Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a registry format GUID, file buffer pointer moved forward
    #   @retval False       Not able to find a registry format GUID, file buffer pointer not changed
    #
    def _GetNextGuid(self):
        if not self._GetNextToken():
            return False
        if GlobalData.gGuidPattern.match(self._Token) is not None:
            return True
        elif self._Token in GlobalData.gGuidDict:
            return True
        else:
            self._UndoToken()
            return False

    @staticmethod
    def _Verify(Name, Value, Scope):
        # value verification only applies to numeric values.
        if Scope not in TAB_PCD_NUMERIC_TYPES:
            return

        ValueNumber = 0
        try:
            ValueNumber = int(Value, 0)
        except:
            EdkLogger.error("FdfParser", FORMAT_INVALID, "The value is not valid dec or hex number for %s." % Name)
        if ValueNumber < 0:
            EdkLogger.error("FdfParser", FORMAT_INVALID, "The value can't be set to negative value for %s." % Name)
        if ValueNumber > MAX_VAL_TYPE[Scope]:
            EdkLogger.error("FdfParser", FORMAT_INVALID, "Too large value for %s." % Name)
        return True

    ## _UndoToken() method
    #
    #   Go back one token unit in file buffer
    #
    #   @param  self        The object pointer
    #
    def _UndoToken(self):
        self._UndoOneChar()
        while self._CurrentChar().isspace():
            if not self._UndoOneChar():
                self._GetOneChar()
                return


        StartPos = self.CurrentOffsetWithinLine
        CurrentLine = self.CurrentLineNumber
        while CurrentLine == self.CurrentLineNumber:

            TempChar = self._CurrentChar()
            # Try to find the end char that is not a space and not in separator tuple.
            # That is, when we got a space or any char in the tuple, we got the end of token.
            if not str(TempChar).isspace() and not TempChar in SEPARATORS:
                if not self._UndoOneChar():
                    return
            # if we happen to meet a separator as the first char, we must proceed to get it.
            # That is, we get a token that is a separator char. normally it is the boundary of other tokens.
            elif StartPos == self.CurrentOffsetWithinLine and TempChar in SEPARATORS:
                return
            else:
                break

        self._GetOneChar()

    ## _GetNextHexNumber() method
    #
    #   Get next HEX data before a separator
    #   If found, the HEX data is put into self._Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a HEX data, file buffer pointer moved forward
    #   @retval False       Not able to find a HEX data, file buffer pointer not changed
    #
    def _GetNextHexNumber(self):
        if not self._GetNextToken():
            return False
        if GlobalData.gHexPatternAll.match(self._Token):
            return True
        else:
            self._UndoToken()
            return False

    ## _GetNextDecimalNumber() method
    #
    #   Get next decimal data before a separator
    #   If found, the decimal data is put into self._Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a decimal data, file buffer pointer moved forward
    #   @retval False       Not able to find a decimal data, file buffer pointer not changed
    #
    def _GetNextDecimalNumber(self):
        if not self._GetNextToken():
            return False
        if self._Token.isdigit():
            return True
        else:
            self._UndoToken()
            return False

    def _GetNextPcdSettings(self):
        if not self._GetNextWord():
            raise Warning.Expected("<PcdTokenSpaceCName>", self.FileName, self.CurrentLineNumber)
        pcdTokenSpaceCName = self._Token

        if not self._IsToken(TAB_SPLIT):
            raise Warning.Expected(".", self.FileName, self.CurrentLineNumber)

        if not self._GetNextWord():
            raise Warning.Expected("<PcdCName>", self.FileName, self.CurrentLineNumber)
        pcdCName = self._Token

        Fields = []
        while self._IsToken(TAB_SPLIT):
            if not self._GetNextPcdWord():
                raise Warning.Expected("Pcd Fields", self.FileName, self.CurrentLineNumber)
            Fields.append(self._Token)

        return (pcdCName, pcdTokenSpaceCName,TAB_SPLIT.join(Fields))

    ## _GetStringData() method
    #
    #   Get string contents quoted in ""
    #   If found, the decimal data is put into self._Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a string data, file buffer pointer moved forward
    #   @retval False       Not able to find a string data, file buffer pointer not changed
    #
    def _GetStringData(self):
        QuoteToUse = None
        if self._Token.startswith(T_CHAR_DOUBLE_QUOTE) or self._Token.startswith("L\""):
            QuoteToUse = T_CHAR_DOUBLE_QUOTE
        elif self._Token.startswith(T_CHAR_SINGLE_QUOTE) or self._Token.startswith("L\'"):
            QuoteToUse = T_CHAR_SINGLE_QUOTE
        else:
            return False

        self._UndoToken()
        self._SkipToToken(QuoteToUse)
        currentLineNumber = self.CurrentLineNumber

        if not self._SkipToToken(QuoteToUse):
            raise Warning(QuoteToUse, self.FileName, self.CurrentLineNumber)
        if currentLineNumber != self.CurrentLineNumber:
            raise Warning(QuoteToUse, self.FileName, self.CurrentLineNumber)
        self._Token = self._SkippedChars.rstrip(QuoteToUse)
        return True

    ## _SkipToToken() method
    #
    #   Search forward in file buffer for the string
    #   The skipped chars are put into self._SkippedChars
    #
    #   @param  self        The object pointer
    #   @param  String      The string to search
    #   @param  IgnoreCase  Indicate case sensitive/non-sensitive search, default is case sensitive
    #   @retval True        Successfully find the string, file buffer pointer moved forward
    #   @retval False       Not able to find the string, file buffer pointer not changed
    #
    def _SkipToToken(self, String, IgnoreCase = False):
        StartPos = self.GetFileBufferPos()

        self._SkippedChars = ""
        while not self._EndOfFile():
            index = -1
            if IgnoreCase:
                index = self._CurrentLine()[self.CurrentOffsetWithinLine: ].upper().find(String.upper())
            else:
                index = self._CurrentLine()[self.CurrentOffsetWithinLine: ].find(String)
            if index == 0:
                self.CurrentOffsetWithinLine += len(String)
                self._SkippedChars += String
                return True
            self._SkippedChars += str(self._CurrentChar())
            self._GetOneChar()

        self.SetFileBufferPos(StartPos)
        self._SkippedChars = ""
        return False

    ## GetFileBufferPos() method
    #
    #   Return the tuple of current line and offset within the line
    #
    #   @param  self        The object pointer
    #   @retval Tuple       Line number and offset pair
    #
    def GetFileBufferPos(self):
        return (self.CurrentLineNumber, self.CurrentOffsetWithinLine)

    ## SetFileBufferPos() method
    #
    #   Restore the file buffer position
    #
    #   @param  self        The object pointer
    #   @param  Pos         The new file buffer position
    #
    def SetFileBufferPos(self, Pos):
        (self.CurrentLineNumber, self.CurrentOffsetWithinLine) = Pos

    ## Preprocess() method
    #
    #   Preprocess comment, conditional directive, include directive, replace macro.
    #   Exception will be raised if syntax error found
    #
    #   @param  self        The object pointer
    #
    def Preprocess(self):
        self._StringToList()
        self.PreprocessFile()
        self.PreprocessIncludeFile()
        self._StringToList()
        self.PreprocessFile()
        self.PreprocessConditionalStatement()
        self._StringToList()
        for Pos in self._WipeOffArea:
            self._ReplaceFragment(Pos[0], Pos[1])
        self.Profile.FileLinesList = ["".join(list) for list in self.Profile.FileLinesList]

        while self._GetDefines():
            pass

    ## ParseFile() method
    #
    #   Parse the file profile buffer to extract fd, fv ... information
    #   Exception will be raised if syntax error found
    #
    #   @param  self        The object pointer
    #
    def ParseFile(self):
        try:
            self.Preprocess()
            self._GetError()
            #
            # Keep processing sections of the FDF until no new sections or a syntax error is found
            #
            while self._GetFd() or self._GetFv() or self._GetFmp() or self._GetCapsule() or self._GetRule() or self._GetOptionRom():
                pass

        except Warning as X:
            self._UndoToken()
            #'\n\tGot Token: \"%s\" from File %s\n' % (self._Token, FileLineTuple[0]) + \
            # At this point, the closest parent would be the included file itself
            Profile = GetParentAtLine(X.OriginalLineNumber)
            if Profile is not None:
                X.Message += ' near line %d, column %d: %s' \
                % (X.LineNumber, 0, Profile.FileLinesList[X.LineNumber-1])
            else:
                FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
                X.Message += ' near line %d, column %d: %s' \
                % (FileLineTuple[1], self.CurrentOffsetWithinLine + 1, self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine:].rstrip(TAB_LINE_BREAK).rstrip(T_CHAR_CR))
            raise

    ## SectionParser() method
    #
    #   Parse the file section info
    #   Exception will be raised if syntax error found
    #
    #   @param  self          The object pointer
    #   @param  section       The section string

    def SectionParser(self, section):
        S = section.upper()
        if not S.startswith("[DEFINES") and not S.startswith("[FD.") and not S.startswith("[FV.") and not S.startswith("[CAPSULE.") \
             and not S.startswith("[RULE.") and not S.startswith("[OPTIONROM.") and not S.startswith('[FMPPAYLOAD.'):
            raise Warning("Unknown section or section appear sequence error (The correct sequence should be [DEFINES], [FD.], [FV.], [Capsule.], [Rule.], [OptionRom.], [FMPPAYLOAD.])", self.FileName, self.CurrentLineNumber)

    ## _GetDefines() method
    #
    #   Get Defines section contents and store its data into AllMacrosList
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a Defines
    #   @retval False       Not able to find a Defines
    #
    def _GetDefines(self):
        if not self._GetNextToken():
            return False

        S = self._Token.upper()
        if S.startswith(TAB_SECTION_START) and not S.startswith("[DEFINES"):
            self.SectionParser(S)
            self._UndoToken()
            return False

        self._UndoToken()
        if not self._IsToken("[DEFINES", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            #print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
            #        % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine:], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning.Expected("[DEFINES", self.FileName, self.CurrentLineNumber)

        if not self._IsToken(TAB_SECTION_END):
            raise Warning.ExpectedBracketClose(self.FileName, self.CurrentLineNumber)

        while self._GetNextWord():
            # handle the SET statement
            if self._Token == 'SET':
                self._UndoToken()
                self._GetSetStatement(None)
                continue

            Macro = self._Token

            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken() or self._Token.startswith(TAB_SECTION_START):
                raise Warning.Expected("MACRO value", self.FileName, self.CurrentLineNumber)
            Value = self._Token

        return False

    ##_GetError() method
    def _GetError(self):
        #save the Current information
        CurrentLine = self.CurrentLineNumber
        CurrentOffset = self.CurrentOffsetWithinLine
        while self._GetNextToken():
            if self._Token == TAB_ERROR:
                EdkLogger.error('FdfParser', ERROR_STATEMENT, self._CurrentLine().replace(TAB_ERROR, '', 1), File=self.FileName, Line=self.CurrentLineNumber)
        self.CurrentLineNumber = CurrentLine
        self.CurrentOffsetWithinLine = CurrentOffset

    ## _GetFd() method
    #
    #   Get FD section contents and store its data into FD dictionary of self.Profile
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a FD
    #   @retval False       Not able to find a FD
    #
    def _GetFd(self):
        if not self._GetNextToken():
            return False

        S = self._Token.upper()
        if S.startswith(TAB_SECTION_START) and not S.startswith("[FD."):
            if not S.startswith("[FV.") and not S.startswith('[FMPPAYLOAD.') and not S.startswith("[CAPSULE.") \
                and not S.startswith("[RULE.") and not S.startswith("[OPTIONROM."):
                raise Warning("Unknown section", self.FileName, self.CurrentLineNumber)
            self._UndoToken()
            return False

        self._UndoToken()
        if not self._IsToken("[FD.", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            #print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
            #        % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine:], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning.Expected("[FD.]", self.FileName, self.CurrentLineNumber)

        FdName = self._GetUiName()
        if FdName == "":
            if len (self.Profile.FdDict) == 0:
                FdName = GenFdsGlobalVariable.PlatformName
                if FdName == "" and GlobalData.gActivePlatform:
                    FdName = GlobalData.gActivePlatform.PlatformName
                self.Profile.FdNameNotSet = True
            else:
                raise Warning.Expected("FdName in [FD.] section", self.FileName, self.CurrentLineNumber)
        self.CurrentFdName = FdName.upper()

        if self.CurrentFdName in self.Profile.FdDict:
            raise Warning("Unexpected the same FD name", self.FileName, self.CurrentLineNumber)

        if not self._IsToken(TAB_SECTION_END):
            raise Warning.ExpectedBracketClose(self.FileName, self.CurrentLineNumber)

        FdObj = FD()
        FdObj.FdUiName = self.CurrentFdName
        self.Profile.FdDict[self.CurrentFdName] = FdObj

        if len (self.Profile.FdDict) > 1 and self.Profile.FdNameNotSet:
            raise Warning.Expected("all FDs have their name", self.FileName, self.CurrentLineNumber)

        Status = self._GetCreateFile(FdObj)
        if not Status:
            raise Warning("FD name error", self.FileName, self.CurrentLineNumber)

        while self._GetTokenStatements(FdObj):
            pass
        for Attr in ("BaseAddress", "Size", "ErasePolarity"):
            if getattr(FdObj, Attr) is None:
                self._GetNextToken()
                raise Warning("Keyword %s missing" % Attr, self.FileName, self.CurrentLineNumber)

        if not FdObj.BlockSizeList:
            FdObj.BlockSizeList.append((1, FdObj.Size, None))

        self._GetDefineStatements(FdObj)

        self._GetSetStatements(FdObj)

        if not self._GetRegionLayout(FdObj):
            raise Warning.Expected("region layout", self.FileName, self.CurrentLineNumber)

        while self._GetRegionLayout(FdObj):
            pass
        return True

    ## _GetUiName() method
    #
    #   Return the UI name of a section
    #
    #   @param  self        The object pointer
    #   @retval FdName      UI name
    #
    def _GetUiName(self):
        Name = ""
        if self._GetNextWord():
            Name = self._Token

        return Name

    ## _GetCreateFile() method
    #
    #   Return the output file name of object
    #
    #   @param  self        The object pointer
    #   @param  Obj         object whose data will be stored in file
    #   @retval FdName      UI name
    #
    def _GetCreateFile(self, Obj):
        if self._IsKeyword("CREATE_FILE"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("file name", self.FileName, self.CurrentLineNumber)

            FileName = self._Token
            Obj.CreateFileName = FileName

        return True

    def SetPcdLocalation(self,pcdpair):
        self.Profile.PcdLocalDict[pcdpair] = (self.Profile.FileName,self.CurrentLineNumber)

    ## _GetTokenStatements() method
    #
    #   Get token statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom token statement is got
    #
    def _GetTokenStatements(self, Obj):
        if self._IsKeyword("BaseAddress"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextHexNumber():
                raise Warning.Expected("Hex base address", self.FileName, self.CurrentLineNumber)

            Obj.BaseAddress = self._Token

            if self._IsToken(TAB_VALUE_SPLIT):
                pcdPair = self._GetNextPcdSettings()
                Obj.BaseAddressPcd = pcdPair
                self.Profile.PcdDict[pcdPair] = Obj.BaseAddress
                self.SetPcdLocalation(pcdPair)
                FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
                self.Profile.PcdFileLineDict[pcdPair] = FileLineTuple
            return True

        if self._IsKeyword("Size"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextHexNumber():
                raise Warning.Expected("Hex size", self.FileName, self.CurrentLineNumber)

            Size = self._Token
            if self._IsToken(TAB_VALUE_SPLIT):
                pcdPair = self._GetNextPcdSettings()
                Obj.SizePcd = pcdPair
                self.Profile.PcdDict[pcdPair] = Size
                self.SetPcdLocalation(pcdPair)
                FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
                self.Profile.PcdFileLineDict[pcdPair] = FileLineTuple
            Obj.Size = int(Size, 0)
            return True

        if self._IsKeyword("ErasePolarity"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("Erase Polarity", self.FileName, self.CurrentLineNumber)

            if not self._Token in {"1", "0"}:
                raise Warning.Expected("1 or 0 Erase Polarity", self.FileName, self.CurrentLineNumber)

            Obj.ErasePolarity = self._Token
            return True

        return self._GetBlockStatements(Obj)

    ## _GetAddressStatements() method
    #
    #   Get address statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom address statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def _GetAddressStatements(self, Obj):
        if self._IsKeyword("BsBaseAddress"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextDecimalNumber() and not self._GetNextHexNumber():
                raise Warning.Expected("address", self.FileName, self.CurrentLineNumber)

            BsAddress = int(self._Token, 0)
            Obj.BsBaseAddress = BsAddress

        if self._IsKeyword("RtBaseAddress"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextDecimalNumber() and not self._GetNextHexNumber():
                raise Warning.Expected("address", self.FileName, self.CurrentLineNumber)

            RtAddress = int(self._Token, 0)
            Obj.RtBaseAddress = RtAddress

    ## _GetBlockStatements() method
    #
    #   Get block statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom block statement is got
    #
    def _GetBlockStatements(self, Obj):
        IsBlock = False
        while self._GetBlockStatement(Obj):
            IsBlock = True

            Item = Obj.BlockSizeList[-1]
            if Item[0] is None or Item[1] is None:
                raise Warning.Expected("block statement", self.FileName, self.CurrentLineNumber)
        return IsBlock

    ## _GetBlockStatement() method
    #
    #   Get block statement
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom block statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def _GetBlockStatement(self, Obj):
        if not self._IsKeyword("BlockSize"):
            return False

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextHexNumber() and not self._GetNextDecimalNumber():
            raise Warning.Expected("Hex or Integer block size", self.FileName, self.CurrentLineNumber)

        BlockSize = self._Token
        BlockSizePcd = None
        if self._IsToken(TAB_VALUE_SPLIT):
            PcdPair = self._GetNextPcdSettings()
            BlockSizePcd = PcdPair
            self.Profile.PcdDict[PcdPair] = BlockSize
            self.SetPcdLocalation(PcdPair)
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            self.Profile.PcdFileLineDict[PcdPair] = FileLineTuple
        BlockSize = int(BlockSize, 0)

        BlockNumber = None
        if self._IsKeyword("NumBlocks"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextDecimalNumber() and not self._GetNextHexNumber():
                raise Warning.Expected("block numbers", self.FileName, self.CurrentLineNumber)

            BlockNumber = int(self._Token, 0)

        Obj.BlockSizeList.append((BlockSize, BlockNumber, BlockSizePcd))
        return True

    ## _GetDefineStatements() method
    #
    #   Get define statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom define statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def _GetDefineStatements(self, Obj):
        while self._GetDefineStatement(Obj):
            pass

    ## _GetDefineStatement() method
    #
    #   Get define statement
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom define statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def _GetDefineStatement(self, Obj):
        if self._IsKeyword(TAB_DEFINE):
            self._GetNextToken()
            Macro = self._Token
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("value", self.FileName, self.CurrentLineNumber)

            Value = self._Token
            Macro = '$(' + Macro + ')'
            Obj.DefineVarDict[Macro] = Value
            return True

        return False

    ## _GetSetStatements() method
    #
    #   Get set statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom set statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def _GetSetStatements(self, Obj):
        while self._GetSetStatement(Obj):
            pass

    ## _GetSetStatement() method
    #
    #   Get set statement
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom set statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def _GetSetStatement(self, Obj):
        if self._IsKeyword("SET"):
            PcdPair = self._GetNextPcdSettings()

            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            Value = self._GetExpression()
            Value = self._EvaluateConditional(Value, self.CurrentLineNumber, 'eval', True)

            if Obj:
                Obj.SetVarDict[PcdPair] = Value
            self.Profile.PcdDict[PcdPair] = Value
            self.SetPcdLocalation(PcdPair)
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            self.Profile.PcdFileLineDict[PcdPair] = FileLineTuple
            return True

        return False

    ## _CalcRegionExpr(self)
    #
    #   Calculate expression for offset or size of a region
    #
    #   @return: None if invalid expression
    #            Calculated number if successfully
    #
    def _CalcRegionExpr(self):
        StartPos = self.GetFileBufferPos()
        Expr = ''
        PairCount = 0
        while not self._EndOfFile():
            CurCh = self._CurrentChar()
            if CurCh == '(':
                PairCount += 1
            elif CurCh == ')':
                PairCount -= 1

            if CurCh in '|\r\n' and PairCount == 0:
                break
            Expr += CurCh
            self._GetOneChar()
        try:
            return int(
                ValueExpression(Expr,
                                self._CollectMacroPcd()
                                )(True), 0)
        except Exception:
            self.SetFileBufferPos(StartPos)
            return None

    ## _GetRegionLayout() method
    #
    #   Get region layout for FD
    #
    #   @param  self        The object pointer
    #   @param  theFd       for whom region is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def _GetRegionLayout(self, theFd):
        Offset = self._CalcRegionExpr()
        if Offset is None:
            return False

        RegionObj = Region()
        RegionObj.Offset = Offset
        theFd.RegionList.append(RegionObj)

        if not self._IsToken(TAB_VALUE_SPLIT):
            raise Warning.Expected("'|'", self.FileName, self.CurrentLineNumber)

        Size = self._CalcRegionExpr()
        if Size is None:
            raise Warning.Expected("Region Size", self.FileName, self.CurrentLineNumber)
        RegionObj.Size = Size

        if not self._GetNextWord():
            return True

        if not self._Token in {"SET", BINARY_FILE_TYPE_FV, "FILE", "DATA", "CAPSULE", "INF"}:
            #
            # If next token is a word which is not a valid FV type, it might be part of [PcdOffset[|PcdSize]]
            # Or it might be next region's offset described by an expression which starts with a PCD.
            #    PcdOffset[|PcdSize] or OffsetPcdExpression|Size
            #
            self._UndoToken()
            IsRegionPcd = (RegionSizeGuidPattern.match(self._CurrentLine()[self.CurrentOffsetWithinLine:]) or
                           RegionOffsetPcdPattern.match(self._CurrentLine()[self.CurrentOffsetWithinLine:]))
            if IsRegionPcd:
                RegionObj.PcdOffset = self._GetNextPcdSettings()
                self.Profile.PcdDict[RegionObj.PcdOffset] = "0x%08X" % (RegionObj.Offset + int(theFd.BaseAddress, 0))
                self.SetPcdLocalation(RegionObj.PcdOffset)
                self._PcdDict['%s.%s' % (RegionObj.PcdOffset[1], RegionObj.PcdOffset[0])] = "0x%x" % RegionObj.Offset
                FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
                self.Profile.PcdFileLineDict[RegionObj.PcdOffset] = FileLineTuple
                if self._IsToken(TAB_VALUE_SPLIT):
                    RegionObj.PcdSize = self._GetNextPcdSettings()
                    self.Profile.PcdDict[RegionObj.PcdSize] = "0x%08X" % RegionObj.Size
                    self.SetPcdLocalation(RegionObj.PcdSize)
                    self._PcdDict['%s.%s' % (RegionObj.PcdSize[1], RegionObj.PcdSize[0])] = "0x%x" % RegionObj.Size
                    FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
                    self.Profile.PcdFileLineDict[RegionObj.PcdSize] = FileLineTuple

            if not self._GetNextWord():
                return True

        if self._Token == "SET":
            self._UndoToken()
            self._GetSetStatements(RegionObj)
            if not self._GetNextWord():
                return True

        elif self._Token == BINARY_FILE_TYPE_FV:
            self._UndoToken()
            self._GetRegionFvType(RegionObj)

        elif self._Token == "CAPSULE":
            self._UndoToken()
            self._GetRegionCapType(RegionObj)

        elif self._Token == "FILE":
            self._UndoToken()
            self._GetRegionFileType(RegionObj)

        elif self._Token == "INF":
            self._UndoToken()
            RegionObj.RegionType = "INF"
            while self._IsKeyword("INF"):
                self._UndoToken()
                ffsInf = self._ParseInfStatement()
                if not ffsInf:
                    break
                RegionObj.RegionDataList.append(ffsInf)

        elif self._Token == "DATA":
            self._UndoToken()
            self._GetRegionDataType(RegionObj)
        else:
            self._UndoToken()
            if self._GetRegionLayout(theFd):
                return True
            raise Warning("A valid region type was not found. "
                          "Valid types are [SET, FV, CAPSULE, FILE, DATA, INF]. This error occurred",
                          self.FileName, self.CurrentLineNumber)

        return True

    ## _GetRegionFvType() method
    #
    #   Get region fv data for region
    #
    #   @param  self        The object pointer
    #   @param  RegionObj   for whom region data is got
    #
    def _GetRegionFvType(self, RegionObj):
        if not self._IsKeyword(BINARY_FILE_TYPE_FV):
            raise Warning.Expected("'FV'", self.FileName, self.CurrentLineNumber)

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("FV name", self.FileName, self.CurrentLineNumber)

        RegionObj.RegionType = BINARY_FILE_TYPE_FV
        RegionObj.RegionDataList.append((self._Token).upper())

        while self._IsKeyword(BINARY_FILE_TYPE_FV):

            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("FV name", self.FileName, self.CurrentLineNumber)

            RegionObj.RegionDataList.append((self._Token).upper())

    ## _GetRegionCapType() method
    #
    #   Get region capsule data for region
    #
    #   @param  self        The object pointer
    #   @param  RegionObj   for whom region data is got
    #
    def _GetRegionCapType(self, RegionObj):
        if not self._IsKeyword("CAPSULE"):
            raise Warning.Expected("'CAPSULE'", self.FileName, self.CurrentLineNumber)

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("CAPSULE name", self.FileName, self.CurrentLineNumber)

        RegionObj.RegionType = "CAPSULE"
        RegionObj.RegionDataList.append(self._Token)

        while self._IsKeyword("CAPSULE"):

            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("CAPSULE name", self.FileName, self.CurrentLineNumber)

            RegionObj.RegionDataList.append(self._Token)

    ## _GetRegionFileType() method
    #
    #   Get region file data for region
    #
    #   @param  self        The object pointer
    #   @param  RegionObj   for whom region data is got
    #
    def _GetRegionFileType(self, RegionObj):
        if not self._IsKeyword("FILE"):
            raise Warning.Expected("'FILE'", self.FileName, self.CurrentLineNumber)

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("File name", self.FileName, self.CurrentLineNumber)

        RegionObj.RegionType = "FILE"
        RegionObj.RegionDataList.append(self._Token)

        while self._IsKeyword("FILE"):

            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("FILE name", self.FileName, self.CurrentLineNumber)

            RegionObj.RegionDataList.append(self._Token)

    ## _GetRegionDataType() method
    #
    #   Get region array data for region
    #
    #   @param  self        The object pointer
    #   @param  RegionObj   for whom region data is got
    #
    def _GetRegionDataType(self, RegionObj):
        if not self._IsKeyword("DATA"):
            raise Warning.Expected("Region Data type", self.FileName, self.CurrentLineNumber)

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._IsToken("{"):
            raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)

        if not self._GetNextHexNumber():
            raise Warning.Expected("Hex byte", self.FileName, self.CurrentLineNumber)

        if len(self._Token) > 18:
            raise Warning("Hex string can't be converted to a valid UINT64 value", self.FileName, self.CurrentLineNumber)

        # convert hex string value to byte hex string array
        AllString = self._Token
        AllStrLen = len (AllString)
        DataString = ""
        while AllStrLen > 4:
            DataString = DataString + "0x" + AllString[AllStrLen - 2: AllStrLen] + TAB_COMMA_SPLIT
            AllStrLen  = AllStrLen - 2
        DataString = DataString + AllString[:AllStrLen] + TAB_COMMA_SPLIT

        # byte value array
        if len (self._Token) <= 4:
            while self._IsToken(TAB_COMMA_SPLIT):
                if not self._GetNextHexNumber():
                    raise Warning("Invalid Hex number", self.FileName, self.CurrentLineNumber)
                if len(self._Token) > 4:
                    raise Warning("Hex byte(must be 2 digits) too long", self.FileName, self.CurrentLineNumber)
                DataString += self._Token
                DataString += TAB_COMMA_SPLIT

        if not self._IsToken(T_CHAR_BRACE_R):
            raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)

        DataString = DataString.rstrip(TAB_COMMA_SPLIT)
        RegionObj.RegionType = "DATA"
        RegionObj.RegionDataList.append(DataString)

        while self._IsKeyword("DATA"):

            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._IsToken("{"):
                raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)

            if not self._GetNextHexNumber():
                raise Warning.Expected("Hex byte", self.FileName, self.CurrentLineNumber)

            if len(self._Token) > 18:
                raise Warning("Hex string can't be converted to a valid UINT64 value", self.FileName, self.CurrentLineNumber)

            # convert hex string value to byte hex string array
            AllString = self._Token
            AllStrLen = len (AllString)
            DataString = ""
            while AllStrLen > 4:
                DataString = DataString + "0x" + AllString[AllStrLen - 2: AllStrLen] + TAB_COMMA_SPLIT
                AllStrLen  = AllStrLen - 2
            DataString = DataString + AllString[:AllStrLen] + TAB_COMMA_SPLIT

            # byte value array
            if len (self._Token) <= 4:
                while self._IsToken(TAB_COMMA_SPLIT):
                    if not self._GetNextHexNumber():
                        raise Warning("Invalid Hex number", self.FileName, self.CurrentLineNumber)
                    if len(self._Token) > 4:
                        raise Warning("Hex byte(must be 2 digits) too long", self.FileName, self.CurrentLineNumber)
                    DataString += self._Token
                    DataString += TAB_COMMA_SPLIT

            if not self._IsToken(T_CHAR_BRACE_R):
                raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)

            DataString = DataString.rstrip(TAB_COMMA_SPLIT)
            RegionObj.RegionDataList.append(DataString)

    ## _GetFv() method
    #
    #   Get FV section contents and store its data into FV dictionary of self.Profile
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a FV
    #   @retval False       Not able to find a FV
    #
    def _GetFv(self):
        if not self._GetNextToken():
            return False

        S = self._Token.upper()
        if S.startswith(TAB_SECTION_START) and not S.startswith("[FV."):
            self.SectionParser(S)
            self._UndoToken()
            return False

        self._UndoToken()
        if not self._IsToken("[FV.", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            #print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
            #        % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine:], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning("Unknown Keyword '%s'" % self._Token, self.FileName, self.CurrentLineNumber)

        FvName = self._GetUiName()
        self.CurrentFvName = FvName.upper()

        if not self._IsToken(TAB_SECTION_END):
            raise Warning.ExpectedBracketClose(self.FileName, self.CurrentLineNumber)

        FvObj = FV(Name=self.CurrentFvName)
        self.Profile.FvDict[self.CurrentFvName] = FvObj

        Status = self._GetCreateFile(FvObj)
        if not Status:
            raise Warning("FV name error", self.FileName, self.CurrentLineNumber)

        self._GetDefineStatements(FvObj)

        self._GetAddressStatements(FvObj)

        while True:
            self._GetSetStatements(FvObj)

            if not (self._GetBlockStatement(FvObj) or self._GetFvBaseAddress(FvObj) or
                self._GetFvForceRebase(FvObj) or self._GetFvAlignment(FvObj) or
                self._GetFvAttributes(FvObj) or self._GetFvNameGuid(FvObj) or
                self._GetFvExtEntryStatement(FvObj) or self._GetFvNameString(FvObj)):
                break

        if FvObj.FvNameString == 'TRUE' and not FvObj.FvNameGuid:
            raise Warning("FvNameString found but FvNameGuid was not found", self.FileName, self.CurrentLineNumber)

        self._GetAprioriSection(FvObj)
        self._GetAprioriSection(FvObj)

        while True:
            isInf = self._GetInfStatement(FvObj)
            isFile = self._GetFileStatement(FvObj)
            if not isInf and not isFile:
                break

        return True

    ## _GetFvAlignment() method
    #
    #   Get alignment for FV
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom alignment is got
    #   @retval True        Successfully find a alignment statement
    #   @retval False       Not able to find a alignment statement
    #
    def _GetFvAlignment(self, Obj):
        if not self._IsKeyword("FvAlignment"):
            return False

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("alignment value", self.FileName, self.CurrentLineNumber)

        if self._Token.upper() not in {"1", "2", "4", "8", "16", "32", "64", "128", "256", "512", \
                                        "1K", "2K", "4K", "8K", "16K", "32K", "64K", "128K", "256K", "512K", \
                                        "1M", "2M", "4M", "8M", "16M", "32M", "64M", "128M", "256M", "512M", \
                                        "1G", "2G"}:
            raise Warning("Unknown alignment value '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
        Obj.FvAlignment = self._Token
        return True

    ## _GetFvBaseAddress() method
    #
    #   Get BaseAddress for FV
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom FvBaseAddress is got
    #   @retval True        Successfully find a FvBaseAddress statement
    #   @retval False       Not able to find a FvBaseAddress statement
    #
    def _GetFvBaseAddress(self, Obj):
        if not self._IsKeyword("FvBaseAddress"):
            return False

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("FV base address value", self.FileName, self.CurrentLineNumber)

        if not BaseAddrValuePattern.match(self._Token.upper()):
            raise Warning("Unknown FV base address value '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
        Obj.FvBaseAddress = self._Token
        return True

    ## _GetFvForceRebase() method
    #
    #   Get FvForceRebase for FV
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom FvForceRebase is got
    #   @retval True        Successfully find a FvForceRebase statement
    #   @retval False       Not able to find a FvForceRebase statement
    #
    def _GetFvForceRebase(self, Obj):
        if not self._IsKeyword("FvForceRebase"):
            return False

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("FvForceRebase value", self.FileName, self.CurrentLineNumber)

        if self._Token.upper() not in {"TRUE", "FALSE", "0", "0X0", "0X00", "1", "0X1", "0X01"}:
            raise Warning("Unknown FvForceRebase value '%s'" % self._Token, self.FileName, self.CurrentLineNumber)

        if self._Token.upper() in {"TRUE", "1", "0X1", "0X01"}:
            Obj.FvForceRebase = True
        elif self._Token.upper() in {"FALSE", "0", "0X0", "0X00"}:
            Obj.FvForceRebase = False
        else:
            Obj.FvForceRebase = None

        return True


    ## _GetFvAttributes() method
    #
    #   Get attributes for FV
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom attribute is got
    #   @retval None
    #
    def _GetFvAttributes(self, FvObj):
        IsWordToken = False
        while self._GetNextWord():
            IsWordToken = True
            name = self._Token
            if name not in {"ERASE_POLARITY", "MEMORY_MAPPED", \
                           "STICKY_WRITE", "LOCK_CAP", "LOCK_STATUS", "WRITE_ENABLED_CAP", \
                           "WRITE_DISABLED_CAP", "WRITE_STATUS", "READ_ENABLED_CAP", \
                           "READ_DISABLED_CAP", "READ_STATUS", "READ_LOCK_CAP", \
                           "READ_LOCK_STATUS", "WRITE_LOCK_CAP", "WRITE_LOCK_STATUS", \
                           "WRITE_POLICY_RELIABLE", "WEAK_ALIGNMENT", "FvUsedSizeEnable"}:
                self._UndoToken()
                return False

            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken() or self._Token.upper() not in {"TRUE", "FALSE", "1", "0"}:
                raise Warning.Expected("TRUE/FALSE (1/0)", self.FileName, self.CurrentLineNumber)

            FvObj.FvAttributeDict[name] = self._Token

        return IsWordToken

    ## _GetFvNameGuid() method
    #
    #   Get FV GUID for FV
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom GUID is got
    #   @retval None
    #
    def _GetFvNameGuid(self, FvObj):
        if not self._IsKeyword("FvNameGuid"):
            return False

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextGuid():
            raise Warning.Expected("GUID value", self.FileName, self.CurrentLineNumber)
        if self._Token in GlobalData.gGuidDict:
            self._Token = GuidStructureStringToGuidString(GlobalData.gGuidDict[self._Token]).upper()

        FvObj.FvNameGuid = self._Token

        return True

    def _GetFvNameString(self, FvObj):
        if not self._IsKeyword("FvNameString"):
            return False

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken() or self._Token.upper() not in {'TRUE', 'FALSE'}:
            raise Warning.Expected("TRUE or FALSE for FvNameString", self.FileName, self.CurrentLineNumber)

        FvObj.FvNameString = self._Token

        return True

    def _GetFvExtEntryStatement(self, FvObj):
        if not (self._IsKeyword("FV_EXT_ENTRY") or self._IsKeyword("FV_EXT_ENTRY_TYPE")):
            return False

        if not self._IsKeyword ("TYPE"):
            raise Warning.Expected("'TYPE'", self.FileName, self.CurrentLineNumber)

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextHexNumber() and not self._GetNextDecimalNumber():
            raise Warning.Expected("Hex FV extension entry type value At Line ", self.FileName, self.CurrentLineNumber)

        FvObj.FvExtEntryTypeValue.append(self._Token)

        if not self._IsToken("{"):
            raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)

        if not self._IsKeyword("FILE") and not self._IsKeyword("DATA"):
            raise Warning.Expected("'FILE' or 'DATA'", self.FileName, self.CurrentLineNumber)

        FvObj.FvExtEntryType.append(self._Token)

        if self._Token == 'DATA':
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._IsToken("{"):
                raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)

            if not self._GetNextHexNumber():
                raise Warning.Expected("Hex byte", self.FileName, self.CurrentLineNumber)

            if len(self._Token) > 4:
                raise Warning("Hex byte(must be 2 digits) too long", self.FileName, self.CurrentLineNumber)

            DataString = self._Token
            DataString += TAB_COMMA_SPLIT

            while self._IsToken(TAB_COMMA_SPLIT):
                if not self._GetNextHexNumber():
                    raise Warning("Invalid Hex number", self.FileName, self.CurrentLineNumber)
                if len(self._Token) > 4:
                    raise Warning("Hex byte(must be 2 digits) too long", self.FileName, self.CurrentLineNumber)
                DataString += self._Token
                DataString += TAB_COMMA_SPLIT

            if not self._IsToken(T_CHAR_BRACE_R):
                raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)

            if not self._IsToken(T_CHAR_BRACE_R):
                raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)

            DataString = DataString.rstrip(TAB_COMMA_SPLIT)
            FvObj.FvExtEntryData.append(DataString)

        if self._Token == 'FILE':
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("FV Extension Entry file path At Line ", self.FileName, self.CurrentLineNumber)

            FvObj.FvExtEntryData.append(self._Token)

            if not self._IsToken(T_CHAR_BRACE_R):
                raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)

        return True

    ## _GetAprioriSection() method
    #
    #   Get token statements
    #
    #   @param  self        The object pointer
    #   @param  FvObj       for whom apriori is got
    #   @retval True        Successfully find apriori statement
    #   @retval False       Not able to find apriori statement
    #
    def _GetAprioriSection(self, FvObj):
        if not self._IsKeyword("APRIORI"):
            return False

        if not self._IsKeyword("PEI") and not self._IsKeyword("DXE"):
            raise Warning.Expected("Apriori file type", self.FileName, self.CurrentLineNumber)
        AprType = self._Token

        if not self._IsToken("{"):
            raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)

        AprSectionObj = AprioriSection()
        AprSectionObj.AprioriType = AprType

        self._GetDefineStatements(AprSectionObj)

        while True:
            IsInf = self._GetInfStatement(AprSectionObj)
            IsFile = self._GetFileStatement(AprSectionObj)
            if not IsInf and not IsFile:
                break

        if not self._IsToken(T_CHAR_BRACE_R):
            raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)

        FvObj.AprioriSectionList.append(AprSectionObj)
        return True

    def _ParseInfStatement(self):
        if not self._IsKeyword("INF"):
            return None

        ffsInf = FfsInfStatement()
        self._GetInfOptions(ffsInf)

        if not self._GetNextToken():
            raise Warning.Expected("INF file path", self.FileName, self.CurrentLineNumber)
        ffsInf.InfFileName = self._Token
        if not ffsInf.InfFileName.endswith('.inf'):
            raise Warning.Expected(".inf file path", self.FileName, self.CurrentLineNumber)

        ffsInf.CurrentLineNum = self.CurrentLineNumber
        ffsInf.CurrentLineContent = self._CurrentLine()

        #Replace $(SAPCE) with real space
        ffsInf.InfFileName = ffsInf.InfFileName.replace('$(SPACE)', ' ')

        if ffsInf.InfFileName.replace(TAB_WORKSPACE, '').find('$') == -1:
            #do case sensitive check for file path
            ErrorCode, ErrorInfo = PathClass(NormPath(ffsInf.InfFileName), GenFdsGlobalVariable.WorkSpaceDir).Validate()
            if ErrorCode != 0:
                EdkLogger.error("GenFds", ErrorCode, ExtraData=ErrorInfo)

        NewFileName = ffsInf.InfFileName
        if ffsInf.OverrideGuid:
            NewFileName = ProcessDuplicatedInf(PathClass(ffsInf.InfFileName,GenFdsGlobalVariable.WorkSpaceDir), ffsInf.OverrideGuid, GenFdsGlobalVariable.WorkSpaceDir).Path

        if not NewFileName in self.Profile.InfList:
            self.Profile.InfList.append(NewFileName)
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            self.Profile.InfFileLineList.append(FileLineTuple)
            if ffsInf.UseArch:
                if ffsInf.UseArch not in self.Profile.InfDict:
                    self.Profile.InfDict[ffsInf.UseArch] = [ffsInf.InfFileName]
                else:
                    self.Profile.InfDict[ffsInf.UseArch].append(ffsInf.InfFileName)
            else:
                self.Profile.InfDict['ArchTBD'].append(ffsInf.InfFileName)

        if self._IsToken(TAB_VALUE_SPLIT):
            if self._IsKeyword('RELOCS_STRIPPED'):
                ffsInf.KeepReloc = False
            elif self._IsKeyword('RELOCS_RETAINED'):
                ffsInf.KeepReloc = True
            else:
                raise Warning("Unknown reloc strip flag '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
        return ffsInf

    ## _GetInfStatement() method
    #
    #   Get INF statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom inf statement is got
    #   @retval True        Successfully find inf statement
    #   @retval False       Not able to find inf statement
    #
    def _GetInfStatement(self, Obj, ForCapsule=False):
        ffsInf = self._ParseInfStatement()
        if not ffsInf:
            return False

        if ForCapsule:
            myCapsuleFfs = CapsuleFfs()
            myCapsuleFfs.Ffs = ffsInf
            Obj.CapsuleDataList.append(myCapsuleFfs)
        else:
            Obj.FfsList.append(ffsInf)
        return True

    ## _GetInfOptions() method
    #
    #   Get options for INF
    #
    #   @param  self        The object pointer
    #   @param  FfsInfObj   for whom option is got
    #
    def _GetInfOptions(self, FfsInfObj):
        if self._IsKeyword("FILE_GUID"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextGuid():
                raise Warning.Expected("GUID value", self.FileName, self.CurrentLineNumber)
            if self._Token in GlobalData.gGuidDict:
                self._Token = GuidStructureStringToGuidString(GlobalData.gGuidDict[self._Token]).upper()
            FfsInfObj.OverrideGuid = self._Token

        if self._IsKeyword("RuleOverride"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken():
                raise Warning.Expected("Rule name", self.FileName, self.CurrentLineNumber)
            FfsInfObj.Rule = self._Token

        if self._IsKeyword("VERSION"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken():
                raise Warning.Expected("Version", self.FileName, self.CurrentLineNumber)

            if self._GetStringData():
                FfsInfObj.Version = self._Token

        if self._IsKeyword(BINARY_FILE_TYPE_UI):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken():
                raise Warning.Expected("UI name", self.FileName, self.CurrentLineNumber)

            if self._GetStringData():
                FfsInfObj.Ui = self._Token

        if self._IsKeyword("USE"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken():
                raise Warning.Expected("ARCH name", self.FileName, self.CurrentLineNumber)
            FfsInfObj.UseArch = self._Token


        if self._GetNextToken():
            p = compile(r'([a-zA-Z0-9\-]+|\$\(TARGET\)|\*)_([a-zA-Z0-9\-]+|\$\(TOOL_CHAIN_TAG\)|\*)_([a-zA-Z0-9\-]+|\$\(ARCH\))')
            if p.match(self._Token) and p.match(self._Token).span()[1] == len(self._Token):
                FfsInfObj.KeyStringList.append(self._Token)
                if not self._IsToken(TAB_COMMA_SPLIT):
                    return
            else:
                self._UndoToken()
                return

            while self._GetNextToken():
                if not p.match(self._Token):
                    raise Warning.Expected("KeyString \"Target_Tag_Arch\"", self.FileName, self.CurrentLineNumber)
                FfsInfObj.KeyStringList.append(self._Token)

                if not self._IsToken(TAB_COMMA_SPLIT):
                    break

    ## _GetFileStatement() method
    #
    #   Get FILE statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom FILE statement is got
    #   @retval True        Successfully find FILE statement
    #   @retval False       Not able to find FILE statement
    #
    def _GetFileStatement(self, Obj, ForCapsule = False):
        if not self._IsKeyword("FILE"):
            return False

        if not self._GetNextWord():
            raise Warning.Expected("FFS type", self.FileName, self.CurrentLineNumber)

        if ForCapsule and self._Token == 'DATA':
            self._UndoToken()
            self._UndoToken()
            return False

        FfsFileObj = FileStatement()
        FfsFileObj.FvFileType = self._Token

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextGuid():
            if not self._GetNextWord():
                raise Warning.Expected("File GUID", self.FileName, self.CurrentLineNumber)
            if self._Token == 'PCD':
                if not self._IsToken("("):
                    raise Warning.Expected("'('", self.FileName, self.CurrentLineNumber)
                PcdPair = self._GetNextPcdSettings()
                if not self._IsToken(")"):
                    raise Warning.Expected("')'", self.FileName, self.CurrentLineNumber)
                self._Token = 'PCD('+PcdPair[1]+TAB_SPLIT+PcdPair[0]+')'

        if self._Token in GlobalData.gGuidDict:
            self._Token = GuidStructureStringToGuidString(GlobalData.gGuidDict[self._Token]).upper()
        FfsFileObj.NameGuid = self._Token

        self._GetFilePart(FfsFileObj)

        if ForCapsule:
            capsuleFfs = CapsuleFfs()
            capsuleFfs.Ffs = FfsFileObj
            Obj.CapsuleDataList.append(capsuleFfs)
        else:
            Obj.FfsList.append(FfsFileObj)

        return True

    ## _FileCouldHaveRelocFlag() method
    #
    #   Check whether reloc strip flag can be set for a file type.
    #
    #   @param  FileType    The file type to check with
    #   @retval True        This type could have relocation strip flag
    #   @retval False       No way to have it
    #
    @staticmethod
    def _FileCouldHaveRelocFlag (FileType):
        if FileType in {SUP_MODULE_SEC, SUP_MODULE_PEI_CORE, SUP_MODULE_PEIM, SUP_MODULE_MM_CORE_STANDALONE, 'PEI_DXE_COMBO'}:
            return True
        else:
            return False

    ## _SectionCouldHaveRelocFlag() method
    #
    #   Check whether reloc strip flag can be set for a section type.
    #
    #   @param  SectionType The section type to check with
    #   @retval True        This type could have relocation strip flag
    #   @retval False       No way to have it
    #
    @staticmethod
    def _SectionCouldHaveRelocFlag (SectionType):
        if SectionType in {BINARY_FILE_TYPE_TE, BINARY_FILE_TYPE_PE32}:
            return True
        else:
            return False

    ## _GetFilePart() method
    #
    #   Get components for FILE statement
    #
    #   @param  self        The object pointer
    #   @param  FfsFileObj   for whom component is got
    #
    def _GetFilePart(self, FfsFileObj):
        self._GetFileOpts(FfsFileObj)

        if not self._IsToken("{"):
            if self._IsKeyword('RELOCS_STRIPPED') or self._IsKeyword('RELOCS_RETAINED'):
                if self._FileCouldHaveRelocFlag(FfsFileObj.FvFileType):
                    if self._Token == 'RELOCS_STRIPPED':
                        FfsFileObj.KeepReloc = False
                    else:
                        FfsFileObj.KeepReloc = True
                else:
                    raise Warning("File type %s could not have reloc strip flag%d" % (FfsFileObj.FvFileType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)

            if not self._IsToken("{"):
                raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("File name or section data", self.FileName, self.CurrentLineNumber)

        if self._Token == BINARY_FILE_TYPE_FV:
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken():
                raise Warning.Expected("FV name", self.FileName, self.CurrentLineNumber)
            FfsFileObj.FvName = self._Token

        elif self._Token == "FD":
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken():
                raise Warning.Expected("FD name", self.FileName, self.CurrentLineNumber)
            FfsFileObj.FdName = self._Token

        elif self._Token in {TAB_DEFINE, "APRIORI", "SECTION"}:
            self._UndoToken()
            self._GetSectionData(FfsFileObj)

        elif hasattr(FfsFileObj, 'FvFileType') and FfsFileObj.FvFileType == 'RAW':
            self._UndoToken()
            self._GetRAWData(FfsFileObj)

        else:
            FfsFileObj.CurrentLineNum = self.CurrentLineNumber
            FfsFileObj.CurrentLineContent = self._CurrentLine()
            FfsFileObj.FileName = self._Token.replace('$(SPACE)', ' ')
            self._VerifyFile(FfsFileObj.FileName)

        if not self._IsToken(T_CHAR_BRACE_R):
            raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)

    ## _GetRAWData() method
    #
    #   Get RAW data for FILE statement
    #
    #   @param  self         The object pointer
    #   @param  FfsFileObj   for whom section is got
    #
    def _GetRAWData(self, FfsFileObj):
        FfsFileObj.FileName = []
        FfsFileObj.SubAlignment = []
        while True:
            AlignValue = None
            if self._GetAlignment():
                if self._Token not in ALIGNMENTS:
                    raise Warning("Incorrect alignment '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
                #For FFS, Auto is default option same to ""
                if not self._Token == "Auto":
                    AlignValue = self._Token
            if not self._GetNextToken():
                raise Warning.Expected("Filename value", self.FileName, self.CurrentLineNumber)

            FileName = self._Token.replace('$(SPACE)', ' ')
            if FileName == T_CHAR_BRACE_R:
                self._UndoToken()
                raise Warning.Expected("Filename value", self.FileName, self.CurrentLineNumber)

            self._VerifyFile(FileName)
            File = PathClass(NormPath(FileName), GenFdsGlobalVariable.WorkSpaceDir)
            FfsFileObj.FileName.append(File.Path)
            FfsFileObj.SubAlignment.append(AlignValue)

            if self._IsToken(T_CHAR_BRACE_R):
                self._UndoToken()
                break

        if len(FfsFileObj.SubAlignment) == 1:
            FfsFileObj.SubAlignment = FfsFileObj.SubAlignment[0]
        if len(FfsFileObj.FileName) == 1:
            FfsFileObj.FileName = FfsFileObj.FileName[0]

    ## _GetFileOpts() method
    #
    #   Get options for FILE statement
    #
    #   @param  self        The object pointer
    #   @param  FfsFileObj   for whom options is got
    #
    def _GetFileOpts(self, FfsFileObj):
        if self._GetNextToken():
            if TokenFindPattern.match(self._Token):
                FfsFileObj.KeyStringList.append(self._Token)
                if self._IsToken(TAB_COMMA_SPLIT):
                    while self._GetNextToken():
                        if not TokenFindPattern.match(self._Token):
                            raise Warning.Expected("KeyString \"Target_Tag_Arch\"", self.FileName, self.CurrentLineNumber)
                        FfsFileObj.KeyStringList.append(self._Token)

                        if not self._IsToken(TAB_COMMA_SPLIT):
                            break

            else:
                self._UndoToken()

        if self._IsKeyword("FIXED", True):
            FfsFileObj.Fixed = True

        if self._IsKeyword("CHECKSUM", True):
            FfsFileObj.CheckSum = True

        if self._GetAlignment():
            if self._Token not in ALIGNMENTS:
                raise Warning("Incorrect alignment '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
            #For FFS, Auto is default option same to ""
            if not self._Token == "Auto":
                FfsFileObj.Alignment = self._Token

    ## _GetAlignment() method
    #
    #   Return the alignment value
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find alignment
    #   @retval False       Not able to find alignment
    #
    def _GetAlignment(self):
        if self._IsKeyword("Align", True):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("alignment value", self.FileName, self.CurrentLineNumber)
            return True

        return False

    ## _GetSectionData() method
    #
    #   Get section data for FILE statement
    #
    #   @param  self        The object pointer
    #   @param  FfsFileObj   for whom section is got
    #
    def _GetSectionData(self, FfsFileObj):
        self._GetDefineStatements(FfsFileObj)

        while True:
            IsLeafSection = self._GetLeafSection(FfsFileObj)
            IsEncapSection = self._GetEncapsulationSec(FfsFileObj)
            if not IsLeafSection and not IsEncapSection:
                break

    ## _GetLeafSection() method
    #
    #   Get leaf section for Obj
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom leaf section is got
    #   @retval True        Successfully find section statement
    #   @retval False       Not able to find section statement
    #
    def _GetLeafSection(self, Obj):
        OldPos = self.GetFileBufferPos()

        if not self._IsKeyword("SECTION"):
            if len(Obj.SectionList) == 0:
                raise Warning.Expected("SECTION", self.FileName, self.CurrentLineNumber)
            else:
                return False

        AlignValue = None
        if self._GetAlignment():
            if self._Token not in ALIGNMENTS:
                raise Warning("Incorrect alignment '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
            AlignValue = self._Token

        BuildNum = None
        if self._IsKeyword("BUILD_NUM"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("Build number value", self.FileName, self.CurrentLineNumber)

            BuildNum = self._Token

        if self._IsKeyword("VERSION"):
            if AlignValue == 'Auto':
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken():
                raise Warning.Expected("version", self.FileName, self.CurrentLineNumber)
            VerSectionObj = VerSection()
            VerSectionObj.Alignment = AlignValue
            VerSectionObj.BuildNum = BuildNum
            if self._GetStringData():
                VerSectionObj.StringData = self._Token
            else:
                VerSectionObj.FileName = self._Token
            Obj.SectionList.append(VerSectionObj)

        elif self._IsKeyword(BINARY_FILE_TYPE_UI):
            if AlignValue == 'Auto':
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken():
                raise Warning.Expected("UI", self.FileName, self.CurrentLineNumber)
            UiSectionObj = UiSection()
            UiSectionObj.Alignment = AlignValue
            if self._GetStringData():
                UiSectionObj.StringData = self._Token
            else:
                UiSectionObj.FileName = self._Token
            Obj.SectionList.append(UiSectionObj)

        elif self._IsKeyword("FV_IMAGE"):
            if AlignValue == 'Auto':
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken():
                raise Warning.Expected("FV name or FV file path", self.FileName, self.CurrentLineNumber)

            FvName = self._Token
            FvObj = None

            if self._IsToken("{"):
                FvObj = FV()
                FvObj.UiFvName = FvName.upper()
                self._GetDefineStatements(FvObj)

                self._GetBlockStatement(FvObj)
                self._GetSetStatements(FvObj)
                self._GetFvAlignment(FvObj)
                self._GetFvAttributes(FvObj)

                while True:
                    IsInf = self._GetInfStatement(FvObj)
                    IsFile = self._GetFileStatement(FvObj)
                    if not IsInf and not IsFile:
                        break

                if not self._IsToken(T_CHAR_BRACE_R):
                    raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)

            FvImageSectionObj = FvImageSection()
            FvImageSectionObj.Alignment = AlignValue
            if FvObj is not None:
                FvImageSectionObj.Fv = FvObj
                FvImageSectionObj.FvName = None
            else:
                FvImageSectionObj.FvName = FvName.upper()
                FvImageSectionObj.FvFileName = FvName

            Obj.SectionList.append(FvImageSectionObj)

        elif self._IsKeyword("PEI_DEPEX_EXP") or self._IsKeyword("DXE_DEPEX_EXP") or self._IsKeyword("SMM_DEPEX_EXP"):
            if AlignValue == 'Auto':
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            DepexSectionObj = DepexSection()
            DepexSectionObj.Alignment = AlignValue
            DepexSectionObj.DepexType = self._Token

            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._IsToken("{"):
                raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)
            if not self._SkipToToken(T_CHAR_BRACE_R):
                raise Warning.Expected("Depex expression ending '}'", self.FileName, self.CurrentLineNumber)

            DepexSectionObj.Expression = self._SkippedChars.rstrip(T_CHAR_BRACE_R)
            Obj.SectionList.append(DepexSectionObj)

        else:
            if not self._GetNextWord():
                raise Warning.Expected("section type", self.FileName, self.CurrentLineNumber)

            # Encapsulation section appear, UndoToken and return
            if self._Token == "COMPRESS" or self._Token == "GUIDED":
                self.SetFileBufferPos(OldPos)
                return False

            if self._Token not in {"COMPAT16", BINARY_FILE_TYPE_PE32, BINARY_FILE_TYPE_PIC, BINARY_FILE_TYPE_TE, "FV_IMAGE", "RAW", BINARY_FILE_TYPE_DXE_DEPEX,\
                               BINARY_FILE_TYPE_UI, "VERSION", BINARY_FILE_TYPE_PEI_DEPEX, "SUBTYPE_GUID", BINARY_FILE_TYPE_SMM_DEPEX}:
                raise Warning("Unknown section type '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
            if AlignValue == 'Auto'and (not self._Token == BINARY_FILE_TYPE_PE32) and (not self._Token == BINARY_FILE_TYPE_TE):
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)

            # DataSection
            DataSectionObj = DataSection()
            DataSectionObj.Alignment = AlignValue
            DataSectionObj.SecType = self._Token

            if self._IsKeyword('RELOCS_STRIPPED') or self._IsKeyword('RELOCS_RETAINED'):
                if self._FileCouldHaveRelocFlag(Obj.FvFileType) and self._SectionCouldHaveRelocFlag(DataSectionObj.SecType):
                    if self._Token == 'RELOCS_STRIPPED':
                        DataSectionObj.KeepReloc = False
                    else:
                        DataSectionObj.KeepReloc = True
                else:
                    raise Warning("File type %s, section type %s, could not have reloc strip flag%d" % (Obj.FvFileType, DataSectionObj.SecType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)

            if self._IsToken(TAB_EQUAL_SPLIT):
                if not self._GetNextToken():
                    raise Warning.Expected("section file path", self.FileName, self.CurrentLineNumber)
                DataSectionObj.SectFileName = self._Token
                self._VerifyFile(DataSectionObj.SectFileName)
            else:
                if not self._GetCglSection(DataSectionObj):
                    return False

            Obj.SectionList.append(DataSectionObj)

        return True

    ## _VerifyFile
    #
    #    Check if file exists or not:
    #      If current phase if GenFds, the file must exist;
    #      If current phase is AutoGen and the file is not in $(OUTPUT_DIRECTORY), the file must exist
    #    @param FileName: File path to be verified.
    #
    def _VerifyFile(self, FileName):
        if FileName.replace(TAB_WORKSPACE, '').find('$') != -1:
            return
        if not GlobalData.gAutoGenPhase or not self._GetMacroValue(TAB_DSC_DEFINES_OUTPUT_DIRECTORY) in FileName:
            ErrorCode, ErrorInfo = PathClass(NormPath(FileName), GenFdsGlobalVariable.WorkSpaceDir).Validate()
            if ErrorCode != 0:
                EdkLogger.error("GenFds", ErrorCode, ExtraData=ErrorInfo)

    ## _GetCglSection() method
    #
    #   Get compressed or GUIDed section for Obj
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom leaf section is got
    #   @param  AlignValue  alignment value for complex section
    #   @retval True        Successfully find section statement
    #   @retval False       Not able to find section statement
    #
    def _GetCglSection(self, Obj, AlignValue = None):

        if self._IsKeyword("COMPRESS"):
            type = "PI_STD"
            if self._IsKeyword("PI_STD") or self._IsKeyword("PI_NONE"):
                type = self._Token

            if not self._IsToken("{"):
                raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)

            CompressSectionObj = CompressSection()
            CompressSectionObj.Alignment = AlignValue
            CompressSectionObj.CompType = type
            # Recursive sections...
            while True:
                IsLeafSection = self._GetLeafSection(CompressSectionObj)
                IsEncapSection = self._GetEncapsulationSec(CompressSectionObj)
                if not IsLeafSection and not IsEncapSection:
                    break


            if not self._IsToken(T_CHAR_BRACE_R):
                raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)
            Obj.SectionList.append(CompressSectionObj)
            return True

        elif self._IsKeyword("GUIDED"):
            GuidValue = None
            if self._GetNextGuid():
                if self._Token in GlobalData.gGuidDict:
                    self._Token = GuidStructureStringToGuidString(GlobalData.gGuidDict[self._Token]).upper()
                GuidValue = self._Token

            AttribDict = self._GetGuidAttrib()
            if not self._IsToken("{"):
                raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)
            GuidSectionObj = GuidSection()
            GuidSectionObj.Alignment = AlignValue
            GuidSectionObj.NameGuid = GuidValue
            GuidSectionObj.SectionType = "GUIDED"
            GuidSectionObj.ProcessRequired = AttribDict["PROCESSING_REQUIRED"]
            GuidSectionObj.AuthStatusValid = AttribDict["AUTH_STATUS_VALID"]
            GuidSectionObj.ExtraHeaderSize = AttribDict["EXTRA_HEADER_SIZE"]
            # Recursive sections...
            while True:
                IsLeafSection = self._GetLeafSection(GuidSectionObj)
                IsEncapSection = self._GetEncapsulationSec(GuidSectionObj)
                if not IsLeafSection and not IsEncapSection:
                    break

            if not self._IsToken(T_CHAR_BRACE_R):
                raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)
            Obj.SectionList.append(GuidSectionObj)

            return True

        return False

    ## _GetGuidAttri() method
    #
    #   Get attributes for GUID section
    #
    #   @param  self        The object pointer
    #   @retval AttribDict  Dictionary of key-value pair of section attributes
    #
    def _GetGuidAttrib(self):
        AttribDict = {}
        AttribDict["PROCESSING_REQUIRED"] = "NONE"
        AttribDict["AUTH_STATUS_VALID"] = "NONE"
        AttribDict["EXTRA_HEADER_SIZE"] = -1
        while self._IsKeyword("PROCESSING_REQUIRED") or self._IsKeyword("AUTH_STATUS_VALID") \
            or self._IsKeyword("EXTRA_HEADER_SIZE"):
            AttribKey = self._Token

            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("TRUE(1)/FALSE(0)/Number", self.FileName, self.CurrentLineNumber)
            elif AttribKey == "EXTRA_HEADER_SIZE":
                Base = 10
                if self._Token[0:2].upper() == "0X":
                    Base = 16
                try:
                    AttribDict[AttribKey] = int(self._Token, Base)
                    continue
                except ValueError:
                    raise Warning.Expected("Number", self.FileName, self.CurrentLineNumber)
            elif self._Token.upper() not in {"TRUE", "FALSE", "1", "0"}:
                raise Warning.Expected("TRUE/FALSE (1/0)", self.FileName, self.CurrentLineNumber)
            AttribDict[AttribKey] = self._Token

        return AttribDict

    ## _GetEncapsulationSec() method
    #
    #   Get encapsulation section for FILE
    #
    #   @param  self        The object pointer
    #   @param  FfsFile     for whom section is got
    #   @retval True        Successfully find section statement
    #   @retval False       Not able to find section statement
    #
    def _GetEncapsulationSec(self, FfsFileObj):
        OldPos = self.GetFileBufferPos()
        if not self._IsKeyword("SECTION"):
            if len(FfsFileObj.SectionList) == 0:
                raise Warning.Expected("SECTION", self.FileName, self.CurrentLineNumber)
            else:
                return False

        AlignValue = None
        if self._GetAlignment():
            if self._Token not in ALIGNMENT_NOAUTO:
                raise Warning("Incorrect alignment '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
            AlignValue = self._Token

        if not self._GetCglSection(FfsFileObj, AlignValue):
            self.SetFileBufferPos(OldPos)
            return False
        else:
            return True

    def _GetFmp(self):
        if not self._GetNextToken():
            return False
        S = self._Token.upper()
        if S.startswith(TAB_SECTION_START) and not S.startswith("[FMPPAYLOAD."):
            self.SectionParser(S)
            self._UndoToken()
            return False

        self._UndoToken()
        self._SkipToToken("[FMPPAYLOAD.", True)
        FmpUiName = self._GetUiName().upper()
        if FmpUiName in self.Profile.FmpPayloadDict:
            raise Warning("Duplicated FMP UI name found: %s" % FmpUiName, self.FileName, self.CurrentLineNumber)

        FmpData = CapsulePayload()
        FmpData.UiName = FmpUiName

        if not self._IsToken(TAB_SECTION_END):
            raise Warning.ExpectedBracketClose(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning("The FMP payload section is empty!", self.FileName, self.CurrentLineNumber)
        FmpKeyList = ['IMAGE_HEADER_INIT_VERSION', 'IMAGE_TYPE_ID', 'IMAGE_INDEX', 'HARDWARE_INSTANCE', 'CERTIFICATE_GUID', 'MONOTONIC_COUNT']
        while self._Token in FmpKeyList:
            Name = self._Token
            FmpKeyList.remove(Name)
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if Name == 'IMAGE_TYPE_ID':
                if not self._GetNextGuid():
                    raise Warning.Expected("GUID value for IMAGE_TYPE_ID.", self.FileName, self.CurrentLineNumber)
                FmpData.ImageTypeId = self._Token
            elif Name == 'CERTIFICATE_GUID':
                if not self._GetNextGuid():
                    raise Warning.Expected("GUID value for CERTIFICATE_GUID.", self.FileName, self.CurrentLineNumber)
                FmpData.Certificate_Guid = self._Token
                if UUID(FmpData.Certificate_Guid) != EFI_CERT_TYPE_RSA2048_SHA256_GUID and UUID(FmpData.Certificate_Guid) != EFI_CERT_TYPE_PKCS7_GUID:
                    raise Warning("Only support EFI_CERT_TYPE_RSA2048_SHA256_GUID or EFI_CERT_TYPE_PKCS7_GUID for CERTIFICATE_GUID.", self.FileName, self.CurrentLineNumber)
            else:
                if not self._GetNextToken():
                    raise Warning.Expected("value of %s" % Name, self.FileName, self.CurrentLineNumber)
                Value = self._Token
                if Name == 'IMAGE_HEADER_INIT_VERSION':
                    if FdfParser._Verify(Name, Value, 'UINT8'):
                        FmpData.Version = Value
                elif Name == 'IMAGE_INDEX':
                    if FdfParser._Verify(Name, Value, 'UINT8'):
                        FmpData.ImageIndex = Value
                elif Name == 'HARDWARE_INSTANCE':
                    if FdfParser._Verify(Name, Value, 'UINT8'):
                        FmpData.HardwareInstance = Value
                elif Name == 'MONOTONIC_COUNT':
                    if FdfParser._Verify(Name, Value, 'UINT64'):
                        FmpData.MonotonicCount = Value
                        if FmpData.MonotonicCount.upper().startswith('0X'):
                            FmpData.MonotonicCount = int(FmpData.MonotonicCount, 16)
                        else:
                            FmpData.MonotonicCount = int(FmpData.MonotonicCount)
            if not self._GetNextToken():
                break
        else:
            self._UndoToken()

        if (FmpData.MonotonicCount and not FmpData.Certificate_Guid) or (not FmpData.MonotonicCount and FmpData.Certificate_Guid):
            EdkLogger.error("FdfParser", FORMAT_INVALID, "CERTIFICATE_GUID and MONOTONIC_COUNT must be work as a pair.")

        # Only the IMAGE_TYPE_ID is required item
        if FmpKeyList and 'IMAGE_TYPE_ID' in FmpKeyList:
            raise Warning("'IMAGE_TYPE_ID' in FMP payload section.", self.FileName, self.CurrentLineNumber)
        # get the Image file and Vendor code file
        self._GetFMPCapsuleData(FmpData)
        if not FmpData.ImageFile:
            raise Warning("Missing image file in FMP payload section.", self.FileName, self.CurrentLineNumber)
        # check whether more than one Vendor code file
        if len(FmpData.VendorCodeFile) > 1:
            raise Warning("Vendor code file max of 1 per FMP payload section.", self.FileName, self.CurrentLineNumber)
        self.Profile.FmpPayloadDict[FmpUiName] = FmpData
        return True

    ## _GetCapsule() method
    #
    #   Get capsule section contents and store its data into capsule list of self.Profile
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a capsule
    #   @retval False       Not able to find a capsule
    #
    def _GetCapsule(self):
        if not self._GetNextToken():
            return False

        S = self._Token.upper()
        if S.startswith(TAB_SECTION_START) and not S.startswith("[CAPSULE."):
            self.SectionParser(S)
            self._UndoToken()
            return False

        self._UndoToken()
        if not self._IsToken("[CAPSULE.", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            #print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
            #        % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine:], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning.Expected("[Capsule.]", self.FileName, self.CurrentLineNumber)

        CapsuleObj = Capsule()

        CapsuleName = self._GetUiName()
        if not CapsuleName:
            raise Warning.Expected("capsule name", self.FileName, self.CurrentLineNumber)

        CapsuleObj.UiCapsuleName = CapsuleName.upper()

        if not self._IsToken(TAB_SECTION_END):
            raise Warning.ExpectedBracketClose(self.FileName, self.CurrentLineNumber)

        if self._IsKeyword("CREATE_FILE"):
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("file name", self.FileName, self.CurrentLineNumber)

            CapsuleObj.CreateFile = self._Token

        self._GetCapsuleStatements(CapsuleObj)
        self.Profile.CapsuleDict[CapsuleObj.UiCapsuleName] = CapsuleObj
        return True

    ## _GetCapsuleStatements() method
    #
    #   Get statements for capsule
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom statements are got
    #
    def _GetCapsuleStatements(self, Obj):
        self._GetCapsuleTokens(Obj)
        self._GetDefineStatements(Obj)
        self._GetSetStatements(Obj)
        self._GetCapsuleData(Obj)

    ## _GetCapsuleTokens() method
    #
    #   Get token statements for capsule
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom token statements are got
    #
    def _GetCapsuleTokens(self, Obj):
        if not self._GetNextToken():
            return False
        while self._Token in {"CAPSULE_GUID", "CAPSULE_HEADER_SIZE", "CAPSULE_FLAGS", "OEM_CAPSULE_FLAGS", "CAPSULE_HEADER_INIT_VERSION"}:
            Name = self._Token.strip()
            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
            if not self._GetNextToken():
                raise Warning.Expected("value", self.FileName, self.CurrentLineNumber)
            if Name == 'CAPSULE_FLAGS':
                if not self._Token in {"PersistAcrossReset", "PopulateSystemTable", "InitiateReset"}:
                    raise Warning.Expected("PersistAcrossReset, PopulateSystemTable, or InitiateReset", self.FileName, self.CurrentLineNumber)
                Value = self._Token.strip()
                while self._IsToken(TAB_COMMA_SPLIT):
                    Value += TAB_COMMA_SPLIT
                    if not self._GetNextToken():
                        raise Warning.Expected("value", self.FileName, self.CurrentLineNumber)
                    if not self._Token in {"PersistAcrossReset", "PopulateSystemTable", "InitiateReset"}:
                        raise Warning.Expected("PersistAcrossReset, PopulateSystemTable, or InitiateReset", self.FileName, self.CurrentLineNumber)
                    Value += self._Token.strip()
            elif Name == 'OEM_CAPSULE_FLAGS':
                Value = self._Token.strip()
                if not Value.upper().startswith('0X'):
                    raise Warning.Expected("hex value starting with 0x", self.FileName, self.CurrentLineNumber)
                try:
                    Value = int(Value, 0)
                except ValueError:
                    raise Warning.Expected("hex string failed to convert to value", self.FileName, self.CurrentLineNumber)
                if not 0x0000 <= Value <= 0xFFFF:
                    raise Warning.Expected("hex value between 0x0000 and 0xFFFF", self.FileName, self.CurrentLineNumber)
                Value = self._Token.strip()
            else:
                Value = self._Token.strip()
            Obj.TokensDict[Name] = Value
            if not self._GetNextToken():
                return False
        self._UndoToken()

    ## _GetCapsuleData() method
    #
    #   Get capsule data for capsule
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom capsule data are got
    #
    def _GetCapsuleData(self, Obj):
        while True:
            IsInf = self._GetInfStatement(Obj, True)
            IsFile = self._GetFileStatement(Obj, True)
            IsFv = self._GetFvStatement(Obj)
            IsFd = self._GetFdStatement(Obj)
            IsAnyFile = self._GetAnyFileStatement(Obj)
            IsAfile = self._GetAfileStatement(Obj)
            IsFmp = self._GetFmpStatement(Obj)
            if not (IsInf or IsFile or IsFv or IsFd or IsAnyFile or IsAfile or IsFmp):
                break

    ## _GetFMPCapsuleData() method
    #
    #   Get capsule data for FMP capsule
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom capsule data are got
    #
    def _GetFMPCapsuleData(self, Obj):
        while True:
            IsFv = self._GetFvStatement(Obj, True)
            IsFd = self._GetFdStatement(Obj, True)
            IsAnyFile = self._GetAnyFileStatement(Obj, True)
            if not (IsFv or IsFd or IsAnyFile):
                break

    ## _GetFvStatement() method
    #
    #   Get FV for capsule
    #
    #   @param  self        The object pointer
    #   @param  CapsuleObj  for whom FV is got
    #   @retval True        Successfully find a FV statement
    #   @retval False       Not able to find a FV statement
    #
    def _GetFvStatement(self, CapsuleObj, FMPCapsule = False):
        if not self._IsKeyword(BINARY_FILE_TYPE_FV):
            return False

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("FV name", self.FileName, self.CurrentLineNumber)

        if self._Token.upper() not in self.Profile.FvDict:
            raise Warning("FV name does not exist", self.FileName, self.CurrentLineNumber)

        myCapsuleFv = CapsuleFv()
        myCapsuleFv.FvName = self._Token
        if FMPCapsule:
            if not CapsuleObj.ImageFile:
                CapsuleObj.ImageFile.append(myCapsuleFv)
            else:
                CapsuleObj.VendorCodeFile.append(myCapsuleFv)
        else:
            CapsuleObj.CapsuleDataList.append(myCapsuleFv)
        return True

    ## _GetFdStatement() method
    #
    #   Get FD for capsule
    #
    #   @param  self        The object pointer
    #   @param  CapsuleObj  for whom FD is got
    #   @retval True        Successfully find a FD statement
    #   @retval False       Not able to find a FD statement
    #
    def _GetFdStatement(self, CapsuleObj, FMPCapsule = False):
        if not self._IsKeyword("FD"):
            return False

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("FD name", self.FileName, self.CurrentLineNumber)

        if self._Token.upper() not in self.Profile.FdDict:
            raise Warning("FD name does not exist", self.FileName, self.CurrentLineNumber)

        myCapsuleFd = CapsuleFd()
        myCapsuleFd.FdName = self._Token
        if FMPCapsule:
            if not CapsuleObj.ImageFile:
                CapsuleObj.ImageFile.append(myCapsuleFd)
            else:
                CapsuleObj.VendorCodeFile.append(myCapsuleFd)
        else:
            CapsuleObj.CapsuleDataList.append(myCapsuleFd)
        return True

    def _GetFmpStatement(self, CapsuleObj):
        if not self._IsKeyword("FMP_PAYLOAD"):
            if not self._IsKeyword("FMP"):
                return False

            if not self._IsKeyword("PAYLOAD"):
                self._UndoToken()
                return False

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("payload name after FMP_PAYLOAD =", self.FileName, self.CurrentLineNumber)
        Payload = self._Token.upper()
        if Payload not in self.Profile.FmpPayloadDict:
            raise Warning("This FMP Payload does not exist: %s" % self._Token, self.FileName, self.CurrentLineNumber)
        CapsuleObj.FmpPayloadList.append(self.Profile.FmpPayloadDict[Payload])
        return True

    def _ParseRawFileStatement(self):
        if not self._IsKeyword("FILE"):
            return None

        if not self._IsKeyword("DATA"):
            self._UndoToken()
            return None

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("File name", self.FileName, self.CurrentLineNumber)

        AnyFileName = self._Token
        self._VerifyFile(AnyFileName)

        if not os.path.isabs(AnyFileName):
            AnyFileName = mws.join(GenFdsGlobalVariable.WorkSpaceDir, AnyFileName)

        return AnyFileName

    ## _GetAnyFileStatement() method
    #
    #   Get AnyFile for capsule
    #
    #   @param  self        The object pointer
    #   @param  CapsuleObj  for whom AnyFile is got
    #   @retval True        Successfully find a Anyfile statement
    #   @retval False       Not able to find a AnyFile statement
    #
    def _GetAnyFileStatement(self, CapsuleObj, FMPCapsule = False):
        AnyFileName = self._ParseRawFileStatement()
        if not AnyFileName:
            return False

        myCapsuleAnyFile = CapsuleAnyFile()
        myCapsuleAnyFile.FileName = AnyFileName
        if FMPCapsule:
            if not CapsuleObj.ImageFile:
                CapsuleObj.ImageFile.append(myCapsuleAnyFile)
            else:
                CapsuleObj.VendorCodeFile.append(myCapsuleAnyFile)
        else:
            CapsuleObj.CapsuleDataList.append(myCapsuleAnyFile)
        return True

    ## _GetAfileStatement() method
    #
    #   Get Afile for capsule
    #
    #   @param  self        The object pointer
    #   @param  CapsuleObj  for whom Afile is got
    #   @retval True        Successfully find a Afile statement
    #   @retval False       Not able to find a Afile statement
    #
    def _GetAfileStatement(self, CapsuleObj):
        if not self._IsKeyword("APPEND"):
            return False

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._GetNextToken():
            raise Warning.Expected("Afile name", self.FileName, self.CurrentLineNumber)

        AfileName = self._Token
        AfileBaseName = os.path.basename(AfileName)

        if os.path.splitext(AfileBaseName)[1]  not in {".bin", ".BIN", ".Bin", ".dat", ".DAT", ".Dat", ".data", ".DATA", ".Data"}:
            raise Warning('invalid binary file type, should be one of "bin",BINARY_FILE_TYPE_BIN,"Bin","dat","DAT","Dat","data","DATA","Data"', \
                          self.FileName, self.CurrentLineNumber)

        if not os.path.isabs(AfileName):
            AfileName = GenFdsGlobalVariable.ReplaceWorkspaceMacro(AfileName)
            self._VerifyFile(AfileName)
        else:
            if not os.path.exists(AfileName):
                raise Warning('%s does not exist' % AfileName, self.FileName, self.CurrentLineNumber)
            else:
                pass

        myCapsuleAfile = CapsuleAfile()
        myCapsuleAfile.FileName = AfileName
        CapsuleObj.CapsuleDataList.append(myCapsuleAfile)
        return True

    ## _GetRule() method
    #
    #   Get Rule section contents and store its data into rule list of self.Profile
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a Rule
    #   @retval False       Not able to find a Rule
    #
    def _GetRule(self):
        if not self._GetNextToken():
            return False

        S = self._Token.upper()
        if S.startswith(TAB_SECTION_START) and not S.startswith("[RULE."):
            self.SectionParser(S)
            self._UndoToken()
            return False
        self._UndoToken()
        if not self._IsToken("[Rule.", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            #print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
            #        % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine:], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning.Expected("[Rule.]", self.FileName, self.CurrentLineNumber)

        if not self._SkipToToken(TAB_SPLIT):
            raise Warning.Expected("'.'", self.FileName, self.CurrentLineNumber)

        Arch = self._SkippedChars.rstrip(TAB_SPLIT)

        ModuleType = self._GetModuleType()

        TemplateName = ""
        if self._IsToken(TAB_SPLIT):
            if not self._GetNextWord():
                raise Warning.Expected("template name", self.FileName, self.CurrentLineNumber)
            TemplateName = self._Token

        if not self._IsToken(TAB_SECTION_END):
            raise Warning.ExpectedBracketClose(self.FileName, self.CurrentLineNumber)

        RuleObj = self._GetRuleFileStatements()
        RuleObj.Arch = Arch.upper()
        RuleObj.ModuleType = ModuleType
        RuleObj.TemplateName = TemplateName
        if TemplateName == '':
            self.Profile.RuleDict['RULE'             + \
                              TAB_SPLIT              + \
                              Arch.upper()           + \
                              TAB_SPLIT              + \
                              ModuleType.upper()     ] = RuleObj
        else:
            self.Profile.RuleDict['RULE'             + \
                              TAB_SPLIT              + \
                              Arch.upper()           + \
                              TAB_SPLIT              + \
                              ModuleType.upper()     + \
                              TAB_SPLIT              + \
                              TemplateName.upper() ] = RuleObj
        return True

    ## _GetModuleType() method
    #
    #   Return the module type
    #
    #   @param  self        The object pointer
    #   @retval string      module type
    #
    def _GetModuleType(self):
        if not self._GetNextWord():
            raise Warning.Expected("Module type", self.FileName, self.CurrentLineNumber)
        if self._Token.upper() not in {
                SUP_MODULE_SEC, SUP_MODULE_PEI_CORE, SUP_MODULE_PEIM,
                SUP_MODULE_DXE_CORE, SUP_MODULE_DXE_DRIVER,
                SUP_MODULE_DXE_SAL_DRIVER, SUP_MODULE_DXE_SMM_DRIVER,
                SUP_MODULE_DXE_RUNTIME_DRIVER, SUP_MODULE_UEFI_DRIVER,
                SUP_MODULE_UEFI_APPLICATION, SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION,
                TAB_DEFAULT, SUP_MODULE_BASE,
                EDK_COMPONENT_TYPE_SECURITY_CORE,
                EDK_COMPONENT_TYPE_COMBINED_PEIM_DRIVER,
                EDK_COMPONENT_TYPE_PIC_PEIM,
                EDK_COMPONENT_TYPE_RELOCATABLE_PEIM, "PE32_PEIM",
                EDK_COMPONENT_TYPE_BS_DRIVER, EDK_COMPONENT_TYPE_RT_DRIVER,
                EDK_COMPONENT_TYPE_SAL_RT_DRIVER,
                EDK_COMPONENT_TYPE_APPLICATION, "ACPITABLE",
                SUP_MODULE_SMM_CORE, SUP_MODULE_MM_STANDALONE,
                SUP_MODULE_MM_CORE_STANDALONE}:
            raise Warning("Unknown Module type '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
        return self._Token

    ## _GetFileExtension() method
    #
    #   Return the file extension
    #
    #   @param  self        The object pointer
    #   @retval string      file name extension
    #
    def _GetFileExtension(self):
        if not self._IsToken(TAB_SPLIT):
            raise Warning.Expected("'.'", self.FileName, self.CurrentLineNumber)

        Ext = ""
        if self._GetNextToken():
            if FileExtensionPattern.match(self._Token):
                Ext = self._Token
                return TAB_SPLIT + Ext
            else:
                raise Warning("Unknown file extension '%s'" % self._Token, self.FileName, self.CurrentLineNumber)

        else:
            raise Warning.Expected("file extension", self.FileName, self.CurrentLineNumber)

    ## _GetRuleFileStatement() method
    #
    #   Get rule contents
    #
    #   @param  self        The object pointer
    #   @retval Rule        Rule object
    #
    def _GetRuleFileStatements(self):
        if not self._IsKeyword("FILE"):
            raise Warning.Expected("FILE", self.FileName, self.CurrentLineNumber)

        if not self._GetNextWord():
            raise Warning.Expected("FFS type", self.FileName, self.CurrentLineNumber)

        Type = self._Token.strip().upper()
        if Type not in {"RAW", "FREEFORM", SUP_MODULE_SEC, SUP_MODULE_PEI_CORE, SUP_MODULE_PEIM,
                        "PEI_DXE_COMBO", "DRIVER", SUP_MODULE_DXE_CORE, EDK_COMPONENT_TYPE_APPLICATION,
                        "FV_IMAGE", "SMM", SUP_MODULE_SMM_CORE, SUP_MODULE_MM_STANDALONE,
                        SUP_MODULE_MM_CORE_STANDALONE}:
            raise Warning("Unknown FV type '%s'" % self._Token, self.FileName, self.CurrentLineNumber)

        if not self._IsToken(TAB_EQUAL_SPLIT):
            raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

        if not self._IsKeyword("$(NAMED_GUID)"):
            if not self._GetNextWord():
                NamedGuid = self._CurrentLine()[self.CurrentOffsetWithinLine:].split()[0].strip()
                if GlobalData.gGuidPatternEnd.match(NamedGuid):
                    self.CurrentOffsetWithinLine += len(NamedGuid)
                    self._Token = NamedGuid
                else:
                    raise Warning.Expected("$(NAMED_GUID)", self.FileName, self.CurrentLineNumber)
            if self._Token == 'PCD':
                if not self._IsToken("("):
                    raise Warning.Expected("'('", self.FileName, self.CurrentLineNumber)
                PcdPair = self._GetNextPcdSettings()
                if not self._IsToken(")"):
                    raise Warning.Expected("')'", self.FileName, self.CurrentLineNumber)
                self._Token = 'PCD('+PcdPair[1]+TAB_SPLIT+PcdPair[0]+')'

        NameGuid = self._Token

        KeepReloc = None
        if self._IsKeyword('RELOCS_STRIPPED') or self._IsKeyword('RELOCS_RETAINED'):
            if self._FileCouldHaveRelocFlag(Type):
                if self._Token == 'RELOCS_STRIPPED':
                    KeepReloc = False
                else:
                    KeepReloc = True
            else:
                raise Warning("File type %s could not have reloc strip flag%d" % (Type, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)

        KeyStringList = []
        if self._GetNextToken():
            if TokenFindPattern.match(self._Token):
                KeyStringList.append(self._Token)
                if self._IsToken(TAB_COMMA_SPLIT):
                    while self._GetNextToken():
                        if not TokenFindPattern.match(self._Token):
                            raise Warning.Expected("KeyString \"Target_Tag_Arch\"", self.FileName, self.CurrentLineNumber)
                        KeyStringList.append(self._Token)

                        if not self._IsToken(TAB_COMMA_SPLIT):
                            break

            else:
                self._UndoToken()


        Fixed = False
        if self._IsKeyword("Fixed", True):
            Fixed = True

        CheckSum = False
        if self._IsKeyword("CheckSum", True):
            CheckSum = True

        AlignValue = ""
        if self._GetAlignment():
            if self._Token not in ALIGNMENTS:
                raise Warning("Incorrect alignment '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
            #For FFS, Auto is default option same to ""
            if not self._Token == "Auto":
                AlignValue = self._Token

        if self._IsToken("{"):
            # Complex file rule expected
            NewRule = RuleComplexFile()
            NewRule.FvFileType = Type
            NewRule.NameGuid = NameGuid
            NewRule.Alignment = AlignValue
            NewRule.CheckSum = CheckSum
            NewRule.Fixed = Fixed
            NewRule.KeyStringList = KeyStringList
            if KeepReloc is not None:
                NewRule.KeepReloc = KeepReloc

            while True:
                IsEncapsulate = self._GetRuleEncapsulationSection(NewRule)
                IsLeaf = self._GetEfiSection(NewRule)
                if not IsEncapsulate and not IsLeaf:
                    break

            if not self._IsToken(T_CHAR_BRACE_R):
                raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)

            return NewRule

        else:
            # Simple file rule expected
            if not self._GetNextWord():
                raise Warning.Expected("leaf section type", self.FileName, self.CurrentLineNumber)

            SectionName = self._Token

            if SectionName not in {
                    "COMPAT16", BINARY_FILE_TYPE_PE32,
                    BINARY_FILE_TYPE_PIC, BINARY_FILE_TYPE_TE, "FV_IMAGE",
                    "RAW",BINARY_FILE_TYPE_DXE_DEPEX, BINARY_FILE_TYPE_UI,
                    BINARY_FILE_TYPE_PEI_DEPEX, "VERSION", "SUBTYPE_GUID",
                    BINARY_FILE_TYPE_SMM_DEPEX}:
                raise Warning("Unknown leaf section name '%s'" % SectionName, self.FileName, self.CurrentLineNumber)


            if self._IsKeyword("Fixed", True):
                Fixed = True

            if self._IsKeyword("CheckSum", True):
                CheckSum = True

            SectAlignment = ""
            if self._GetAlignment():
                if self._Token not in ALIGNMENTS:
                    raise Warning("Incorrect alignment '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
                if self._Token == 'Auto' and (not SectionName == BINARY_FILE_TYPE_PE32) and (not SectionName == BINARY_FILE_TYPE_TE):
                    raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
                SectAlignment = self._Token

            Ext = None
            if self._IsToken(TAB_VALUE_SPLIT):
                Ext = self._GetFileExtension()
            elif not self._GetNextToken():
                raise Warning.Expected("File name", self.FileName, self.CurrentLineNumber)

            NewRule = RuleSimpleFile()
            NewRule.SectionType = SectionName
            NewRule.FvFileType = Type
            NewRule.NameGuid = NameGuid
            NewRule.Alignment = AlignValue
            NewRule.SectAlignment = SectAlignment
            NewRule.CheckSum = CheckSum
            NewRule.Fixed = Fixed
            NewRule.KeyStringList = KeyStringList
            if KeepReloc is not None:
                NewRule.KeepReloc = KeepReloc
            NewRule.FileExtension = Ext
            NewRule.FileName = self._Token
            return NewRule

    ## _GetEfiSection() method
    #
    #   Get section list for Rule
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom section is got
    #   @retval True        Successfully find section statement
    #   @retval False       Not able to find section statement
    #
    def _GetEfiSection(self, Obj):
        OldPos = self.GetFileBufferPos()
        EfiSectionObj = EfiSection()
        if not self._GetNextWord():
            CurrentLine = self._CurrentLine()[self.CurrentOffsetWithinLine:].split()[0].strip()
            if self._Token == '{' and Obj.FvFileType == "RAW" and TAB_SPLIT in CurrentLine:
                if self._IsToken(TAB_VALUE_SPLIT):
                    EfiSectionObj.FileExtension = self._GetFileExtension()
                elif self._GetNextToken():
                    EfiSectionObj.FileName = self._Token
                EfiSectionObj.SectionType = BINARY_FILE_TYPE_RAW
                Obj.SectionList.append(EfiSectionObj)
                return True
            else:
                return False
        SectionName = self._Token

        if SectionName not in {
                    "COMPAT16", BINARY_FILE_TYPE_PE32,
                    BINARY_FILE_TYPE_PIC, BINARY_FILE_TYPE_TE, "FV_IMAGE",
                    "RAW",BINARY_FILE_TYPE_DXE_DEPEX, BINARY_FILE_TYPE_UI,
                    BINARY_FILE_TYPE_PEI_DEPEX, "VERSION", "SUBTYPE_GUID",
                    BINARY_FILE_TYPE_SMM_DEPEX, BINARY_FILE_TYPE_GUID}:
            self._UndoToken()
            return False

        if SectionName == "FV_IMAGE":
            FvImageSectionObj = FvImageSection()
            if self._IsKeyword("FV_IMAGE"):
                pass
            if self._IsToken("{"):
                FvObj = FV()
                self._GetDefineStatements(FvObj)
                self._GetBlockStatement(FvObj)
                self._GetSetStatements(FvObj)
                self._GetFvAlignment(FvObj)
                self._GetFvAttributes(FvObj)
                self._GetAprioriSection(FvObj)
                self._GetAprioriSection(FvObj)

                while True:
                    IsInf = self._GetInfStatement(FvObj)
                    IsFile = self._GetFileStatement(FvObj)
                    if not IsInf and not IsFile:
                        break

                if not self._IsToken(T_CHAR_BRACE_R):
                    raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)
                FvImageSectionObj.Fv = FvObj
                FvImageSectionObj.FvName = None

            else:
                if not self._IsKeyword(BINARY_FILE_TYPE_FV):
                    raise Warning.Expected("'FV'", self.FileName, self.CurrentLineNumber)
                FvImageSectionObj.FvFileType = self._Token

                if self._GetAlignment():
                    if self._Token not in ALIGNMENT_NOAUTO:
                        raise Warning("Incorrect alignment '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
                    FvImageSectionObj.Alignment = self._Token

                if self._IsToken(TAB_VALUE_SPLIT):
                    FvImageSectionObj.FvFileExtension = self._GetFileExtension()
                elif self._GetNextToken():
                    if self._Token not in {
                            T_CHAR_BRACE_R, "COMPAT16", BINARY_FILE_TYPE_PE32,
                            BINARY_FILE_TYPE_PIC, BINARY_FILE_TYPE_TE,
                            "FV_IMAGE", "RAW", BINARY_FILE_TYPE_DXE_DEPEX,
                            BINARY_FILE_TYPE_UI, "VERSION",
                            BINARY_FILE_TYPE_PEI_DEPEX, BINARY_FILE_TYPE_GUID,
                            BINARY_FILE_TYPE_SMM_DEPEX}:
                        FvImageSectionObj.FvFileName = self._Token
                    else:
                        self._UndoToken()
                else:
                    raise Warning.Expected("FV file name", self.FileName, self.CurrentLineNumber)

            Obj.SectionList.append(FvImageSectionObj)
            return True

        EfiSectionObj.SectionType = SectionName

        if not self._GetNextToken():
            raise Warning.Expected("file type", self.FileName, self.CurrentLineNumber)

        if self._Token == "STRING":
            if not self._RuleSectionCouldHaveString(EfiSectionObj.SectionType):
                raise Warning("%s section could NOT have string data%d" % (EfiSectionObj.SectionType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)

            if not self._IsToken(TAB_EQUAL_SPLIT):
                raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)

            if not self._GetNextToken():
                raise Warning.Expected("Quoted String", self.FileName, self.CurrentLineNumber)

            if self._GetStringData():
                EfiSectionObj.StringData = self._Token

            if self._IsKeyword("BUILD_NUM"):
                if not self._RuleSectionCouldHaveBuildNum(EfiSectionObj.SectionType):
                    raise Warning("%s section could NOT have BUILD_NUM%d" % (EfiSectionObj.SectionType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)

                if not self._IsToken(TAB_EQUAL_SPLIT):
                    raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
                if not self._GetNextToken():
                    raise Warning.Expected("Build number", self.FileName, self.CurrentLineNumber)
                EfiSectionObj.BuildNum = self._Token

        else:
            EfiSectionObj.FileType = self._Token
            self._CheckRuleSectionFileType(EfiSectionObj.SectionType, EfiSectionObj.FileType)

        if self._IsKeyword("Optional"):
            if not self._RuleSectionCouldBeOptional(EfiSectionObj.SectionType):
                raise Warning("%s section could NOT be optional%d" % (EfiSectionObj.SectionType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)
            EfiSectionObj.Optional = True

            if self._IsKeyword("BUILD_NUM"):
                if not self._RuleSectionCouldHaveBuildNum(EfiSectionObj.SectionType):
                    raise Warning("%s section could NOT have BUILD_NUM%d" % (EfiSectionObj.SectionType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)

                if not self._IsToken(TAB_EQUAL_SPLIT):
                    raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
                if not self._GetNextToken():
                    raise Warning.Expected("Build number", self.FileName, self.CurrentLineNumber)
                EfiSectionObj.BuildNum = self._Token

        if self._GetAlignment():
            if self._Token not in ALIGNMENTS:
                raise Warning("Incorrect alignment '%s'" % self._Token, self.FileName, self.CurrentLineNumber)
            if self._Token == 'Auto' and (not SectionName == BINARY_FILE_TYPE_PE32) and (not SectionName == BINARY_FILE_TYPE_TE):
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            EfiSectionObj.Alignment = self._Token

        if self._IsKeyword('RELOCS_STRIPPED') or self._IsKeyword('RELOCS_RETAINED'):
            if self._SectionCouldHaveRelocFlag(EfiSectionObj.SectionType):
                if self._Token == 'RELOCS_STRIPPED':
                    EfiSectionObj.KeepReloc = False
                else:
                    EfiSectionObj.KeepReloc = True
                if Obj.KeepReloc is not None and Obj.KeepReloc != EfiSectionObj.KeepReloc:
                    raise Warning("Section type %s has reloc strip flag conflict with Rule" % EfiSectionObj.SectionType, self.FileName, self.CurrentLineNumber)
            else:
                raise Warning("Section type %s could not have reloc strip flag" % EfiSectionObj.SectionType, self.FileName, self.CurrentLineNumber)


        if self._IsToken(TAB_VALUE_SPLIT):
            EfiSectionObj.FileExtension = self._GetFileExtension()
        elif self._GetNextToken():
            if self._Token not in {
                    T_CHAR_BRACE_R, "COMPAT16", BINARY_FILE_TYPE_PE32,
                    BINARY_FILE_TYPE_PIC, BINARY_FILE_TYPE_TE,
                    "FV_IMAGE", "RAW", BINARY_FILE_TYPE_DXE_DEPEX,
                    BINARY_FILE_TYPE_UI, "VERSION",
                    BINARY_FILE_TYPE_PEI_DEPEX, BINARY_FILE_TYPE_GUID,
                    BINARY_FILE_TYPE_SMM_DEPEX}:

                if self._Token.startswith('PCD'):
                    self._UndoToken()
                    self._GetNextWord()

                    if self._Token == 'PCD':
                        if not self._IsToken("("):
                            raise Warning.Expected("'('", self.FileName, self.CurrentLineNumber)
                        PcdPair = self._GetNextPcdSettings()
                        if not self._IsToken(")"):
                            raise Warning.Expected("')'", self.FileName, self.CurrentLineNumber)
                        self._Token = 'PCD('+PcdPair[1]+TAB_SPLIT+PcdPair[0]+')'

                EfiSectionObj.FileName = self._Token

            else:
                self._UndoToken()
        else:
            raise Warning.Expected("section file name", self.FileName, self.CurrentLineNumber)

        Obj.SectionList.append(EfiSectionObj)
        return True

    ## _RuleSectionCouldBeOptional() method
    #
    #   Get whether a section could be optional
    #
    #   @param  SectionType The section type to check
    #   @retval True        section could be optional
    #   @retval False       section never optional
    #
    @staticmethod
    def _RuleSectionCouldBeOptional(SectionType):
        if SectionType in {BINARY_FILE_TYPE_DXE_DEPEX, BINARY_FILE_TYPE_UI, "VERSION", BINARY_FILE_TYPE_PEI_DEPEX, "RAW", BINARY_FILE_TYPE_SMM_DEPEX}:
            return True
        else:
            return False

    ## _RuleSectionCouldHaveBuildNum() method
    #
    #   Get whether a section could have build number information
    #
    #   @param  SectionType The section type to check
    #   @retval True        section could have build number information
    #   @retval False       section never have build number information
    #
    @staticmethod
    def _RuleSectionCouldHaveBuildNum(SectionType):
        if SectionType == "VERSION":
            return True
        else:
            return False

    ## _RuleSectionCouldHaveString() method
    #
    #   Get whether a section could have string
    #
    #   @param  SectionType The section type to check
    #   @retval True        section could have string
    #   @retval False       section never have string
    #
    @staticmethod
    def _RuleSectionCouldHaveString(SectionType):
        if SectionType in {BINARY_FILE_TYPE_UI, "VERSION"}:
            return True
        else:
            return False

    ## _CheckRuleSectionFileType() method
    #
    #   Get whether a section matches a file type
    #
    #   @param  self        The object pointer
    #   @param  SectionType The section type to check
    #   @param  FileType    The file type to check
    #
    def _CheckRuleSectionFileType(self, SectionType, FileType):
        WarningString = "Incorrect section file type '%s'"
        if SectionType == "COMPAT16":
            if FileType not in {"COMPAT16", "SEC_COMPAT16"}:
                raise Warning(WarningString % FileType, self.FileName, self.CurrentLineNumber)
        elif SectionType == BINARY_FILE_TYPE_PE32:
            if FileType not in {BINARY_FILE_TYPE_PE32, "SEC_PE32"}:
                raise Warning(WarningString % FileType, self.FileName, self.CurrentLineNumber)
        elif SectionType == BINARY_FILE_TYPE_PIC:
            if FileType not in {BINARY_FILE_TYPE_PIC, BINARY_FILE_TYPE_PIC}:
                raise Warning(WarningString % FileType, self.FileName, self.CurrentLineNumber)
        elif SectionType == BINARY_FILE_TYPE_TE:
            if FileType not in {BINARY_FILE_TYPE_TE, "SEC_TE"}:
                raise Warning(WarningString % FileType, self.FileName, self.CurrentLineNumber)
        elif SectionType == "RAW":
            if FileType not in {BINARY_FILE_TYPE_BIN, "SEC_BIN", "RAW", "ASL", "ACPI"}:
                raise Warning(WarningString % FileType, self.FileName, self.CurrentLineNumber)
        elif SectionType == BINARY_FILE_TYPE_DXE_DEPEX or SectionType == BINARY_FILE_TYPE_SMM_DEPEX:
            if FileType not in {BINARY_FILE_TYPE_DXE_DEPEX, "SEC_DXE_DEPEX", BINARY_FILE_TYPE_SMM_DEPEX}:
                raise Warning(WarningString % FileType, self.FileName, self.CurrentLineNumber)
        elif SectionType == BINARY_FILE_TYPE_UI:
            if FileType not in {BINARY_FILE_TYPE_UI, "SEC_UI"}:
                raise Warning(WarningString % FileType, self.FileName, self.CurrentLineNumber)
        elif SectionType == "VERSION":
            if FileType not in {"VERSION", "SEC_VERSION"}:
                raise Warning(WarningString % FileType, self.FileName, self.CurrentLineNumber)
        elif SectionType == BINARY_FILE_TYPE_PEI_DEPEX:
            if FileType not in {BINARY_FILE_TYPE_PEI_DEPEX, "SEC_PEI_DEPEX"}:
                raise Warning(WarningString % FileType, self.FileName, self.CurrentLineNumber)
        elif SectionType == BINARY_FILE_TYPE_GUID:
            if FileType not in {BINARY_FILE_TYPE_PE32, "SEC_GUID"}:
                raise Warning(WarningString % FileType, self.FileName, self.CurrentLineNumber)

    ## _GetRuleEncapsulationSection() method
    #
    #   Get encapsulation section for Rule
    #
    #   @param  self        The object pointer
    #   @param  theRule     for whom section is got
    #   @retval True        Successfully find section statement
    #   @retval False       Not able to find section statement
    #
    def _GetRuleEncapsulationSection(self, theRule):
        if self._IsKeyword("COMPRESS"):
            Type = "PI_STD"
            if self._IsKeyword("PI_STD") or self._IsKeyword("PI_NONE"):
                Type = self._Token

            if not self._IsToken("{"):
                raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)

            CompressSectionObj = CompressSection()

            CompressSectionObj.CompType = Type
            # Recursive sections...
            while True:
                IsEncapsulate = self._GetRuleEncapsulationSection(CompressSectionObj)
                IsLeaf = self._GetEfiSection(CompressSectionObj)
                if not IsEncapsulate and not IsLeaf:
                    break

            if not self._IsToken(T_CHAR_BRACE_R):
                raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)
            theRule.SectionList.append(CompressSectionObj)

            return True

        elif self._IsKeyword("GUIDED"):
            GuidValue = None
            if self._GetNextGuid():
                if self._Token in GlobalData.gGuidDict:
                    self._Token = GuidStructureStringToGuidString(GlobalData.gGuidDict[self._Token]).upper()
                GuidValue = self._Token

            if self._IsKeyword("$(NAMED_GUID)"):
                GuidValue = self._Token

            AttribDict = self._GetGuidAttrib()

            if not self._IsToken("{"):
                raise Warning.ExpectedCurlyOpen(self.FileName, self.CurrentLineNumber)
            GuidSectionObj = GuidSection()
            GuidSectionObj.NameGuid = GuidValue
            GuidSectionObj.SectionType = "GUIDED"
            GuidSectionObj.ProcessRequired = AttribDict["PROCESSING_REQUIRED"]
            GuidSectionObj.AuthStatusValid = AttribDict["AUTH_STATUS_VALID"]
            GuidSectionObj.ExtraHeaderSize = AttribDict["EXTRA_HEADER_SIZE"]

            # Efi sections...
            while True:
                IsEncapsulate = self._GetRuleEncapsulationSection(GuidSectionObj)
                IsLeaf = self._GetEfiSection(GuidSectionObj)
                if not IsEncapsulate and not IsLeaf:
                    break

            if not self._IsToken(T_CHAR_BRACE_R):
                raise Warning.ExpectedCurlyClose(self.FileName, self.CurrentLineNumber)
            theRule.SectionList.append(GuidSectionObj)

            return True

        return False

    ## _GetOptionRom() method
    #
    #   Get OptionROM section contents and store its data into OptionROM list of self.Profile
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a OptionROM
    #   @retval False       Not able to find a OptionROM
    #
    def _GetOptionRom(self):
        if not self._GetNextToken():
            return False

        S = self._Token.upper()
        if S.startswith(TAB_SECTION_START) and not S.startswith("[OPTIONROM."):
            self.SectionParser(S)
            self._UndoToken()
            return False

        self._UndoToken()
        if not self._IsToken("[OptionRom.", True):
            raise Warning("Unknown Keyword '%s'" % self._Token, self.FileName, self.CurrentLineNumber)

        OptRomName = self._GetUiName()

        if not self._IsToken(TAB_SECTION_END):
            raise Warning.ExpectedBracketClose(self.FileName, self.CurrentLineNumber)

        OptRomObj = OPTIONROM(OptRomName)
        self.Profile.OptRomDict[OptRomName] = OptRomObj

        while True:
            isInf = self._GetOptRomInfStatement(OptRomObj)
            isFile = self._GetOptRomFileStatement(OptRomObj)
            if not isInf and not isFile:
                break

        return True

    ## _GetOptRomInfStatement() method
    #
    #   Get INF statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom inf statement is got
    #   @retval True        Successfully find inf statement
    #   @retval False       Not able to find inf statement
    #
    def _GetOptRomInfStatement(self, Obj):
        if not self._IsKeyword("INF"):
            return False

        ffsInf = OptRomInfStatement()
        self._GetInfOptions(ffsInf)

        if not self._GetNextToken():
            raise Warning.Expected("INF file path", self.FileName, self.CurrentLineNumber)
        ffsInf.InfFileName = self._Token
        if ffsInf.InfFileName.replace(TAB_WORKSPACE, '').find('$') == -1:
            #check for file path
            ErrorCode, ErrorInfo = PathClass(NormPath(ffsInf.InfFileName), GenFdsGlobalVariable.WorkSpaceDir).Validate()
            if ErrorCode != 0:
                EdkLogger.error("GenFds", ErrorCode, ExtraData=ErrorInfo)

        NewFileName = ffsInf.InfFileName
        if ffsInf.OverrideGuid:
            NewFileName = ProcessDuplicatedInf(PathClass(ffsInf.InfFileName,GenFdsGlobalVariable.WorkSpaceDir), ffsInf.OverrideGuid, GenFdsGlobalVariable.WorkSpaceDir).Path

        if not NewFileName in self.Profile.InfList:
            self.Profile.InfList.append(NewFileName)
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            self.Profile.InfFileLineList.append(FileLineTuple)
            if ffsInf.UseArch:
                if ffsInf.UseArch not in self.Profile.InfDict:
                    self.Profile.InfDict[ffsInf.UseArch] = [ffsInf.InfFileName]
                else:
                    self.Profile.InfDict[ffsInf.UseArch].append(ffsInf.InfFileName)
            else:
                self.Profile.InfDict['ArchTBD'].append(ffsInf.InfFileName)


        self._GetOptRomOverrides (ffsInf)

        Obj.FfsList.append(ffsInf)
        return True

    ## _GetOptRomOverrides() method
    #
    #   Get overrides for OptROM INF & FILE
    #
    #   @param  self        The object pointer
    #   @param  FfsInfObj   for whom overrides is got
    #
    def _GetOptRomOverrides(self, Obj):
        if self._IsToken('{'):
            Overrides = OverrideAttribs()
            while True:
                if self._IsKeyword("PCI_VENDOR_ID"):
                    if not self._IsToken(TAB_EQUAL_SPLIT):
                        raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
                    if not self._GetNextHexNumber():
                        raise Warning.Expected("Hex vendor id", self.FileName, self.CurrentLineNumber)
                    Overrides.PciVendorId = self._Token
                    continue

                if self._IsKeyword("PCI_CLASS_CODE"):
                    if not self._IsToken(TAB_EQUAL_SPLIT):
                        raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
                    if not self._GetNextHexNumber():
                        raise Warning.Expected("Hex class code", self.FileName, self.CurrentLineNumber)
                    Overrides.PciClassCode = self._Token
                    continue

                if self._IsKeyword("PCI_DEVICE_ID"):
                    if not self._IsToken(TAB_EQUAL_SPLIT):
                        raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
                    # Get a list of PCI IDs
                    Overrides.PciDeviceId = ""
                    while (self._GetNextHexNumber()):
                        Overrides.PciDeviceId = "{} {}".format(Overrides.PciDeviceId, self._Token)
                    if not Overrides.PciDeviceId:
                        raise Warning.Expected("one or more Hex device ids", self.FileName, self.CurrentLineNumber)
                    continue

                if self._IsKeyword("PCI_REVISION"):
                    if not self._IsToken(TAB_EQUAL_SPLIT):
                        raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
                    if not self._GetNextHexNumber():
                        raise Warning.Expected("Hex revision", self.FileName, self.CurrentLineNumber)
                    Overrides.PciRevision = self._Token
                    continue

                if self._IsKeyword("PCI_COMPRESS"):
                    if not self._IsToken(TAB_EQUAL_SPLIT):
                        raise Warning.ExpectedEquals(self.FileName, self.CurrentLineNumber)
                    if not self._GetNextToken():
                        raise Warning.Expected("TRUE/FALSE for compress", self.FileName, self.CurrentLineNumber)
                    Overrides.NeedCompress = self._Token.upper() == 'TRUE'
                    continue

                if self._IsToken(T_CHAR_BRACE_R):
                    break
                else:
                    EdkLogger.error("FdfParser", FORMAT_INVALID, File=self.FileName, Line=self.CurrentLineNumber)

            Obj.OverrideAttribs = Overrides

    ## _GetOptRomFileStatement() method
    #
    #   Get FILE statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom FILE statement is got
    #   @retval True        Successfully find FILE statement
    #   @retval False       Not able to find FILE statement
    #
    def _GetOptRomFileStatement(self, Obj):
        if not self._IsKeyword("FILE"):
            return False

        FfsFileObj = OptRomFileStatement()

        if not self._IsKeyword("EFI") and not self._IsKeyword(BINARY_FILE_TYPE_BIN):
            raise Warning.Expected("Binary type (EFI/BIN)", self.FileName, self.CurrentLineNumber)
        FfsFileObj.FileType = self._Token

        if not self._GetNextToken():
            raise Warning.Expected("File path", self.FileName, self.CurrentLineNumber)
        FfsFileObj.FileName = self._Token
        if FfsFileObj.FileName.replace(TAB_WORKSPACE, '').find('$') == -1:
            #check for file path
            ErrorCode, ErrorInfo = PathClass(NormPath(FfsFileObj.FileName), GenFdsGlobalVariable.WorkSpaceDir).Validate()
            if ErrorCode != 0:
                EdkLogger.error("GenFds", ErrorCode, ExtraData=ErrorInfo)

        if FfsFileObj.FileType == 'EFI':
            self._GetOptRomOverrides(FfsFileObj)

        Obj.FfsList.append(FfsFileObj)

        return True

    ## _GetCapInFd() method
    #
    #   Get Cap list contained in FD
    #
    #   @param  self        The object pointer
    #   @param  FdName      FD name
    #   @retval CapList     List of Capsule in FD
    #
    def _GetCapInFd (self, FdName):
        CapList = []
        if FdName.upper() in self.Profile.FdDict:
            FdObj = self.Profile.FdDict[FdName.upper()]
            for elementRegion in FdObj.RegionList:
                if elementRegion.RegionType == 'CAPSULE':
                    for elementRegionData in elementRegion.RegionDataList:
                        if elementRegionData.endswith(".cap"):
                            continue
                        if elementRegionData is not None and elementRegionData.upper() not in CapList:
                            CapList.append(elementRegionData.upper())
        return CapList

    ## _GetReferencedFdCapTuple() method
    #
    #   Get FV and FD list referenced by a capsule image
    #
    #   @param  self        The object pointer
    #   @param  CapObj      Capsule section to be searched
    #   @param  RefFdList   referenced FD by section
    #   @param  RefFvList   referenced FV by section
    #
    def _GetReferencedFdCapTuple(self, CapObj, RefFdList = [], RefFvList = []):
        for CapsuleDataObj in CapObj.CapsuleDataList:
            if hasattr(CapsuleDataObj, 'FvName') and CapsuleDataObj.FvName is not None and CapsuleDataObj.FvName.upper() not in RefFvList:
                RefFvList.append (CapsuleDataObj.FvName.upper())
            elif hasattr(CapsuleDataObj, 'FdName') and CapsuleDataObj.FdName is not None and CapsuleDataObj.FdName.upper() not in RefFdList:
                RefFdList.append (CapsuleDataObj.FdName.upper())
            elif CapsuleDataObj.Ffs is not None:
                if isinstance(CapsuleDataObj.Ffs, FileStatement):
                    if CapsuleDataObj.Ffs.FvName is not None and CapsuleDataObj.Ffs.FvName.upper() not in RefFvList:
                        RefFvList.append(CapsuleDataObj.Ffs.FvName.upper())
                    elif CapsuleDataObj.Ffs.FdName is not None and CapsuleDataObj.Ffs.FdName.upper() not in RefFdList:
                        RefFdList.append(CapsuleDataObj.Ffs.FdName.upper())
                    else:
                        self._GetReferencedFdFvTupleFromSection(CapsuleDataObj.Ffs, RefFdList, RefFvList)

    ## _GetFvInFd() method
    #
    #   Get FV list contained in FD
    #
    #   @param  self        The object pointer
    #   @param  FdName      FD name
    #   @retval FvList      list of FV in FD
    #
    def _GetFvInFd (self, FdName):
        FvList = []
        if FdName.upper() in self.Profile.FdDict:
            FdObj = self.Profile.FdDict[FdName.upper()]
            for elementRegion in FdObj.RegionList:
                if elementRegion.RegionType == BINARY_FILE_TYPE_FV:
                    for elementRegionData in elementRegion.RegionDataList:
                        if elementRegionData.endswith(".fv"):
                            continue
                        if elementRegionData is not None and elementRegionData.upper() not in FvList:
                            FvList.append(elementRegionData.upper())
        return FvList

    ## _GetReferencedFdFvTuple() method
    #
    #   Get FD and FV list referenced by a FFS file
    #
    #   @param  self        The object pointer
    #   @param  FfsFile     contains sections to be searched
    #   @param  RefFdList   referenced FD by section
    #   @param  RefFvList   referenced FV by section
    #
    def _GetReferencedFdFvTuple(self, FvObj, RefFdList = [], RefFvList = []):
        for FfsObj in FvObj.FfsList:
            if isinstance(FfsObj, FileStatement):
                if FfsObj.FvName is not None and FfsObj.FvName.upper() not in RefFvList:
                    RefFvList.append(FfsObj.FvName.upper())
                elif FfsObj.FdName is not None and FfsObj.FdName.upper() not in RefFdList:
                    RefFdList.append(FfsObj.FdName.upper())
                else:
                    self._GetReferencedFdFvTupleFromSection(FfsObj, RefFdList, RefFvList)

    ## _GetReferencedFdFvTupleFromSection() method
    #
    #   Get FD and FV list referenced by a FFS section
    #
    #   @param  self        The object pointer
    #   @param  FfsFile     contains sections to be searched
    #   @param  FdList      referenced FD by section
    #   @param  FvList      referenced FV by section
    #
    def _GetReferencedFdFvTupleFromSection(self, FfsFile, FdList = [], FvList = []):
        SectionStack = list(FfsFile.SectionList)
        while SectionStack != []:
            SectionObj = SectionStack.pop()
            if isinstance(SectionObj, FvImageSection):
                if SectionObj.FvName is not None and SectionObj.FvName.upper() not in FvList:
                    FvList.append(SectionObj.FvName.upper())
                if SectionObj.Fv is not None and SectionObj.Fv.UiFvName is not None and SectionObj.Fv.UiFvName.upper() not in FvList:
                    FvList.append(SectionObj.Fv.UiFvName.upper())
                    self._GetReferencedFdFvTuple(SectionObj.Fv, FdList, FvList)

            if isinstance(SectionObj, CompressSection) or isinstance(SectionObj, GuidSection):
                SectionStack.extend(SectionObj.SectionList)

    ## CycleReferenceCheck() method
    #
    #   Check whether cycle reference exists in FDF
    #
    #   @param  self        The object pointer
    #   @retval True        cycle reference exists
    #   @retval False       Not exists cycle reference
    #
    def CycleReferenceCheck(self):
        #
        # Check the cycle between FV and FD image
        #
        MaxLength = len (self.Profile.FvDict)
        for FvName in self.Profile.FvDict:
            LogStr = "\nCycle Reference Checking for FV: %s\n" % FvName
            RefFvStack = set(FvName)
            FdAnalyzedList = set()

            Index = 0
            while RefFvStack and Index < MaxLength:
                Index = Index + 1
                FvNameFromStack = RefFvStack.pop()
                if FvNameFromStack.upper() in self.Profile.FvDict:
                    FvObj = self.Profile.FvDict[FvNameFromStack.upper()]
                else:
                    continue

                RefFdList = []
                RefFvList = []
                self._GetReferencedFdFvTuple(FvObj, RefFdList, RefFvList)

                for RefFdName in RefFdList:
                    if RefFdName in FdAnalyzedList:
                        continue

                    LogStr += "FV %s contains FD %s\n" % (FvNameFromStack, RefFdName)
                    FvInFdList = self._GetFvInFd(RefFdName)
                    if FvInFdList != []:
                        for FvNameInFd in FvInFdList:
                            LogStr += "FD %s contains FV %s\n" % (RefFdName, FvNameInFd)
                            if FvNameInFd not in RefFvStack:
                                RefFvStack.add(FvNameInFd)

                            if FvName in RefFvStack or FvNameFromStack in RefFvStack:
                                EdkLogger.info(LogStr)
                                return True
                    FdAnalyzedList.add(RefFdName)

                for RefFvName in RefFvList:
                    LogStr += "FV %s contains FV %s\n" % (FvNameFromStack, RefFvName)
                    if RefFvName not in RefFvStack:
                        RefFvStack.add(RefFvName)

                    if FvName in RefFvStack or FvNameFromStack in RefFvStack:
                        EdkLogger.info(LogStr)
                        return True

        #
        # Check the cycle between Capsule and FD image
        #
        MaxLength = len (self.Profile.CapsuleDict)
        for CapName in self.Profile.CapsuleDict:
            #
            # Capsule image to be checked.
            #
            LogStr = "\n\n\nCycle Reference Checking for Capsule: %s\n" % CapName
            RefCapStack = {CapName}
            FdAnalyzedList = set()
            FvAnalyzedList = set()

            Index = 0
            while RefCapStack and Index < MaxLength:
                Index = Index + 1
                CapNameFromStack = RefCapStack.pop()
                if CapNameFromStack.upper() in self.Profile.CapsuleDict:
                    CapObj = self.Profile.CapsuleDict[CapNameFromStack.upper()]
                else:
                    continue

                RefFvList = []
                RefFdList = []
                self._GetReferencedFdCapTuple(CapObj, RefFdList, RefFvList)

                FvListLength = 0
                FdListLength = 0
                while FvListLength < len (RefFvList) or FdListLength < len (RefFdList):
                    for RefFdName in RefFdList:
                        if RefFdName in FdAnalyzedList:
                            continue

                        LogStr += "Capsule %s contains FD %s\n" % (CapNameFromStack, RefFdName)
                        for CapNameInFd in self._GetCapInFd(RefFdName):
                            LogStr += "FD %s contains Capsule %s\n" % (RefFdName, CapNameInFd)
                            if CapNameInFd not in RefCapStack:
                                RefCapStack.append(CapNameInFd)

                            if CapName in RefCapStack or CapNameFromStack in RefCapStack:
                                EdkLogger.info(LogStr)
                                return True

                        for FvNameInFd in self._GetFvInFd(RefFdName):
                            LogStr += "FD %s contains FV %s\n" % (RefFdName, FvNameInFd)
                            if FvNameInFd not in RefFvList:
                                RefFvList.append(FvNameInFd)

                        FdAnalyzedList.add(RefFdName)
                    #
                    # the number of the parsed FV and FD image
                    #
                    FvListLength = len (RefFvList)
                    FdListLength = len (RefFdList)
                    for RefFvName in RefFvList:
                        if RefFvName in FvAnalyzedList:
                            continue
                        LogStr += "Capsule %s contains FV %s\n" % (CapNameFromStack, RefFvName)
                        if RefFvName.upper() in self.Profile.FvDict:
                            FvObj = self.Profile.FvDict[RefFvName.upper()]
                        else:
                            continue
                        self._GetReferencedFdFvTuple(FvObj, RefFdList, RefFvList)
                        FvAnalyzedList.add(RefFvName)

        return False

    def GetAllIncludedFile (self):
        global AllIncludeFileList
        return AllIncludeFileList

if __name__ == "__main__":
    import sys
    try:
        test_file = sys.argv[1]
    except IndexError as v:
        print("Usage: %s filename" % sys.argv[0])
        sys.exit(1)

    parser = FdfParser(test_file)
    try:
        parser.ParseFile()
        parser.CycleReferenceCheck()
    except Warning as X:
        print(str(X))
    else:
        print("Success!")

