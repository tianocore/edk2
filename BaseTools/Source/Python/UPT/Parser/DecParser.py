## @file
# This file is used to parse DEC file. It will consumed by DecParser
#
# Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
'''
DecParser
'''
## Import modules
#
import Logger.Log as Logger
from Logger.ToolError import FILE_PARSE_FAILURE
from Logger.ToolError import FILE_OPEN_FAILURE
from Logger import StringTable as ST
from Logger.ToolError import FORMAT_INVALID

import Library.DataType as DT
from Library.ParserValidate import IsValidToken
from Library.ParserValidate import IsValidPath
from Library.ParserValidate import IsValidCFormatGuid
from Library.ParserValidate import IsValidIdString
from Library.ParserValidate import IsValidUserId
from Library.ParserValidate import IsValidArch
from Library.ParserValidate import IsValidWord
from Parser.DecParserMisc import TOOL_NAME
from Parser.DecParserMisc import CleanString
from Parser.DecParserMisc import IsValidPcdDatum
from Parser.DecParserMisc import ParserHelper
from Parser.DecParserMisc import StripRoot
from Parser.DecParserMisc import VERSION_PATTERN
from Parser.DecParserMisc import CVAR_PATTERN
from Parser.DecParserMisc import PCD_TOKEN_PATTERN
from Parser.DecParserMisc import MACRO_PATTERN
from Parser.DecParserMisc import FileContent
from Object.Parser.DecObject import _DecComments
from Object.Parser.DecObject import DecDefineObject
from Object.Parser.DecObject import DecDefineItemObject
from Object.Parser.DecObject import DecIncludeObject
from Object.Parser.DecObject import DecIncludeItemObject
from Object.Parser.DecObject import DecLibraryclassObject
from Object.Parser.DecObject import DecLibraryclassItemObject
from Object.Parser.DecObject import DecGuidObject
from Object.Parser.DecObject import DecPpiObject
from Object.Parser.DecObject import DecProtocolObject
from Object.Parser.DecObject import DecGuidItemObject
from Object.Parser.DecObject import DecUserExtensionObject
from Object.Parser.DecObject import DecUserExtensionItemObject
from Object.Parser.DecObject import DecPcdObject
from Object.Parser.DecObject import DecPcdItemObject
from Library.Misc import GuidStructureStringToGuidString
from Library.Misc import CheckGuidRegFormat
from Library.String import ReplaceMacro
from Library.String import GetSplitValueList
from Library.String import gMACRO_PATTERN
from Library.String import ConvertSpecialChar
from Library.CommentParsing import ParsePcdErrorCode

