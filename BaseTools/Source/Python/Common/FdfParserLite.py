## @file
# parse FDF file
#
#  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
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
import re
import Common.LongFilePathOs as os

import CommonDataClass.FdfClass
from Common.LongFilePathSupport import OpenLongFilePath as open

##define T_CHAR_SPACE                ' '
##define T_CHAR_NULL                 '\0'
##define T_CHAR_CR                   '\r'
##define T_CHAR_TAB                  '\t'
##define T_CHAR_LF                   '\n'
##define T_CHAR_SLASH                '/'
##define T_CHAR_BACKSLASH            '\\'
##define T_CHAR_DOUBLE_QUOTE         '\"'
##define T_CHAR_SINGLE_QUOTE         '\''
##define T_CHAR_STAR                 '*'
##define T_CHAR_HASH                 '#'

(T_CHAR_SPACE, T_CHAR_NULL, T_CHAR_CR, T_CHAR_TAB, T_CHAR_LF, T_CHAR_SLASH, \
T_CHAR_BACKSLASH, T_CHAR_DOUBLE_QUOTE, T_CHAR_SINGLE_QUOTE, T_CHAR_STAR, T_CHAR_HASH) = \
(' ', '\0', '\r', '\t', '\n', '/', '\\', '\"', '\'', '*', '#')

SEPERATOR_TUPLE = ('=', '|', ',', '{', '}') 

IncludeFileList = []
# Macro passed from command line, which has greatest priority and can NOT be overridden by those in FDF
InputMacroDict = {}
# All Macro values when parsing file, not replace existing Macro
AllMacroList = []

def GetRealFileLine (File, Line):
    
    InsertedLines = 0
    for Profile in IncludeFileList:
        if Line >= Profile.InsertStartLineNumber and Line < Profile.InsertStartLineNumber + Profile.InsertAdjust + len(Profile.FileLinesList):
            return (Profile.FileName, Line - Profile.InsertStartLineNumber + 1)
        if Line >= Profile.InsertStartLineNumber + Profile.InsertAdjust + len(Profile.FileLinesList):
            InsertedLines += Profile.InsertAdjust + len(Profile.FileLinesList)
    
    return (File, Line - InsertedLines)

## The exception class that used to report error messages when parsing FDF
#
# Currently the "ToolName" is set to be "FDF Parser".
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
        self.message = Str + str(self.LineNumber)
        self.ToolName = 'FDF Parser'
        
## The MACRO class that used to record macro value data when parsing include file
#
#
class MacroProfile :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  FileName    The file that to be parsed
    #
    def __init__(self, FileName, Line):
        self.FileName = FileName        
        self.DefinedAtLine  = Line
        self.MacroName = None
        self.MacroValue = None

## The Include file content class that used to record file data when parsing include file
#
# May raise Exception when opening file.
#
class IncludeFileProfile :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  FileName    The file that to be parsed
    #
    def __init__(self, FileName):
        self.FileName = FileName
        self.FileLinesList = []
        try:
            fsock = open(FileName, "rb", 0)
            try:
                self.FileLinesList = fsock.readlines()
            finally:
                fsock.close()

        except IOError:
            raise Warning("Error when opening file %s" % FileName)
        
        self.InsertStartLineNumber = None
        self.InsertAdjust = 0

## The FDF content class that used to record file data when parsing FDF
#
# May raise Exception when opening file.
#
class FileProfile :
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  FileName    The file that to be parsed
    #
    def __init__(self, FileName):
        self.FileLinesList = []
        try:
            fsock = open(FileName, "rb", 0)
            try:
                self.FileLinesList = fsock.readlines()
            finally:
                fsock.close()

        except IOError:
            raise Warning("Error when opening file %s" % FileName)
        
        self.PcdDict = {}
        self.InfList = []
        
        self.PcdFileLineDict = {}
        self.InfFileLineList = []
        
        self.FdDict = {}
        self.FvDict = {}
        self.CapsuleList = []
#        self.VtfList = []
#        self.RuleDict = {}

