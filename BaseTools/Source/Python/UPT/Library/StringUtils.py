## @file
# This file is used to define common string related functions used in parsing
# process
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
'''
StringUtils
'''
##
# Import Modules
#
import re
import os.path
import Logger.Log as Logger
import Library.DataType as DataType
from Logger.ToolError import FORMAT_INVALID
from Logger.ToolError import PARSER_ERROR
from Logger import StringTable as ST

#
# Regular expression for matching macro used in DSC/DEC/INF file inclusion
#
gMACRO_PATTERN = re.compile("\$\(([_A-Z][_A-Z0-9]*)\)", re.UNICODE)

## GetSplitValueList
#
# Get a value list from a string with multiple values split with SplitTag
# The default SplitTag is DataType.TAB_VALUE_SPLIT
# 'AAA|BBB|CCC' -> ['AAA', 'BBB', 'CCC']
#
# @param String:    The input string to be splitted
# @param SplitTag:  The split key, default is DataType.TAB_VALUE_SPLIT
# @param MaxSplit:  The max number of split values, default is -1
#
#
def GetSplitValueList(String, SplitTag=DataType.TAB_VALUE_SPLIT, MaxSplit= -1):
    return list(map(lambda l: l.strip(), String.split(SplitTag, MaxSplit)))

## MergeArches
#
# Find a key's all arches in dict, add the new arch to the list
# If not exist any arch, set the arch directly
#
# @param Dict:  The input value for Dict
# @param Key:   The input value for Key
# @param Arch:  The Arch to be added or merged
#
def MergeArches(Dict, Key, Arch):
    if Key in Dict.keys():
        Dict[Key].append(Arch)
    else:
        Dict[Key] = Arch.split()

## GenDefines
#
# Parse a string with format "DEFINE <VarName> = <PATH>"
# Generate a map Defines[VarName] = PATH
# Return False if invalid format
#
# @param String:   String with DEFINE statement
# @param Arch:     Supported Arch
# @param Defines:  DEFINE statement to be parsed
#
def GenDefines(String, Arch, Defines):
    if String.find(DataType.TAB_DEFINE + ' ') > -1:
        List = String.replace(DataType.TAB_DEFINE + ' ', '').\
        split(DataType.TAB_EQUAL_SPLIT)
        if len(List) == 2:
            Defines[(CleanString(List[0]), Arch)] = CleanString(List[1])
            return 0
        else:
            return -1
    return 1

## GetLibraryClassesWithModuleType
#
# Get Library Class definition when no module type defined
#
# @param Lines:             The content to be parsed
# @param Key:               Reserved
# @param KeyValues:         To store data after parsing
# @param CommentCharacter:  Comment char, used to ignore comment content
#
def GetLibraryClassesWithModuleType(Lines, Key, KeyValues, CommentCharacter):
    NewKey = SplitModuleType(Key)
    Lines = Lines.split(DataType.TAB_SECTION_END, 1)[1]
    LineList = Lines.splitlines()
    for Line in LineList:
        Line = CleanString(Line, CommentCharacter)
        if Line != '' and Line[0] != CommentCharacter:
            KeyValues.append([CleanString(Line, CommentCharacter), NewKey[1]])

    return True

## GetDynamics
#
# Get Dynamic Pcds
#
# @param Lines:             The content to be parsed
# @param Key:               Reserved
# @param KeyValues:         To store data after parsing
# @param CommentCharacter:  Comment char, used to ignore comment content
#
def GetDynamics(Lines, Key, KeyValues, CommentCharacter):
    #
    # Get SkuId Name List
    #
    SkuIdNameList = SplitModuleType(Key)

    Lines = Lines.split(DataType.TAB_SECTION_END, 1)[1]
    LineList = Lines.splitlines()
    for Line in LineList:
        Line = CleanString(Line, CommentCharacter)
        if Line != '' and Line[0] != CommentCharacter:
            KeyValues.append([CleanString(Line, CommentCharacter), SkuIdNameList[1]])

    return True