##
# _DecBase class for parsing
#
class _DecBase:
    def __init__(self, RawData):
        self._RawData = RawData
        self._ItemDict = {}
        self._LocalMacro = {}
        #
        # Data parsed by 'self' are saved to this object
        #
        self.ItemObject = None
    
    def GetDataObject(self):
        return self.ItemObject
    
    def GetLocalMacro(self):
        return self._LocalMacro
    
    ## BlockStart
    #
    # Called if a new section starts
    #
    def BlockStart(self):
        self._LocalMacro = {}
    
    ## _CheckReDefine
    #
    # @param Key: to be checked if multi-defined
    # @param Scope: Format: [[SectionName, Arch], ...]. 
    #               If scope is none, use global scope
    #
    def _CheckReDefine(self, Key, Scope = None):
        if not Scope:
            Scope = self._RawData.CurrentScope
            return
        
        SecArch = []
        #
        # Copy scope to SecArch, avoid Scope be changed outside
        #
        SecArch[0:1] = Scope[:]
        if Key not in self._ItemDict:
            self._ItemDict[Key] = [[SecArch, self._RawData.LineIndex]]
            return
        
        for Value in self._ItemDict[Key]:
            for SubValue in Scope:
                #
                # If current is common section
                #
                if SubValue[-1] == 'COMMON':
                    for Other in Value[0]:
                        # Key in common cannot be redefined in other arches
                        # [:-1] means stripping arch info
                        if Other[:-1] == SubValue[:-1]:
                            self._LoggerError(ST.ERR_DECPARSE_REDEFINE % (Key, Value[1]))
                            return
                    continue
                CommonScope = []
                CommonScope[0:1] = SubValue
                CommonScope[-1] = 'COMMON'
                #
                # Cannot be redefined if this key already defined in COMMON Or defined in same arch
                #
                if SubValue in Value[0] or CommonScope in Value[0]:
                    self._LoggerError(ST.ERR_DECPARSE_REDEFINE % (Key, Value[1]))
                    return
        self._ItemDict[Key].append([SecArch, self._RawData.LineIndex])
    
    ## CheckRequiredFields
    # Some sections need to check if some fields exist, define section for example
    # Derived class can re-implement, top parser will call this function after all parsing done
    #  
    def CheckRequiredFields(self):
        if self._RawData:
            pass
        return True
    
    ## IsItemRequired
    # In DEC spec, sections must have at least one statement except user 
    # extension.
    # For example: "[guids" [<attribs>] "]" <EOL> <statements>+
    # sub class can override this method to indicate if statement is a must.
    #
    def _IsStatementRequired(self):
        if self._RawData:
            pass
        return False
    
    def _LoggerError(self, ErrorString):
        Logger.Error(TOOL_NAME, FILE_PARSE_FAILURE, File=self._RawData.Filename, 
                     Line = self._RawData.LineIndex,
                     ExtraData=ErrorString + ST.ERR_DECPARSE_LINE % self._RawData.CurrentLine)
    
    def _ReplaceMacro(self, String):
        if gMACRO_PATTERN.findall(String):
            String = ReplaceMacro(String, self._LocalMacro, False,
                                  FileName = self._RawData.Filename,
                                  Line = ['', self._RawData.LineIndex])
            String = ReplaceMacro(String, self._RawData.Macros, False,
                                  FileName = self._RawData.Filename,
                                  Line = ['', self._RawData.LineIndex])
            MacroUsed = gMACRO_PATTERN.findall(String)
            if MacroUsed:
                Logger.Error(TOOL_NAME, FILE_PARSE_FAILURE,
                             File=self._RawData.Filename, 
                             Line = self._RawData.LineIndex,
                             ExtraData = ST.ERR_DECPARSE_MACRO_RESOLVE % (str(MacroUsed), String))
        return String
    
    def _MacroParser(self, String):
        TokenList = GetSplitValueList(String, ' ', 1)
        if len(TokenList) < 2 or TokenList[1] == '':
            self._LoggerError(ST.ERR_DECPARSE_MACRO_PAIR)

        TokenList = GetSplitValueList(TokenList[1], DT.TAB_EQUAL_SPLIT, 1)
        if TokenList[0] == '':
            self._LoggerError(ST.ERR_DECPARSE_MACRO_NAME)
        elif not IsValidToken(MACRO_PATTERN, TokenList[0]):
            self._LoggerError(ST.ERR_DECPARSE_MACRO_NAME_UPPER % TokenList[0])
        
        if len(TokenList) == 1:
            self._LocalMacro[TokenList[0]] = ''
        else:
            self._LocalMacro[TokenList[0]] = self._ReplaceMacro(TokenList[1])

    ## _ParseItem
    #
    # Parse specified item, this function must be derived by subclass
    #
    def _ParseItem(self):
        if self._RawData:
            pass
        #
        # Should never be called
        #
        return None

    
    ## _TailCommentStrategy
    #
    # This function can be derived to parse tail comment
    # default is it will not consume any lines
    #
    # @param Comment: Comment of current line
    #
    def _TailCommentStrategy(self, Comment):
        if Comment:
            pass
        if self._RawData:
            pass
        return False
    
    ## _StopCurrentParsing
    #
    # Called in Parse if current parsing should be stopped when encounter some
    # keyword
    # Default is section start and end
    #
    # @param Line: Current line
    #
    def _StopCurrentParsing(self, Line):
        if self._RawData:
            pass
        return Line[0] == DT.TAB_SECTION_START and Line[-1] == DT.TAB_SECTION_END
    
    ## _TryBackSlash
    #
    # Split comment and DEC content, concatenate lines if end of char is '\'
    #
    # @param ProcessedLine: ProcessedLine line
    # @param ProcessedComments: ProcessedComments line
    #
    def _TryBackSlash(self, ProcessedLine, ProcessedComments):
        CatLine = ''
        Comment = ''
        Line = ProcessedLine
        CommentList = ProcessedComments
        while not self._RawData.IsEndOfFile():
            if Line == '':
                self._LoggerError(ST.ERR_DECPARSE_BACKSLASH_EMPTY)
                break
            
            if Comment:
                CommentList.append((Comment, self._RawData.LineIndex))
            if Line[-1] != DT.TAB_SLASH:
                CatLine += Line
                break
            elif len(Line) < 2 or Line[-2] != ' ':
                self._LoggerError(ST.ERR_DECPARSE_BACKSLASH)
            else:
                CatLine += Line[:-1]
                Line, Comment = CleanString(self._RawData.GetNextLine())
        #
        # Reach end of content
        #
        if self._RawData.IsEndOfFile():
            if not CatLine:
                if ProcessedLine[-1] == DT.TAB_SLASH:
                    self._LoggerError(ST.ERR_DECPARSE_BACKSLASH_EMPTY)
                CatLine = ProcessedLine
            else:
                if not Line or Line[-1] == DT.TAB_SLASH:
                    self._LoggerError(ST.ERR_DECPARSE_BACKSLASH_EMPTY)
                CatLine += Line
         
        self._RawData.CurrentLine = self._ReplaceMacro(CatLine)
        return CatLine, CommentList
    
    ## Parse
    # This is a template method in which other member functions which might 
    # override by sub class are called. It is responsible for reading file 
    # line by line, and call other member functions to parse. This function
    # should not be re-implement by sub class.
    #
    def Parse(self):
        HeadComments = []
        TailComments = []
        
        #======================================================================
        # CurComments may pointer to HeadComments or TailComments
        #======================================================================
        CurComments = HeadComments
        CurObj = None
        ItemNum = 0
        FromBuf = False
        
        #======================================================================
        # Used to report error information if empty section found
        #======================================================================
        Index = self._RawData.LineIndex
        LineStr = self._RawData.CurrentLine
        while not self._RawData.IsEndOfFile() or self._RawData.NextLine:
            if self._RawData.NextLine:
                #==============================================================
                # Have processed line in buffer
                #==============================================================
                Line = self._RawData.NextLine
                HeadComments.extend(self._RawData.HeadComment)
                TailComments.extend(self._RawData.TailComment)
                self._RawData.ResetNext()
                Comment = ''
                FromBuf = True
            else:
                #==============================================================
                # No line in buffer, read next line
                #==============================================================
                Line, Comment = CleanString(self._RawData.GetNextLine())
                FromBuf = False
            if Line:
                if not FromBuf and CurObj and TailComments:
                    #==========================================================
                    # Set tail comments to previous statement if not empty.
                    #==========================================================
                    CurObj.SetTailComment(CurObj.GetTailComment()+TailComments)
                
                if not FromBuf:
                    del TailComments[:]
                CurComments = TailComments
                Comments = []
                if Comment:
                    Comments = [(Comment, self._RawData.LineIndex)]
                
                #==============================================================
                # Try if last char of line has backslash
                #==============================================================
                Line, Comments = self._TryBackSlash(Line, Comments)
                CurComments.extend(Comments)
                
                #==============================================================
                # Macro found
                #==============================================================
                if Line.startswith('DEFINE '):
                    self._MacroParser(Line)
                    del HeadComments[:]
                    del TailComments[:]
                    CurComments = HeadComments
                    continue
                
                if self._StopCurrentParsing(Line):
                    #==========================================================
                    # This line does not belong to this parse,
                    # Save it, can be used by next parse
                    #==========================================================
                    self._RawData.SetNext(Line, HeadComments, TailComments)
                    break
                
                Obj = self._ParseItem()
                ItemNum += 1
                if Obj:
                    Obj.SetHeadComment(Obj.GetHeadComment()+HeadComments)
                    Obj.SetTailComment(Obj.GetTailComment()+TailComments)
                    del HeadComments[:]
                    del TailComments[:]
                    CurObj = Obj
                else:
                    CurObj = None
            else:
                if id(CurComments) == id(TailComments):
                    #==========================================================
                    # Check if this comment belongs to tail comment
                    #==========================================================
                    if not self._TailCommentStrategy(Comment):
                        CurComments = HeadComments

                if Comment:
                    CurComments.append(((Comment, self._RawData.LineIndex)))
                else:
                    del CurComments[:]
        
        if self._IsStatementRequired() and ItemNum == 0:
            Logger.Error(
                    TOOL_NAME, FILE_PARSE_FAILURE,
                    File=self._RawData.Filename,
                    Line=Index,
                    ExtraData=ST.ERR_DECPARSE_STATEMENT_EMPTY % LineStr
            )

