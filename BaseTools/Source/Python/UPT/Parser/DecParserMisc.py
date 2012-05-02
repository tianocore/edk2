## @file
# This file is used to define helper class and function for DEC parser
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

'''
DecParserMisc
'''

## Import modules
#
import os
import Logger.Log as Logger
from Logger.ToolError import FILE_PARSE_FAILURE
from Logger import StringTable as ST
from Library.DataType import TAB_COMMENT_SPLIT
from Library.DataType import TAB_COMMENT_EDK1_SPLIT
from Library.ExpressionValidate import IsValidBareCString
from Library.ParserValidate import IsValidCFormatGuid
from Library.ExpressionValidate import IsValidLogicalExpr
from Library.ExpressionValidate import IsValidStringTest
from Library.Misc import CheckGuidRegFormat

TOOL_NAME = 'DecParser'
VERSION_PATTERN = '[0-9]+(\.[0-9]+)?'
CVAR_PATTERN = '[_a-zA-Z][a-zA-Z0-9_]*'
PCD_TOKEN_PATTERN = '(0[xX]0*[a-fA-F0-9]{1,8})|([0-9]+)'
MACRO_PATTERN = '[A-Z][_A-Z0-9]*'

## FileContent
# Class to hold DEC file information
#
class FileContent:
    def __init__(self, Filename, FileContent2):
        self.Filename = Filename
        self.PackagePath, self.PackageFile = os.path.split(Filename)
        self.LineIndex = 0
        self.CurrentLine = ''
        self.NextLine = ''
        self.HeadComment = []
        self.TailComment = []
        self.CurrentScope = None
        self.Content = FileContent2
        self.Macros = {}
        self.FileLines = len(FileContent2)

    def GetNextLine(self):
        if self.LineIndex >= self.FileLines:
            return ''
        Line = self.Content[self.LineIndex]
        self.LineIndex += 1
        return Line

    def UndoNextLine(self):
        if self.LineIndex > 0:
            self.LineIndex -= 1

    def ResetNext(self):
        self.HeadComment = []
        self.TailComment = []
        self.NextLine = ''

    def SetNext(self, Line, HeadComment, TailComment):
        self.NextLine = Line
        self.HeadComment = HeadComment
        self.TailComment = TailComment

    def IsEndOfFile(self):
        return self.LineIndex >= self.FileLines


## StripRoot
#
# Strip root path
#
# @param Root: Root must be absolute path
# @param Path: Path to be stripped
#
def StripRoot(Root, Path):
    OrigPath = Path
    Root = os.path.normpath(Root)
    Path = os.path.normpath(Path)
    if not os.path.isabs(Root):
        return OrigPath
    if Path.startswith(Root):
        Path = Path[len(Root):]
        if Path and Path[0] == os.sep:
            Path = Path[1:]
        return Path
    return OrigPath

## CleanString
#
# Split comments in a string
# Remove spaces
#
# @param Line:              The string to be cleaned
# @param CommentCharacter:  Comment char, used to ignore comment content, 
#                           default is DataType.TAB_COMMENT_SPLIT
#
def CleanString(Line, CommentCharacter=TAB_COMMENT_SPLIT, \
                AllowCppStyleComment=False):
    #
    # remove whitespace
    #
    Line = Line.strip()
    #
    # Replace EDK1's comment character
    #
    if AllowCppStyleComment:
        Line = Line.replace(TAB_COMMENT_EDK1_SPLIT, CommentCharacter)
    #
    # separate comments and statements
    #
    Comment = ''
    InQuote = False
    for Index in range(0, len(Line)):
        if Line[Index] == '"':
            InQuote = not InQuote
            continue
        if Line[Index] == CommentCharacter and not InQuote:
            Comment = Line[Index:].strip()
            Line = Line[0:Index].strip()
            break

    return Line, Comment


## IsValidHexByte
#
# Check if Token is HexByte: <HexByte> ::= 0x <HexDigit>{1,2}
#
# @param Token: Token to be checked
#
def IsValidHexByte(Token):
    Token = Token.strip()
    if not Token.lower().startswith('0x') or not (len(Token) < 5 and len(Token) > 2):
        return False
    try:
        Token = long(Token, 0)
    except BaseException:
        return False
    return True

## IsValidNList
#
# Check if Value has the format of <HexByte> ["," <HexByte>]{0,}
# <HexByte> ::= "0x" <HexDigit>{1,2}
#
# @param Value: Value to be checked
#
def IsValidNList(Value):
    Par = ParserHelper(Value)
    if Par.End():
        return False
    while not Par.End():
        Token = Par.GetToken(',\t ')
        if not IsValidHexByte(Token):
            return False
        if Par.Expect(','):
            if Par.End():
                return False
            continue
        else:
            break
    return Par.End()

## IsValidCArray
#
# check Array is valid
#
# @param Array:    The input Array
#
def IsValidCArray(Array):
    Par = ParserHelper(Array)
    if not Par.Expect('{'):
        return False
    if Par.End():
        return False
    while not Par.End():
        Token = Par.GetToken(',}\t ')
        #
        # 0xa, 0xaa
        #
        if not IsValidHexByte(Token):
            return False
        if Par.Expect(','):
            if Par.End():
                return False
            continue
        elif Par.Expect('}'):
            #
            # End of C array
            #
            break
        else:
            return False
    return Par.End()