## SplitModuleType
#
# Split ModuleType out of section defien to get key
# [LibraryClass.Arch.ModuleType|ModuleType|ModuleType] -> [
# 'LibraryClass.Arch', ['ModuleType', 'ModuleType', 'ModuleType'] ]
#
# @param Key:  String to be parsed
#
def SplitModuleType(Key):
    KeyList = Key.split(DataType.TAB_SPLIT)
    #
    # Fill in for arch
    #
    KeyList.append('')
    #
    # Fill in for moduletype
    #
    KeyList.append('')
    ReturnValue = []
    KeyValue = KeyList[0]
    if KeyList[1] != '':
        KeyValue = KeyValue + DataType.TAB_SPLIT + KeyList[1]
    ReturnValue.append(KeyValue)
    ReturnValue.append(GetSplitValueList(KeyList[2]))

    return ReturnValue

## Replace macro in string
#
# This method replace macros used in given string. The macros are given in a
# dictionary.
#
# @param String             String to be processed
# @param MacroDefinitions   The macro definitions in the form of dictionary
# @param SelfReplacement    To decide whether replace un-defined macro to ''
# @param Line:              The content contain line string and line number
# @param FileName:        The meta-file file name
#
def ReplaceMacro(String, MacroDefinitions=None, SelfReplacement=False, Line=None, FileName=None, Flag=False):
    LastString = String
    if MacroDefinitions is None:
        MacroDefinitions = {}
    while MacroDefinitions:
        QuotedStringList = []
        HaveQuotedMacroFlag = False
        if not Flag:
            MacroUsed = gMACRO_PATTERN.findall(String)
        else:
            ReQuotedString = re.compile('\"')
            QuotedStringList = ReQuotedString.split(String)
            if len(QuotedStringList) >= 3:
                HaveQuotedMacroFlag = True
            Count = 0
            MacroString = ""
            for QuotedStringItem in QuotedStringList:
                Count += 1
                if Count % 2 != 0:
                    MacroString += QuotedStringItem

                if Count == len(QuotedStringList) and Count % 2 == 0:
                    MacroString += QuotedStringItem

            MacroUsed = gMACRO_PATTERN.findall(MacroString)
        #
        # no macro found in String, stop replacing
        #
        if len(MacroUsed) == 0:
            break
        for Macro in MacroUsed:
            if Macro not in MacroDefinitions:
                if SelfReplacement:
                    String = String.replace("$(%s)" % Macro, '')
                    Logger.Debug(5, "Delete undefined MACROs in file %s line %d: %s!" % (FileName, Line[1], Line[0]))
                continue
            if not HaveQuotedMacroFlag:
                String = String.replace("$(%s)" % Macro, MacroDefinitions[Macro])
            else:
                Count = 0
                for QuotedStringItem in QuotedStringList:
                    Count += 1
                    if Count % 2 != 0:
                        QuotedStringList[Count - 1] = QuotedStringList[Count - 1].replace("$(%s)" % Macro,
                                                                        MacroDefinitions[Macro])
                    elif Count == len(QuotedStringList) and Count % 2 == 0:
                        QuotedStringList[Count - 1] = QuotedStringList[Count - 1].replace("$(%s)" % Macro,
                                                                        MacroDefinitions[Macro])

        RetString = ''
        if HaveQuotedMacroFlag:
            Count = 0
            for QuotedStringItem in QuotedStringList:
                Count += 1
                if Count != len(QuotedStringList):
                    RetString += QuotedStringList[Count - 1] + "\""
                else:
                    RetString += QuotedStringList[Count - 1]

            String = RetString

        #
        # in case there's macro not defined
        #
        if String == LastString:
            break
        LastString = String

    return String

## NormPath
#
# Create a normal path
# And replace DEFINE in the path
#
# @param Path:     The input value for Path to be converted
# @param Defines:  A set for DEFINE statement
#
def NormPath(Path, Defines=None):
    IsRelativePath = False
    if Defines is None:
        Defines = {}
    if Path:
        if Path[0] == '.':
            IsRelativePath = True
        #
        # Replace with Define
        #
        if Defines:
            Path = ReplaceMacro(Path, Defines)
        #
        # To local path format
        #
        Path = os.path.normpath(Path)

    if IsRelativePath and Path[0] != '.':
        Path = os.path.join('.', Path)
    return Path