## _DecDefine
# Parse define section
#
class _DecDefine(_DecBase):
    def __init__(self, RawData):
        _DecBase.__init__(self, RawData)
        self.ItemObject = DecDefineObject(RawData.Filename)
        self._LocalMacro = self._RawData.Macros
        self._DefSecNum = 0
        
        #
        # Each field has a function to validate
        #
        self.DefineValidation = {
            DT.TAB_DEC_DEFINES_DEC_SPECIFICATION   :   self._SetDecSpecification,
            DT.TAB_DEC_DEFINES_PACKAGE_NAME        :   self._SetPackageName,
            DT.TAB_DEC_DEFINES_PACKAGE_GUID        :   self._SetPackageGuid,
            DT.TAB_DEC_DEFINES_PACKAGE_VERSION     :   self._SetPackageVersion,
            DT.TAB_DEC_DEFINES_PKG_UNI_FILE        :   self._SetPackageUni,
        }
    
    def BlockStart(self):
        self._DefSecNum += 1
        if self._DefSecNum > 1:
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_MULTISEC)
    
    ## CheckRequiredFields
    #
    # Check required fields: DEC_SPECIFICATION, PACKAGE_NAME
    #                        PACKAGE_GUID, PACKAGE_VERSION
    #
    def CheckRequiredFields(self):
        Ret = False
        if self.ItemObject.GetPackageSpecification() == '':
            Logger.Error(TOOL_NAME, FILE_PARSE_FAILURE, File=self._RawData.Filename, 
                         ExtraData=ST.ERR_DECPARSE_DEFINE_REQUIRED % DT.TAB_DEC_DEFINES_DEC_SPECIFICATION)
        elif self.ItemObject.GetPackageName() == '':
            Logger.Error(TOOL_NAME, FILE_PARSE_FAILURE, File=self._RawData.Filename, 
                         ExtraData=ST.ERR_DECPARSE_DEFINE_REQUIRED % DT.TAB_DEC_DEFINES_PACKAGE_NAME)
        elif self.ItemObject.GetPackageGuid() == '':
            Logger.Error(TOOL_NAME, FILE_PARSE_FAILURE, File=self._RawData.Filename, 
                         ExtraData=ST.ERR_DECPARSE_DEFINE_REQUIRED % DT.TAB_DEC_DEFINES_PACKAGE_GUID)
        elif self.ItemObject.GetPackageVersion() == '':
            Logger.Error(TOOL_NAME, FILE_PARSE_FAILURE, File=self._RawData.Filename, 
                         ExtraData=ST.ERR_DECPARSE_DEFINE_REQUIRED % DT.TAB_DEC_DEFINES_PACKAGE_VERSION)
        else:
            Ret = True
        return Ret
    
    def _ParseItem(self):
        Line = self._RawData.CurrentLine
        TokenList = GetSplitValueList(Line, DT.TAB_EQUAL_SPLIT, 1)
        if TokenList[0] == DT.TAB_DEC_DEFINES_PKG_UNI_FILE:
            self.DefineValidation[TokenList[0]](TokenList[1])
        elif len(TokenList) < 2:
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_FORMAT)
        elif TokenList[0] not in self.DefineValidation:
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_UNKNOWKEY % TokenList[0])
        else:
            self.DefineValidation[TokenList[0]](TokenList[1])
        
        DefineItem = DecDefineItemObject()
        DefineItem.Key   = TokenList[0]
        DefineItem.Value = TokenList[1]
        self.ItemObject.AddItem(DefineItem, self._RawData.CurrentScope)
        return DefineItem
    
    def _SetDecSpecification(self, Token):
        if self.ItemObject.GetPackageSpecification():
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_DEFINED % DT.TAB_DEC_DEFINES_DEC_SPECIFICATION)
        if not IsValidToken('0[xX][0-9a-fA-F]{8}', Token):
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_SPEC)
        self.ItemObject.SetPackageSpecification(Token)
    
    def _SetPackageName(self, Token):
        if self.ItemObject.GetPackageName():
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_DEFINED % DT.TAB_DEC_DEFINES_PACKAGE_NAME)
        if not IsValidWord(Token):
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_PKGNAME)
        self.ItemObject.SetPackageName(Token)
    
    def _SetPackageGuid(self, Token):
        if self.ItemObject.GetPackageGuid():
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_DEFINED % DT.TAB_DEC_DEFINES_PACKAGE_GUID)
        if not CheckGuidRegFormat(Token):
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_PKGGUID)
        self.ItemObject.SetPackageGuid(Token)
    
    def _SetPackageVersion(self, Token):
        if self.ItemObject.GetPackageVersion():
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_DEFINED % DT.TAB_DEC_DEFINES_PACKAGE_VERSION)
        if not IsValidToken(VERSION_PATTERN, Token):
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_PKGVERSION)
        else:
            if not DT.TAB_SPLIT in Token:
                Token = Token + '.0'
            self.ItemObject.SetPackageVersion(Token)
            
    def _SetPackageUni(self, Token):
        if self.ItemObject.GetPackageUniFile():
            self._LoggerError(ST.ERR_DECPARSE_DEFINE_DEFINED % DT.TAB_DEC_DEFINES_PKG_UNI_FILE)
        self.ItemObject.SetPackageUniFile(Token)