## IsValidPcdDatum
#
# check PcdDatum is valid
#
# @param Type:    The pcd Type
# @param Value:    The pcd Value
#
def IsValidPcdDatum(Type, Value):
    if Type not in ["UINT8", "UINT16", "UINT32", "UINT64", "VOID*", "BOOLEAN"]:
        return False, ST.ERR_DECPARSE_PCD_TYPE
    if Type == "VOID*":
        if not ((Value.startswith('L"') or Value.startswith('"') and \
                 Value.endswith('"'))
                or (IsValidCArray(Value)) or (IsValidCFormatGuid(Value)) \
                or (IsValidNList(Value)) or (CheckGuidRegFormat(Value))
               ):
            return False, ST.ERR_DECPARSE_PCD_VOID % (Value, Type)
        RealString = Value[Value.find('"') + 1 :-1]
        if RealString:
            if not IsValidBareCString(RealString):
                return False, ST.ERR_DECPARSE_PCD_VOID % (Value, Type)
    elif Type == 'BOOLEAN':
        if Value in ['TRUE', 'FALSE', 'true', 'false', 'True', 'False',
                     '0x1', '0x01', '1', '0x0', '0x00', '0']:
            return True, ""
        Valid, Cause = IsValidStringTest(Value)
        if not Valid:
            Valid, Cause = IsValidLogicalExpr(Value)
        if not Valid:
            return False, Cause
    else:
        if Value and (Value[0] == '-' or Value[0] == '+'):
            return False, ST.ERR_DECPARSE_PCD_INT_NEGTIVE % (Value, Type)
        try:
            StrVal = Value
            if Value and not Value.startswith('0x') \
                and not Value.startswith('0X'):
                Value = Value.lstrip('0')
                if not Value:
                    return True, ""
            Value = long(Value, 0)
            TypeLenMap = {
                #
                # 0x00 - 0xff
                #
                'UINT8'  : 2,
                #
                # 0x0000 - 0xffff
                #
                'UINT16' : 4,
                #
                # 0x00000000 - 0xffffffff
                #
                'UINT32' : 8,
                #
                # 0x0 - 0xffffffffffffffff
                #
                'UINT64' : 16
            }
            HexStr = hex(Value)
            #
            # First two chars of HexStr are 0x and tail char is L
            #
            if TypeLenMap[Type] < len(HexStr) - 3:
                return False, ST.ERR_DECPARSE_PCD_INT_EXCEED % (StrVal, Type)
        except BaseException:
            return False, ST.ERR_DECPARSE_PCD_INT % (Value, Type)

    return True, ""

## ParserHelper
#
class ParserHelper:
    def __init__(self, String, File=''):
        self._String = String
        self._StrLen = len(String)
        self._Index = 0
        self._File = File

    ## End
    #
    # End
    #
    def End(self):
        self.__SkipWhitespace()
        return self._Index >= self._StrLen

    ## __SkipWhitespace
    #
    # Skip whitespace
    #
    def __SkipWhitespace(self):
        for Char in self._String[self._Index:]:
            if Char not in ' \t':
                break
            self._Index += 1

    ## Expect
    #
    # Expect char in string
    #
    # @param ExpectChar: char expected in index of string
    #
    def Expect(self, ExpectChar):
        self.__SkipWhitespace()
        for Char in self._String[self._Index:]:
            if Char != ExpectChar:
                return False
            else:
                self._Index += 1
                return True
        #
        # Index out of bound of String
        #
        return False

    ## GetToken
    #
    # Get token until encounter StopChar, front whitespace is consumed
    #
    # @param StopChar: Get token until encounter char in StopChar
    # @param StkipPair: Only can be ' or ", StopChar in SkipPair are skipped
    #
    def GetToken(self, StopChar='.,|\t ', SkipPair='"'):
        self.__SkipWhitespace()
        PreIndex = self._Index
        InQuote = False
        LastChar = ''
        for Char in self._String[self._Index:]:
            if Char == SkipPair and LastChar != '\\':
                InQuote = not InQuote
            if Char in StopChar and not InQuote:
                break
            self._Index += 1
            if Char == '\\' and LastChar == '\\':
                LastChar = ''
            else:
                LastChar = Char
        return self._String[PreIndex:self._Index]

    ## AssertChar
    #
    # Assert char at current index of string is AssertChar, or will report 
    # error message
    #
    # @param AssertChar: AssertChar
    # @param ErrorString: ErrorString
    # @param ErrorLineNum: ErrorLineNum
    #
    def AssertChar(self, AssertChar, ErrorString, ErrorLineNum):
        if not self.Expect(AssertChar):
            Logger.Error(TOOL_NAME, FILE_PARSE_FAILURE, File=self._File,
                         Line=ErrorLineNum, ExtraData=ErrorString)

    ## AssertEnd
    #
    # @param ErrorString: ErrorString
    # @param ErrorLineNum: ErrorLineNum
    #
    def AssertEnd(self, ErrorString, ErrorLineNum):
        self.__SkipWhitespace()
        if self._Index != self._StrLen:
            Logger.Error(TOOL_NAME, FILE_PARSE_FAILURE, File=self._File,
                         Line=ErrorLineNum, ExtraData=ErrorString)