## CleanString
#
# Remove comments in a string
# Remove spaces
#
# @param Line:              The string to be cleaned
# @param CommentCharacter:  Comment char, used to ignore comment content,
#                           default is DataType.TAB_COMMENT_SPLIT
#
def CleanString(Line, CommentCharacter=DataType.TAB_COMMENT_SPLIT, AllowCppStyleComment=False):
    #
    # remove whitespace
    #
    Line = Line.strip()
    #
    # Replace EDK1's comment character
    #
    if AllowCppStyleComment:
        Line = Line.replace(DataType.TAB_COMMENT_EDK1_SPLIT, CommentCharacter)
    #
    # remove comments, but we should escape comment character in string
    #
    InString = False
    for Index in range(0, len(Line)):
        if Line[Index] == '"':
            InString = not InString
        elif Line[Index] == CommentCharacter and not InString:
            Line = Line[0: Index]
            break
    #
    # remove whitespace again
    #
    Line = Line.strip()

    return Line

## CleanString2
#
# Split comments in a string
# Remove spaces
#
# @param Line:              The string to be cleaned
# @param CommentCharacter:  Comment char, used to ignore comment content,
#                           default is DataType.TAB_COMMENT_SPLIT
#
def CleanString2(Line, CommentCharacter=DataType.TAB_COMMENT_SPLIT, AllowCppStyleComment=False):
    #
    # remove whitespace
    #
    Line = Line.strip()
    #
    # Replace EDK1's comment character
    #
    if AllowCppStyleComment:
        Line = Line.replace(DataType.TAB_COMMENT_EDK1_SPLIT, CommentCharacter)
    #
    # separate comments and statements
    #
    LineParts = Line.split(CommentCharacter, 1)
    #
    # remove whitespace again
    #
    Line = LineParts[0].strip()
    if len(LineParts) > 1:
        Comment = LineParts[1].strip()
        #
        # Remove prefixed and trailing comment characters
        #
        Start = 0
        End = len(Comment)
        while Start < End and Comment.startswith(CommentCharacter, Start, End):
            Start += 1
        while End >= 0 and Comment.endswith(CommentCharacter, Start, End):
            End -= 1
        Comment = Comment[Start:End]
        Comment = Comment.strip()
    else:
        Comment = ''

    return Line, Comment

## GetMultipleValuesOfKeyFromLines
#
# Parse multiple strings to clean comment and spaces
# The result is saved to KeyValues
#
# @param Lines:             The content to be parsed
# @param Key:               Reserved
# @param KeyValues:         To store data after parsing
# @param CommentCharacter:  Comment char, used to ignore comment content
#
def GetMultipleValuesOfKeyFromLines(Lines, Key, KeyValues, CommentCharacter):
    if Key:
        pass
    if KeyValues:
        pass
    Lines = Lines.split(DataType.TAB_SECTION_END, 1)[1]
    LineList = Lines.split('\n')
    for Line in LineList:
        Line = CleanString(Line, CommentCharacter)
        if Line != '' and Line[0] != CommentCharacter:
            KeyValues += [Line]
    return True

## GetDefineValue
#
# Parse a DEFINE statement to get defined value
# DEFINE Key Value
#
# @param String:            The content to be parsed
# @param Key:               The key of DEFINE statement
# @param CommentCharacter:  Comment char, used to ignore comment content
#
def GetDefineValue(String, Key, CommentCharacter):
    if CommentCharacter:
        pass
    String = CleanString(String)
    return String[String.find(Key + ' ') + len(Key + ' ') : ]