## _DecInclude
#
# Parse include section
#
class _DecInclude(_DecBase):
    def __init__(self, RawData):
        _DecBase.__init__(self, RawData)
        self.ItemObject = DecIncludeObject(RawData.Filename)
    
    def _ParseItem(self):
        Line = self._RawData.CurrentLine
        
        if not IsValidPath(Line, self._RawData.PackagePath):
            self._LoggerError(ST.ERR_DECPARSE_INCLUDE % Line) 
        
        Item = DecIncludeItemObject(StripRoot(self._RawData.PackagePath, Line), self._RawData.PackagePath)
        self.ItemObject.AddItem(Item, self._RawData.CurrentScope)
        return Item

## _DecLibraryclass
#
# Parse library class section
#
class _DecLibraryclass(_DecBase):
    def __init__(self, RawData):
        _DecBase.__init__(self, RawData)
        self.ItemObject = DecLibraryclassObject(RawData.Filename)
    
    def _ParseItem(self):
        Line = self._RawData.CurrentLine
        TokenList = GetSplitValueList(Line, DT.TAB_VALUE_SPLIT)
        if len(TokenList) != 2:
            self._LoggerError(ST.ERR_DECPARSE_LIBCLASS_SPLIT) 
        if TokenList[0] == '' or TokenList[1] == '':
            self._LoggerError(ST.ERR_DECPARSE_LIBCLASS_EMPTY)
        if not IsValidToken('[A-Z][0-9A-Za-z]*', TokenList[0]):
            self._LoggerError(ST.ERR_DECPARSE_LIBCLASS_LIB)
        
        self._CheckReDefine(TokenList[0])
        
        Value = TokenList[1]
        #
        # Must end with .h
        #
        if not Value.endswith('.h'):
            self._LoggerError(ST.ERR_DECPARSE_LIBCLASS_PATH_EXT)
        
        #
        # Path must be existed
        #
        if not IsValidPath(Value, self._RawData.PackagePath):
            self._LoggerError(ST.ERR_DECPARSE_INCLUDE % Value)
        
        Item = DecLibraryclassItemObject(TokenList[0], StripRoot(self._RawData.PackagePath, Value),
                                         self._RawData.PackagePath)
        self.ItemObject.AddItem(Item, self._RawData.CurrentScope)
        return Item