## The syntax parser for FDF
#
# PreprocessFile method should be called prior to ParseFile
# CycleReferenceCheck method can detect cycles in FDF contents
#
# GetNext*** procedures mean these procedures will get next token first, then make judgement.
# Get*** procedures mean these procedures will make judgement on current token only.
#        
class FdfParser(object):
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
        self.__Token = ""
        self.__SkippedChars = ""
        
        self.__WipeOffArea = []

    ## __IsWhiteSpace() method
    #
    #   Whether char at current FileBufferPos is whitespace
    #
    #   @param  self        The object pointer
    #   @param  Char        The char to test
    #   @retval True        The char is a kind of white space
    #   @retval False       The char is NOT a kind of white space
    #
    def __IsWhiteSpace(self, Char):
        if Char in (T_CHAR_NULL, T_CHAR_CR, T_CHAR_SPACE, T_CHAR_TAB, T_CHAR_LF):
            return True
        else:
            return False

    ## __SkipWhiteSpace() method
    #
    #   Skip white spaces from current char, return number of chars skipped
    #
    #   @param  self        The object pointer
    #   @retval Count       The number of chars skipped
    #
    def __SkipWhiteSpace(self):
        Count = 0
        while not self.__EndOfFile():
            Count += 1
            if self.__CurrentChar() in (T_CHAR_NULL, T_CHAR_CR, T_CHAR_LF, T_CHAR_SPACE, T_CHAR_TAB):
                self.__SkippedChars += str(self.__CurrentChar())
                self.__GetOneChar()

            else:
                Count = Count - 1
                return Count

    ## __EndOfFile() method
    #
    #   Judge current buffer pos is at file end
    #
    #   @param  self        The object pointer
    #   @retval True        Current File buffer position is at file end
    #   @retval False       Current File buffer position is NOT at file end
    #
    def __EndOfFile(self):
        NumberOfLines = len(self.Profile.FileLinesList)
        SizeOfLastLine = len(self.Profile.FileLinesList[-1])
        if self.CurrentLineNumber == NumberOfLines and self.CurrentOffsetWithinLine >= SizeOfLastLine - 1:
            return True
        elif self.CurrentLineNumber > NumberOfLines:
            return True
        else:
            return False

    ## __EndOfLine() method
    #
    #   Judge current buffer pos is at line end
    #
    #   @param  self        The object pointer
    #   @retval True        Current File buffer position is at line end
    #   @retval False       Current File buffer position is NOT at line end
    #
    def __EndOfLine(self):
        if self.CurrentLineNumber > len(self.Profile.FileLinesList):
            return True
        SizeOfCurrentLine = len(self.Profile.FileLinesList[self.CurrentLineNumber - 1])
        if self.CurrentOffsetWithinLine >= SizeOfCurrentLine:
            return True
        else:
            return False
    
    ## Rewind() method
    #
    #   Reset file data buffer to the initial state
    #
    #   @param  self        The object pointer
    #
    def Rewind(self):
        self.CurrentLineNumber = 1
        self.CurrentOffsetWithinLine = 0
    
    ## __UndoOneChar() method
    #
    #   Go back one char in the file buffer
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully go back one char
    #   @retval False       Not able to go back one char as file beginning reached
    #    
    def __UndoOneChar(self):
        
        if self.CurrentLineNumber == 1 and self.CurrentOffsetWithinLine == 0:
            return False
        elif self.CurrentOffsetWithinLine == 0:
            self.CurrentLineNumber -= 1
            self.CurrentOffsetWithinLine = len(self.__CurrentLine()) - 1
        else:
            self.CurrentOffsetWithinLine -= 1
        return True
        
    ## __GetOneChar() method
    #
    #   Move forward one char in the file buffer
    #
    #   @param  self        The object pointer
    #  
    def __GetOneChar(self):
        if self.CurrentOffsetWithinLine == len(self.Profile.FileLinesList[self.CurrentLineNumber - 1]) - 1:
            self.CurrentLineNumber += 1
            self.CurrentOffsetWithinLine = 0
        else:
            self.CurrentOffsetWithinLine += 1

    ## __CurrentChar() method
    #
    #   Get the char pointed to by the file buffer pointer
    #
    #   @param  self        The object pointer
    #   @retval Char        Current char
    #  
    def __CurrentChar(self):
        return self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine]
    
    ## __NextChar() method
    #
    #   Get the one char pass the char pointed to by the file buffer pointer
    #
    #   @param  self        The object pointer
    #   @retval Char        Next char
    #
    def __NextChar(self):
        if self.CurrentOffsetWithinLine == len(self.Profile.FileLinesList[self.CurrentLineNumber - 1]) - 1:
            return self.Profile.FileLinesList[self.CurrentLineNumber][0]
        else:
            return self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine + 1]
        
    ## __SetCurrentCharValue() method
    #
    #   Modify the value of current char
    #
    #   @param  self        The object pointer
    #   @param  Value       The new value of current char
    #
    def __SetCurrentCharValue(self, Value):
        self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine] = Value
        
    ## __CurrentLine() method
    #
    #   Get the list that contains current line contents
    #
    #   @param  self        The object pointer
    #   @retval List        current line contents
    #
    def __CurrentLine(self):
        return self.Profile.FileLinesList[self.CurrentLineNumber - 1]
        
    def __StringToList(self):
        self.Profile.FileLinesList = [list(s) for s in self.Profile.FileLinesList]
        self.Profile.FileLinesList[-1].append(' ')

    def __ReplaceMacros(self, Str, File, Line):
        MacroEnd = 0
        while Str.find('$(', MacroEnd) >= 0:
            MacroStart = Str.find('$(', MacroEnd)
            if Str.find(')', MacroStart) > 0:
                MacroEnd = Str.find(')', MacroStart)
                Name = Str[MacroStart + 2 : MacroEnd]
                Value = None
                if Name in InputMacroDict:
                    Value = InputMacroDict[Name]
                   
                else:
                    for Profile in AllMacroList:
                        if Profile.FileName == File and Profile.MacroName == Name and Profile.DefinedAtLine <= Line:
                            Value = Profile.MacroValue
                            
                if Value != None:
                    Str = Str.replace('$(' + Name + ')', Value)
                    MacroEnd = MacroStart + len(Value) 
                
            else:
                raise Warning("Macro not complete At Line ", self.FileName, self.CurrentLineNumber)
        return Str
    
    def __ReplaceFragment(self, StartPos, EndPos, Value = ' '):
        if StartPos[0] == EndPos[0]:
            Offset = StartPos[1]
            while Offset <= EndPos[1]:
                self.Profile.FileLinesList[StartPos[0]][Offset] = Value
                Offset += 1
            return
        
        Offset = StartPos[1]
        while self.Profile.FileLinesList[StartPos[0]][Offset] not in ('\r', '\n'):
            self.Profile.FileLinesList[StartPos[0]][Offset] = Value
            Offset += 1
        
        Line = StartPos[0]
        while Line < EndPos[0]:
            Offset = 0
            while self.Profile.FileLinesList[Line][Offset] not in ('\r', '\n'):
                self.Profile.FileLinesList[Line][Offset] = Value
                Offset += 1
            Line += 1
            
        Offset = 0
        while Offset <= EndPos[1]:
            self.Profile.FileLinesList[EndPos[0]][Offset] = Value
            Offset += 1


    def __GetMacroName(self):
        if not self.__GetNextToken():
            raise Warning("expected Macro name", self.FileName, self.CurrentLineNumber)
        MacroName = self.__Token
        NotFlag = False
        if MacroName.startswith('!'):
            NotFlag = True
            MacroName = MacroName[1:].strip()
         
        if not MacroName.startswith('$(') or not MacroName.endswith(')'):
            raise Warning("Macro name expected(Please use '$(%(Token)s)' if '%(Token)s' is a macro.)" % {"Token" : MacroName},
                          self.FileName, self.CurrentLineNumber)
        MacroName = MacroName[2:-1]
        return MacroName, NotFlag
    
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
        
        while not self.__EndOfFile():
            
            if self.__CurrentChar() == T_CHAR_DOUBLE_QUOTE and not InComment:
                InString = not InString
            # meet new line, then no longer in a comment for // and '#'
            if self.__CurrentChar() == T_CHAR_LF:
                self.CurrentLineNumber += 1
                self.CurrentOffsetWithinLine = 0
                if InComment and DoubleSlashComment:
                    InComment = False
                    DoubleSlashComment = False
                if InComment and HashComment:
                    InComment = False
                    HashComment = False
            # check for */ comment end
            elif InComment and not DoubleSlashComment and not HashComment and self.__CurrentChar() == T_CHAR_STAR and self.__NextChar() == T_CHAR_SLASH:
                self.__SetCurrentCharValue(T_CHAR_SPACE)
                self.__GetOneChar()
                self.__SetCurrentCharValue(T_CHAR_SPACE)
                self.__GetOneChar()
                InComment = False
            # set comments to spaces
            elif InComment:
                self.__SetCurrentCharValue(T_CHAR_SPACE)
                self.__GetOneChar()
            # check for // comment
            elif self.__CurrentChar() == T_CHAR_SLASH and self.__NextChar() == T_CHAR_SLASH and not self.__EndOfLine():
                InComment = True
                DoubleSlashComment = True
            # check for '#' comment
            elif self.__CurrentChar() == T_CHAR_HASH and not self.__EndOfLine() and not InString:
                InComment = True
                HashComment = True
            # check for /* comment start
            elif self.__CurrentChar() == T_CHAR_SLASH and self.__NextChar() == T_CHAR_STAR:
                self.__SetCurrentCharValue( T_CHAR_SPACE)
                self.__GetOneChar()
                self.__SetCurrentCharValue( T_CHAR_SPACE)
                self.__GetOneChar()
                InComment = True
            else:
                self.__GetOneChar()
        
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
        
        while self.__GetNextToken():
            
            if self.__Token == '!include':
                IncludeLine = self.CurrentLineNumber
                IncludeOffset = self.CurrentOffsetWithinLine - len('!include')
                if not self.__GetNextToken():
                    raise Warning("expected include file name At Line ", self.FileName, self.CurrentLineNumber)
                IncFileName = self.__Token
                if not os.path.isabs(IncFileName):
                    if IncFileName.startswith('$(WORKSPACE)'):
                        Str = IncFileName.replace('$(WORKSPACE)', os.environ.get('WORKSPACE'))
                        if os.path.exists(Str):
                            if not os.path.isabs(Str):
                                Str = os.path.abspath(Str)
                        IncFileName = Str
                    else:
                        # file is in the same dir with FDF file
                        FullFdf = self.FileName
                        if not os.path.isabs(self.FileName):
                            FullFdf = os.path.join(os.environ.get('WORKSPACE'), self.FileName)
                
                        IncFileName = os.path.join(os.path.dirname(FullFdf), IncFileName)
                    
                if not os.path.exists(os.path.normpath(IncFileName)):
                    raise Warning("Include file not exists At Line ", self.FileName, self.CurrentLineNumber)
                
                IncFileProfile = IncludeFileProfile(os.path.normpath(IncFileName))
                
                CurrentLine = self.CurrentLineNumber
                CurrentOffset = self.CurrentOffsetWithinLine
                # list index of the insertion, note that line number is 'CurrentLine + 1'
                InsertAtLine = CurrentLine
                IncFileProfile.InsertStartLineNumber = InsertAtLine + 1
                # deal with remaining portions after "!include filename", if exists.
                if self.__GetNextToken():
                    if self.CurrentLineNumber == CurrentLine:
                        RemainingLine = self.__CurrentLine()[CurrentOffset:]
                        self.Profile.FileLinesList.insert(self.CurrentLineNumber, RemainingLine)
                        IncFileProfile.InsertAdjust += 1
                        self.CurrentLineNumber += 1
                        self.CurrentOffsetWithinLine = 0
                
                for Line in IncFileProfile.FileLinesList:
                    self.Profile.FileLinesList.insert(InsertAtLine, Line)
                    self.CurrentLineNumber += 1
                    InsertAtLine += 1    
                
                IncludeFileList.append(IncFileProfile)
                
                # comment out the processed include file statement
                TempList = list(self.Profile.FileLinesList[IncludeLine - 1])
                TempList.insert(IncludeOffset, '#')
                self.Profile.FileLinesList[IncludeLine - 1] = ''.join(TempList)
                
        self.Rewind()

    ## PreprocessIncludeFile() method
    #
    #   Preprocess file contents, replace !include statements with file contents.
    #   In the end, rewind the file buffer pointer to the beginning
    #
    #   @param  self        The object pointer
    #   
    def PreprocessConditionalStatement(self):
        # IfList is a stack of if branches with elements of list [Pos, CondSatisfied, BranchDetermined]
        IfList = []
        while self.__GetNextToken():
            if self.__Token == 'DEFINE':
                DefineLine = self.CurrentLineNumber - 1
                DefineOffset = self.CurrentOffsetWithinLine - len('DEFINE')
                if not self.__GetNextToken():
                    raise Warning("expected Macro name At Line ", self.FileName, self.CurrentLineNumber)
                Macro = self.__Token
                if not self.__IsToken( "="):
                    raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
                
                if not self.__GetNextToken():
                    raise Warning("expected value At Line ", self.FileName, self.CurrentLineNumber)
                
                if self.__GetStringData():
                    pass
                Value = self.__Token
                if not Macro in InputMacroDict:
                    FileLineTuple = GetRealFileLine(self.FileName, DefineLine + 1)
                    MacProfile = MacroProfile(FileLineTuple[0], FileLineTuple[1])
                    MacProfile.MacroName = Macro
                    MacProfile.MacroValue = Value
                    AllMacroList.append(MacProfile)
                self.__WipeOffArea.append(((DefineLine, DefineOffset), (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
            
            elif self.__Token in ('!ifdef', '!ifndef', '!if'):
                IfStartPos = (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - len(self.__Token))
                IfList.append([IfStartPos, None, None])
                CondLabel = self.__Token
                
                MacroName, NotFlag = self.__GetMacroName() 
                NotDefineFlag = False
                if CondLabel == '!ifndef':
                    NotDefineFlag = True
                if CondLabel == '!ifdef' or CondLabel == '!ifndef':
                    if NotFlag:
                        raise Warning("'NOT' operation not allowed for Macro name At Line ", self.FileName, self.CurrentLineNumber)
                    
                if CondLabel == '!if':
                        
                    if not self.__GetNextOp():
                        raise Warning("expected !endif At Line ", self.FileName, self.CurrentLineNumber)
                    
                    if self.__Token in ('!=', '==', '>', '<', '>=', '<='):
                        Op = self.__Token
                        if not self.__GetNextToken():
                            raise Warning("expected value At Line ", self.FileName, self.CurrentLineNumber)
                        if self.__GetStringData():
                            pass
                        MacroValue = self.__Token
                        ConditionSatisfied = self.__EvaluateConditional(MacroName, IfList[-1][0][0] + 1, Op, MacroValue)
                        if NotFlag:
                            ConditionSatisfied = not ConditionSatisfied
                        BranchDetermined = ConditionSatisfied
                    else:
                        self.CurrentOffsetWithinLine -= len(self.__Token)                        
                        ConditionSatisfied = self.__EvaluateConditional(MacroName, IfList[-1][0][0] + 1, None, 'Bool')
                        if NotFlag:
                            ConditionSatisfied = not ConditionSatisfied
                        BranchDetermined = ConditionSatisfied
                    IfList[-1] = [IfList[-1][0], ConditionSatisfied, BranchDetermined]
                    if ConditionSatisfied:
                        self.__WipeOffArea.append((IfList[-1][0], (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
                        
                else:
                    ConditionSatisfied = self.__EvaluateConditional(MacroName, IfList[-1][0][0] + 1)
                    if NotDefineFlag:
                        ConditionSatisfied = not ConditionSatisfied
                    BranchDetermined = ConditionSatisfied
                    IfList[-1] = [IfList[-1][0], ConditionSatisfied, BranchDetermined]
                    if ConditionSatisfied:
                        self.__WipeOffArea.append((IfStartPos, (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
                    
            elif self.__Token in ('!elseif', '!else'):
                ElseStartPos = (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - len(self.__Token))
                if len(IfList) <= 0:
                    raise Warning("Missing !if statement At Line ", self.FileName, self.CurrentLineNumber)
                if IfList[-1][1]:
                    IfList[-1] = [ElseStartPos, False, True]
                    self.__WipeOffArea.append((ElseStartPos, (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
                else:
                    self.__WipeOffArea.append((IfList[-1][0], ElseStartPos))
                    IfList[-1] = [ElseStartPos, True, IfList[-1][2]]
                    if self.__Token == '!elseif':
                        MacroName, NotFlag = self.__GetMacroName() 
                        if not self.__GetNextOp():
                            raise Warning("expected !endif At Line ", self.FileName, self.CurrentLineNumber)
                    
                        if self.__Token in ('!=', '==', '>', '<', '>=', '<='):
                            Op = self.__Token
                            if not self.__GetNextToken():
                                raise Warning("expected value At Line ", self.FileName, self.CurrentLineNumber)
                            if self.__GetStringData():
                                pass
                            MacroValue = self.__Token
                            ConditionSatisfied = self.__EvaluateConditional(MacroName, IfList[-1][0][0] + 1, Op, MacroValue)
                            if NotFlag:
                                ConditionSatisfied = not ConditionSatisfied

                        else:
                            self.CurrentOffsetWithinLine -= len(self.__Token)                        
                            ConditionSatisfied = self.__EvaluateConditional(MacroName, IfList[-1][0][0] + 1, None, 'Bool')
                            if NotFlag:
                                ConditionSatisfied = not ConditionSatisfied

                        IfList[-1] = [IfList[-1][0], ConditionSatisfied, IfList[-1][2]]
                    
                    if IfList[-1][1]:
                        if IfList[-1][2]:
                            IfList[-1][1] = False
                        else:
                            IfList[-1][2] = True
                            self.__WipeOffArea.append((IfList[-1][0], (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))

                
            elif self.__Token == '!endif':
                if IfList[-1][1]:
                    self.__WipeOffArea.append(((self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - len('!endif')), (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
                else:
                    self.__WipeOffArea.append((IfList[-1][0], (self.CurrentLineNumber - 1, self.CurrentOffsetWithinLine - 1)))
                
                IfList.pop()
                
                    
        if len(IfList) > 0:
            raise Warning("Missing !endif At Line ", self.FileName, self.CurrentLineNumber)
        self.Rewind()

    def __EvaluateConditional(self, Name, Line, Op = None, Value = None):
        
        FileLineTuple = GetRealFileLine(self.FileName, Line)
        if Name in InputMacroDict:
            MacroValue = InputMacroDict[Name]
            if Op == None:
                if Value == 'Bool' and MacroValue == None or MacroValue.upper() == 'FALSE':
                    return False
                return True
            elif Op == '!=':
                if Value != MacroValue:
                    return True
                else:
                    return False
            elif Op == '==':
                if Value == MacroValue:
                    return True
                else:
                    return False
            else:
                if (self.__IsHex(Value) or Value.isdigit()) and (self.__IsHex(MacroValue) or (MacroValue != None and MacroValue.isdigit())):
                    InputVal = long(Value, 0)
                    MacroVal = long(MacroValue, 0)
                    if Op == '>':
                        if MacroVal > InputVal:
                            return True
                        else:
                            return False
                    elif Op == '>=':
                        if MacroVal >= InputVal:
                            return True
                        else:
                            return False
                    elif Op == '<':
                        if MacroVal < InputVal:
                            return True
                        else:
                            return False
                    elif Op == '<=':
                        if MacroVal <= InputVal:
                            return True
                        else:
                            return False
                    else:
                        return False
                else:
                    raise Warning("Value %s is not a number At Line ", self.FileName, Line)
                
        for Profile in AllMacroList:
            if Profile.FileName == FileLineTuple[0] and Profile.MacroName == Name and Profile.DefinedAtLine <= FileLineTuple[1]:
                if Op == None:
                    if Value == 'Bool' and Profile.MacroValue == None or Profile.MacroValue.upper() == 'FALSE':
                        return False
                    return True
                elif Op == '!=':
                    if Value != Profile.MacroValue:
                        return True
                    else:
                        return False
                elif Op == '==':
                    if Value == Profile.MacroValue:
                        return True
                    else:
                        return False
                else:
                    if (self.__IsHex(Value) or Value.isdigit()) and (self.__IsHex(Profile.MacroValue) or (Profile.MacroValue != None and Profile.MacroValue.isdigit())):
                        InputVal = long(Value, 0)
                        MacroVal = long(Profile.MacroValue, 0)
                        if Op == '>':
                            if MacroVal > InputVal:
                                return True
                            else:
                                return False
                        elif Op == '>=':
                            if MacroVal >= InputVal:
                                return True
                            else:
                                return False
                        elif Op == '<':
                            if MacroVal < InputVal:
                                return True
                            else:
                                return False
                        elif Op == '<=':
                            if MacroVal <= InputVal:
                                return True
                            else:
                                return False
                        else:
                            return False
                    else:
                        raise Warning("Value %s is not a number At Line ", self.FileName, Line)
        
        return False

    ## __IsToken() method
    #
    #   Check whether input string is found from current char position along
    #   If found, the string value is put into self.__Token
    #
    #   @param  self        The object pointer
    #   @param  String      The string to search
    #   @param  IgnoreCase  Indicate case sensitive/non-sensitive search, default is case sensitive
    #   @retval True        Successfully find string, file buffer pointer moved forward
    #   @retval False       Not able to find string, file buffer pointer not changed
    #
    def __IsToken(self, String, IgnoreCase = False):
        self.__SkipWhiteSpace()

        # Only consider the same line, no multi-line token allowed
        StartPos = self.CurrentOffsetWithinLine
        index = -1
        if IgnoreCase:
            index = self.__CurrentLine()[self.CurrentOffsetWithinLine : ].upper().find(String.upper()) 
        else:
            index = self.__CurrentLine()[self.CurrentOffsetWithinLine : ].find(String)
        if index == 0:
            self.CurrentOffsetWithinLine += len(String)
            self.__Token = self.__CurrentLine()[StartPos : self.CurrentOffsetWithinLine]
            return True
        return False

    ## __IsKeyword() method
    #
    #   Check whether input keyword is found from current char position along, whole word only!
    #   If found, the string value is put into self.__Token
    #
    #   @param  self        The object pointer
    #   @param  Keyword     The string to search
    #   @param  IgnoreCase  Indicate case sensitive/non-sensitive search, default is case sensitive
    #   @retval True        Successfully find string, file buffer pointer moved forward
    #   @retval False       Not able to find string, file buffer pointer not changed
    #
    def __IsKeyword(self, KeyWord, IgnoreCase = False):
        self.__SkipWhiteSpace()

        # Only consider the same line, no multi-line token allowed
        StartPos = self.CurrentOffsetWithinLine
        index = -1
        if IgnoreCase:
            index = self.__CurrentLine()[self.CurrentOffsetWithinLine : ].upper().find(KeyWord.upper()) 
        else:
            index = self.__CurrentLine()[self.CurrentOffsetWithinLine : ].find(KeyWord)
        if index == 0:
            followingChar = self.__CurrentLine()[self.CurrentOffsetWithinLine + len(KeyWord)]
            if not str(followingChar).isspace() and followingChar not in SEPERATOR_TUPLE:
                return False
            self.CurrentOffsetWithinLine += len(KeyWord)
            self.__Token = self.__CurrentLine()[StartPos : self.CurrentOffsetWithinLine]
            return True
        return False

    ## __GetNextWord() method
    #
    #   Get next C name from file lines
    #   If found, the string value is put into self.__Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a C name string, file buffer pointer moved forward
    #   @retval False       Not able to find a C name string, file buffer pointer not changed
    #
    def __GetNextWord(self):
        self.__SkipWhiteSpace()
        if self.__EndOfFile():
            return False
        
        TempChar = self.__CurrentChar()
        StartPos = self.CurrentOffsetWithinLine
        if (TempChar >= 'a' and TempChar <= 'z') or (TempChar >= 'A' and TempChar <= 'Z') or TempChar == '_':
            self.__GetOneChar()
            while not self.__EndOfLine():
                TempChar = self.__CurrentChar()
                if (TempChar >= 'a' and TempChar <= 'z') or (TempChar >= 'A' and TempChar <= 'Z') \
                or (TempChar >= '0' and TempChar <= '9') or TempChar == '_' or TempChar == '-':
                    self.__GetOneChar()
                    
                else:
                    break

            self.__Token = self.__CurrentLine()[StartPos : self.CurrentOffsetWithinLine]
            return True
            
        return False
    
    ## __GetNextToken() method
    #
    #   Get next token unit before a seperator
    #   If found, the string value is put into self.__Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a token unit, file buffer pointer moved forward
    #   @retval False       Not able to find a token unit, file buffer pointer not changed
    #
    def __GetNextToken(self):
        # Skip leading spaces, if exist.
        self.__SkipWhiteSpace()
        if self.__EndOfFile():
            return False
        # Record the token start position, the position of the first non-space char.
        StartPos = self.CurrentOffsetWithinLine
        StartLine = self.CurrentLineNumber
        while not self.__EndOfLine():
            TempChar = self.__CurrentChar()
            # Try to find the end char that is not a space and not in seperator tuple.
            # That is, when we got a space or any char in the tuple, we got the end of token.
            if not str(TempChar).isspace() and TempChar not in SEPERATOR_TUPLE:
                self.__GetOneChar()
            # if we happen to meet a seperator as the first char, we must proceed to get it.
            # That is, we get a token that is a seperator char. nomally it is the boundary of other tokens.
            elif StartPos == self.CurrentOffsetWithinLine and TempChar in SEPERATOR_TUPLE:
                self.__GetOneChar()
                break
            else:
                break
#        else:
#            return False

        EndPos = self.CurrentOffsetWithinLine
        if self.CurrentLineNumber != StartLine:
            EndPos = len(self.Profile.FileLinesList[StartLine-1])
        self.__Token = self.Profile.FileLinesList[StartLine-1][StartPos : EndPos]
        if StartPos != self.CurrentOffsetWithinLine:
            return True
        else:
            return False
    
    def __GetNextOp(self):
        # Skip leading spaces, if exist.
        self.__SkipWhiteSpace()
        if self.__EndOfFile():
            return False
        # Record the token start position, the position of the first non-space char.
        StartPos = self.CurrentOffsetWithinLine
        while not self.__EndOfLine():
            TempChar = self.__CurrentChar()
            # Try to find the end char that is not a space
            if not str(TempChar).isspace():
                self.__GetOneChar()
            else:
                break
        else:
            return False
        
        if StartPos != self.CurrentOffsetWithinLine:
            self.__Token = self.__CurrentLine()[StartPos : self.CurrentOffsetWithinLine]
            return True
        else:
            return False
    ## __GetNextGuid() method
    #
    #   Get next token unit before a seperator
    #   If found, the GUID string is put into self.__Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a registry format GUID, file buffer pointer moved forward
    #   @retval False       Not able to find a registry format GUID, file buffer pointer not changed
    #
    def __GetNextGuid(self):
        
        if not self.__GetNextToken():
            return False
        p = re.compile('[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}')
        if p.match(self.__Token) != None:
            return True
        else:
            self.__UndoToken()
            return False

    ## __UndoToken() method
    #
    #   Go back one token unit in file buffer
    #
    #   @param  self        The object pointer
    #
    def __UndoToken(self):
        self.__UndoOneChar()
        while self.__CurrentChar().isspace():
            if not self.__UndoOneChar():
                self.__GetOneChar()
                return
        
        
        StartPos = self.CurrentOffsetWithinLine
        CurrentLine = self.CurrentLineNumber
        while CurrentLine == self.CurrentLineNumber:
            
            TempChar = self.__CurrentChar()
            # Try to find the end char that is not a space and not in seperator tuple.
            # That is, when we got a space or any char in the tuple, we got the end of token.
            if not str(TempChar).isspace() and not TempChar in SEPERATOR_TUPLE:
                if not self.__UndoOneChar():
                    break
            # if we happen to meet a seperator as the first char, we must proceed to get it.
            # That is, we get a token that is a seperator char. nomally it is the boundary of other tokens.
            elif StartPos == self.CurrentOffsetWithinLine and TempChar in SEPERATOR_TUPLE:
                return
            else:
                break
            
        self.__GetOneChar()
    
    ## __HexDigit() method
    #
    #   Whether char input is a Hex data bit
    #
    #   @param  self        The object pointer
    #   @param  TempChar    The char to test
    #   @retval True        The char is a Hex data bit
    #   @retval False       The char is NOT a Hex data bit
    #
    def __HexDigit(self, TempChar):
        if (TempChar >= 'a' and TempChar <= 'f') or (TempChar >= 'A' and TempChar <= 'F') \
                or (TempChar >= '0' and TempChar <= '9'):
                    return True
        else:
            return False
    
    def __IsHex(self, HexStr):
        if not HexStr.upper().startswith("0X"):
            return False
        if len(self.__Token) <= 2:
            return False
        charList = [c for c in HexStr[2 : ] if not self.__HexDigit( c)]
        if len(charList) == 0:
            return True
        else:
            return False    
    ## __GetNextHexNumber() method
    #
    #   Get next HEX data before a seperator
    #   If found, the HEX data is put into self.__Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a HEX data, file buffer pointer moved forward
    #   @retval False       Not able to find a HEX data, file buffer pointer not changed
    #
    def __GetNextHexNumber(self):
        if not self.__GetNextToken():
            return False
        if self.__IsHex(self.__Token):
            return True
        else:
            self.__UndoToken()
            return False
        
    ## __GetNextDecimalNumber() method
    #
    #   Get next decimal data before a seperator
    #   If found, the decimal data is put into self.__Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a decimal data, file buffer pointer moved forward
    #   @retval False       Not able to find a decimal data, file buffer pointer not changed
    #
    def __GetNextDecimalNumber(self):
        if not self.__GetNextToken():
            return False
        if self.__Token.isdigit():
            return True
        else:
            self.__UndoToken()
            return False
    
    ## __GetNextPcdName() method
    #
    #   Get next PCD token space C name and PCD C name pair before a seperator
    #   If found, the decimal data is put into self.__Token
    #
    #   @param  self        The object pointer
    #   @retval Tuple       PCD C name and PCD token space C name pair 
    #
    def __GetNextPcdName(self):
        if not self.__GetNextWord():
            raise Warning("expected PcdTokenSpaceCName.PcdCName At Line ", self.FileName, self.CurrentLineNumber)
        pcdTokenSpaceCName = self.__Token
        
        if not self.__IsToken( "."):
            raise Warning("expected PcdTokenSpaceCName.PcdCName At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__GetNextWord():
            raise Warning("expected PcdTokenSpaceCName.PcdCName At Line ", self.FileName, self.CurrentLineNumber)
        pcdCName = self.__Token
        
        return (pcdCName, pcdTokenSpaceCName) 
            
    ## __GetStringData() method
    #
    #   Get string contents quoted in ""
    #   If found, the decimal data is put into self.__Token
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a string data, file buffer pointer moved forward
    #   @retval False       Not able to find a string data, file buffer pointer not changed
    #    
    def __GetStringData(self):
        if self.__Token.startswith("\"") or self.__Token.startswith("L\""):
            self.__UndoToken()
            self.__SkipToToken("\"")
            currentLineNumber = self.CurrentLineNumber
            
            if not self.__SkipToToken("\""):
                raise Warning("Missing Quote \" for String At Line ", self.FileName, self.CurrentLineNumber)
            if currentLineNumber != self.CurrentLineNumber:
                raise Warning("Missing Quote \" for String At Line ", self.FileName, self.CurrentLineNumber)
            self.__Token = self.__SkippedChars.rstrip('\"')
            return True
        
        elif self.__Token.startswith("\'") or self.__Token.startswith("L\'"):
            self.__UndoToken()
            self.__SkipToToken("\'")
            currentLineNumber = self.CurrentLineNumber
            
            if not self.__SkipToToken("\'"):
                raise Warning("Missing Quote \' for String At Line ", self.FileName, self.CurrentLineNumber)
            if currentLineNumber != self.CurrentLineNumber:
                raise Warning("Missing Quote \' for String At Line ", self.FileName, self.CurrentLineNumber)
            self.__Token = self.__SkippedChars.rstrip('\'')
            return True
        
        else:
            return False
            
    ## __SkipToToken() method
    #
    #   Search forward in file buffer for the string
    #   The skipped chars are put into self.__SkippedChars
    #
    #   @param  self        The object pointer
    #   @param  String      The string to search
    #   @param  IgnoreCase  Indicate case sensitive/non-sensitive search, default is case sensitive
    #   @retval True        Successfully find the string, file buffer pointer moved forward
    #   @retval False       Not able to find the string, file buffer pointer not changed
    #
    def __SkipToToken(self, String, IgnoreCase = False):
        StartPos = self.GetFileBufferPos()
        
        self.__SkippedChars = ""
        while not self.__EndOfFile():
            index = -1
            if IgnoreCase:
                index = self.__CurrentLine()[self.CurrentOffsetWithinLine : ].upper().find(String.upper()) 
            else:
                index = self.__CurrentLine()[self.CurrentOffsetWithinLine : ].find(String)
            if index == 0:
                self.CurrentOffsetWithinLine += len(String)
                self.__SkippedChars += String
                return True
            self.__SkippedChars += str(self.__CurrentChar())
            self.__GetOneChar()
            
        self.SetFileBufferPos( StartPos)
        self.__SkippedChars = ""
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
            
    ## ParseFile() method
    #
    #   Parse the file profile buffer to extract fd, fv ... information
    #   Exception will be raised if syntax error found
    #
    #   @param  self        The object pointer
    #
    def ParseFile(self):

        try:
            self.__StringToList()
            self.PreprocessFile()
            self.PreprocessIncludeFile()
            self.__StringToList()
            self.PreprocessFile()
            self.PreprocessConditionalStatement()
            self.__StringToList()
            for Pos in self.__WipeOffArea:
                self.__ReplaceFragment(Pos[0], Pos[1])
            self.Profile.FileLinesList = ["".join(list) for list in self.Profile.FileLinesList]
            
            while self.__GetDefines():
                pass
            
            Index = 0
            while Index < len(self.Profile.FileLinesList):
                FileLineTuple = GetRealFileLine(self.FileName, Index + 1)
                self.Profile.FileLinesList[Index] = self.__ReplaceMacros(self.Profile.FileLinesList[Index], FileLineTuple[0], FileLineTuple[1])
                Index += 1
                
            while self.__GetFd():
                pass

            while self.__GetFv():
                pass

            while self.__GetCapsule():
                pass

#            while self.__GetVtf():
#                pass
#
#            while self.__GetRule():
#                pass
            

        except Warning, X:
            self.__UndoToken()
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            X.message += '\nGot Token: \"%s\" from File %s\n' % (self.__Token, FileLineTuple[0]) + \
                'Previous Token: \"%s\" At line: %d, Offset Within Line: %d\n' \
                    % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine :].rstrip('\n').rstrip('\r'), FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise

    ## __GetDefines() method
    #
    #   Get Defines section contents and store its data into AllMacrosList
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a Defines
    #   @retval False       Not able to find a Defines
    #
    def __GetDefines(self):

        if not self.__GetNextToken():
            return False

        S = self.__Token.upper()
        if S.startswith("[") and not S.startswith("[DEFINES"):
            if not S.startswith("[FD.") and not S.startswith("[FV.") and not S.startswith("[CAPSULE.") \
                and not S.startswith("[VTF.") and not S.startswith("[RULE.") and not S.startswith("[OPTIONROM."):
                raise Warning("Unknown section or section appear sequence error (The correct sequence should be [DEFINES], [FD.], [FV.], [Capsule.], [VTF.], [Rule.], [OptionRom.])", self.FileName, self.CurrentLineNumber)
            self.__UndoToken()
            return False

        self.__UndoToken()
        if not self.__IsToken("[DEFINES", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            #print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
            #        % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine :], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning("expected [DEFINES", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken( "]"):
            raise Warning("expected ']'", self.FileName, self.CurrentLineNumber)

        while self.__GetNextWord():
            Macro = self.__Token
            
            if not self.__IsToken("="):
                raise Warning("expected '='", self.FileName, self.CurrentLineNumber)
            if not self.__GetNextToken() or self.__Token.startswith('['):
                raise Warning("expected MACRO value", self.FileName, self.CurrentLineNumber)
            Value = self.__Token
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            MacProfile = MacroProfile(FileLineTuple[0], FileLineTuple[1])
            MacProfile.MacroName = Macro
            MacProfile.MacroValue = Value
            AllMacroList.append(MacProfile)

        return False

    ## __GetFd() method
    #
    #   Get FD section contents and store its data into FD dictionary of self.Profile
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a FD
    #   @retval False       Not able to find a FD
    #
    def __GetFd(self):

        if not self.__GetNextToken():
            return False
        
        S = self.__Token.upper()
        if S.startswith("[") and not S.startswith("[FD."):
            if not S.startswith("[FV.") and not S.startswith("[CAPSULE.") \
                and not S.startswith("[VTF.") and not S.startswith("[RULE."):
                raise Warning("Unknown section At Line ", self.FileName, self.CurrentLineNumber)
            self.__UndoToken()
            return False
        
        self.__UndoToken()
        if not self.__IsToken("[FD.", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
                    % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine :], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning("expected [FD.] At Line ", self.FileName, self.CurrentLineNumber)
        
        FdName = self.__GetUiName()
        self.CurrentFdName = FdName.upper()
        
        if not self.__IsToken( "]"):
            raise Warning("expected ']' At Line ", self.FileName, self.CurrentLineNumber)
        
        FdObj = CommonDataClass.FdfClass.FDClassObject()
        FdObj.FdUiName = self.CurrentFdName
        self.Profile.FdDict[self.CurrentFdName] = FdObj
        Status = self.__GetCreateFile(FdObj)
        if not Status:
            raise Warning("FD name error At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__GetTokenStatements(FdObj):
            return False
        
        self.__GetDefineStatements(FdObj)

        self.__GetSetStatements(FdObj)

        if not self.__GetRegionLayout(FdObj):
            raise Warning("expected region layout At Line ", self.FileName, self.CurrentLineNumber)
            
        while self.__GetRegionLayout(FdObj):
            pass
        return True
    
    ## __GetUiName() method
    #
    #   Return the UI name of a section
    #
    #   @param  self        The object pointer
    #   @retval FdName      UI name
    #
    def __GetUiName(self):
        FdName = ""
        if self.__GetNextWord():
            FdName = self.__Token
            
        return FdName

    ## __GetCreateFile() method
    #
    #   Return the output file name of object
    #
    #   @param  self        The object pointer
    #   @param  Obj         object whose data will be stored in file
    #   @retval FdName      UI name
    #
    def __GetCreateFile(self, Obj):

        if self.__IsKeyword( "CREATE_FILE"):
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
                
            if not self.__GetNextToken():
                raise Warning("expected file name At Line ", self.FileName, self.CurrentLineNumber)
                
            FileName = self.__Token
            Obj.CreateFileName = FileName

        return True

    ## __GetTokenStatements() method
    #
    #   Get token statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom token statement is got
    #   @retval True        Successfully find a token statement
    #   @retval False       Not able to find a token statement
    #
    def __GetTokenStatements(self, Obj):
        if not self.__IsKeyword( "BaseAddress"):
            raise Warning("BaseAddress missing At Line ", self.FileName, self.CurrentLineNumber)
           
        if not self.__IsToken( "="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
        if not self.__GetNextHexNumber():
            raise Warning("expected Hex base address At Line ", self.FileName, self.CurrentLineNumber)
            
        Obj.BaseAddress = self.__Token
        
        if self.__IsToken( "|"):
            pcdPair = self.__GetNextPcdName()
            Obj.BaseAddressPcd = pcdPair
            self.Profile.PcdDict[pcdPair] = long(Obj.BaseAddress, 0)
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            self.Profile.PcdFileLineDict[pcdPair] = FileLineTuple
            
        if not self.__IsKeyword( "Size"):
            raise Warning("Size missing At Line ", self.FileName, self.CurrentLineNumber)
            
        if not self.__IsToken( "="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
        if not self.__GetNextHexNumber():
            raise Warning("expected Hex size At Line ", self.FileName, self.CurrentLineNumber)
            
      
        Obj.Size = long(self.__Token, 0)

        if self.__IsToken( "|"):
            pcdPair = self.__GetNextPcdName()
            Obj.SizePcd = pcdPair
            self.Profile.PcdDict[pcdPair] = Obj.Size
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            self.Profile.PcdFileLineDict[pcdPair] = FileLineTuple
                    
        if not self.__IsKeyword( "ErasePolarity"):
            raise Warning("ErasePolarity missing At Line ", self.FileName, self.CurrentLineNumber)
           
        if not self.__IsToken( "="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
        if not self.__GetNextToken():
            raise Warning("expected Erase Polarity At Line ", self.FileName, self.CurrentLineNumber)
            
        if self.__Token != "1" and self.__Token != "0":
            raise Warning("expected 1 or 0 Erase Polarity At Line ", self.FileName, self.CurrentLineNumber)
            
        Obj.ErasePolarity = self.__Token

        Status = self.__GetBlockStatements(Obj)
        return Status
    
    ## __GetAddressStatements() method
    #
    #   Get address statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom address statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def __GetAddressStatements(self, Obj):
        
        if self.__IsKeyword("BsBaseAddress"):
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
            if not self.__GetNextDecimalNumber() and not self.__GetNextHexNumber():
                raise Warning("expected address At Line ", self.FileName, self.CurrentLineNumber)
                
            BsAddress = long(self.__Token, 0)
            Obj.BsBaseAddress = BsAddress
            
        if self.__IsKeyword("RtBaseAddress"):
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
            if not self.__GetNextDecimalNumber() and not self.__GetNextHexNumber():
                raise Warning("expected address At Line ", self.FileName, self.CurrentLineNumber)
                
            RtAddress = long(self.__Token, 0)
            Obj.RtBaseAddress = RtAddress
    
    ## __GetBlockStatements() method
    #
    #   Get block statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom block statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def __GetBlockStatements(self, Obj):
        
        if not self.__GetBlockStatement(Obj):
            #set default block size is 1
            Obj.BlockSizeList.append((1, Obj.Size, None))
            return True

        while self.__GetBlockStatement(Obj):
            pass
        
        for Item in Obj.BlockSizeList:
            if Item[0] == None or Item[1] == None:
                raise Warning("expected block statement for Fd Section", self.FileName, self.CurrentLineNumber)

        return True
    
    ## __GetBlockStatement() method
    #
    #   Get block statement
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom block statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def __GetBlockStatement(self, Obj):
        if not self.__IsKeyword( "BlockSize"):
            return False
        
        if not self.__IsToken( "="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
        if not self.__GetNextHexNumber() and not self.__GetNextDecimalNumber():
            raise Warning("expected Hex block size At Line ", self.FileName, self.CurrentLineNumber)

        BlockSize = long(self.__Token, 0)
        BlockSizePcd = None
        if self.__IsToken( "|"):
            PcdPair = self.__GetNextPcdName()
            BlockSizePcd = PcdPair
            self.Profile.PcdDict[PcdPair] = BlockSize
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            self.Profile.PcdFileLineDict[pcdPair] = FileLineTuple
            
        BlockNumber = None
        if self.__IsKeyword( "NumBlocks"):
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
                
            if not self.__GetNextDecimalNumber() and not self.__GetNextHexNumber():
                raise Warning("expected block numbers At Line ", self.FileName, self.CurrentLineNumber)
                
            BlockNumber = long(self.__Token, 0)
        
        Obj.BlockSizeList.append((BlockSize, BlockNumber, BlockSizePcd))
        return True

    ## __GetDefineStatements() method
    #
    #   Get define statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom define statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def __GetDefineStatements(self, Obj):
        while self.__GetDefineStatement( Obj):
            pass
    
    ## __GetDefineStatement() method
    #
    #   Get define statement
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom define statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def __GetDefineStatement(self, Obj):
        if self.__IsKeyword("DEFINE"):
            self.__GetNextToken()
            Macro = self.__Token
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
                
            if not self.__GetNextToken():
                raise Warning("expected value At Line ", self.FileName, self.CurrentLineNumber)

            Value = self.__Token
            Macro = '$(' + Macro + ')'
            Obj.DefineVarDict[Macro] = Value
            return True
        
        return False
    
    ## __GetSetStatements() method
    #
    #   Get set statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom set statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def __GetSetStatements(self, Obj):
        while self.__GetSetStatement(Obj):
            pass

    ## __GetSetStatement() method
    #
    #   Get set statement
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom set statement is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def __GetSetStatement(self, Obj):
        if self.__IsKeyword("SET"):
            PcdPair = self.__GetNextPcdName()
            
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
                
            if not self.__GetNextToken():
                raise Warning("expected value At Line ", self.FileName, self.CurrentLineNumber)
                
            Value = self.__Token
            if Value.startswith("{"):
                # deal with value with {}
                if not self.__SkipToToken( "}"):
                    raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
                Value += self.__SkippedChars
                
            Obj.SetVarDict[PcdPair] = Value
            self.Profile.PcdDict[PcdPair] = Value
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            self.Profile.PcdFileLineDict[PcdPair] = FileLineTuple
            return True

        return False

    ## __GetRegionLayout() method
    #
    #   Get region layout for FD
    #
    #   @param  self        The object pointer
    #   @param  Fd          for whom region is got
    #   @retval True        Successfully find
    #   @retval False       Not able to find
    #
    def __GetRegionLayout(self, Fd):
        if not self.__GetNextHexNumber():
            return False
        
        RegionObj = CommonDataClass.FdfClass.RegionClassObject()
        RegionObj.Offset = long(self.__Token, 0)
        Fd.RegionList.append(RegionObj)
        
        if not self.__IsToken( "|"):
            raise Warning("expected '|' At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__GetNextHexNumber():
            raise Warning("expected Region Size At Line ", self.FileName, self.CurrentLineNumber)
        RegionObj.Size = long(self.__Token, 0)
        
        if not self.__GetNextWord():
            return True
        
        if not self.__Token in ("SET", "FV", "FILE", "DATA", "CAPSULE"):
            self.__UndoToken()
            RegionObj.PcdOffset = self.__GetNextPcdName()
            self.Profile.PcdDict[RegionObj.PcdOffset] = RegionObj.Offset + long(Fd.BaseAddress, 0)
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            self.Profile.PcdFileLineDict[RegionObj.PcdOffset] = FileLineTuple
            if self.__IsToken( "|"):
                RegionObj.PcdSize = self.__GetNextPcdName()
                self.Profile.PcdDict[RegionObj.PcdSize] = RegionObj.Size
                FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
                self.Profile.PcdFileLineDict[RegionObj.PcdSize] = FileLineTuple
            
            if not self.__GetNextWord():
                return True

        if self.__Token == "SET":
            self.__UndoToken()
            self.__GetSetStatements( RegionObj)
            if not self.__GetNextWord():
                return True
            
        elif self.__Token == "FV":
            self.__UndoToken()
            self.__GetRegionFvType( RegionObj)

        elif self.__Token == "CAPSULE":
            self.__UndoToken()
            self.__GetRegionCapType( RegionObj)

        elif self.__Token == "FILE":
            self.__UndoToken()
            self.__GetRegionFileType( RegionObj)

        else:
            self.__UndoToken()
            self.__GetRegionDataType( RegionObj)
            
        return True
            
    ## __GetRegionFvType() method
    #
    #   Get region fv data for region
    #
    #   @param  self        The object pointer
    #   @param  RegionObj   for whom region data is got
    #
    def __GetRegionFvType(self, RegionObj):

        if not self.__IsKeyword( "FV"):
            raise Warning("expected Keyword 'FV' At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__IsToken( "="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__GetNextToken():
            raise Warning("expected FV name At Line ", self.FileName, self.CurrentLineNumber)
        
        RegionObj.RegionType = "FV"
        RegionObj.RegionDataList.append(self.__Token)
        
        while self.__IsKeyword( "FV"):
        
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
        
            if not self.__GetNextToken():
                raise Warning("expected FV name At Line ", self.FileName, self.CurrentLineNumber)
        
            RegionObj.RegionDataList.append(self.__Token)

    ## __GetRegionCapType() method
    #
    #   Get region capsule data for region
    #
    #   @param  self        The object pointer
    #   @param  RegionObj   for whom region data is got
    #
    def __GetRegionCapType(self, RegionObj):

        if not self.__IsKeyword("CAPSULE"):
            raise Warning("expected Keyword 'CAPSULE' at line", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken("="):
            raise Warning("expected '=' at line", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextToken():
            raise Warning("expected CAPSULE name at line", self.FileName, self.CurrentLineNumber)

        RegionObj.RegionType = "CAPSULE"
        RegionObj.RegionDataList.append(self.__Token)

        while self.__IsKeyword("CAPSULE"):

            if not self.__IsToken("="):
                raise Warning("expected '=' at line", self.FileName, self.CurrentLineNumber)

            if not self.__GetNextToken():
                raise Warning("expected CAPSULE name at line", self.FileName, self.CurrentLineNumber)

            RegionObj.RegionDataList.append(self.__Token)

    ## __GetRegionFileType() method
    #
    #   Get region file data for region
    #
    #   @param  self        The object pointer
    #   @param  RegionObj   for whom region data is got
    #
    def __GetRegionFileType(self, RegionObj):

        if not self.__IsKeyword( "FILE"):
            raise Warning("expected Keyword 'FILE' At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken( "="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextToken():
            raise Warning("expected File name At Line ", self.FileName, self.CurrentLineNumber)

        RegionObj.RegionType = "FILE"
        RegionObj.RegionDataList.append( self.__Token)
        
        while self.__IsKeyword( "FILE"):
        
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
        
            if not self.__GetNextToken():
                raise Warning("expected FILE name At Line ", self.FileName, self.CurrentLineNumber)
        
            RegionObj.RegionDataList.append(self.__Token)

    ## __GetRegionDataType() method
    #
    #   Get region array data for region
    #
    #   @param  self        The object pointer
    #   @param  RegionObj   for whom region data is got
    #
    def __GetRegionDataType(self, RegionObj):
        
        if not self.__IsKeyword( "DATA"):
            raise Warning("expected Region Data type At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken( "="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__IsToken( "{"):
            raise Warning("expected '{' At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__GetNextHexNumber():
            raise Warning("expected Hex byte At Line ", self.FileName, self.CurrentLineNumber)
        
        if len(self.__Token) > 18:
            raise Warning("Hex string can't be converted to a valid UINT64 value", self.FileName, self.CurrentLineNumber)
        
        DataString = self.__Token
        DataString += ","
        
        while self.__IsToken(","):
            if not self.__GetNextHexNumber():
                raise Warning("Invalid Hex number At Line ", self.FileName, self.CurrentLineNumber)
            if len(self.__Token) > 4:
                raise Warning("Hex byte(must be 2 digits) too long At Line ", self.FileName, self.CurrentLineNumber)
            DataString += self.__Token
            DataString += ","
            
        if not self.__IsToken( "}"):
            raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
        
        DataString = DataString.rstrip(",")
        RegionObj.RegionType = "DATA"
        RegionObj.RegionDataList.append( DataString)
        
        while self.__IsKeyword( "DATA"):

            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
        
            if not self.__IsToken( "{"):
                raise Warning("expected '{' At Line ", self.FileName, self.CurrentLineNumber)
        
            if not self.__GetNextHexNumber():
                raise Warning("expected Hex byte At Line ", self.FileName, self.CurrentLineNumber)
        
            if len(self.__Token) > 18:
                raise Warning("Hex string can't be converted to a valid UINT64 value", self.FileName, self.CurrentLineNumber)
        
            DataString = self.__Token
            DataString += ","
        
            while self.__IsToken(","):
                self.__GetNextHexNumber()
                if len(self.__Token) > 4:
                    raise Warning("Hex byte(must be 2 digits) too long At Line ", self.FileName, self.CurrentLineNumber)
                DataString += self.__Token
                DataString += ","
            
            if not self.__IsToken( "}"):
                raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
        
            DataString = DataString.rstrip(",")
            RegionObj.RegionDataList.append( DataString)
    
    ## __GetFv() method
    #
    #   Get FV section contents and store its data into FV dictionary of self.Profile
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a FV
    #   @retval False       Not able to find a FV
    #    
    def __GetFv(self):
        if not self.__GetNextToken():
            return False

        S = self.__Token.upper()
        if S.startswith("[") and not S.startswith("[FV."):
            if not S.startswith("[CAPSULE.") \
                and not S.startswith("[VTF.") and not S.startswith("[RULE."):
                raise Warning("Unknown section or section appear sequence error \n(The correct sequence should be [FD.], [FV.], [Capsule.], [VTF.], [Rule.]) At Line ", self.FileName, self.CurrentLineNumber)
            self.__UndoToken()
            return False

        self.__UndoToken()
        if not self.__IsToken("[FV.", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
                    % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine :], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning("Unknown Keyword At Line ", self.FileName, self.CurrentLineNumber)
        
        FvName = self.__GetUiName()
        self.CurrentFvName = FvName.upper()
        
        if not self.__IsToken( "]"):
            raise Warning("expected ']' At Line ", self.FileName, self.CurrentLineNumber)
        
        FvObj = CommonDataClass.FdfClass.FvClassObject()
        FvObj.UiFvName = self.CurrentFvName
        self.Profile.FvDict[self.CurrentFvName] = FvObj
        
        Status = self.__GetCreateFile(FvObj)
        if not Status:
            raise Warning("FV name error At Line ", self.FileName, self.CurrentLineNumber)

        self.__GetDefineStatements(FvObj)

        self.__GetAddressStatements(FvObj)
        
        self.__GetBlockStatement(FvObj)

        self.__GetSetStatements(FvObj)

        self.__GetFvAlignment(FvObj)

        self.__GetFvAttributes(FvObj)
        
        self.__GetFvNameGuid(FvObj)

        self.__GetAprioriSection(FvObj, FvObj.DefineVarDict.copy())
        self.__GetAprioriSection(FvObj, FvObj.DefineVarDict.copy())
        
        while True:
            isInf = self.__GetInfStatement(FvObj, MacroDict = FvObj.DefineVarDict.copy())
            isFile = self.__GetFileStatement(FvObj, MacroDict = FvObj.DefineVarDict.copy())
            if not isInf and not isFile:
                break
        
        return True

    ## __GetFvAlignment() method
    #
    #   Get alignment for FV
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom alignment is got
    #   @retval True        Successfully find a alignment statement
    #   @retval False       Not able to find a alignment statement
    #
    def __GetFvAlignment(self, Obj):
        
        if not self.__IsKeyword( "FvAlignment"):
            return False
        
        if not self.__IsToken( "="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__GetNextToken():
            raise Warning("expected alignment value At Line ", self.FileName, self.CurrentLineNumber)
        
        if self.__Token.upper() not in ("1", "2", "4", "8", "16", "32", "64", "128", "256", "512", \
                                        "1K", "2K", "4K", "8K", "16K", "32K", "64K", "128K", "256K", "512K", \
                                        "1M", "2M", "4M", "8M", "16M", "32M", "64M", "128M", "256M", "512M", \
                                        "1G", "2G"):
            raise Warning("Unknown alignment value At Line ", self.FileName, self.CurrentLineNumber)
        Obj.FvAlignment = self.__Token
        return True
    
    ## __GetFvAttributes() method
    #
    #   Get attributes for FV
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom attribute is got
    #   @retval None
    #
    def __GetFvAttributes(self, FvObj):
        
        while self.__GetNextWord():
            name = self.__Token
            if name not in ("ERASE_POLARITY", "MEMORY_MAPPED", \
                           "STICKY_WRITE", "LOCK_CAP", "LOCK_STATUS", "WRITE_ENABLED_CAP", \
                           "WRITE_DISABLED_CAP", "WRITE_STATUS", "READ_ENABLED_CAP", \
                           "READ_DISABLED_CAP", "READ_STATUS", "READ_LOCK_CAP", \
                           "READ_LOCK_STATUS", "WRITE_LOCK_CAP", "WRITE_LOCK_STATUS", \
                           "WRITE_POLICY_RELIABLE"):
                self.__UndoToken()
                return

            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
            if not self.__GetNextToken() or self.__Token.upper() not in ("TRUE", "FALSE", "1", "0"):
                raise Warning("expected TRUE/FALSE (1/0) At Line ", self.FileName, self.CurrentLineNumber)
            
            FvObj.FvAttributeDict[name] = self.__Token

        return
    
    ## __GetFvNameGuid() method
    #
    #   Get FV GUID for FV
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom GUID is got
    #   @retval None
    #
    def __GetFvNameGuid(self, FvObj):

        if not self.__IsKeyword( "FvNameGuid"):
            return

        if not self.__IsToken( "="):
            raise Warning("expected '='", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextGuid():
            raise Warning("expected FV GUID value", self.FileName, self.CurrentLineNumber)

        FvObj.FvNameGuid = self.__Token

        return

    ## __GetAprioriSection() method
    #
    #   Get token statements
    #
    #   @param  self        The object pointer
    #   @param  FvObj       for whom apriori is got
    #   @param  MacroDict   dictionary used to replace macro
    #   @retval True        Successfully find apriori statement
    #   @retval False       Not able to find apriori statement
    #
    def __GetAprioriSection(self, FvObj, MacroDict = {}):
        
        if not self.__IsKeyword( "APRIORI"):
            return False
        
        if not self.__IsKeyword("PEI") and not self.__IsKeyword("DXE"):
            raise Warning("expected Apriori file type At Line ", self.FileName, self.CurrentLineNumber)
        AprType = self.__Token
        
        if not self.__IsToken( "{"):
            raise Warning("expected '{' At Line ", self.FileName, self.CurrentLineNumber)
        
        AprSectionObj = CommonDataClass.FdfClass.AprioriSectionClassObject()
        AprSectionObj.AprioriType = AprType
        
        self.__GetDefineStatements(AprSectionObj)
        MacroDict.update(AprSectionObj.DefineVarDict)
        
        while True:
            IsInf = self.__GetInfStatement( AprSectionObj, MacroDict = MacroDict)
            IsFile = self.__GetFileStatement( AprSectionObj)
            if not IsInf and not IsFile:
                break
        
        if not self.__IsToken( "}"):
            raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)

        FvObj.AprioriSectionList.append(AprSectionObj)
        return True

    ## __GetInfStatement() method
    #
    #   Get INF statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom inf statement is got
    #   @param  MacroDict   dictionary used to replace macro
    #   @retval True        Successfully find inf statement
    #   @retval False       Not able to find inf statement
    #
    def __GetInfStatement(self, Obj, ForCapsule = False, MacroDict = {}):

        if not self.__IsKeyword( "INF"):
            return False
        
        ffsInf = CommonDataClass.FdfClass.FfsInfStatementClassObject()
        self.__GetInfOptions( ffsInf)
        
        if not self.__GetNextToken():
            raise Warning("expected INF file path At Line ", self.FileName, self.CurrentLineNumber)
        ffsInf.InfFileName = self.__Token
        
#        if ffsInf.InfFileName.find('$') >= 0:
#            ffsInf.InfFileName = GenFdsGlobalVariable.GenFdsGlobalVariable.MacroExtend(ffsInf.InfFileName, MacroDict)
            
        if not ffsInf.InfFileName in self.Profile.InfList:
            self.Profile.InfList.append(ffsInf.InfFileName)
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            self.Profile.InfFileLineList.append(FileLineTuple)
        
        if self.__IsToken('|'):
            if self.__IsKeyword('RELOCS_STRIPPED'):
                ffsInf.KeepReloc = False
            elif self.__IsKeyword('RELOCS_RETAINED'):
                ffsInf.KeepReloc = True
            else:
                raise Warning("Unknown reloc strip flag At Line ", self.FileName, self.CurrentLineNumber)
        
        if ForCapsule:
            capsuleFfs = CapsuleData.CapsuleFfs()
            capsuleFfs.Ffs = ffsInf
            Obj.CapsuleDataList.append(capsuleFfs)
        else:
            Obj.FfsList.append(ffsInf)
        return True
    
    ## __GetInfOptions() method
    #
    #   Get options for INF
    #
    #   @param  self        The object pointer
    #   @param  FfsInfObj   for whom option is got
    #
    def __GetInfOptions(self, FfsInfObj):
        
        if self.__IsKeyword( "RuleOverride"):
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            if not self.__GetNextToken():
                raise Warning("expected Rule name At Line ", self.FileName, self.CurrentLineNumber)
            FfsInfObj.Rule = self.__Token
            
        if self.__IsKeyword( "VERSION"):
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            if not self.__GetNextToken():
                raise Warning("expected Version At Line ", self.FileName, self.CurrentLineNumber)
            
            if self.__GetStringData():
                FfsInfObj.Version = self.__Token
        
        if self.__IsKeyword( "UI"):
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            if not self.__GetNextToken():
                raise Warning("expected UI name At Line ", self.FileName, self.CurrentLineNumber)
            
            if self.__GetStringData():
                FfsInfObj.Ui = self.__Token

        if self.__IsKeyword( "USE"):
            if not self.__IsToken( "="):
                raise Warning("expected '='", self.FileName, self.CurrentLineNumber)
            if not self.__GetNextToken():
                raise Warning("expected ARCH name", self.FileName, self.CurrentLineNumber)
            FfsInfObj.UseArch = self.__Token

                
        if self.__GetNextToken():
            p = re.compile(r'([a-zA-Z0-9\-]+|\$\(TARGET\)|\*)_([a-zA-Z0-9\-]+|\$\(TOOL_CHAIN_TAG\)|\*)_([a-zA-Z0-9\-]+|\$\(ARCH\)|\*)')
            if p.match(self.__Token):
                FfsInfObj.KeyStringList.append(self.__Token)
                if not self.__IsToken(","):
                    return
            else:
                self.__UndoToken()
                return
                
            while self.__GetNextToken():
                if not p.match(self.__Token):
                    raise Warning("expected KeyString \"Target_Tag_Arch\" At Line ", self.FileName, self.CurrentLineNumber)
                FfsInfObj.KeyStringList.append(self.__Token)
    
                if not self.__IsToken(","):
                    break
                
    ## __GetFileStatement() method
    #
    #   Get FILE statements
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom FILE statement is got
    #   @param  MacroDict   dictionary used to replace macro
    #   @retval True        Successfully find FILE statement
    #   @retval False       Not able to find FILE statement
    #
    def __GetFileStatement(self, Obj, ForCapsule = False, MacroDict = {}):

        if not self.__IsKeyword( "FILE"):
            return False
        
        FfsFileObj = CommonDataClass.FdfClass.FileStatementClassObject()
        
        if not self.__GetNextWord():
            raise Warning("expected FFS type At Line ", self.FileName, self.CurrentLineNumber)
        FfsFileObj.FvFileType = self.__Token
        
        if not self.__IsToken( "="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextGuid():
            if not self.__GetNextWord():
                raise Warning("expected File GUID", self.FileName, self.CurrentLineNumber)
            if self.__Token == 'PCD':
                if not self.__IsToken( "("):
                    raise Warning("expected '('", self.FileName, self.CurrentLineNumber)
                PcdPair = self.__GetNextPcdName()
                if not self.__IsToken( ")"):
                    raise Warning("expected ')'", self.FileName, self.CurrentLineNumber)
                self.__Token = 'PCD('+PcdPair[1]+'.'+PcdPair[0]+')'
                
        FfsFileObj.NameGuid = self.__Token
    
        self.__GetFilePart( FfsFileObj, MacroDict.copy())
        
        if ForCapsule:
            capsuleFfs = CapsuleData.CapsuleFfs()
            capsuleFfs.Ffs = FfsFileObj
            Obj.CapsuleDataList.append(capsuleFfs)
        else:
            Obj.FfsList.append(FfsFileObj)
                
        return True
    
    ## __FileCouldHaveRelocFlag() method
    #
    #   Check whether reloc strip flag can be set for a file type.
    #
    #   @param  self        The object pointer
    #   @param  FileType    The file type to check with
    #   @retval True        This type could have relocation strip flag
    #   @retval False       No way to have it
    #
    
    def __FileCouldHaveRelocFlag (self, FileType):
        if FileType in ('SEC', 'PEI_CORE', 'PEIM', 'PEI_DXE_COMBO'):
            return True
        else:
            return False
    
    ## __SectionCouldHaveRelocFlag() method
    #
    #   Check whether reloc strip flag can be set for a section type.
    #
    #   @param  self        The object pointer
    #   @param  SectionType The section type to check with
    #   @retval True        This type could have relocation strip flag
    #   @retval False       No way to have it
    #
    
    def __SectionCouldHaveRelocFlag (self, SectionType):
        if SectionType in ('TE', 'PE32'):
            return True
        else:
            return False
        
    ## __GetFilePart() method
    #
    #   Get components for FILE statement
    #
    #   @param  self        The object pointer
    #   @param  FfsFileObj   for whom component is got
    #   @param  MacroDict   dictionary used to replace macro
    #
    def __GetFilePart(self, FfsFileObj, MacroDict = {}):
        
        self.__GetFileOpts( FfsFileObj)
        
        if not self.__IsToken("{"):
#            if self.__IsKeyword('RELOCS_STRIPPED') or self.__IsKeyword('RELOCS_RETAINED'):
#                if self.__FileCouldHaveRelocFlag(FfsFileObj.FvFileType):
#                    if self.__Token == 'RELOCS_STRIPPED':
#                        FfsFileObj.KeepReloc = False
#                    else:
#                        FfsFileObj.KeepReloc = True
#                else:
#                    raise Warning("File type %s could not have reloc strip flag At Line %d" % (FfsFileObj.FvFileType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)
#            
#            if not self.__IsToken("{"):
            raise Warning("expected '{' At Line ", self.FileName, self.CurrentLineNumber)
            
        if not self.__GetNextToken():
            raise Warning("expected File name or section data At Line ", self.FileName, self.CurrentLineNumber)
        
        if self.__Token == "FV":
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            if not self.__GetNextToken():
                raise Warning("expected FV name At Line ", self.FileName, self.CurrentLineNumber)
            FfsFileObj.FvName = self.__Token
            
        elif self.__Token == "FD":
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            if not self.__GetNextToken():
                raise Warning("expected FD name At Line ", self.FileName, self.CurrentLineNumber)
            FfsFileObj.FdName = self.__Token
            
        elif self.__Token in ("DEFINE", "APRIORI", "SECTION"):
            self.__UndoToken()
            self.__GetSectionData( FfsFileObj, MacroDict)
        else:
            FfsFileObj.FileName = self.__Token
        
        if not self.__IsToken( "}"):
            raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
    
    ## __GetFileOpts() method
    #
    #   Get options for FILE statement
    #
    #   @param  self        The object pointer
    #   @param  FfsFileObj   for whom options is got
    #
    def __GetFileOpts(self, FfsFileObj):
        
        if self.__GetNextToken():
            Pattern = re.compile(r'([a-zA-Z0-9\-]+|\$\(TARGET\)|\*)_([a-zA-Z0-9\-]+|\$\(TOOL_CHAIN_TAG\)|\*)_([a-zA-Z0-9\-]+|\$\(ARCH\)|\*)')
            if Pattern.match(self.__Token):
                FfsFileObj.KeyStringList.append(self.__Token)
                if self.__IsToken(","):
                    while self.__GetNextToken():
                        if not Pattern.match(self.__Token):
                            raise Warning("expected KeyString \"Target_Tag_Arch\" At Line ", self.FileName, self.CurrentLineNumber)
                        FfsFileObj.KeyStringList.append(self.__Token)

                        if not self.__IsToken(","):
                            break
                    
            else:
                self.__UndoToken()

        if self.__IsKeyword( "FIXED", True):
            FfsFileObj.Fixed = True
            
        if self.__IsKeyword( "CHECKSUM", True):
            FfsFileObj.CheckSum = True
            
        if self.__GetAlignment():
            FfsFileObj.Alignment = self.__Token
            
        
    
    ## __GetAlignment() method
    #
    #   Return the alignment value
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find alignment
    #   @retval False       Not able to find alignment
    #
    def __GetAlignment(self):
        if self.__IsKeyword( "Align", True):
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
            if not self.__GetNextToken():
                raise Warning("expected alignment value At Line ", self.FileName, self.CurrentLineNumber)
            return True
            
        return False
    
    ## __GetFilePart() method
    #
    #   Get section data for FILE statement
    #
    #   @param  self        The object pointer
    #   @param  FfsFileObj   for whom section is got
    #   @param  MacroDict   dictionary used to replace macro
    #
    def __GetSectionData(self, FfsFileObj, MacroDict = {}):
        Dict = {}
        Dict.update(MacroDict)
        
        self.__GetDefineStatements(FfsFileObj)
        
        Dict.update(FfsFileObj.DefineVarDict)
        self.__GetAprioriSection(FfsFileObj, Dict.copy())
        self.__GetAprioriSection(FfsFileObj, Dict.copy())
        
        while True:
            IsLeafSection = self.__GetLeafSection(FfsFileObj, Dict)
            IsEncapSection = self.__GetEncapsulationSec(FfsFileObj)
            if not IsLeafSection and not IsEncapSection:
                break
         
    ## __GetLeafSection() method
    #
    #   Get leaf section for Obj
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom leaf section is got
    #   @param  MacroDict   dictionary used to replace macro
    #   @retval True        Successfully find section statement
    #   @retval False       Not able to find section statement
    #
    def __GetLeafSection(self, Obj, MacroDict = {}):
        
        OldPos = self.GetFileBufferPos()
        
        if not self.__IsKeyword( "SECTION"):
            if len(Obj.SectionList) == 0:
                raise Warning("expected SECTION At Line ", self.FileName, self.CurrentLineNumber)
            else:
                return False
        
        AlignValue = None
        if self.__GetAlignment():
            if self.__Token not in ("Auto", "8", "16", "32", "64", "128", "512", "1K", "4K", "32K" ,"64K"):
                raise Warning("Incorrect alignment '%s'" % self.__Token, self.FileName, self.CurrentLineNumber)
            AlignValue = self.__Token
            
        BuildNum = None
        if self.__IsKeyword( "BUILD_NUM"):
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
            if not self.__GetNextToken():
                raise Warning("expected Build number value At Line ", self.FileName, self.CurrentLineNumber)
            
            BuildNum = self.__Token
            
        if self.__IsKeyword( "VERSION"):
            if AlignValue == 'Auto':
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            if not self.__GetNextToken():
                raise Warning("expected version At Line ", self.FileName, self.CurrentLineNumber)
            VerSectionObj = CommonDataClass.FdfClass.VerSectionClassObject()
            VerSectionObj.Alignment = AlignValue
            VerSectionObj.BuildNum = BuildNum
            if self.__GetStringData():
                VerSectionObj.StringData = self.__Token
            else:
                VerSectionObj.FileName = self.__Token
            Obj.SectionList.append(VerSectionObj)

        elif self.__IsKeyword( "UI"):
            if AlignValue == 'Auto':
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            if not self.__GetNextToken():
                raise Warning("expected UI At Line ", self.FileName, self.CurrentLineNumber)
            UiSectionObj = CommonDataClass.FdfClass.UiSectionClassObject()
            UiSectionObj.Alignment = AlignValue
            if self.__GetStringData():
                UiSectionObj.StringData = self.__Token
            else:
                UiSectionObj.FileName = self.__Token
            Obj.SectionList.append(UiSectionObj)
            
        elif self.__IsKeyword( "FV_IMAGE"):
            if AlignValue == 'Auto':
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            if not self.__GetNextWord():
                raise Warning("expected FV name At Line ", self.FileName, self.CurrentLineNumber)
            
            FvName = self.__Token.upper()
            FvObj = None

            if self.__IsToken( "{"):
                FvObj = Fv.FV()
                FvObj.UiFvName = FvName
                self.__GetDefineStatements(FvObj)
                MacroDict.update(FvObj.DefineVarDict)
                self.__GetBlockStatement(FvObj)
                self.__GetSetStatements(FvObj)
                self.__GetFvAlignment(FvObj)
                self.__GetFvAttributes(FvObj)
                self.__GetAprioriSection(FvObj, MacroDict.copy())
                self.__GetAprioriSection(FvObj, MacroDict.copy())
    
                while True:
                    IsInf = self.__GetInfStatement(FvObj, MacroDict.copy())
                    IsFile = self.__GetFileStatement(FvObj, MacroDict.copy())
                    if not IsInf and not IsFile:
                        break
    
                if not self.__IsToken( "}"):
                    raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
            
            FvImageSectionObj = CommonDataClass.FdfClass.FvImageSectionClassObject()
            FvImageSectionObj.Alignment = AlignValue
            if FvObj != None:
                FvImageSectionObj.Fv = FvObj
                FvImageSectionObj.FvName = None
            else:
                FvImageSectionObj.FvName = FvName
                
            Obj.SectionList.append(FvImageSectionObj) 
           
        elif self.__IsKeyword("PEI_DEPEX_EXP") or self.__IsKeyword("DXE_DEPEX_EXP") or self.__IsKeyword("SMM_DEPEX_EXP"):
            if AlignValue == 'Auto':
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            DepexSectionObj = CommonDataClass.FdfClass.DepexSectionClassObject()
            DepexSectionObj.Alignment = AlignValue
            DepexSectionObj.DepexType = self.__Token
            
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            if not self.__IsToken( "{"):
                raise Warning("expected '{' At Line ", self.FileName, self.CurrentLineNumber)
            if not self.__SkipToToken( "}"):
                raise Warning("expected Depex expression ending '}' At Line ", self.FileName, self.CurrentLineNumber)
            
            DepexSectionObj.Expression = self.__SkippedChars.rstrip('}')
            Obj.SectionList.append(DepexSectionObj)
        
        else:
            
            if not self.__GetNextWord():
                raise Warning("expected section type At Line ", self.FileName, self.CurrentLineNumber)
            
            # Encapsulation section appear, UndoToken and return
            if self.__Token == "COMPRESS" or self.__Token == "GUIDED":
                self.SetFileBufferPos(OldPos)
                return False
            
            if self.__Token not in ("COMPAT16", "PE32", "PIC", "TE", "FV_IMAGE", "RAW", "DXE_DEPEX",\
                               "UI", "VERSION", "PEI_DEPEX", "SUBTYPE_GUID", "SMM_DEPEX"):
                raise Warning("Unknown section type '%s'" % self.__Token, self.FileName, self.CurrentLineNumber)
            if AlignValue == 'Auto'and (not self.__Token == 'PE32') and (not self.__Token == 'TE'):
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            # DataSection
            DataSectionObj = CommonDataClass.FdfClass.DataSectionClassObject()
            DataSectionObj.Alignment = AlignValue
            DataSectionObj.SecType = self.__Token
            
            if self.__IsKeyword('RELOCS_STRIPPED') or self.__IsKeyword('RELOCS_RETAINED'):
                if self.__FileCouldHaveRelocFlag(Obj.FvFileType) and self.__SectionCouldHaveRelocFlag(DataSectionObj.SecType):
                    if self.__Token == 'RELOCS_STRIPPED':
                        DataSectionObj.KeepReloc = False
                    else:
                        DataSectionObj.KeepReloc = True
                else:
                    raise Warning("File type %s, section type %s, could not have reloc strip flag At Line %d" % (Obj.FvFileType, DataSectionObj.SecType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)
            
            if self.__IsToken("="):
                if not self.__GetNextToken():
                    raise Warning("expected section file path At Line ", self.FileName, self.CurrentLineNumber)
                DataSectionObj.SectFileName = self.__Token
            else:
                if not self.__GetCglSection(DataSectionObj):
                    return False
            
            Obj.SectionList.append(DataSectionObj)
            
        return True
            
    ## __GetCglSection() method
    #
    #   Get compressed or GUIDed section for Obj
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom leaf section is got
    #   @param  AlignValue  alignment value for complex section
    #   @retval True        Successfully find section statement
    #   @retval False       Not able to find section statement
    #
    def __GetCglSection(self, Obj, AlignValue = None):
        
        if self.__IsKeyword( "COMPRESS"):
            type = "PI_STD"
            if self.__IsKeyword("PI_STD") or self.__IsKeyword("PI_NONE"):
                type = self.__Token
                
            if not self.__IsToken("{"):
                raise Warning("expected '{' At Line ", self.FileName, self.CurrentLineNumber)

            CompressSectionObj = CommonDataClass.FdfClass.CompressSectionClassObject()
            CompressSectionObj.Alignment = AlignValue
            CompressSectionObj.CompType = type
            # Recursive sections...
            while True:
                IsLeafSection = self.__GetLeafSection(CompressSectionObj)
                IsEncapSection = self.__GetEncapsulationSec(CompressSectionObj)
                if not IsLeafSection and not IsEncapSection:
                    break
            
            
            if not self.__IsToken( "}"):
                raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
            Obj.SectionList.append(CompressSectionObj)
                
#            else:
#               raise Warning("Compress type not known At Line ") 
           
            return True
        
        elif self.__IsKeyword( "GUIDED"):
            GuidValue = None
            if self.__GetNextGuid():
                GuidValue = self.__Token

            AttribDict = self.__GetGuidAttrib()
            if not self.__IsToken("{"):
                raise Warning("expected '{' At Line ", self.FileName, self.CurrentLineNumber)
            GuidSectionObj = CommonDataClass.FdfClass.GuidSectionClassObject()
            GuidSectionObj.Alignment = AlignValue
            GuidSectionObj.NameGuid = GuidValue
            GuidSectionObj.SectionType = "GUIDED"
            GuidSectionObj.ProcessRequired = AttribDict["PROCESSING_REQUIRED"]
            GuidSectionObj.AuthStatusValid = AttribDict["AUTH_STATUS_VALID"]
            # Recursive sections...
            while True:
                IsLeafSection = self.__GetLeafSection(GuidSectionObj)
                IsEncapSection = self.__GetEncapsulationSec(GuidSectionObj)
                if not IsLeafSection and not IsEncapSection:
                    break
            
            if not self.__IsToken( "}"):
                raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
            Obj.SectionList.append(GuidSectionObj)
                
            return True
        
        return False

    ## __GetGuidAttri() method
    #
    #   Get attributes for GUID section
    #
    #   @param  self        The object pointer
    #   @retval AttribDict  Dictionary of key-value pair of section attributes
    #
    def __GetGuidAttrib(self):
        
        AttribDict = {}
        AttribDict["PROCESSING_REQUIRED"] = False
        AttribDict["AUTH_STATUS_VALID"] = False
        if self.__IsKeyword("PROCESSING_REQUIRED") or self.__IsKeyword("AUTH_STATUS_VALID"):
            AttribKey = self.__Token

            if not self.__IsToken("="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
            if not self.__GetNextToken() or self.__Token.upper() not in ("TRUE", "FALSE", "1", "0"):
                raise Warning("expected TRUE/FALSE (1/0) At Line ", self.FileName, self.CurrentLineNumber)
            AttribDict[AttribKey] = self.__Token
            
        if self.__IsKeyword("PROCESSING_REQUIRED") or self.__IsKeyword("AUTH_STATUS_VALID"):
            AttribKey = self.__Token

            if not self.__IsToken("="):
                raise Warning("expected '=' At Line ")

            if not self.__GetNextToken() or self.__Token.upper() not in ("TRUE", "FALSE", "1", "0"):
                raise Warning("expected TRUE/FALSE (1/0) At Line ", self.FileName, self.CurrentLineNumber)
            AttribDict[AttribKey] = self.__Token
            
        return AttribDict
            
    ## __GetEncapsulationSec() method
    #
    #   Get encapsulation section for FILE
    #
    #   @param  self        The object pointer
    #   @param  FfsFile     for whom section is got
    #   @retval True        Successfully find section statement
    #   @retval False       Not able to find section statement
    #
    def __GetEncapsulationSec(self, FfsFileObj):        
        
        OldPos = self.GetFileBufferPos()
        if not self.__IsKeyword( "SECTION"):
            if len(FfsFileObj.SectionList) == 0:
                raise Warning("expected SECTION At Line ", self.FileName, self.CurrentLineNumber)
            else:
                return False
        
        AlignValue = None
        if self.__GetAlignment():
            if self.__Token not in ("8", "16", "32", "64", "128", "512", "1K", "4K", "32K" ,"64K"):
                raise Warning("Incorrect alignment '%s'" % self.__Token, self.FileName, self.CurrentLineNumber)
            AlignValue = self.__Token
            
        if not self.__GetCglSection(FfsFileObj, AlignValue):
            self.SetFileBufferPos(OldPos)
            return False
        else:
            return True

    ## __GetCapsule() method
    #
    #   Get capsule section contents and store its data into capsule list of self.Profile
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a capsule
    #   @retval False       Not able to find a capsule
    #
    def __GetCapsule(self):
        
        if not self.__GetNextToken():
            return False

        S = self.__Token.upper()
        if S.startswith("[") and not S.startswith("[CAPSULE."):
            if not S.startswith("[VTF.") and not S.startswith("[RULE.") and not S.startswith("[OPTIONROM."):
                raise Warning("Unknown section or section appear sequence error (The correct sequence should be [FD.], [FV.], [Capsule.], [VTF.], [Rule.], [OptionRom.])", self.FileName, self.CurrentLineNumber)
            self.__UndoToken()
            return False

        self.__UndoToken()
        if not self.__IsToken("[CAPSULE.", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
                    % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine :], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning("expected [Capsule.] At Line ", self.FileName, self.CurrentLineNumber)        
            
        CapsuleObj = CommonDataClass.FdfClass.CapsuleClassObject()

        CapsuleName = self.__GetUiName()
        if not CapsuleName:
            raise Warning("expected capsule name At line ", self.FileName, self.CurrentLineNumber)
        
        CapsuleObj.UiCapsuleName = CapsuleName.upper()
        
        if not self.__IsToken( "]"):
            raise Warning("expected ']' At Line ", self.FileName, self.CurrentLineNumber)
        
        if self.__IsKeyword("CREATE_FILE"):
            if not self.__IsToken( "="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
            if not self.__GetNextToken():
                raise Warning("expected file name At Line ", self.FileName, self.CurrentLineNumber)
            
            CapsuleObj.CreateFile = self.__Token
        
        self.__GetCapsuleStatements(CapsuleObj)
        self.Profile.CapsuleList.append(CapsuleObj)
        return True    
            
    ## __GetCapsuleStatements() method
    #
    #   Get statements for capsule
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom statements are got
    #
    def __GetCapsuleStatements(self, Obj):
        self.__GetCapsuleTokens(Obj)
        self.__GetDefineStatements(Obj)
        self.__GetSetStatements(Obj)
        
        self.__GetCapsuleData(Obj)
                
    ## __GetCapsuleStatements() method
    #
    #   Get token statements for capsule
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom token statements are got
    #
    def __GetCapsuleTokens(self, Obj):
        
        if not self.__IsKeyword("CAPSULE_GUID"):
            raise Warning("expected 'CAPSULE_GUID' At Line ", self.FileName, self.CurrentLineNumber)
        
        while self.__CurrentLine().find("=") != -1:
            NameValue = self.__CurrentLine().split("=")
            Obj.TokensDict[NameValue[0].strip()] = NameValue[1].strip()
            self.CurrentLineNumber += 1
            self.CurrentOffsetWithinLine = 0
    
    ## __GetCapsuleData() method
    #
    #   Get capsule data for capsule
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom capsule data are got
    #
    def __GetCapsuleData(self, Obj):
        
        while True:
            IsInf = self.__GetInfStatement(Obj, True)
            IsFile = self.__GetFileStatement(Obj, True)
            IsFv = self.__GetFvStatement(Obj)
            if not IsInf and not IsFile and not IsFv:
                break
    
    ## __GetFvStatement() method
    #
    #   Get FV for capsule
    #
    #   @param  self        The object pointer
    #   @param  CapsuleObj  for whom FV is got
    #   @retval True        Successfully find a FV statement
    #   @retval False       Not able to find a FV statement
    #
    def __GetFvStatement(self, CapsuleObj):
        
        if not self.__IsKeyword("FV"):
            return False
        
        if not self.__IsToken("="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__GetNextToken():
            raise Warning("expected FV name At Line ", self.FileName, self.CurrentLineNumber)
        
#        CapsuleFv = CapsuleData.CapsuleFv()
#        CapsuleFv.FvName = self.__Token
#        CapsuleObj.CapsuleDataList.append(CapsuleFv)
        return True
    
    ## __GetRule() method
    #
    #   Get Rule section contents and store its data into rule list of self.Profile
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a Rule
    #   @retval False       Not able to find a Rule
    #
    def __GetRule(self):
        
        if not self.__GetNextToken():
            return False

        S = self.__Token.upper()
        if S.startswith("[") and not S.startswith("[RULE."):
            if not S.startswith("[OPTIONROM."):
                raise Warning("Unknown section or section appear sequence error (The correct sequence should be [FD.], [FV.], [Capsule.], [VTF.], [Rule.], [OptionRom.])", self.FileName, self.CurrentLineNumber)
            self.__UndoToken()
            return False
        self.__UndoToken()
        if not self.__IsToken("[Rule.", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
                    % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine :], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning("expected [Rule.] At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__SkipToToken("."):
            raise Warning("expected '.' At Line ", self.FileName, self.CurrentLineNumber)
        
        Arch = self.__SkippedChars.rstrip(".")
        if Arch.upper() not in ("IA32", "X64", "IPF", "EBC", "ARM", "AARCH64", "COMMON"):
            raise Warning("Unknown Arch '%s'" % Arch, self.FileName, self.CurrentLineNumber)
        
        ModuleType = self.__GetModuleType()
        
        TemplateName = ""
        if self.__IsToken("."):
            if not self.__GetNextWord():
                raise Warning("expected template name At Line ", self.FileName, self.CurrentLineNumber)
            TemplateName = self.__Token
            
        if not self.__IsToken( "]"):
            raise Warning("expected ']' At Line ", self.FileName, self.CurrentLineNumber)
        
        RuleObj = self.__GetRuleFileStatements()
        RuleObj.Arch = Arch.upper()
        RuleObj.ModuleType = ModuleType
        RuleObj.TemplateName = TemplateName
        if TemplateName == '' :
            self.Profile.RuleDict['RULE'             + \
                              '.'                    + \
                              Arch.upper()           + \
                              '.'                    + \
                              ModuleType.upper()     ] = RuleObj
        else :
            self.Profile.RuleDict['RULE'             + \
                              '.'                    + \
                              Arch.upper()           + \
                              '.'                    + \
                              ModuleType.upper()     + \
                              '.'                    + \
                              TemplateName.upper() ] = RuleObj
#        self.Profile.RuleList.append(rule)
        return True
    
    ## __GetModuleType() method
    #
    #   Return the module type
    #
    #   @param  self        The object pointer
    #   @retval string      module type
    #
    def __GetModuleType(self):
        
        if not self.__GetNextWord():
            raise Warning("expected Module type At Line ", self.FileName, self.CurrentLineNumber)
        if self.__Token.upper() not in ("SEC", "PEI_CORE", "PEIM", "DXE_CORE", \
                             "DXE_DRIVER", "DXE_SAL_DRIVER", \
                             "DXE_SMM_DRIVER", "DXE_RUNTIME_DRIVER", \
                             "UEFI_DRIVER", "UEFI_APPLICATION", "USER_DEFINED", "DEFAULT", "BASE", \
                             "SECURITY_CORE", "COMBINED_PEIM_DRIVER", "PIC_PEIM", "RELOCATABLE_PEIM", \
                             "PE32_PEIM", "BS_DRIVER", "RT_DRIVER", "SAL_RT_DRIVER", "APPLICATION", "ACPITABLE", "SMM_CORE"):
            raise Warning("Unknown Module type At line ", self.FileName, self.CurrentLineNumber)
        return self.__Token
    
    ## __GetFileExtension() method
    #
    #   Return the file extension
    #
    #   @param  self        The object pointer
    #   @retval string      file name extension
    #
    def __GetFileExtension(self):
        if not self.__IsToken("."):
                raise Warning("expected '.' At Line ", self.FileName, self.CurrentLineNumber)
            
        Ext = ""
        if self.__GetNextToken():
            Pattern = re.compile(r'([a-zA-Z][a-zA-Z0-9]*)')
            if Pattern.match(self.__Token):
                Ext = self.__Token                            
                return '.' + Ext    
            else:
                raise Warning("Unknown file extension At Line ", self.FileName, self.CurrentLineNumber)
                
        else:
            raise Warning("expected file extension At Line ", self.FileName, self.CurrentLineNumber)
        
    ## __GetRuleFileStatement() method
    #
    #   Get rule contents
    #
    #   @param  self        The object pointer
    #   @retval Rule        Rule object
    #
    def __GetRuleFileStatements(self):
        
        if not self.__IsKeyword("FILE"):
            raise Warning("expected FILE At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__GetNextWord():
            raise Warning("expected FFS type At Line ", self.FileName, self.CurrentLineNumber)
        
        Type = self.__Token.strip().upper()
        if Type not in ("RAW", "FREEFORM", "SEC", "PEI_CORE", "PEIM",\
                             "PEI_DXE_COMBO", "DRIVER", "DXE_CORE", "APPLICATION", "FV_IMAGE", "SMM", "SMM_CORE"):
            raise Warning("Unknown FV type At line ", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken("="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
        
        if not self.__IsKeyword("$(NAMED_GUID)"):
            if not self.__GetNextWord():
                raise Warning("expected $(NAMED_GUID)", self.FileName, self.CurrentLineNumber)
            if self.__Token == 'PCD':
                if not self.__IsToken( "("):
                    raise Warning("expected '('", self.FileName, self.CurrentLineNumber)
                PcdPair = self.__GetNextPcdName()
                if not self.__IsToken( ")"):
                    raise Warning("expected ')'", self.FileName, self.CurrentLineNumber)
                self.__Token = 'PCD('+PcdPair[1]+'.'+PcdPair[0]+')'
            
        NameGuid = self.__Token

        KeepReloc = None
        if self.__IsKeyword('RELOCS_STRIPPED') or self.__IsKeyword('RELOCS_RETAINED'):
            if self.__FileCouldHaveRelocFlag(Type):
                if self.__Token == 'RELOCS_STRIPPED':
                    KeepReloc = False
                else:
                    KeepReloc = True
            else:
                raise Warning("File type %s could not have reloc strip flag At Line %d" % (Type, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)
        
        KeyStringList = []
        if self.__GetNextToken():
            Pattern = re.compile(r'([a-zA-Z0-9\-]+|\$\(TARGET\)|\*)_([a-zA-Z0-9\-]+|\$\(TOOL_CHAIN_TAG\)|\*)_([a-zA-Z0-9\-]+|\$\(ARCH\)|\*)')
            if Pattern.match(self.__Token):
                KeyStringList.append(self.__Token)
                if self.__IsToken(","):
                    while self.__GetNextToken():
                        if not Pattern.match(self.__Token):
                            raise Warning("expected KeyString \"Target_Tag_Arch\" At Line ", self.FileName, self.CurrentLineNumber)
                        KeyStringList.append(self.__Token)

                        if not self.__IsToken(","):
                            break
                    
            else:
                self.__UndoToken()

        
        Fixed = False
        if self.__IsKeyword("Fixed", True):
            Fixed = True
            
        CheckSum = False
        if self.__IsKeyword("CheckSum", True):
            CheckSum = True
            
        AlignValue = ""
        if self.__GetAlignment():
            if self.__Token not in ("Auto", "8", "16", "32", "64", "128", "512", "1K", "4K", "32K" ,"64K"):
                raise Warning("Incorrect alignment At Line ", self.FileName, self.CurrentLineNumber)
            AlignValue = self.__Token

        if self.__IsToken("{"):
            # Complex file rule expected
            Rule = RuleComplexFile.RuleComplexFile()
            Rule.FvFileType = Type
            Rule.NameGuid = NameGuid
            Rule.Alignment = AlignValue
            Rule.CheckSum = CheckSum
            Rule.Fixed = Fixed
            Rule.KeyStringList = KeyStringList
            if KeepReloc != None:
                Rule.KeepReloc = KeepReloc
            
            while True:
                IsEncapsulate = self.__GetRuleEncapsulationSection(Rule)
                IsLeaf = self.__GetEfiSection(Rule)
                if not IsEncapsulate and not IsLeaf:
                    break
                
            if not self.__IsToken("}"):
                raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
            
            return Rule
        
        elif self.__IsToken("|"):
            # Ext rule expected
            Ext = self.__GetFileExtension()
            
            Rule = RuleSimpleFile.RuleSimpleFile()

            Rule.FvFileType = Type
            Rule.NameGuid = NameGuid
            Rule.Alignment = AlignValue
            Rule.CheckSum = CheckSum
            Rule.Fixed = Fixed
            Rule.FileExtension = Ext
            Rule.KeyStringList = KeyStringList
            if KeepReloc != None:
                Rule.KeepReloc = KeepReloc
            
            return Rule
            
        else:
            # Simple file rule expected
            if not self.__GetNextWord():
                raise Warning("expected leaf section type At Line ", self.FileName, self.CurrentLineNumber)

            SectionName = self.__Token
        
            if SectionName not in ("COMPAT16", "PE32", "PIC", "TE", "FV_IMAGE", "RAW", "DXE_DEPEX",\
                                    "UI", "PEI_DEPEX", "VERSION", "SUBTYPE_GUID", "SMM_DEPEX"):
                raise Warning("Unknown leaf section name '%s'" % SectionName, self.FileName, self.CurrentLineNumber)


            if self.__IsKeyword("Fixed", True):
                Fixed = True              
    
            if self.__IsKeyword("CheckSum", True):
                CheckSum = True
    
            if self.__GetAlignment():
                if self.__Token not in ("Auto", "8", "16", "32", "64", "128", "512", "1K", "4K", "32K" ,"64K"):
                    raise Warning("Incorrect alignment At Line ", self.FileName, self.CurrentLineNumber)
                if self.__Token == 'Auto' and (not SectionName == 'PE32') and (not SectionName == 'TE'):
                    raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
                AlignValue = self.__Token
            
            if not self.__GetNextToken():
                raise Warning("expected File name At Line ", self.FileName, self.CurrentLineNumber)
            
            Rule = RuleSimpleFile.RuleSimpleFile()
            Rule.SectionType = SectionName
            Rule.FvFileType = Type
            Rule.NameGuid = NameGuid
            Rule.Alignment = AlignValue
            Rule.CheckSum = CheckSum
            Rule.Fixed = Fixed
            Rule.FileName = self.__Token
            Rule.KeyStringList = KeyStringList
            if KeepReloc != None:
                Rule.KeepReloc = KeepReloc
            return Rule
        
    ## __GetEfiSection() method
    #
    #   Get section list for Rule
    #
    #   @param  self        The object pointer
    #   @param  Obj         for whom section is got
    #   @retval True        Successfully find section statement
    #   @retval False       Not able to find section statement
    #
    def __GetEfiSection(self, Obj):
        
        OldPos = self.GetFileBufferPos()
        if not self.__GetNextWord():
            return False
        SectionName = self.__Token
        
        if SectionName not in ("COMPAT16", "PE32", "PIC", "TE", "FV_IMAGE", "RAW", "DXE_DEPEX",\
                               "UI", "VERSION", "PEI_DEPEX", "GUID", "SMM_DEPEX"):
            self.__UndoToken()
            return False
        
        if SectionName == "FV_IMAGE":
            FvImageSectionObj = FvImageSection.FvImageSection()
            if self.__IsKeyword("FV_IMAGE"):
                pass
            if self.__IsToken( "{"):
                FvObj = Fv.FV()
                self.__GetDefineStatements(FvObj)
                self.__GetBlockStatement(FvObj)
                self.__GetSetStatements(FvObj)
                self.__GetFvAlignment(FvObj)
                self.__GetFvAttributes(FvObj)
                self.__GetAprioriSection(FvObj)
                self.__GetAprioriSection(FvObj)
    
                while True:
                    IsInf = self.__GetInfStatement(FvObj)
                    IsFile = self.__GetFileStatement(FvObj)
                    if not IsInf and not IsFile:
                        break
    
                if not self.__IsToken( "}"):
                    raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
                FvImageSectionObj.Fv = FvObj
                FvImageSectionObj.FvName = None
                
            else:
                if not self.__IsKeyword("FV"):
                    raise Warning("expected 'FV' At Line ", self.FileName, self.CurrentLineNumber)
                FvImageSectionObj.FvFileType = self.__Token
                
                if self.__GetAlignment():
                    if self.__Token not in ("8", "16", "32", "64", "128", "512", "1K", "4K", "32K" ,"64K"):
                        raise Warning("Incorrect alignment At Line ", self.FileName, self.CurrentLineNumber)
                    FvImageSectionObj.Alignment = self.__Token
                
                if self.__IsToken('|'):
                    FvImageSectionObj.FvFileExtension = self.__GetFileExtension()
                elif self.__GetNextToken():
                    if self.__Token not in ("}", "COMPAT16", "PE32", "PIC", "TE", "FV_IMAGE", "RAW", "DXE_DEPEX",\
                               "UI", "VERSION", "PEI_DEPEX", "GUID", "SMM_DEPEX"):
                        FvImageSectionObj.FvFileName = self.__Token
                    else:
                        self.__UndoToken()
                else:
                    raise Warning("expected FV file name At Line ", self.FileName, self.CurrentLineNumber)
                    
            Obj.SectionList.append(FvImageSectionObj)
            return True
        
        EfiSectionObj = EfiSection.EfiSection()
        EfiSectionObj.SectionType = SectionName
        
        if not self.__GetNextToken():
            raise Warning("expected file type At Line ", self.FileName, self.CurrentLineNumber)
        
        if self.__Token == "STRING":
            if not self.__RuleSectionCouldHaveString(EfiSectionObj.SectionType):
                raise Warning("%s section could NOT have string data At Line %d" % (EfiSectionObj.SectionType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)
            
            if not self.__IsToken('='):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
            
            if not self.__GetNextToken():
                raise Warning("expected Quoted String At Line ", self.FileName, self.CurrentLineNumber)
        
            if self.__GetStringData():
                EfiSectionObj.StringData = self.__Token
            
            if self.__IsKeyword("BUILD_NUM"):
                if not self.__RuleSectionCouldHaveBuildNum(EfiSectionObj.SectionType):
                    raise Warning("%s section could NOT have BUILD_NUM At Line %d" % (EfiSectionObj.SectionType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)
            
                if not self.__IsToken("="):
                    raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
                if not self.__GetNextToken():
                    raise Warning("expected Build number At Line ", self.FileName, self.CurrentLineNumber)
                EfiSectionObj.BuildNum = self.__Token
                
        else:
            EfiSectionObj.FileType = self.__Token
            self.__CheckRuleSectionFileType(EfiSectionObj.SectionType, EfiSectionObj.FileType)
            
        if self.__IsKeyword("Optional"):
            if not self.__RuleSectionCouldBeOptional(EfiSectionObj.SectionType):
                raise Warning("%s section could NOT be optional At Line %d" % (EfiSectionObj.SectionType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)
            EfiSectionObj.Optional = True
        
            if self.__IsKeyword("BUILD_NUM"):
                if not self.__RuleSectionCouldHaveBuildNum(EfiSectionObj.SectionType):
                    raise Warning("%s section could NOT have BUILD_NUM At Line %d" % (EfiSectionObj.SectionType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)
                
                if not self.__IsToken("="):
                    raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)
                if not self.__GetNextToken():
                    raise Warning("expected Build number At Line ", self.FileName, self.CurrentLineNumber)
                EfiSectionObj.BuildNum = self.__Token
                
        if self.__GetAlignment():
            if self.__Token not in ("Auto", "8", "16", "32", "64", "128", "512", "1K", "4K", "32K" ,"64K"):
                raise Warning("Incorrect alignment '%s'" % self.__Token, self.FileName, self.CurrentLineNumber)
            if self.__Token == 'Auto' and (not SectionName == 'PE32') and (not SectionName == 'TE'):
                raise Warning("Auto alignment can only be used in PE32 or TE section ", self.FileName, self.CurrentLineNumber)
            EfiSectionObj.Alignment = self.__Token
        
        if self.__IsKeyword('RELOCS_STRIPPED') or self.__IsKeyword('RELOCS_RETAINED'):
            if self.__SectionCouldHaveRelocFlag(EfiSectionObj.SectionType):
                if self.__Token == 'RELOCS_STRIPPED':
                    EfiSectionObj.KeepReloc = False
                else:
                    EfiSectionObj.KeepReloc = True
                if Obj.KeepReloc != None and Obj.KeepReloc != EfiSectionObj.KeepReloc:
                    raise Warning("Section type %s has reloc strip flag conflict with Rule At Line %d" % (EfiSectionObj.SectionType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)
            else:
                raise Warning("Section type %s could not have reloc strip flag At Line %d" % (EfiSectionObj.SectionType, self.CurrentLineNumber), self.FileName, self.CurrentLineNumber)
        
        
        if self.__IsToken('|'):
            EfiSectionObj.FileExtension = self.__GetFileExtension()
        elif self.__GetNextToken():
            if self.__Token not in ("}", "COMPAT16", "PE32", "PIC", "TE", "FV_IMAGE", "RAW", "DXE_DEPEX",\
                       "UI", "VERSION", "PEI_DEPEX", "GUID", "SMM_DEPEX"):
                
                if self.__Token.startswith('PCD'):
                    self.__UndoToken()
                    self.__GetNextWord()
                
                    if self.__Token == 'PCD':
                        if not self.__IsToken( "("):
                            raise Warning("expected '('", self.FileName, self.CurrentLineNumber)
                        PcdPair = self.__GetNextPcdName()
                        if not self.__IsToken( ")"):
                            raise Warning("expected ')'", self.FileName, self.CurrentLineNumber)
                        self.__Token = 'PCD('+PcdPair[1]+'.'+PcdPair[0]+')'
                        
                EfiSectionObj.FileName = self.__Token        
                            
            else:
                self.__UndoToken()
        else:
            raise Warning("expected section file name At Line ", self.FileName, self.CurrentLineNumber)
                
        Obj.SectionList.append(EfiSectionObj)
        return True
        
    ## __RuleSectionCouldBeOptional() method
    #
    #   Get whether a section could be optional
    #
    #   @param  self        The object pointer
    #   @param  SectionType The section type to check
    #   @retval True        section could be optional
    #   @retval False       section never optional
    #
    def __RuleSectionCouldBeOptional(self, SectionType):
        if SectionType in ("DXE_DEPEX", "UI", "VERSION", "PEI_DEPEX", "RAW", "SMM_DEPEX"):
            return True
        else:
            return False
    
    ## __RuleSectionCouldHaveBuildNum() method
    #
    #   Get whether a section could have build number information
    #
    #   @param  self        The object pointer
    #   @param  SectionType The section type to check
    #   @retval True        section could have build number information
    #   @retval False       section never have build number information
    #
    def __RuleSectionCouldHaveBuildNum(self, SectionType):
        if SectionType in ("VERSION"):
            return True
        else:
            return False
    
    ## __RuleSectionCouldHaveString() method
    #
    #   Get whether a section could have string
    #
    #   @param  self        The object pointer
    #   @param  SectionType The section type to check
    #   @retval True        section could have string
    #   @retval False       section never have string
    #
    def __RuleSectionCouldHaveString(self, SectionType):
        if SectionType in ("UI", "VERSION"):
            return True
        else:
            return False
    
    ## __CheckRuleSectionFileType() method
    #
    #   Get whether a section matches a file type
    #
    #   @param  self        The object pointer
    #   @param  SectionType The section type to check
    #   @param  FileType    The file type to check
    #
    def __CheckRuleSectionFileType(self, SectionType, FileType):
        if SectionType == "COMPAT16":
            if FileType not in ("COMPAT16", "SEC_COMPAT16"):
                raise Warning("Incorrect section file type At Line ", self.FileName, self.CurrentLineNumber)
        elif SectionType == "PE32":
            if FileType not in ("PE32", "SEC_PE32"):
                raise Warning("Incorrect section file type At Line ", self.FileName, self.CurrentLineNumber)
        elif SectionType == "PIC":
            if FileType not in ("PIC", "PIC"):
                raise Warning("Incorrect section file type At Line ", self.FileName, self.CurrentLineNumber)
        elif SectionType == "TE":
            if FileType not in ("TE", "SEC_TE"):
                raise Warning("Incorrect section file type At Line ", self.FileName, self.CurrentLineNumber)
        elif SectionType == "RAW":
            if FileType not in ("BIN", "SEC_BIN", "RAW", "ASL", "ACPI"):
                raise Warning("Incorrect section file type At Line ", self.FileName, self.CurrentLineNumber)
        elif SectionType == "DXE_DEPEX" or SectionType == "SMM_DEPEX":
            if FileType not in ("DXE_DEPEX", "SEC_DXE_DEPEX", "SMM_DEPEX"):
                raise Warning("Incorrect section file type At Line ", self.FileName, self.CurrentLineNumber)
        elif SectionType == "UI":
            if FileType not in ("UI", "SEC_UI"):
                raise Warning("Incorrect section file type At Line ", self.FileName, self.CurrentLineNumber)
        elif SectionType == "VERSION":
            if FileType not in ("VERSION", "SEC_VERSION"):
                raise Warning("Incorrect section file type At Line ", self.FileName, self.CurrentLineNumber)
        elif SectionType == "PEI_DEPEX":
            if FileType not in ("PEI_DEPEX", "SEC_PEI_DEPEX"):
                raise Warning("Incorrect section file type At Line ", self.FileName, self.CurrentLineNumber)
        elif SectionType == "GUID":
            if FileType not in ("PE32", "SEC_GUID"):
                raise Warning("Incorrect section file type At Line ", self.FileName, self.CurrentLineNumber)    
              
    ## __GetRuleEncapsulationSection() method
    #
    #   Get encapsulation section for Rule
    #
    #   @param  self        The object pointer
    #   @param  Rule        for whom section is got
    #   @retval True        Successfully find section statement
    #   @retval False       Not able to find section statement
    #
    def __GetRuleEncapsulationSection(self, Rule):

        if self.__IsKeyword( "COMPRESS"):
            Type = "PI_STD"
            if self.__IsKeyword("PI_STD") or self.__IsKeyword("PI_NONE"):
                Type = self.__Token
                
            if not self.__IsToken("{"):
                raise Warning("expected '{' At Line ", self.FileName, self.CurrentLineNumber)

            CompressSectionObj = CompressSection.CompressSection()
            
            CompressSectionObj.CompType = Type
            # Recursive sections...
            while True:
                IsEncapsulate = self.__GetRuleEncapsulationSection(CompressSectionObj)
                IsLeaf = self.__GetEfiSection(CompressSectionObj)
                if not IsEncapsulate and not IsLeaf:
                    break
            
            if not self.__IsToken( "}"):
                raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
            Rule.SectionList.append(CompressSectionObj)
                
            return True

        elif self.__IsKeyword( "GUIDED"):
            GuidValue = None
            if self.__GetNextGuid():
                GuidValue = self.__Token
            
            if self.__IsKeyword( "$(NAMED_GUID)"):
                GuidValue = self.__Token
                
            AttribDict = self.__GetGuidAttrib()
            
            if not self.__IsToken("{"):
                raise Warning("expected '{' At Line ", self.FileName, self.CurrentLineNumber)
            GuidSectionObj = GuidSection.GuidSection()
            GuidSectionObj.NameGuid = GuidValue
            GuidSectionObj.SectionType = "GUIDED"
            GuidSectionObj.ProcessRequired = AttribDict["PROCESSING_REQUIRED"]
            GuidSectionObj.AuthStatusValid = AttribDict["AUTH_STATUS_VALID"]
            
            # Efi sections...
            while True:
                IsEncapsulate = self.__GetRuleEncapsulationSection(GuidSectionObj)
                IsLeaf = self.__GetEfiSection(GuidSectionObj)
                if not IsEncapsulate and not IsLeaf:
                    break
            
            if not self.__IsToken( "}"):
                raise Warning("expected '}' At Line ", self.FileName, self.CurrentLineNumber)
            Rule.SectionList.append(GuidSectionObj)

            return True

        return False
        
    ## __GetVtf() method
    #
    #   Get VTF section contents and store its data into VTF list of self.Profile
    #
    #   @param  self        The object pointer
    #   @retval True        Successfully find a VTF
    #   @retval False       Not able to find a VTF
    #
    def __GetVtf(self):
        
        if not self.__GetNextToken():
            return False

        S = self.__Token.upper()
        if S.startswith("[") and not S.startswith("[VTF."):
            if not S.startswith("[RULE.") and not S.startswith("[OPTIONROM."):
                raise Warning("Unknown section or section appear sequence error (The correct sequence should be [FD.], [FV.], [Capsule.], [VTF.], [Rule.], [OptionRom.])", self.FileName, self.CurrentLineNumber)
            self.__UndoToken()
            return False

        self.__UndoToken()
        if not self.__IsToken("[VTF.", True):
            FileLineTuple = GetRealFileLine(self.FileName, self.CurrentLineNumber)
            print 'Parsing String: %s in File %s, At line: %d, Offset Within Line: %d' \
                    % (self.Profile.FileLinesList[self.CurrentLineNumber - 1][self.CurrentOffsetWithinLine :], FileLineTuple[0], FileLineTuple[1], self.CurrentOffsetWithinLine)
            raise Warning("expected [VTF.] At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__SkipToToken("."):
            raise Warning("expected '.' At Line ", self.FileName, self.CurrentLineNumber)

        Arch = self.__SkippedChars.rstrip(".").upper()
        if Arch not in ("IA32", "X64", "IPF", "ARM", "AARCH64"):
            raise Warning("Unknown Arch At line ", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextWord():
            raise Warning("expected VTF name At Line ", self.FileName, self.CurrentLineNumber)
        Name = self.__Token.upper()

        VtfObj = Vtf.Vtf()
        VtfObj.UiName = Name
        VtfObj.KeyArch = Arch
        
        if self.__IsToken(","):
            if not self.__GetNextWord():
                raise Warning("expected Arch list At Line ", self.FileName, self.CurrentLineNumber)
            if self.__Token.upper() not in ("IA32", "X64", "IPF", "ARM", "AARCH64"):
                raise Warning("Unknown Arch At line ", self.FileName, self.CurrentLineNumber)
            VtfObj.ArchList = self.__Token.upper()

        if not self.__IsToken( "]"):
            raise Warning("expected ']' At Line ", self.FileName, self.CurrentLineNumber)
        
        if self.__IsKeyword("IA32_RST_BIN"):
            if not self.__IsToken("="):
                raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

            if not self.__GetNextToken():
                raise Warning("expected Reset file At Line ", self.FileName, self.CurrentLineNumber)

            VtfObj.ResetBin = self.__Token
            
        while self.__GetComponentStatement(VtfObj):
            pass
        
        self.Profile.VtfList.append(VtfObj)
        return True
    
    ## __GetComponentStatement() method
    #
    #   Get components in VTF
    #
    #   @param  self        The object pointer
    #   @param  VtfObj         for whom component is got
    #   @retval True        Successfully find a component
    #   @retval False       Not able to find a component
    #
    def __GetComponentStatement(self, VtfObj):
        
        if not self.__IsKeyword("COMP_NAME"):
            return False
        
        if not self.__IsToken("="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextWord():
            raise Warning("expected Component Name At Line ", self.FileName, self.CurrentLineNumber)

        CompStatementObj = ComponentStatement.ComponentStatement()
        CompStatementObj.CompName = self.__Token
        
        if not self.__IsKeyword("COMP_LOC"):
            raise Warning("expected COMP_LOC At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken("="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

        CompStatementObj.CompLoc = ""
        if self.__GetNextWord():
            CompStatementObj.CompLoc = self.__Token
            if self.__IsToken('|'):
                if not self.__GetNextWord():
                    raise Warning("Expected Region Name At Line ", self.FileName, self.CurrentLineNumber)
                
                if self.__Token not in ("F", "N", "S"):    #, "H", "L", "PH", "PL"): not support
                    raise Warning("Unknown location type At line ", self.FileName, self.CurrentLineNumber)
    
                CompStatementObj.FilePos = self.__Token
        else:
            self.CurrentLineNumber += 1
            self.CurrentOffsetWithinLine = 0
        
        if not self.__IsKeyword("COMP_TYPE"):
            raise Warning("expected COMP_TYPE At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken("="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextToken():
            raise Warning("expected Component type At Line ", self.FileName, self.CurrentLineNumber)
        if self.__Token not in ("FIT", "PAL_B", "PAL_A", "OEM"):
            if not self.__Token.startswith("0x") or len(self.__Token) < 3 or len(self.__Token) > 4 or \
                not self.__HexDigit(self.__Token[2]) or not self.__HexDigit(self.__Token[-1]):
                raise Warning("Unknown location type At line ", self.FileName, self.CurrentLineNumber)
        CompStatementObj.CompType = self.__Token
        
        if not self.__IsKeyword("COMP_VER"):
            raise Warning("expected COMP_VER At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken("="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextToken():
            raise Warning("expected Component version At Line ", self.FileName, self.CurrentLineNumber)

        Pattern = re.compile('-$|[0-9]{0,1}[0-9]{1}\.[0-9]{0,1}[0-9]{1}')
        if Pattern.match(self.__Token) == None:
            raise Warning("Unknown version format At line ", self.FileName, self.CurrentLineNumber)
        CompStatementObj.CompVer = self.__Token
        
        if not self.__IsKeyword("COMP_CS"):
            raise Warning("expected COMP_CS At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken("="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextToken():
            raise Warning("expected Component CS At Line ", self.FileName, self.CurrentLineNumber)
        if self.__Token not in ("1", "0"):
            raise Warning("Unknown  Component CS At line ", self.FileName, self.CurrentLineNumber)
        CompStatementObj.CompCs = self.__Token
        
        
        if not self.__IsKeyword("COMP_BIN"):
            raise Warning("expected COMP_BIN At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken("="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextToken():
            raise Warning("expected Component file At Line ", self.FileName, self.CurrentLineNumber)

        CompStatementObj.CompBin = self.__Token
        
        if not self.__IsKeyword("COMP_SYM"):
            raise Warning("expected COMP_SYM At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken("="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__GetNextToken():
            raise Warning("expected Component symbol file At Line ", self.FileName, self.CurrentLineNumber)

        CompStatementObj.CompSym = self.__Token
    
        if not self.__IsKeyword("COMP_SIZE"):
            raise Warning("expected COMP_SIZE At Line ", self.FileName, self.CurrentLineNumber)

        if not self.__IsToken("="):
            raise Warning("expected '=' At Line ", self.FileName, self.CurrentLineNumber)

        if self.__IsToken("-"):
            CompStatementObj.CompSize = self.__Token
        elif self.__GetNextDecimalNumber():
            CompStatementObj.CompSize = self.__Token
        elif self.__GetNextHexNumber():
            CompStatementObj.CompSize = self.__Token
        else:
            raise Warning("Unknown size At line ", self.FileName, self.CurrentLineNumber)
        
        VtfObj.ComponentStatementList.append(CompStatementObj)
        return True
    
    ## __GetFvInFd() method
    #
    #   Get FV list contained in FD
    #
    #   @param  self        The object pointer
    #   @param  FdName      FD name
    #   @retval FvList      list of FV in FD
    #
    def __GetFvInFd (self, FdName):
    
        FvList = []
        if FdName.upper() in self.Profile.FdDict.keys():
            FdObj = self.Profile.FdDict[FdName.upper()]
            for elementRegion in FdObj.RegionList:
                if elementRegion.RegionType == 'FV':
                    for elementRegionData in elementRegion.RegionDataList:
                        if elementRegionData != None and elementRegionData.upper() not in FvList:
                            FvList.append(elementRegionData.upper())
        return FvList
    
    ## __GetReferencedFdFvTuple() method
    #
    #   Get FD and FV list referenced by a FFS file
    #
    #   @param  self        The object pointer
    #   @param  FfsFile     contains sections to be searched
    #   @param  RefFdList   referenced FD by section
    #   @param  RefFvList   referenced FV by section
    #
    def __GetReferencedFdFvTuple(self, FvObj, RefFdList = [], RefFvList = []):
        
        for FfsObj in FvObj.FfsList:
            if isinstance(FfsObj, FfsFileStatement.FileStatement):
                if FfsObj.FvName != None and FfsObj.FvName.upper() not in RefFvList:
                    RefFvList.append(FfsObj.FvName.upper())
                elif FfsObj.FdName != None and FfsObj.FdName.upper() not in RefFdList:
                    RefFdList.append(FfsObj.FdName.upper())
                else:
                    self.__GetReferencedFdFvTupleFromSection(FfsObj, RefFdList, RefFvList)    
    
    ## __GetReferencedFdFvTupleFromSection() method
    #
    #   Get FD and FV list referenced by a FFS section
    #
    #   @param  self        The object pointer
    #   @param  FfsFile     contains sections to be searched
    #   @param  FdList      referenced FD by section
    #   @param  FvList      referenced FV by section
    #
    def __GetReferencedFdFvTupleFromSection(self, FfsFile, FdList = [], FvList = []):
        
        SectionStack = []
        SectionStack.extend(FfsFile.SectionList)
        while SectionStack != []:
            SectionObj = SectionStack.pop()
            if isinstance(SectionObj, FvImageSection.FvImageSection):
                if SectionObj.FvName != None and SectionObj.FvName.upper() not in FvList:
                    FvList.append(SectionObj.FvName.upper())
                if SectionObj.Fv != None and SectionObj.Fv.UiFvName != None and SectionObj.Fv.UiFvName.upper() not in FvList:
                    FvList.append(SectionObj.Fv.UiFvName.upper())
                    self.__GetReferencedFdFvTuple(SectionObj.Fv, FdList, FvList)
            
            if isinstance(SectionObj, CompressSection.CompressSection) or isinstance(SectionObj, GuidSection.GuidSection):
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
        
        CycleRefExists = False
        
        try:
            for FvName in self.Profile.FvDict.keys():
                LogStr = "Cycle Reference Checking for FV: %s\n" % FvName
                RefFvStack = []
                RefFvStack.append(FvName)
                FdAnalyzedList = []
                
                while RefFvStack != []:
                    FvNameFromStack = RefFvStack.pop()
                    if FvNameFromStack.upper() in self.Profile.FvDict.keys():
                        FvObj = self.Profile.FvDict[FvNameFromStack.upper()]
                    else:
                        continue
                    
                    RefFdList = []
                    RefFvList = []
                    self.__GetReferencedFdFvTuple(FvObj, RefFdList, RefFvList)
                    
                    for RefFdName in RefFdList:
                        if RefFdName in FdAnalyzedList:
                            continue
                        
                        LogStr += "FD %s is referenced by FV %s\n" % (RefFdName, FvNameFromStack) 
                        FvInFdList = self.__GetFvInFd(RefFdName)
                        if FvInFdList != []:
                            LogStr += "FD %s contains FV: " % RefFdName
                            for FvObj in FvInFdList:
                                LogStr += FvObj
                                LogStr += ' \n'
                                if FvObj not in RefFvStack:
                                    RefFvStack.append(FvObj)
                        
                                if FvName in RefFvStack:
                                    CycleRefExists = True
                                    raise Warning(LogStr)
                        FdAnalyzedList.append(RefFdName)
                                   
                    for RefFvName in RefFvList:
                        LogStr += "FV %s is referenced by FV %s\n" % (RefFvName, FvNameFromStack)
                        if RefFvName not in RefFvStack:
                            RefFvStack.append(RefFvName)
                            
                        if FvName in RefFvStack:
                            CycleRefExists = True
                            raise Warning(LogStr)
        
        except Warning:
            print LogStr
        
        finally:
            return CycleRefExists
        
if __name__ == "__main__":
    import sys
    try:
        test_file = sys.argv[1]
    except IndexError, v:
        print "Usage: %s filename" % sys.argv[0]
        sys.exit(1)

    parser = FdfParser(test_file)
    try:
        parser.ParseFile()
        parser.CycleReferenceCheck()
    except Warning, X:
        print X.message
    else:
        print "Success!"