## GetSingleValueOfKeyFromLines
#
# Parse multiple strings as below to get value of each definition line
# Key1 = Value1
# Key2 = Value2
# The result is saved to Dictionary
#
# @param Lines:                The content to be parsed
# @param Dictionary:           To store data after parsing
# @param CommentCharacter:     Comment char, be used to ignore comment content
# @param KeySplitCharacter:    Key split char, between key name and key value.
#                              Key1 = Value1, '=' is the key split char
# @param ValueSplitFlag:       Value split flag, be used to decide if has
#                              multiple values
# @param ValueSplitCharacter:  Value split char, be used to split multiple
#                              values. Key1 = Value1|Value2, '|' is the value
#                              split char
#
def GetSingleValueOfKeyFromLines(Lines, Dictionary, CommentCharacter, KeySplitCharacter, \
                                 ValueSplitFlag, ValueSplitCharacter):
    Lines = Lines.split('\n')
    Keys = []
    Value = ''
    DefineValues = ['']
    SpecValues = ['']

    for Line in Lines:
        #
        # Handle DEFINE and SPEC
        #
        if Line.find(DataType.TAB_INF_DEFINES_DEFINE + ' ') > -1:
            if '' in DefineValues:
                DefineValues.remove('')
            DefineValues.append(GetDefineValue(Line, DataType.TAB_INF_DEFINES_DEFINE, CommentCharacter))
            continue
        if Line.find(DataType.TAB_INF_DEFINES_SPEC + ' ') > -1:
            if '' in SpecValues:
                SpecValues.remove('')
            SpecValues.append(GetDefineValue(Line, DataType.TAB_INF_DEFINES_SPEC, CommentCharacter))
            continue

        #
        # Handle Others
        #
        LineList = Line.split(KeySplitCharacter, 1)
        if len(LineList) >= 2:
            Key = LineList[0].split()
            if len(Key) == 1 and Key[0][0] != CommentCharacter:
                #
                # Remove comments and white spaces
                #
                LineList[1] = CleanString(LineList[1], CommentCharacter)
                if ValueSplitFlag:
                    Value = list(map(lambda x: x.strip(), LineList[1].split(ValueSplitCharacter)))
                else:
                    Value = CleanString(LineList[1], CommentCharacter).splitlines()

                if Key[0] in Dictionary:
                    if Key[0] not in Keys:
                        Dictionary[Key[0]] = Value
                        Keys.append(Key[0])
                    else:
                        Dictionary[Key[0]].extend(Value)
                else:
                    Dictionary[DataType.TAB_INF_DEFINES_MACRO][Key[0]] = Value[0]

    if DefineValues == []:
        DefineValues = ['']
    if SpecValues == []:
        SpecValues = ['']
    Dictionary[DataType.TAB_INF_DEFINES_DEFINE] = DefineValues
    Dictionary[DataType.TAB_INF_DEFINES_SPEC] = SpecValues

    return True

## The content to be parsed
#
# Do pre-check for a file before it is parsed
# Check $()
# Check []
#
# @param FileName:       Used for error report
# @param FileContent:    File content to be parsed
# @param SupSectionTag:  Used for error report
#
def PreCheck(FileName, FileContent, SupSectionTag):
    if SupSectionTag:
        pass
    LineNo = 0
    IsFailed = False
    NewFileContent = ''
    for Line in FileContent.splitlines():
        LineNo = LineNo + 1
        #
        # Clean current line
        #
        Line = CleanString(Line)
        #
        # Remove commented line
        #
        if Line.find(DataType.TAB_COMMA_SPLIT) == 0:
            Line = ''
        #
        # Check $()
        #
        if Line.find('$') > -1:
            if Line.find('$(') < 0 or Line.find(')') < 0:
                Logger.Error("Parser", FORMAT_INVALID, Line=LineNo, File=FileName, RaiseError=Logger.IS_RAISE_ERROR)
        #
        # Check []
        #
        if Line.find('[') > -1 or Line.find(']') > -1:
            #
            # Only get one '[' or one ']'
            #
            if not (Line.find('[') > -1 and Line.find(']') > -1):
                Logger.Error("Parser", FORMAT_INVALID, Line=LineNo, File=FileName, RaiseError=Logger.IS_RAISE_ERROR)
        #
        # Regenerate FileContent
        #
        NewFileContent = NewFileContent + Line + '\r\n'

    if IsFailed:
        Logger.Error("Parser", FORMAT_INVALID, Line=LineNo, File=FileName, RaiseError=Logger.IS_RAISE_ERROR)

    return NewFileContent

## CheckFileType
#
# Check if the Filename is including ExtName
# Return True if it exists
# Raise a error message if it not exists
#
# @param CheckFilename:      Name of the file to be checked
# @param ExtName:            Ext name of the file to be checked
# @param ContainerFilename:  The container file which describes the file to be
#                            checked, used for error report
# @param SectionName:        Used for error report
# @param Line:               The line in container file which defines the file
#                            to be checked
#
def CheckFileType(CheckFilename, ExtName, ContainerFilename, SectionName, Line, LineNo= -1):
    if CheckFilename != '' and CheckFilename is not None:
        (Root, Ext) = os.path.splitext(CheckFilename)
        if Ext.upper() != ExtName.upper() and Root:
            ContainerFile = open(ContainerFilename, 'r').read()
            if LineNo == -1:
                LineNo = GetLineNo(ContainerFile, Line)
            ErrorMsg = ST.ERR_SECTIONNAME_INVALID % (SectionName, CheckFilename, ExtName)
            Logger.Error("Parser", PARSER_ERROR, ErrorMsg, Line=LineNo, \
                         File=ContainerFilename, RaiseError=Logger.IS_RAISE_ERROR)

    return True