## _DecPcd
#
# Parse PCD section
#
class _DecPcd(_DecBase):
    def __init__(self, RawData):
        _DecBase.__init__(self, RawData)
        self.ItemObject = DecPcdObject(RawData.Filename)
        #
        # Used to check duplicate token
        # Key is token space and token number (integer), value is C name
        #
        self.TokenMap = {}
    
    def _ParseItem(self):
        Line = self._RawData.CurrentLine
        TokenList = Line.split(DT.TAB_VALUE_SPLIT)
        if len(TokenList) < 4:
            self._LoggerError(ST.ERR_DECPARSE_PCD_SPLIT)
        
        #
        # Token space guid C name
        #
        PcdName = GetSplitValueList(TokenList[0], DT.TAB_SPLIT)
        if len(PcdName) != 2 or PcdName[0] == '' or PcdName[1] == '':
            self._LoggerError(ST.ERR_DECPARSE_PCD_NAME)
        
        Guid = PcdName[0]
        if not IsValidToken(CVAR_PATTERN, Guid):
            self._LoggerError(ST.ERR_DECPARSE_PCD_CVAR_GUID)
        
        #
        # PCD C name
        #
        CName = PcdName[1]
        if not IsValidToken(CVAR_PATTERN, CName):
            self._LoggerError(ST.ERR_DECPARSE_PCD_CVAR_PCDCNAME)
        
        self._CheckReDefine(Guid + DT.TAB_SPLIT + CName)
        
        #
        # Default value, may be C array, string or number
        #
        Data = DT.TAB_VALUE_SPLIT.join(TokenList[1:-2]).strip()
        
        #
        # PCD data type
        #
        DataType = TokenList[-2].strip()
        Valid, Cause = IsValidPcdDatum(DataType, Data)
        if not Valid:
            self._LoggerError(Cause)
        PcdType = self._RawData.CurrentScope[0][0]
        if PcdType == DT.TAB_PCDS_FEATURE_FLAG_NULL.upper() and DataType != 'BOOLEAN':
            self._LoggerError(ST.ERR_DECPARSE_PCD_FEATUREFLAG)
        #
        # Token value is the last element in list.
        #
        Token = TokenList[-1].strip()
        if not IsValidToken(PCD_TOKEN_PATTERN, Token):
            self._LoggerError(ST.ERR_DECPARSE_PCD_TOKEN % Token)
        elif not Token.startswith('0x') and not Token.startswith('0X'):
            if long(Token) > 4294967295:
                self._LoggerError(ST.ERR_DECPARSE_PCD_TOKEN_INT % Token)
            Token = hex(long(Token))[:-1]
        
        IntToken = long(Token, 0)
        if (Guid, IntToken) in self.TokenMap:
            if self.TokenMap[Guid, IntToken] != CName:
                self._LoggerError(ST.ERR_DECPARSE_PCD_TOKEN_UNIQUE%(Token))
        else:
            self.TokenMap[Guid, IntToken] = CName
        
        Item = DecPcdItemObject(Guid, CName, Data, DataType, Token)
        self.ItemObject.AddItem(Item, self._RawData.CurrentScope)
        return Item
        
## _DecGuid
#
# Parse GUID, PPI, Protocol section
#
class _DecGuid(_DecBase):
    def __init__(self, RawData):
        _DecBase.__init__(self, RawData)
        self.GuidObj = DecGuidObject(RawData.Filename)
        self.PpiObj = DecPpiObject(RawData.Filename)
        self.ProtocolObj = DecProtocolObject(RawData.Filename)
        self.ObjectDict = \
        {
            DT.TAB_GUIDS.upper()     :   self.GuidObj,
            DT.TAB_PPIS.upper()      :   self.PpiObj,
            DT.TAB_PROTOCOLS.upper() :   self.ProtocolObj
        }
    
    def GetDataObject(self):
        if self._RawData.CurrentScope:
            return self.ObjectDict[self._RawData.CurrentScope[0][0]]
        return None
    
    def GetGuidObject(self):
        return self.GuidObj
    
    def GetPpiObject(self):
        return self.PpiObj
    
    def GetProtocolObject(self):
        return self.ProtocolObj
    
    def _ParseItem(self):
        Line = self._RawData.CurrentLine
        TokenList = GetSplitValueList(Line, DT.TAB_EQUAL_SPLIT, 1)
        if len(TokenList) < 2:
            self._LoggerError(ST.ERR_DECPARSE_CGUID)
        if TokenList[0] == '':
            self._LoggerError(ST.ERR_DECPARSE_CGUID_NAME)
        if TokenList[1] == '':
            self._LoggerError(ST.ERR_DECPARSE_CGUID_GUID)
        if not IsValidToken(CVAR_PATTERN, TokenList[0]):
            self._LoggerError(ST.ERR_DECPARSE_PCD_CVAR_GUID)
        
        self._CheckReDefine(TokenList[0])
        
        if TokenList[1][0] != '{':
            if not CheckGuidRegFormat(TokenList[1]):
                self._LoggerError(ST.ERR_DECPARSE_DEFINE_PKGGUID)
            GuidString = TokenList[1]
        else:
            #
            # Convert C format GUID to GUID string and Simple error check
            #
            GuidString = GuidStructureStringToGuidString(TokenList[1])
            if TokenList[1][0] != '{' or TokenList[1][-1] != '}' or GuidString == '':
                self._LoggerError(ST.ERR_DECPARSE_CGUID_GUIDFORMAT)
    
            #
            # Check C format GUID
            #
            if not IsValidCFormatGuid(TokenList[1]):
                self._LoggerError(ST.ERR_DECPARSE_CGUID_GUIDFORMAT)

        Item = DecGuidItemObject(TokenList[0], TokenList[1], GuidString)
        ItemObject = self.ObjectDict[self._RawData.CurrentScope[0][0]]
        ItemObject.AddItem(Item, self._RawData.CurrentScope)
        return Item