## CheckFileExist
#
# Check if the file exists
# Return True if it exists
# Raise a error message if it not exists
#
# @param CheckFilename:      Name of the file to be checked
# @param WorkspaceDir:       Current workspace dir
# @param ContainerFilename:  The container file which describes the file to
#                            be checked, used for error report
# @param SectionName:        Used for error report
# @param Line:               The line in container file which defines the
#                            file to be checked
#
def CheckFileExist(WorkspaceDir, CheckFilename, ContainerFilename, SectionName, Line, LineNo= -1):
    CheckFile = ''
    if CheckFilename != '' and CheckFilename is not None:
        CheckFile = WorkspaceFile(WorkspaceDir, CheckFilename)
        if not os.path.isfile(CheckFile):
            ContainerFile = open(ContainerFilename, 'r').read()
            if LineNo == -1:
                LineNo = GetLineNo(ContainerFile, Line)
            ErrorMsg = ST.ERR_CHECKFILE_NOTFOUND % (CheckFile, SectionName)
            Logger.Error("Parser", PARSER_ERROR, ErrorMsg,
                            File=ContainerFilename, Line=LineNo, RaiseError=Logger.IS_RAISE_ERROR)
    return CheckFile

## GetLineNo
#
# Find the index of a line in a file
#
# @param FileContent:  Search scope
# @param Line:         Search key
#
def GetLineNo(FileContent, Line, IsIgnoreComment=True):
    LineList = FileContent.splitlines()
    for Index in range(len(LineList)):
        if LineList[Index].find(Line) > -1:
            #
            # Ignore statement in comment
            #
            if IsIgnoreComment:
                if LineList[Index].strip()[0] == DataType.TAB_COMMENT_SPLIT:
                    continue
            return Index + 1

    return -1

## RaiseParserError
#
# Raise a parser error
#
# @param Line:     String which has error
# @param Section:  Used for error report
# @param File:     File which has the string
# @param Format:   Correct format
#
def RaiseParserError(Line, Section, File, Format='', LineNo= -1):
    if LineNo == -1:
        LineNo = GetLineNo(open(os.path.normpath(File), 'r').read(), Line)
    ErrorMsg = ST.ERR_INVALID_NOTFOUND % (Line, Section)
    if Format != '':
        Format = "Correct format is " + Format
    Logger.Error("Parser", PARSER_ERROR, ErrorMsg, File=File, Line=LineNo, \
                 ExtraData=Format, RaiseError=Logger.IS_RAISE_ERROR)

## WorkspaceFile
#
# Return a full path with workspace dir
#
# @param WorkspaceDir:  Workspace dir
# @param Filename:      Relative file name
#
def WorkspaceFile(WorkspaceDir, Filename):
    return os.path.join(NormPath(WorkspaceDir), NormPath(Filename))

## Split string
#
# Remove '"' which startswith and endswith string
#
# @param String:  The string need to be split
#
def SplitString(String):
    if String.startswith('\"'):
        String = String[1:]
    if String.endswith('\"'):
        String = String[:-1]
    return String

## Convert To Sql String
#
# Replace "'" with "''" in each item of StringList
#
# @param StringList:  A list for strings to be converted
#
def ConvertToSqlString(StringList):
    return list(map(lambda s: s.replace("'", "''"), StringList))

## Convert To Sql String
#
# Replace "'" with "''" in the String
#
# @param String:  A String to be converted
#
def ConvertToSqlString2(String):
    return String.replace("'", "''")

## GetStringOfList
#
# Get String of a List
#
# @param Lines: string list
# @param Split: split character
#
def GetStringOfList(List, Split=' '):
    if not isinstance(List, type([])):
        return List
    Str = ''
    for Item in List:
        Str = Str + Item + Split
    return Str.strip()

## Get HelpTextList
#
# Get HelpTextList from HelpTextClassList
#
# @param HelpTextClassList: Help Text Class List
#
def GetHelpTextList(HelpTextClassList):
    List = []
    if HelpTextClassList:
        for HelpText in HelpTextClassList:
            if HelpText.String.endswith('\n'):
                HelpText.String = HelpText.String[0: len(HelpText.String) - len('\n')]
                List.extend(HelpText.String.split('\n'))
    return List

## Get String Array Length
#
# Get String Array Length
#
# @param String: the source string
#
def StringArrayLength(String):
    if String.startswith('L"'):
        return (len(String) - 3 + 1) * 2
    elif String.startswith('"'):
        return (len(String) - 2 + 1)
    else:
        return len(String.split()) + 1

## RemoveDupOption
#
# Remove Dup Option
#
# @param OptionString: the option string
# @param Which: Which flag
# @param Against: Against flag
#
def RemoveDupOption(OptionString, Which="/I", Against=None):
    OptionList = OptionString.split()
    ValueList = []
    if Against:
        ValueList += Against
    for Index in range(len(OptionList)):
        Opt = OptionList[Index]
        if not Opt.startswith(Which):
            continue
        if len(Opt) > len(Which):
            Val = Opt[len(Which):]
        else:
            Val = ""
        if Val in ValueList:
            OptionList[Index] = ""
        else:
            ValueList.append(Val)
    return " ".join(OptionList)

## Check if the string is HexDgit
#
# Return true if all characters in the string are digits and there is at
# least one character
# or valid Hexs (started with 0x, following by hexdigit letters)
# , false otherwise.
# @param string: input string
#
def IsHexDigit(Str):
    try:
        int(Str, 10)
        return True
    except ValueError:
        if len(Str) > 2 and Str.upper().startswith('0X'):
            try:
                int(Str, 16)
                return True
            except ValueError:
                return False
    return False

## Check if the string is HexDgit and its integer value within limit of UINT32
#
# Return true if all characters in the string are digits and there is at
# least one character
# or valid Hexs (started with 0x, following by hexdigit letters)
# , false otherwise.
# @param string: input string
#
def IsHexDigitUINT32(Str):
    try:
        Value = int(Str, 10)
        if (Value <= 0xFFFFFFFF) and (Value >= 0):
            return True
    except ValueError:
        if len(Str) > 2 and Str.upper().startswith('0X'):
            try:
                Value = int(Str, 16)
                if (Value <= 0xFFFFFFFF) and (Value >= 0):
                    return True
            except ValueError:
                return False
    return False

## CleanSpecialChar
#
# The ASCII text files of type INF, DEC, INI are edited by developers,
# and may contain characters that cannot be directly translated to strings that
# are conformant with the UDP XML Schema.  Any characters in this category
# (0x00-0x08, TAB [0x09], 0x0B, 0x0C, 0x0E-0x1F, 0x80-0xFF)
# must be converted to a space character[0x20] as part of the parsing process.
#
def ConvertSpecialChar(Lines):
    RetLines = []
    for line in Lines:
        ReMatchSpecialChar = re.compile(r"[\x00-\x08]|\x09|\x0b|\x0c|[\x0e-\x1f]|[\x7f-\xff]")
        RetLines.append(ReMatchSpecialChar.sub(' ', line))

    return RetLines

## __GetTokenList
#
# Assume Str is a valid feature flag expression.
# Return a list which contains tokens: alpha numeric token and other token
# Whitespace are not stripped
#
def __GetTokenList(Str):
    InQuote = False
    Token = ''
    TokenOP = ''
    PreChar = ''
    List = []
    for Char in Str:
        if InQuote:
            Token += Char
            if Char == '"' and PreChar != '\\':
                InQuote = not InQuote
                List.append(Token)
                Token = ''
            continue
        if Char == '"':
            if Token and Token != 'L':
                List.append(Token)
                Token = ''
            if TokenOP:
                List.append(TokenOP)
                TokenOP = ''
            InQuote = not InQuote
            Token += Char
            continue

        if not (Char.isalnum() or Char in '_'):
            TokenOP += Char
            if Token:
                List.append(Token)
                Token = ''
        else:
            Token += Char
            if TokenOP:
                List.append(TokenOP)
                TokenOP = ''

        if PreChar == '\\' and Char == '\\':
            PreChar = ''
        else:
            PreChar = Char
    if Token:
        List.append(Token)
    if TokenOP:
        List.append(TokenOP)
    return List