## _DecUserExtension
#
# Parse user extention section
#
class _DecUserExtension(_DecBase):
    def __init__(self, RawData):
        _DecBase.__init__(self, RawData)
        self.ItemObject = DecUserExtensionObject(RawData.Filename)
        self._Headers = []
        self._CurItems = []
    
    def BlockStart(self):
        self._CurItems = []
        for Header in self._RawData.CurrentScope:
            if Header in self._Headers:
                self._LoggerError(ST.ERR_DECPARSE_UE_DUPLICATE)
            else:
                self._Headers.append(Header)
            
            for Item in self._CurItems:
                if Item.UserId == Header[1] and Item.IdString == Header[2]:
                    Item.ArchAndModuleType.append(Header[3])
                    break
            else:
                Item = DecUserExtensionItemObject()
                Item.UserId = Header[1]
                Item.IdString = Header[2]
                Item.ArchAndModuleType.append(Header[3])
                self._CurItems.append(Item)
                self.ItemObject.AddItem(Item, None)
        self._LocalMacro = {}
    
    def _ParseItem(self):
        Line = self._RawData.CurrentLine
        Item = None
        for Item in self._CurItems:
            if Item.UserString:
                Item.UserString = '\n'.join([Item.UserString, Line])
            else:
                Item.UserString = Line
        return Item

## Dec
#
# Top dec parser
#
class Dec(_DecBase, _DecComments):    
    def __init__(self, DecFile, Parse = True):        
        try:
            Content = ConvertSpecialChar(open(DecFile, 'rb').readlines())
        except BaseException:
            Logger.Error(TOOL_NAME, FILE_OPEN_FAILURE, File=DecFile,
                         ExtraData=ST.ERR_DECPARSE_FILEOPEN % DecFile)
        RawData = FileContent(DecFile, Content)
        
        _DecComments.__init__(self)
        _DecBase.__init__(self, RawData)
        
        self.BinaryHeadComment = []
        self.PcdErrorCommentDict = {}
        
        self._Define    = _DecDefine(RawData)
        self._Include   = _DecInclude(RawData)
        self._Guid      = _DecGuid(RawData)
        self._LibClass  = _DecLibraryclass(RawData)
        self._Pcd       = _DecPcd(RawData)
        self._UserEx    = _DecUserExtension(RawData)
        
        #
        # DEC file supported data types (one type per section)
        #
        self._SectionParser = {
            DT.TAB_DEC_DEFINES.upper()                     :   self._Define,
            DT.TAB_INCLUDES.upper()                        :   self._Include,
            DT.TAB_LIBRARY_CLASSES.upper()                 :   self._LibClass,
            DT.TAB_GUIDS.upper()                           :   self._Guid,
            DT.TAB_PPIS.upper()                            :   self._Guid,
            DT.TAB_PROTOCOLS.upper()                       :   self._Guid,
            DT.TAB_PCDS_FIXED_AT_BUILD_NULL.upper()        :   self._Pcd,
            DT.TAB_PCDS_PATCHABLE_IN_MODULE_NULL.upper()   :   self._Pcd,
            DT.TAB_PCDS_FEATURE_FLAG_NULL.upper()          :   self._Pcd,
            DT.TAB_PCDS_DYNAMIC_NULL.upper()               :   self._Pcd,
            DT.TAB_PCDS_DYNAMIC_EX_NULL.upper()            :   self._Pcd,
            DT.TAB_USER_EXTENSIONS.upper()                 :   self._UserEx
        }

        if Parse:
            self.ParseDecComment()
            self.Parse()
            #
            # Parsing done, check required fields
            #
            self.CheckRequiredFields()
    
    def CheckRequiredFields(self):
        for SectionParser in self._SectionParser.values():
            if not SectionParser.CheckRequiredFields():
                return False
        return True

    ##
    # Parse DEC file
    #
    def ParseDecComment(self):
        IsFileHeader = False
        IsBinaryHeader = False
        FileHeaderLineIndex = -1
        BinaryHeaderLineIndex = -1
        TokenSpaceGuidCName = ''
        
        #
        # Parse PCD error comment section
        #
        while not self._RawData.IsEndOfFile():
            self._RawData.CurrentLine = self._RawData.GetNextLine()
            if self._RawData.CurrentLine.startswith(DT.TAB_COMMENT_SPLIT) and \
                DT.TAB_SECTION_START in self._RawData.CurrentLine and \
                DT.TAB_SECTION_END in self._RawData.CurrentLine:
                self._RawData.CurrentLine = self._RawData.CurrentLine.replace(DT.TAB_COMMENT_SPLIT, '').strip()

                if self._RawData.CurrentLine[0] == DT.TAB_SECTION_START and \
                    self._RawData.CurrentLine[-1] == DT.TAB_SECTION_END:
                    RawSection = self._RawData.CurrentLine[1:-1].strip()
                    if RawSection.upper().startswith(DT.TAB_PCD_ERROR.upper()+'.'):
                        TokenSpaceGuidCName = RawSection.split(DT.TAB_PCD_ERROR+'.')[1].strip()
                        continue

            if TokenSpaceGuidCName and self._RawData.CurrentLine.startswith(DT.TAB_COMMENT_SPLIT):
                self._RawData.CurrentLine = self._RawData.CurrentLine.replace(DT.TAB_COMMENT_SPLIT, '').strip()
                if self._RawData.CurrentLine != '':
                    if DT.TAB_VALUE_SPLIT not in self._RawData.CurrentLine:
                        self._LoggerError(ST.ERR_DECPARSE_PCDERRORMSG_MISS_VALUE_SPLIT)   
                          
                    PcdErrorNumber, PcdErrorMsg = GetSplitValueList(self._RawData.CurrentLine, DT.TAB_VALUE_SPLIT, 1)
                    PcdErrorNumber = ParsePcdErrorCode(PcdErrorNumber, self._RawData.Filename, self._RawData.LineIndex)
                    if not PcdErrorMsg.strip():
                        self._LoggerError(ST.ERR_DECPARSE_PCD_MISS_ERRORMSG)
                        
                    self.PcdErrorCommentDict[(TokenSpaceGuidCName, PcdErrorNumber)] = PcdErrorMsg.strip()
            else:
                TokenSpaceGuidCName = ''

        self._RawData.LineIndex = 0
        self._RawData.CurrentLine = ''
        self._RawData.NextLine = ''

        while not self._RawData.IsEndOfFile():
            Line, Comment = CleanString(self._RawData.GetNextLine())
            
            #
            # Header must be pure comment
            #
            if Line != '':
                self._RawData.UndoNextLine()
                break
            
            if Comment and Comment.startswith(DT.TAB_SPECIAL_COMMENT) and Comment.find(DT.TAB_HEADER_COMMENT) > 0 \
                and not Comment[2:Comment.find(DT.TAB_HEADER_COMMENT)].strip():
                IsFileHeader = True
                IsBinaryHeader = False
                FileHeaderLineIndex = self._RawData.LineIndex
                
            #
            # Get license information before '@file' 
            #   
            if not IsFileHeader and not IsBinaryHeader and Comment and Comment.startswith(DT.TAB_COMMENT_SPLIT) and \
            DT.TAB_BINARY_HEADER_COMMENT not in Comment:
                self._HeadComment.append((Comment, self._RawData.LineIndex))
            
            if Comment and IsFileHeader and \
            not(Comment.startswith(DT.TAB_SPECIAL_COMMENT) \
            and Comment.find(DT.TAB_BINARY_HEADER_COMMENT) > 0):
                self._HeadComment.append((Comment, self._RawData.LineIndex))
            #
            # Double '#' indicates end of header comments
            #
            if (not Comment or Comment == DT.TAB_SPECIAL_COMMENT) and IsFileHeader:
                IsFileHeader = False  
                continue
            
            if Comment and Comment.startswith(DT.TAB_SPECIAL_COMMENT) \
            and Comment.find(DT.TAB_BINARY_HEADER_COMMENT) > 0:
                IsBinaryHeader = True
                IsFileHeader = False
                BinaryHeaderLineIndex = self._RawData.LineIndex
                
            if Comment and IsBinaryHeader:
                self.BinaryHeadComment.append((Comment, self._RawData.LineIndex))
            #
            # Double '#' indicates end of header comments
            #
            if (not Comment or Comment == DT.TAB_SPECIAL_COMMENT) and IsBinaryHeader:
                IsBinaryHeader = False
                break
            
            if FileHeaderLineIndex > -1 and not IsFileHeader and not IsBinaryHeader:
                break

        if FileHeaderLineIndex > BinaryHeaderLineIndex and FileHeaderLineIndex > -1 and BinaryHeaderLineIndex > -1:
            self._LoggerError(ST.ERR_BINARY_HEADER_ORDER)
            
        if FileHeaderLineIndex == -1:
#            self._LoggerError(ST.ERR_NO_SOURCE_HEADER)
            Logger.Error(TOOL_NAME, FORMAT_INVALID, 
                         ST.ERR_NO_SOURCE_HEADER,
                         File=self._RawData.Filename)
        return
    
    def _StopCurrentParsing(self, Line):
        return False
    
    def _ParseItem(self):
        self._SectionHeaderParser()
        if len(self._RawData.CurrentScope) == 0:
            self._LoggerError(ST.ERR_DECPARSE_SECTION_EMPTY)
        SectionObj = self._SectionParser[self._RawData.CurrentScope[0][0]]
        SectionObj.BlockStart()
        SectionObj.Parse()
        return SectionObj.GetDataObject()

    def _UserExtentionSectionParser(self):
        self._RawData.CurrentScope = []
        ArchList = set()
        Section = self._RawData.CurrentLine[1:-1]
        Par = ParserHelper(Section, self._RawData.Filename)
        while not Par.End():
            #
            # User extention
            #
            Token = Par.GetToken()
            if Token.upper() != DT.TAB_USER_EXTENSIONS.upper():
                self._LoggerError(ST.ERR_DECPARSE_SECTION_UE)
            UserExtension = Token.upper()
            Par.AssertChar(DT.TAB_SPLIT, ST.ERR_DECPARSE_SECTION_UE, self._RawData.LineIndex)     
            
            #
            # UserID
            #
            Token = Par.GetToken()
            if not IsValidUserId(Token):
                self._LoggerError(ST.ERR_DECPARSE_SECTION_UE_USERID)
            UserId = Token
            Par.AssertChar(DT.TAB_SPLIT, ST.ERR_DECPARSE_SECTION_UE, self._RawData.LineIndex)
            #
            # IdString
            #
            Token = Par.GetToken()
            if not IsValidIdString(Token):
                self._LoggerError(ST.ERR_DECPARSE_SECTION_UE_IDSTRING)
            IdString = Token
            Arch = 'COMMON'
            if Par.Expect(DT.TAB_SPLIT):
                Token = Par.GetToken()
                Arch = Token.upper()
                if not IsValidArch(Arch):
                    self._LoggerError(ST.ERR_DECPARSE_ARCH)
            ArchList.add(Arch)
            if [UserExtension, UserId, IdString, Arch] not in \
                self._RawData.CurrentScope:
                self._RawData.CurrentScope.append(
                    [UserExtension, UserId, IdString, Arch]
                )
            if not Par.Expect(DT.TAB_COMMA_SPLIT):
                break
            elif Par.End():
                self._LoggerError(ST.ERR_DECPARSE_SECTION_COMMA)
        Par.AssertEnd(ST.ERR_DECPARSE_SECTION_UE, self._RawData.LineIndex)
        if 'COMMON' in ArchList and len(ArchList) > 1:
            self._LoggerError(ST.ERR_DECPARSE_SECTION_COMMON)
 
    ## Section header parser
    #
    # The section header is always in following format:
    #
    # [section_name.arch<.platform|module_type>]
    #
    def _SectionHeaderParser(self):
        if self._RawData.CurrentLine[0] != DT.TAB_SECTION_START or self._RawData.CurrentLine[-1] != DT.TAB_SECTION_END:
            self._LoggerError(ST.ERR_DECPARSE_SECTION_IDENTIFY)
        
        RawSection = self._RawData.CurrentLine[1:-1].strip().upper()
        #
        # Check defines section which is only allowed to occur once and
        # no arch can be followed
        #
        if RawSection.startswith(DT.TAB_DEC_DEFINES.upper()):
            if RawSection != DT.TAB_DEC_DEFINES.upper():
                self._LoggerError(ST.ERR_DECPARSE_DEFINE_SECNAME)
        #
        # Check user extension section
        #
        if RawSection.startswith(DT.TAB_USER_EXTENSIONS.upper()):
            return self._UserExtentionSectionParser()
        self._RawData.CurrentScope = []
        SectionNames = []
        ArchList = set()
        for Item in GetSplitValueList(RawSection, DT.TAB_COMMA_SPLIT):
            if Item == '':
                self._LoggerError(ST.ERR_DECPARSE_SECTION_SUBEMPTY % self._RawData.CurrentLine)

            ItemList = GetSplitValueList(Item, DT.TAB_SPLIT)
            #
            # different types of PCD are permissible in one section
            #
            SectionName = ItemList[0]
            if SectionName not in self._SectionParser:
                self._LoggerError(ST.ERR_DECPARSE_SECTION_UNKNOW % SectionName)
            if SectionName not in SectionNames:
                SectionNames.append(SectionName)
            #
            # In DEC specification, all section headers have at most two part:
            # SectionName.Arch except UserExtention
            #
            if len(ItemList) > 2:
                self._LoggerError(ST.ERR_DECPARSE_SECTION_SUBTOOMANY % Item)

            if DT.TAB_PCDS_FEATURE_FLAG_NULL.upper() in SectionNames and len(SectionNames) > 1:
                self._LoggerError(ST.ERR_DECPARSE_SECTION_FEATUREFLAG % DT.TAB_PCDS_FEATURE_FLAG_NULL) 
            #
            # S1 is always Arch
            #
            if len(ItemList) > 1:
                Str1 = ItemList[1]
                if not IsValidArch(Str1):
                    self._LoggerError(ST.ERR_DECPARSE_ARCH)
            else:
                Str1 = 'COMMON'
            ArchList.add(Str1)

            if [SectionName, Str1] not in self._RawData.CurrentScope:
                self._RawData.CurrentScope.append([SectionName, Str1])
        #
        # 'COMMON' must not be used with specific ARCHs at the same section
        #
        if 'COMMON' in ArchList and len(ArchList) > 1:
            self._LoggerError(ST.ERR_DECPARSE_SECTION_COMMON)
        if len(SectionNames) == 0:
            self._LoggerError(ST.ERR_DECPARSE_SECTION_SUBEMPTY % self._RawData.CurrentLine)
        if len(SectionNames) != 1:
            for Sec in SectionNames:
                if not Sec.startswith(DT.TAB_PCDS.upper()):
                    self._LoggerError(ST.ERR_DECPARSE_SECTION_NAME % str(SectionNames))
    
    def GetDefineSectionMacro(self):
        return self._Define.GetLocalMacro()
    def GetDefineSectionObject(self):
        return self._Define.GetDataObject()
    def GetIncludeSectionObject(self):
        return self._Include.GetDataObject()
    def GetGuidSectionObject(self):
        return self._Guid.GetGuidObject()
    def GetProtocolSectionObject(self):
        return self._Guid.GetProtocolObject()
    def GetPpiSectionObject(self):
        return self._Guid.GetPpiObject()
    def GetLibraryClassSectionObject(self):
        return self._LibClass.GetDataObject()
    def GetPcdSectionObject(self):
        return self._Pcd.GetDataObject()
    def GetUserExtensionSectionObject(self):
        return self._UserEx.GetDataObject()
    def GetPackageSpecification(self):
        return self._Define.GetDataObject().GetPackageSpecification()   
    def GetPackageName(self):
        return self._Define.GetDataObject().GetPackageName()   
    def GetPackageGuid(self):
        return self._Define.GetDataObject().GetPackageGuid()    
    def GetPackageVersion(self):
        return self._Define.GetDataObject().GetPackageVersion()
    def GetPackageUniFile(self):
        return self._Define.GetDataObject().GetPackageUniFile()