## ConvertNEToNOTEQ
#
# Convert NE operator to NOT EQ
# For example: 1 NE 2 -> 1 NOT EQ 2
#
# @param Expr: Feature flag expression to be converted
#
def ConvertNEToNOTEQ(Expr):
    List = __GetTokenList(Expr)
    for Index in range(len(List)):
        if List[Index] == 'NE':
            List[Index] = 'NOT EQ'
    return ''.join(List)

## ConvertNOTEQToNE
#
# Convert NOT EQ operator to NE
# For example: 1 NOT NE 2 -> 1 NE 2
#
# @param Expr: Feature flag expression to be converted
#
def ConvertNOTEQToNE(Expr):
    List = __GetTokenList(Expr)
    HasNOT = False
    RetList = []
    for Token in List:
        if HasNOT and Token == 'EQ':
            # At least, 'NOT' is in the list
            while not RetList[-1].strip():
                RetList.pop()
            RetList[-1] = 'NE'
            HasNOT = False
            continue
        if Token == 'NOT':
            HasNOT = True
        elif Token.strip():
            HasNOT = False
        RetList.append(Token)

    return ''.join(RetList)

## SplitPcdEntry
#
# Split an PCD entry string to Token.CName and PCD value and FFE.
# NOTE: PCD Value and FFE can contain "|" in it's expression. And in INF specification, have below rule.
# When using the characters "|" or "||" in an expression, the expression must be encapsulated in
# open "(" and close ")" parenthesis.
#
# @param String    An PCD entry string need to be split.
#
# @return List     [PcdTokenCName, Value, FFE]
#
def SplitPcdEntry(String):
    if not String:
        return ['', '', ''], False

    PcdTokenCName = ''
    PcdValue = ''
    PcdFeatureFlagExp = ''

    ValueList = GetSplitValueList(String, "|", 1)

    #
    # Only contain TokenCName
    #
    if len(ValueList) == 1:
        return [ValueList[0]], True

    NewValueList = []

    if len(ValueList) == 2:
        PcdTokenCName = ValueList[0]

        InQuote = False
        InParenthesis = False
        StrItem = ''
        for StrCh in ValueList[1]:
            if StrCh == '"':
                InQuote = not InQuote
            elif StrCh == '(' or StrCh == ')':
                InParenthesis = not InParenthesis

            if StrCh == '|':
                if not InQuote or not InParenthesis:
                    NewValueList.append(StrItem.strip())
                    StrItem = ' '
                    continue

            StrItem += StrCh

        NewValueList.append(StrItem.strip())

        if len(NewValueList) == 1:
            PcdValue = NewValueList[0]
            return [PcdTokenCName, PcdValue], True
        elif len(NewValueList) == 2:
            PcdValue = NewValueList[0]
            PcdFeatureFlagExp = NewValueList[1]
            return [PcdTokenCName, PcdValue, PcdFeatureFlagExp], True
        else:
            return ['', '', ''], False

    return ['', '', ''], False

## Check if two arches matched?
#
# @param Arch1
# @param Arch2
#
def IsMatchArch(Arch1, Arch2):
    if 'COMMON' in Arch1 or 'COMMON' in Arch2:
        return True
    try:
        if isinstance(Arch1, list) and isinstance(Arch2, list):
            for Item1 in Arch1:
                for Item2 in Arch2:
                    if Item1 == Item2:
                        return True

        elif isinstance(Arch1, list):
            return Arch2 in Arch1

        elif isinstance(Arch2, list):
            return Arch1 in Arch2

        else:
            if Arch1 == Arch2:
                return True
    except:
        return False

# Search all files in FilePath to find the FileName with the largest index
# Return the FileName with index +1 under the FilePath
#
def GetUniFileName(FilePath, FileName):
    Files = []
    try:
        Files = os.listdir(FilePath)
    except:
        pass

    LargestIndex = -1
    IndexNotFound = True
    for File in Files:
        if File.upper().startswith(FileName.upper()) and File.upper().endswith('.UNI'):
            Index = File.upper().replace(FileName.upper(), '').replace('.UNI', '')
            if Index:
                try:
                    Index = int(Index)
                except Exception:
                    Index = -1
            else:
                IndexNotFound = False
                Index = 0
            if Index > LargestIndex:
                LargestIndex = Index + 1

    if LargestIndex > -1 and not IndexNotFound:
        return os.path.normpath(os.path.join(FilePath, FileName + str(LargestIndex) + '.uni'))
    else:
        return os.path.normpath(os.path.join(FilePath, FileName + '.uni'))
